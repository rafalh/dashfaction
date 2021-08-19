#include <patch_common/CallHook.h>
#include <patch_common/FunHook.h>
#include <patch_common/CodeInjection.h>
#include <patch_common/ShortTypes.h>
#include <patch_common/AsmWriter.h>
#include <common/config/BuildConfig.h>
#include <common/version/version.h>
#include <common/rfproto.h>
#include <xlog/xlog.h>
#include <algorithm>
#include <limits>
#include <windows.h>
#include "server.h"
#include "server_internal.h"
#include "multi.h"
#include "../os/console.h"
#include "../misc/player.h"
#include "../main/main.h"
#include <common/utils/list-utils.h>
#include "../rf/player/player.h"
#include "../rf/multi.h"
#include "../rf/parse.h"
#include "../rf/weapon.h"
#include "../rf/entity.h"
#include "../rf/os/os.h"
#include "../rf/os/timer.h"
#include "../rf/level.h"
#include "../rf/collide.h"
#include "../purefaction/pf.h"

const char* g_rcon_cmd_whitelist[] = {
    "kick",
    "level",
    "map",
    "ban",
    "ban_ip",
    "map_ext",
    "map_rest",
    "map_next",
    "map_prev",
};

ServerAdditionalConfig g_additional_server_config;
std::string g_prev_level;

void parse_vote_config(const char* vote_name, VoteConfig& config, rf::Parser& parser)
{
    std::string vote_option_name = string_format("$DF %s:", vote_name);
    if (parser.parse_optional(vote_option_name.c_str())) {
        config.enabled = parser.parse_bool();
        rf::console::printf("DF %s: %s", vote_name, config.enabled ? "true" : "false");

        // if (parser.ParseOptional("+Min Voters:")) {
        //     config.min_voters = parser.ParseUInt();
        // }

        // if (parser.ParseOptional("+Min Percentage:")) {
        //     config.min_percentage = parser.ParseUInt();
        // }

        if (parser.parse_optional("+Time Limit:")) {
            config.time_limit_seconds = parser.parse_uint();
        }
    }
}

void load_additional_server_config(rf::Parser& parser)
{
    parse_vote_config("Vote Kick", g_additional_server_config.vote_kick, parser);
    parse_vote_config("Vote Level", g_additional_server_config.vote_level, parser);
    parse_vote_config("Vote Extend", g_additional_server_config.vote_extend, parser);
    parse_vote_config("Vote Restart", g_additional_server_config.vote_restart, parser);
    parse_vote_config("Vote Next", g_additional_server_config.vote_next, parser);
    parse_vote_config("Vote Previous", g_additional_server_config.vote_previous, parser);
    if (parser.parse_optional("$DF Spawn Protection Duration:")) {
        g_additional_server_config.spawn_protection_duration_ms = parser.parse_uint();
    }
    if (parser.parse_optional("$DF Hitsounds:")) {
        g_additional_server_config.hit_sounds.enabled = parser.parse_bool();
        if (parser.parse_optional("+Sound ID:")) {
            g_additional_server_config.hit_sounds.sound_id = parser.parse_uint();
        }
        if (parser.parse_optional("+Rate Limit:")) {
            g_additional_server_config.hit_sounds.rate_limit = parser.parse_uint();
        }
    }

    while (parser.parse_optional("$DF Item Replacement:")) {
        rf::String old_item;
        rf::String new_item;
        parser.parse_string(&old_item);
        parser.parse_string(&new_item);
        g_additional_server_config.item_replacements.insert({old_item.c_str(), new_item.c_str()});
    }

    if (parser.parse_optional("$DF Weapon Items Give Full Ammo:")) {
        g_additional_server_config.weapon_items_give_full_ammo = parser.parse_bool();
    }

    if (parser.parse_optional("$DF Default Player Weapon:")) {
        rf::String default_weapon;
        parser.parse_string(&default_weapon);
        g_additional_server_config.default_player_weapon = default_weapon.c_str();

        if (parser.parse_optional("+Initial Ammo:")) {
            auto ammo = parser.parse_uint();
            g_additional_server_config.default_player_weapon_ammo = {ammo};

            auto weapon_type = rf::weapon_lookup_type(g_additional_server_config.default_player_weapon.c_str());
            if (weapon_type >= 0) {
                auto& weapon_cls = rf::weapon_types[weapon_type];
                weapon_cls.max_ammo_multi = std::max<int>(weapon_cls.max_ammo_multi, ammo);
            }
        }
    }

    if (parser.parse_optional("$DF Anticheat Level:")) {
        g_additional_server_config.anticheat_level = parser.parse_int();
    }

    if (parser.parse_optional("$DF Require Client Mod:")) {
        g_additional_server_config.require_client_mod = parser.parse_bool();
    }

    if (parser.parse_optional("$DF Player Damage Modifier:")) {
        g_additional_server_config.player_damage_modifier = parser.parse_float();
    }

    if (parser.parse_optional("$DF Saving Enabled:")) {
        g_additional_server_config.saving_enabled = parser.parse_bool();
    }

    if (parser.parse_optional("$DF UPnP Enabled:")) {
        g_additional_server_config.upnp_enabled = parser.parse_bool();
    }

    if (parser.parse_optional("$DF Force Player Character:")) {
        rf::String character_name;
        parser.parse_string(&character_name);
        int character_num = rf::multi_find_character(character_name.c_str());
        if (character_num != -1) {
            g_additional_server_config.force_player_character = {character_num};
        }
        else {
            xlog::warn("Unknown character name in Force Player Character setting: %s", character_name.c_str());
        }
    }

    if (parser.parse_optional("$DF Max FOV:")) {
        float max_fov = parser.parse_float();
        if (max_fov > 0.0f) {
            g_additional_server_config.max_fov = {max_fov};
        }
    }

    if (parser.parse_optional("$DF Send Player Stats Message:")) {
        g_additional_server_config.stats_message_enabled = parser.parse_bool();
    }

    if (parser.parse_optional("$DF Welcome Message:")) {
        rf::String welcome_message;
        parser.parse_string(&welcome_message);
        g_additional_server_config.welcome_message = welcome_message.c_str();
    }

    if (!parser.parse_optional("$Name:") && !parser.parse_optional("#End")) {
        parser.error("end of server configuration");
    }
}

