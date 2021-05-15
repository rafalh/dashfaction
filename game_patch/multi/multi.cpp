#include <regex>
#include <xlog/xlog.h>
#include <patch_common/FunHook.h>
#include <patch_common/CodeInjection.h>
#include "multi.h"
#include "multi_private.h"
#include "../misc/misc.h"
#include "../rf/os/os.h"
#include "../rf/os/timer.h"
#include "../rf/multi.h"
#include "../rf/os/console.h"
#include "../rf/weapon.h"
#include "../rf/entity.h"
#include "../rf/ai.h"
#include "../rf/item.h"

// Note: this must be called from DLL init function
// Note: we can't use global variable because that would lead to crash when launcher loads this DLL to check dependencies
static rf::CmdLineParam& get_url_cmd_line_param()
{
    static rf::CmdLineParam url_param{"-url", "", true};
    return url_param;
}

void handle_url_param()
{
    if (!get_url_cmd_line_param().found()) {
        return;
    }

    const char* url = get_url_cmd_line_param().get_arg();
    std::regex e{R"(^rf://([\w\.-]+):(\d+)/?(?:\?password=(.*))?$)"};
    std::cmatch cm;
    if (!std::regex_match(url, cm, e)) {
        xlog::warn("Unsupported URL: %s", url);
        return;
    }

    auto host_name = cm[1].str();
    auto port = static_cast<uint16_t>(std::stoi(cm[2].str()));
    auto password = cm[3].str();

    hostent* hp = gethostbyname(host_name.c_str());
    if (!hp) {
        xlog::warn("URL host lookup failed");
        return;
    }

    if (hp->h_addrtype != AF_INET) {
        xlog::warn("Unsupported address type (only IPv4 is supported)");
        return;
    }

    rf::console::printf("Connecting to %s:%d...", host_name.c_str(), port);
    auto host = ntohl(reinterpret_cast<in_addr *>(hp->h_addr_list[0])->S_un.S_addr);

    rf::NetAddr addr{host, port};
    start_join_multi_game_sequence(addr, password);
}

FunHook<void()> multi_limbo_init{
    0x0047C280,
    []() {
        multi_limbo_init.call_target();
        if (rf::is_server) {
            server_on_limbo_state_enter();
        }
    },
};

CodeInjection multi_start_injection{
    0x0046D5B0,
    []() {
        void debug_multi_init();
        debug_multi_init();
    },
};

CodeInjection ctf_flag_return_fix{
    0x0047381D,
    [](auto& regs) {
        auto stack_frame = regs.esp + 0x1C;
        bool red = addr_as_ref<bool>(stack_frame + 4);
        if (red) {
            regs.eip = 0x00473827;
        }
        else {
            regs.eip = 0x00473822;
        }
    },
};

FunHook<void()> multi_ctf_level_init_hook{
    0x00472E30,
    []() {
        multi_ctf_level_init_hook.call_target();
        // Make sure CTF flag does not spin in new level if it was dropped in the previous level
        int info_index = rf::item_lookup_type("flag_red");
        if (info_index >= 0) {
            rf::item_info[info_index].flags &= ~rf::IIF_SPINS_IN_MULTI;
        }
        info_index = rf::item_lookup_type("flag_blue");
        if (info_index >= 0) {
            rf::item_info[info_index].flags &= ~rf::IIF_SPINS_IN_MULTI;
        }
    },
};

static rf::Timestamp select_weapon_done_timestamp[rf::multi_max_player_id];

bool multi_is_selecting_weapon(rf::Player* pp)
{
    auto& done_timestamp = select_weapon_done_timestamp[pp->net_data->player_id];
    return done_timestamp.valid() && !done_timestamp.elapsed();
}

FunHook<void(rf::Player*, rf::Entity*, int)> multi_select_weapon_server_side_hook{
    0x004858D0,
    [](rf::Player *pp, rf::Entity *ep, int weapon_type) {
        if (weapon_type == -1 || ep->ai.current_primary_weapon == weapon_type) {
            // Nothing to do
            return;
        }
        bool has_weapon;
        if (weapon_type == rf::remote_charge_det_weapon_type) {
            has_weapon = rf::ai_has_weapon(&ep->ai, rf::remote_charge_weapon_type);
        }
        else {
            has_weapon = rf::ai_has_weapon(&ep->ai, weapon_type);
        }
        if (!has_weapon) {
            xlog::debug("Player %s attempted to select an unpossesed weapon %d", pp->name.c_str(), weapon_type);
        }
        else if (multi_is_selecting_weapon(pp)) {
            xlog::debug("Player %s attempted to select weapon %d while selecting weapon %d",
                pp->name.c_str(), weapon_type, ep->ai.current_primary_weapon);
        }
        else if (rf::entity_is_reloading(ep)) {
            xlog::debug("Player %s attempted to select weapon %d while reloading weapon %d",
                pp->name.c_str(), weapon_type, ep->ai.current_primary_weapon);
        }
        else {
            rf::player_make_weapon_current_selection(pp, weapon_type);
            ep->ai.current_primary_weapon = weapon_type;
            select_weapon_done_timestamp[pp->net_data->player_id].set(300);
        }
    },
};