CodeInjection dedicated_server_load_config_patch{
    0x0046E216,
    [](auto& regs) {
        auto& parser = *reinterpret_cast<rf::Parser*>(regs.esp - 4 + 0x4C0 - 0x470);
        load_additional_server_config(parser);

        // Insert server name in window title when hosting dedicated server
        std::string wnd_name;
        wnd_name.append(rf::netgame.name.c_str());
        wnd_name.append(" - " PRODUCT_NAME " Dedicated Server");
        SetWindowTextA(rf::main_wnd, wnd_name.c_str());
    },
};

std::pair<std::string_view, std::string_view> strip_by_space(std::string_view str)
{
    auto space_pos = str.find(' ');
    if (space_pos == std::string_view::npos) {
        return {str, {}};
    }
    return {str.substr(0, space_pos), str.substr(space_pos + 1)};
}

void handle_next_map_command(rf::Player* player)
{
    int next_idx = (rf::netgame.current_level_index + 1) % rf::netgame.levels.size();
    auto msg = string_format("Next level: %s", rf::netgame.levels[next_idx].c_str());
    send_chat_line_packet(msg.c_str(), player);
}

void handle_save_command(rf::Player* player, std::string_view save_name)
{
    auto& pdata = get_player_additional_data(player);
    rf::Entity* entity = rf::entity_from_handle(player->entity_handle);
    if (entity && g_additional_server_config.saving_enabled) {
        PlayerNetGameSaveData save_data;
        save_data.pos = entity->pos;
        save_data.orient = entity->orient;
        pdata.saves.insert_or_assign(std::string{save_name}, save_data);
        send_chat_line_packet("Your position has been saved!", player);
    }
}

void handle_load_command(rf::Player* player, std::string_view save_name)
{
    auto& pdata = get_player_additional_data(player);
    rf::Entity* entity = rf::entity_from_handle(player->entity_handle);
    if (entity && g_additional_server_config.saving_enabled && !rf::entity_is_dying(entity)) {
        auto it = pdata.saves.find(std::string{save_name});
        if (it != pdata.saves.end()) {
            auto& save_data = it->second;
            entity->p_data.pos = save_data.pos;
            entity->p_data.next_pos = save_data.pos;
            entity->pos = save_data.pos;
            entity->orient = save_data.orient;
            if (entity->obj_interp) {
                entity->obj_interp->Clear();
            }
            rf::send_entity_create_packet_to_all(entity);
            pdata.last_teleport_timestamp.set(300);
            pdata.last_teleport_pos = save_data.pos;
            send_chat_line_packet("Your position has been restored!", player);
        }
        else {
            send_chat_line_packet("You do not have any position saved!", player);
        }
    }
}

CodeInjection process_obj_update_set_pos_injection{
    0x0047E563,
    [](auto& regs) {
        if (!rf::is_server) {
            return;
        }
        auto& entity = addr_as_ref<rf::Entity>(regs.edi);
        auto& pos = addr_as_ref<rf::Vector3>(regs.esp + 0x9C - 0x60);
        auto player = rf::player_from_entity_handle(entity.handle);
        auto& pdata = get_player_additional_data(player);
        if (pdata.last_teleport_timestamp.valid()) {
            float dist = (pos - pdata.last_teleport_pos).len();
            if (!pdata.last_teleport_timestamp.elapsed() && dist > 1.0f) {
                // Ignore obj_update packets for some time after restoring the position
                xlog::trace("ignoring obj_update after teleportation (distance %f)", dist);
                regs.eip = 0x0047DFF6;
            }
            else {
                xlog::trace("not ignoring obj_update anymore after teleportation (distance %f)", dist);
                pdata.last_teleport_timestamp.invalidate();
            }
        }
    },
};

static void send_private_message_with_stats(rf::Player* player)
{
    auto* stats = static_cast<PlayerStatsNew*>(player->stats);
    int accuracy = static_cast<int>(stats->calc_accuracy() * 100.0f);
    auto str = string_format(
        "PLAYER STATS\n"
        "Kills: %d - Deaths: %d - Max Streak: %d\n"
        "Accuracy: %d%% (%.0f/%.0f) - Damage Given: %.0f - Damage Taken: %.0f",
        stats->num_kills, stats->num_deaths, stats->max_streak,
        accuracy, stats->num_shots_hit, stats->num_shots_fired,
        stats->damage_given, stats->damage_received);
    send_chat_line_packet(str.c_str(), player);
}

bool handle_server_chat_command(std::string_view server_command, rf::Player* sender)
{
    auto [cmd_name, cmd_arg] = strip_by_space(server_command);

    if (cmd_name == "info") {
        send_chat_line_packet(string_format("Server powered by Dash Faction %s (build date: %s %s)", VERSION_STR, __DATE__, __TIME__).c_str(), sender);
    }
    else if (cmd_name == "vote") {
        auto [vote_name, vote_arg] = strip_by_space(cmd_arg);
        handle_vote_command(vote_name, vote_arg, sender);
    }
    else if (cmd_name == "nextmap" || cmd_name == "nextlevel") {
        handle_next_map_command(sender);
    }
    else if (cmd_name == "save") {
        handle_save_command(sender, cmd_arg);
    }
    else if (cmd_name == "load") {
        handle_load_command(sender, cmd_arg);
    }
    else if (cmd_name == "stats") {
        send_private_message_with_stats(sender);
    }
    else {
        return false;
    }
    return true;
}

bool check_server_chat_command(const char* msg, rf::Player* sender)
{
    if (msg[0] == '/') {
        if (!handle_server_chat_command(msg + 1, sender))
            send_chat_line_packet("Unrecognized server command!", sender);
        return true;
    }

    auto [cmd, rest] = strip_by_space(msg);
    if (cmd == "server")
        return handle_server_chat_command(rest, sender);
    if (cmd == "vote")
        return handle_server_chat_command(msg, sender);
    return false;
}

CodeInjection spawn_protection_duration_patch{
    0x0048089A,
    [](auto& regs) {
        *static_cast<int*>(regs.esp) = g_additional_server_config.spawn_protection_duration_ms;
    },
};

CodeInjection detect_browser_player_patch{
    0x0047AFFB,
    [](auto& regs) {
        rf::Player* player = regs.esi;
        int conn_rate = regs.eax;
        if (conn_rate == 1 || conn_rate == 256) {
            auto& pdata = get_player_additional_data(player);
            pdata.is_browser = true;
        }
    },
};

void send_hit_sound_packet(rf::Player* target)
{
    // rate limiting - max 5 per second
    int now = rf::timer_get(1000);
    auto& pdata = get_player_additional_data(target);
    if (now - pdata.last_hitsound_sent_ms < 1000 / g_additional_server_config.hit_sounds.rate_limit) {
        return;
    }
    pdata.last_hitsound_sent_ms = now;

    // Send sound packet
    RF_SoundPacket packet;
    packet.header.type = RF_GPT_SOUND;
    packet.header.size = sizeof(packet) - sizeof(packet.header);
    packet.sound_id = g_additional_server_config.hit_sounds.sound_id;
    // FIXME: it does not work on RF 1.21
    packet.pos.x = packet.pos.y = packet.pos.z = std::numeric_limits<float>::quiet_NaN();
    rf::multi_io_send(target, &packet, sizeof(packet));
}