void multi_reload_weapon_server_side(rf::Player* pp, int weapon_type)
{
    rf::Entity* ep = rf::entity_from_handle(pp->entity_handle);
    if (!ep) {
        // Entity is dead
    }
    else if (ep->ai.current_primary_weapon != weapon_type) {
        xlog::debug("Player %s attempted to reload unselected weapon %d", pp->name.c_str(), weapon_type);
    }
    else if (multi_is_selecting_weapon(pp)) {
        xlog::debug("Player %s attempted to reload weapon %d while selecting it", pp->name.c_str(), weapon_type);
    }
    else if (rf::entity_is_reloading(ep)) {
        xlog::debug("Player %s attempted to reload weapon %d while reloading it", pp->name.c_str(), weapon_type);
    }
    else {
        rf::entity_reload_current_primary(ep, false, false);
    }
}

void multi_ensure_ammo_is_not_empty(rf::Entity* ep)
{
    int weapon_type = ep->ai.current_primary_weapon;
    auto& wi = rf::weapon_types[weapon_type];
    // at least ammo for 100 milliseconds
    int min_ammo = std::max(static_cast<int>(0.1f / wi.fire_wait), 1);
    if (rf::weapon_is_melee(weapon_type)) {
        return;
    }
    if (rf::weapon_uses_clip(weapon_type)) {
        auto& clip_ammo = ep->ai.clip_ammo[weapon_type];
        clip_ammo = std::max(clip_ammo, min_ammo);
    }
    else {
        auto& ammo = ep->ai.ammo[wi.ammo_type];
        ammo = std::max(ammo, min_ammo);
    }
}

void multi_turn_weapon_on(rf::Entity* ep, rf::Player* pp, bool alt_fire)
{
    // Note: pp is always null client-side
    auto weapon_type = ep->ai.current_primary_weapon;
    if (!rf::weapon_is_on_off_weapon(weapon_type, alt_fire)) {
        xlog::debug("Player %s attempted to turn on weapon %d which has no continous fire flag", ep->name.c_str(), weapon_type);
    }
    else if (rf::is_server && multi_is_selecting_weapon(pp)) {
        xlog::debug("Player %s attempted to turn on weapon %d while selecting it", ep->name.c_str(), weapon_type);
    }
    else if (rf::is_server && rf::entity_is_reloading(ep)) {
        xlog::debug("Player %s attempted to turn on weapon %d while reloading it", ep->name.c_str(), weapon_type);
    }
    else {
        if (!rf::is_server) {
            // Make sure client-side ammo is not empty when we know that the player is currently shooting
            // It can be empty if a reload packet was lost or if it got desynced because of network delays
            multi_ensure_ammo_is_not_empty(ep);
        }
        rf::entity_turn_weapon_on(ep->handle, weapon_type, alt_fire);
    }
}

void multi_turn_weapon_off(rf::Entity* ep)
{
    auto current_primary_weapon = ep->ai.current_primary_weapon;
    if (rf::weapon_is_on_off_weapon(current_primary_weapon, false)
        || rf::weapon_is_on_off_weapon(current_primary_weapon, true)) {

        rf::entity_turn_weapon_off(ep->handle, current_primary_weapon);
    }
}

bool weapon_uses_ammo(int weapon_type, bool alt_fire)
{
    if (rf::weapon_is_detonator(weapon_type)) {
         return false;
    }
    if (rf::weapon_is_riot_stick(weapon_type) && alt_fire) {
        return true;
    }
    rf::WeaponInfo* winfo = &rf::weapon_types[weapon_type];
    return !(winfo->flags & rf::WTF_MELEE);
}

bool is_entity_out_of_ammo(rf::Entity *entity, int weapon_type, bool alt_fire)
{
    if (!weapon_uses_ammo(weapon_type, alt_fire)) {
        return false;
    }
    rf::WeaponInfo* winfo = &rf::weapon_types[weapon_type];
    if (winfo->clip_size == 0) {
        auto ammo = entity->ai.ammo[winfo->ammo_type];
        return ammo == 0;
    }
    auto clip_ammo = entity->ai.clip_ammo[weapon_type];
    return clip_ammo == 0;
}