FunHook<float(rf::Entity*, float, int, int, int)> entity_damage_hook{
    0x0041A350,
    [](rf::Entity* damaged_ep, float damage, int killer_handle, int damage_type, int killer_uid) {
        rf::Player* damaged_player = rf::player_from_entity_handle(damaged_ep->handle);
        rf::Player* killer_player = rf::player_from_entity_handle(killer_handle);
        bool is_pvp_damage = damaged_player && killer_player && damaged_player != killer_player;
        if (rf::is_server && is_pvp_damage) {
            damage *= g_additional_server_config.player_damage_modifier;
            if (damage == 0.0f) {
                return 0.0f;
            }
        }

        float real_damage = entity_damage_hook.call_target(damaged_ep, damage, killer_handle, damage_type, killer_uid);

        if (rf::is_server && is_pvp_damage && real_damage > 0.0f) {

            auto* killer_player_stats = static_cast<PlayerStatsNew*>(killer_player->stats);
            killer_player_stats->add_damage_given(real_damage);

            auto* damaged_player_stats = static_cast<PlayerStatsNew*>(damaged_player->stats);
            damaged_player_stats->add_damage_received(real_damage);

            if (g_additional_server_config.hit_sounds.enabled) {
                send_hit_sound_packet(killer_player);
            }
        }

        return real_damage;
    },
};

CallHook<int(const char*)> item_lookup_type_hook{
    0x00465102,
    [](const char* cls_name) {
        if (rf::is_dedicated_server) {
            // support item replacement mapping
            auto it = g_additional_server_config.item_replacements.find(cls_name);
            if (it != g_additional_server_config.item_replacements.end())
                cls_name = it->second.c_str();
        }
        return item_lookup_type_hook.call_target(cls_name);
    },
};

CallHook<int(const char*)> find_default_weapon_for_entity_hook{
    0x004A43DA,
    [](const char* weapon_name) {
        if (rf::is_dedicated_server && !g_additional_server_config.default_player_weapon.empty()) {
            weapon_name = g_additional_server_config.default_player_weapon.c_str();
        }
        return find_default_weapon_for_entity_hook.call_target(weapon_name);
    },
};

CallHook<void(rf::Player*, int, int)> give_default_weapon_ammo_hook{
    0x004A4414,
    [](rf::Player* player, int weapon_type, int ammo) {
        if (g_additional_server_config.default_player_weapon_ammo) {
            ammo = g_additional_server_config.default_player_weapon_ammo.value();
        }
        give_default_weapon_ammo_hook.call_target(player, weapon_type, ammo);
    },
};

FunHook<bool (const char*, int)> multi_is_level_matching_game_type_hook{
    0x00445050,
    [](const char *filename, int ng_type) {
        if (ng_type == RF_GT_CTF) {
            return string_starts_with_ignore_case(filename, "ctf") || string_starts_with_ignore_case(filename, "pctf");
        }
        return string_starts_with_ignore_case(filename, "dm") || string_starts_with_ignore_case(filename, "pdm");
    },
};

FunHook<void(rf::Player*)> spawn_player_sync_ammo_hook{
    0x00480820,
    [](rf::Player* player) {
        spawn_player_sync_ammo_hook.call_target(player);
        // if default player weapon has ammo override sync ammo using additional reload packet
        if (g_additional_server_config.default_player_weapon_ammo && !rf::player_is_dead(player)) {
            rf::Entity* entity = rf::entity_from_handle(player->entity_handle);
            RF_ReloadPacket packet;
            packet.header.type = RF_GPT_RELOAD;
            packet.header.size = sizeof(packet) - sizeof(packet.header);
            packet.entity_handle = entity->handle;
            int weapon_type = entity->ai.current_primary_weapon;
            packet.weapon = weapon_type;
            packet.clip_ammo = entity->ai.clip_ammo[weapon_type];
            int ammo_type = rf::weapon_types[weapon_type].ammo_type;
            packet.ammo = entity->ai.ammo[ammo_type];
            rf::multi_io_send_reliable(player, reinterpret_cast<uint8_t*>(&packet), sizeof(packet), 0);
        }
    },
};

CallHook<void(char*)> get_mod_name_require_client_mod_hook{
    {
        0x0047B1E0, // send_game_info_packet
        0x004B32A3, // init_anticheat_checksums
    },
    [](char* mod_name) {
        if (!rf::is_dedicated_server || g_additional_server_config.require_client_mod) {
            get_mod_name_require_client_mod_hook.call_target(mod_name);
        }
        else {
            mod_name[0] = '\0';
        }
    },
};

CodeInjection send_ping_time_wrap_fix{
    0x0047CCE8,
    [](auto& regs) {
        auto& io_stats = addr_as_ref<rf::MultiIoStats>(regs.esi);
        auto player = addr_as_ref<rf::Player*>(regs.esp + 0xC + 0x4);
        if (!io_stats.send_ping_packet_timestamp.valid() || io_stats.send_ping_packet_timestamp.elapsed()) {
            xlog::trace("sending ping");
            io_stats.send_ping_packet_timestamp.set(3000);
            rf::multi_ping_player(player);
            io_stats.last_ping_time = rf::timer_get(1000);
        }
        regs.eip = 0x0047CD64;
    },
};

CodeInjection multi_on_new_player_injection{
    0x0047B013,
    [](auto& regs) {
        rf::Player* player = regs.esi;
        in_addr addr;
        addr.S_un.S_addr = ntohl(player->net_data->addr.ip_addr);
        rf::console::printf("%s%s (%s)", player->name.c_str(),  rf::strings::has_joined, inet_ntoa(addr));
        regs.eip = 0x0047B051;
    },
};

static bool check_player_ac_status(rf::Player* player)
{
#ifdef HAS_PF
    if (g_additional_server_config.anticheat_level > 0) {
        bool verified = pf_is_player_verified(player);
        if (!verified) {
            send_chat_line_packet(
                "Sorry! Your spawn request was rejected because verification of your client software failed. "
                "Please use the latest officially released version of Dash Faction. You can get it from dashfaction.com.",
                player);
            return false;
        }

        int ac_level = pf_get_player_ac_level(player);
        if (ac_level < g_additional_server_config.anticheat_level) {
            auto msg = string_format(
                "Sorry! Your spawn request was rejected because your client did not pass anti-cheat verification (your level %d, required %d). "
                "Please make sure you do not have any mods installed and that your client software is up to date.",
                ac_level, g_additional_server_config.anticheat_level
            );
            send_chat_line_packet(msg.c_str(), player);
            return false;
        }
    }
#endif // HAS_PF
    return true;
}

FunHook<void(rf::Player*)> multi_spawn_player_server_side_hook{
    0x00480820,
    [](rf::Player* player) {
        if (g_additional_server_config.force_player_character) {
            player->settings.multi_character = g_additional_server_config.force_player_character.value();
        }
        if (!check_player_ac_status(player)) {
            return;
        }
        multi_spawn_player_server_side_hook.call_target(player);
    },
};

static float get_weapon_shot_stats_delta(rf::Weapon* wp)
{
    int num_projectiles = wp->info->num_projectiles;
    rf::Entity* parent_ep = rf::entity_from_handle(wp->parent_handle);
    if (parent_ep && parent_ep->entity_flags2 & 0x1000) { // EF2_SHOTGUN_DOUBLE_BULLET_UNK
        num_projectiles *= 2;
    }
    return 1.0f / num_projectiles;
}

static bool multi_is_team_game_type()
{
    return rf::multi_get_game_type() != rf::NG_TYPE_DM;
}

static void maybe_increment_weapon_hits_stat(int hit_obj_handle, rf::Weapon *wp)
{
    rf::Entity* attacker_ep = rf::entity_from_handle(wp->parent_handle);
    if (!attacker_ep) {
        return;
    }

    rf::Entity* hit_ep = rf::entity_from_handle(hit_obj_handle);
    if (!hit_ep) {
        return;
    }

    rf::Player* attacker_pp = rf::player_from_entity_handle(attacker_ep->handle);
    rf::Player* hit_pp = rf::player_from_entity_handle(hit_ep->handle);
    if (!attacker_pp || !hit_pp) {
        return;
    }

    if (!multi_is_team_game_type() || attacker_pp->team != hit_pp->team) {
        auto* stats = static_cast<PlayerStatsNew*>(attacker_pp->stats);
        stats->add_shots_hit(get_weapon_shot_stats_delta(wp));
        xlog::trace("hit a_ep %p wp %p h_ep %p", attacker_ep, wp, hit_ep);
    }
}