std::pair<bool, float> multi_is_cps_above_limit(rf::Player* pp, float max_cps)
{
    constexpr int num_samples = 4;
    int player_id = pp->net_data->player_id;
    static int last_weapon_fire[rf::multi_max_player_id][num_samples];
    int now = rf::timer_get(1000);
    float avg_dt_secs = (now - last_weapon_fire[player_id][0]) / static_cast<float>(num_samples) / 1000.0f;
    float cps = 1.0f / avg_dt_secs;
    if (cps > max_cps) {
        return {true, cps};
    }
    for (int i = 0; i < num_samples - 1; ++i) {
        last_weapon_fire[player_id][i] = last_weapon_fire[player_id][i + 1];
    }
    last_weapon_fire[player_id][num_samples - 1] = now;
    return {false, cps};
}

float multi_get_max_cps(int weapon_type, bool alt_fire)
{
    if (rf::weapon_is_semi_automatic(weapon_type)) {
        // most people cannot do much more than 10 clicks per second so 20 should be safe
        return 20.0f;
    }
    int fire_wait_ms = rf::weapon_get_fire_wait_ms(weapon_type, alt_fire);
    float fire_wait_secs = fire_wait_ms / 1000.0f;
    float fire_rate = 1.0f / fire_wait_secs;
    // allow 10% more to make sure we do not skip any legit packets
    return fire_rate * 1.1f;
}

bool multi_check_cps(rf::Player* pp, int weapon_type, bool alt_fire)
{
    float max_cps = multi_get_max_cps(weapon_type, alt_fire);
    auto [above_limit, cps] = multi_is_cps_above_limit(pp, max_cps);
    if (above_limit) {
        xlog::info("Player %s is shooting too fast: cps %.2f is greater than allowed %.2f",
            pp->name.c_str(), cps, max_cps);
    }
    return above_limit;
}

bool multi_is_weapon_fire_allowed_server_side(rf::Entity *ep, int weapon_type, bool alt_fire)
{
    rf::Player* pp = rf::player_from_entity_handle(ep->handle);
    if (ep->ai.current_primary_weapon != weapon_type) {
        xlog::debug("Player %s attempted to fire unselected weapon %d", pp->name.c_str(), weapon_type);
    }
    else if (is_entity_out_of_ammo(ep, weapon_type, alt_fire)) {
        xlog::debug("Player %s attempted to fire weapon %d without ammunition", pp->name.c_str(), weapon_type);
    }
    else if (rf::weapon_is_on_off_weapon(weapon_type, alt_fire)) {
        xlog::debug("Player %s attempted to fire a single bullet from on/off weapon %d", pp->name.c_str(), weapon_type);
    }
    else if (multi_is_selecting_weapon(pp)) {
        xlog::debug("Player %s attempted to fire weapon %d while selecting it", pp->name.c_str(), weapon_type);
    }
    else if (rf::entity_is_reloading(ep)) {
        xlog::debug("Player %s attempted to fire weapon %d while reloading it", pp->name.c_str(), weapon_type);
    }
    else if (!multi_check_cps(pp, weapon_type, alt_fire)) {
        return true;
    }
    return false;
}

FunHook<void(rf::Entity*, int, rf::Vector3&, rf::Matrix3&, bool)> multi_process_remote_weapon_fire_hook{
    0x0047D220,
    [](rf::Entity *ep, int weapon_type, rf::Vector3& pos, rf::Matrix3& orient, bool alt_fire) {
        if (rf::is_server) {
            // Do some checks server-side to prevent cheating
            if (!multi_is_weapon_fire_allowed_server_side(ep, weapon_type, alt_fire)) {
                return;
            }
        }
        multi_process_remote_weapon_fire_hook.call_target(ep, weapon_type, pos, orient, alt_fire);
    },
};

void multi_init_player(rf::Player* player)
{
    multi_kill_init_player(player);
}

void multi_do_patch()
{
    multi_limbo_init.install();
    multi_start_injection.install();

    // Fix CTF flag not returning to the base if the other flag was returned when the first one was waiting
    ctf_flag_return_fix.install();

    // Weapon select server-side handling
    multi_select_weapon_server_side_hook.install();

    // Check ammo server-side when handling weapon fire packets
    multi_process_remote_weapon_fire_hook.install();

    // Make sure CTF flag does not spin in new level if it was dropped in the previous level
    multi_ctf_level_init_hook.install();

    multi_kill_do_patch();
    level_download_do_patch();
    network_init();
    multi_tdm_apply_patch();

    level_download_init();

    // Init cmd line param
    get_url_cmd_line_param();
}

void multi_after_full_game_init()
{
    handle_url_param();
}