FunHook<int(rf::LevelCollisionOut*, rf::Weapon*)> multi_lag_comp_handle_hit_hook{
    0x0046F380,
    [](rf::LevelCollisionOut *col_info, rf::Weapon *wp) {
        if (rf::is_server) {
            maybe_increment_weapon_hits_stat(col_info->obj_handle, wp);
        }
        return multi_lag_comp_handle_hit_hook.call_target(col_info, wp);
    },
};

FunHook<void(rf::Entity*, rf::Weapon*)> multi_lag_comp_weapon_fire_hook{
    0x0046F7E0,
    [](rf::Entity *ep, rf::Weapon *wp) {
        multi_lag_comp_weapon_fire_hook.call_target(ep, wp);
        rf::Player* pp = rf::player_from_entity_handle(ep->handle);
        if (pp && pp->stats) {
            auto* stats = static_cast<PlayerStatsNew*>(pp->stats);
            stats->add_shots_fired(get_weapon_shot_stats_delta(wp));
            xlog::trace("fired a_ep %p wp %p", ep, wp);
        }
    },
};

void server_reliable_socket_ready(rf::Player* player)
{
    if (!g_additional_server_config.welcome_message.empty()) {
        auto msg = string_replace(g_additional_server_config.welcome_message, "$PLAYER", player->name.c_str());
        send_chat_line_packet(msg.c_str(), player);
    }
}

void server_init()
{
    // Override rcon command whitelist
    write_mem_ptr(0x0046C794 + 1, g_rcon_cmd_whitelist);
    write_mem_ptr(0x0046C7D1 + 2, g_rcon_cmd_whitelist + std::size(g_rcon_cmd_whitelist));

    // Additional server config
    dedicated_server_load_config_patch.install();

    // Apply customized spawn protection duration
    spawn_protection_duration_patch.install();

    // Detect if player joining to the server is a browser
    detect_browser_player_patch.install();

    // Hit sounds
    entity_damage_hook.install();

    // Item replacements
    item_lookup_type_hook.install();

    // Default player weapon class and ammo override
    find_default_weapon_for_entity_hook.install();
    give_default_weapon_ammo_hook.install();
    spawn_player_sync_ammo_hook.install();

    init_server_commands();

    // Remove level prefix restriction (dm/ctf) for 'level' command and dedicated_server.txt
    AsmWriter(0x004350FE).nop(2);
    AsmWriter(0x0046E179).nop(2);

    // In Multi -> Create game fix level filtering so 'pdm' and 'pctf' is supported
    multi_is_level_matching_game_type_hook.install();

    // Allow disabling mod name announcement
    get_mod_name_require_client_mod_hook.install();

    // Fix items not being respawned after time in ms wraps around (~25 days)
    AsmWriter(0x004599DB).nop(2);

    // Fix sending ping packets after time in ms wraps around (~25 days)
    send_ping_time_wrap_fix.install();

    // Ignore obj_update position for some time after teleportation
    process_obj_update_set_pos_injection.install();

    // Customized dedicated server console message when player joins
    multi_on_new_player_injection.install();
    AsmWriter(0x0047B061, 0x0047B064).add(asm_regs::esp, 0x14);

    // Support forcing player character
    multi_spawn_player_server_side_hook.install();

    // Hook lag compensation functions to calculate accuracy only for weapons with bullets
    // Note: weapons with bullets (projectiles) are created twice server-side so hooking weapon_create would
    // be problematic (PF went this way and its accuracy stat is broken)
    multi_lag_comp_handle_hit_hook.install();
    multi_lag_comp_weapon_fire_hook.install();

    // Set lower bound of server max players clamp range to 1 (instead of 2)
    write_mem<i8>(0x0046DD4F + 1, 1);
}

void server_do_frame()
{
    server_vote_do_frame();
    process_delayed_kicks();
}

void server_on_limbo_state_enter()
{
    g_prev_level = rf::level.filename.c_str();
    server_vote_on_limbo_state_enter();

    // Clear save data for all players
    auto player_list = SinglyLinkedList{rf::player_list};
    for (auto& player : player_list) {
        auto& pdata = get_player_additional_data(&player);
        pdata.saves.clear();
        pdata.last_teleport_timestamp.invalidate();
        if (g_additional_server_config.stats_message_enabled) {
            send_private_message_with_stats(&player);
        }
    }
}

bool server_is_saving_enabled()
{
    return g_additional_server_config.saving_enabled;
}

bool server_weapon_items_give_full_ammo()
{
    return g_additional_server_config.weapon_items_give_full_ammo;
}

const ServerAdditionalConfig& server_get_df_config()
{
    return g_additional_server_config;
}
