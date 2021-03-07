#include "multi_spectate.h"
#include "hud.h"
#include "multi_scoreboard.h"
#include "../os/console.h"
#include "../rf/entity.h"
#include "../rf/player.h"
#include "../rf/multi.h"
#include "../rf/gameseq.h"
#include "../rf/weapon.h"
#include "../rf/gr/gr.h"
#include "../rf/gr/gr_font.h"
#include "../rf/hud.h"
#include "../rf/misc.h"
#include "../main/main.h"
#include <common/config/BuildConfig.h>
#include <xlog/xlog.h>
#include <patch_common/CallHook.h>
#include <patch_common/FunHook.h>
#include <patch_common/AsmWriter.h>
#include <shlwapi.h>
#include <windows.h>

static rf::Player* g_spectate_mode_target;
static rf::Camera* g_old_target_camera = nullptr;
static bool g_spectate_mode_enabled = false;
static bool g_spawned_in_current_level = false;

static void set_camera_target(rf::Player* player)
{
    // Based on function set_camera1_view
    if (!rf::local_player || !rf::local_player->cam || !player)
        return;

    rf::Camera* camera = rf::local_player->cam;
    camera->mode = rf::CAMERA_FIRST_PERSON;
    camera->player = player;

    g_old_target_camera = player->cam;
    player->cam = camera; // fix crash 0040D744

    rf::camera_enter_first_person(camera);
}

static bool is_force_respawn()
{
    return g_spawned_in_current_level && (rf::netgame.flags & rf::NG_FLAG_FORCE_RESPAWN);
}

void multi_spectate_set_target_player(rf::Player* player)
{
    if (!player)
        player = rf::local_player;

    if (!rf::local_player || !rf::local_player->cam || !g_spectate_mode_target || g_spectate_mode_target == player)
        return;

    if (is_force_respawn()) {
        rf::String msg{"You cannot use Spectate Mode because Force Respawn option is enabled in this server!"};
        rf::String prefix;
        rf::multi_chat_print(msg, rf::ChatMsgColor::white_white, prefix);
        return;
    }

    // fix old target
    if (g_spectate_mode_target && g_spectate_mode_target != rf::local_player) {
        g_spectate_mode_target->cam = g_old_target_camera;
        g_old_target_camera = nullptr;

#if SPECTATE_MODE_SHOW_WEAPON
        g_spectate_mode_target->flags &= ~(1 << 4);
        rf::Entity* entity = rf::entity_from_handle(g_spectate_mode_target->entity_handle);
        if (entity)
            entity->local_player = nullptr;
#endif // SPECTATE_MODE_SHOW_WEAPON
    }

    g_spectate_mode_enabled = (player != rf::local_player);
    g_spectate_mode_target = player;

    rf::multi_kill_local_player();
    set_camera_target(player);

#if SPECTATE_MODE_SHOW_WEAPON
    player->flags |= 1 << 4;
    rf::Entity* entity = rf::entity_from_handle(player->entity_handle);
    if (entity) {
        // make sure weapon mesh is loaded now
        rf::player_fpgun_setup_mesh(player, entity->ai.current_primary_weapon);
        xlog::trace("FpgunMesh %p", player->weapon_mesh_handle);

        // Hide target player from camera
        entity->local_player = player;
    }
#endif // SPECTATE_MODE_SHOW_WEAPON
}

static void spectate_next_player(bool dir, bool try_alive_players_first = false)
{
    rf::Player* new_target;
    if (g_spectate_mode_enabled)
        new_target = g_spectate_mode_target;
    else
        new_target = rf::local_player;
    while (true) {
        new_target = dir ? new_target->next : new_target->prev;
        if (!new_target || new_target == g_spectate_mode_target)
            break; // nothing found
        if (try_alive_players_first && rf::player_is_dead(new_target))
            continue;
        if (new_target != rf::local_player) {
            multi_spectate_set_target_player(new_target);
            return;
        }
    }

    if (try_alive_players_first)
        spectate_next_player(dir, false);
}

void multi_spectate_enter_freelook()
{
    if (!rf::local_player || !rf::local_player->cam || !rf::is_multi)
        return;

    rf::multi_kill_local_player();
    rf::camera_enter_freelook(rf::local_player->cam);
}

bool multi_spectate_is_freelook()
{
    if (!rf::local_player || !rf::local_player->cam || !rf::is_multi)
        return false;

    auto camera_mode = rf::local_player->cam->mode;
    return camera_mode == rf::CAMERA_FREELOOK;
}

bool multi_spectate_is_spectating()
{
    return g_spectate_mode_enabled || multi_spectate_is_freelook();
}

rf::Player* multi_spectate_get_target_player()
{
    return g_spectate_mode_target;
}

void multi_spectate_leave()
{
    if (g_spectate_mode_enabled)
        multi_spectate_set_target_player(nullptr);
    else
        set_camera_target(rf::local_player);
}

bool multi_spectate_execute_action(rf::ControlConfigAction action, bool was_pressed)
{
    if (!rf::is_multi) {
        return false;
    }

    if (g_spectate_mode_enabled) {
        if (action == rf::CC_ACTION_PRIMARY_ATTACK || action == rf::CC_ACTION_SLIDE_RIGHT) {
            if (was_pressed)
                spectate_next_player(true);
            return true; // dont allow spawn
        }
        else if (action == rf::CC_ACTION_SECONDARY_ATTACK || action == rf::CC_ACTION_SLIDE_LEFT) {
            if (was_pressed)
                spectate_next_player(false);
            return true;
        }
        else if (action == rf::CC_ACTION_JUMP) {
            if (was_pressed)
                multi_spectate_leave();
            return true;
        }
    }
    else if (multi_spectate_is_freelook()) {
        // don't allow respawn in freelook spectate
        if (action == rf::CC_ACTION_PRIMARY_ATTACK || action == rf::CC_ACTION_SECONDARY_ATTACK) {
            if (was_pressed)
                multi_spectate_leave();
            return true;
        }
    }
    else if (!g_spectate_mode_enabled) {
        if (action == rf::CC_ACTION_JUMP && was_pressed && rf::player_is_dead(rf::local_player)) {
            multi_spectate_set_target_player(rf::local_player);
            spectate_next_player(true, true);
            return true;
        }
    }

    return false;
}

void multi_spectate_on_destroy_player(rf::Player* player)
{
    if (g_spectate_mode_target == player)
        spectate_next_player(true);
    if (g_spectate_mode_target == player)
        multi_spectate_set_target_player(nullptr);
}

FunHook<void(rf::Player*)> render_reticle_hook{
    0x0043A2C0,
    [](rf::Player* player) {
        if (rf::gameseq_get_state() == rf::GS_MULTI_LIMBO)
            return;
        if (g_spectate_mode_enabled)
            render_reticle_hook.call_target(g_spectate_mode_target);
        else
            render_reticle_hook.call_target(player);
    },
};

ConsoleCommand2 spectate_cmd{
    "spectate",
    [](std::optional<std::string> player_name) {
        if (!rf::is_multi) {
            rf::console_output("Spectate mode is only supported in multiplayer game!", nullptr);
            return;
        }

        if (is_force_respawn()) {
            rf::console_output("Spectate mode is disabled because of Force Respawn server option!", nullptr);
            return;
        }

        if (player_name) {
            // spectate player using 1st person view
            rf::Player* player = find_best_matching_player(player_name.value().c_str());
            if (!player) {
                // player not found
                return;
            }
            // player found - spectate
            multi_spectate_set_target_player(player);
        }
        else if (g_spectate_mode_enabled || multi_spectate_is_freelook()) {
            // leave spectate mode
            multi_spectate_leave();
        }
        else {
            // enter freelook spectate mode
            multi_spectate_enter_freelook();
        }
    },
    "Toggles spectate mode (first person or free-look depending on the argument)",
    "spectate [<player_name>]",
};

#if SPECTATE_MODE_SHOW_WEAPON

static void player_render_fpgun_new(rf::Player* player)
{
    if (g_spectate_mode_enabled) {
        rf::Entity* entity = rf::entity_from_handle(g_spectate_mode_target->entity_handle);

        // HACKFIX: RF uses function player_fpgun_set_remote_charge_visible for local player only
        g_spectate_mode_target->fpgun_data.remote_charge_in_hand =
            (entity && entity->ai.current_primary_weapon == rf::remote_charge_weapon_type);

        if (g_spectate_mode_target != rf::local_player && entity) {
            static rf::Vector3 old_vel;
            rf::Vector3 vel_diff = entity->p_data.vel - old_vel;
            old_vel = entity->p_data.vel;

            if (vel_diff.y > 0.1f)
                entity->entity_flags |= rf::EF_JUMP_START_ANIM; // jump
        }

        if (g_spectate_mode_target->fpgun_data.zooming_in)
            g_spectate_mode_target->fpgun_data.zoom_factor = 5.0f;
        rf::local_player->fpgun_data.zooming_in = g_spectate_mode_target->fpgun_data.zooming_in;
        rf::local_player->fpgun_data.zoom_factor = g_spectate_mode_target->fpgun_data.zoom_factor;

        rf::player_fpgun_process(g_spectate_mode_target);
        rf::player_fpgun_render(g_spectate_mode_target);
    }
    else
        rf::player_fpgun_render(player);
}

CallHook<float(rf::Player*)> gameplay_render_frame_player_fpgun_get_zoom_hook{
    0x00431B6D,
    [](rf::Player* pp) {
        if (g_spectate_mode_enabled) {
            return gameplay_render_frame_player_fpgun_get_zoom_hook.call_target(g_spectate_mode_target);
        }
        else {
            return gameplay_render_frame_player_fpgun_get_zoom_hook.call_target(pp);
        }
    },
};

#endif // SPECTATE_MODE_SHOW_WEAPON

void multi_spectate_appy_patch()
{
    render_reticle_hook.install();

    spectate_cmd.register_cmd();

    // Note: HUD rendering doesn't make sense because life and armor isn't synced

#if SPECTATE_MODE_SHOW_WEAPON

    AsmWriter(0x0043285D).call(player_render_fpgun_new);
    gameplay_render_frame_player_fpgun_get_zoom_hook.install();

    write_mem_ptr(0x0048857E + 2, &g_spectate_mode_target); // obj_mark_all_for_room
    write_mem_ptr(0x00488598 + 1, &g_spectate_mode_target); // obj_mark_all_for_room
    write_mem_ptr(0x00421889 + 2, &g_spectate_mode_target); // entity_render
    write_mem_ptr(0x004218A2 + 2, &g_spectate_mode_target); // entity_render
    write_mem_ptr(0x00458FB0 + 2, &g_spectate_mode_target); // item_render
    write_mem_ptr(0x00458FDF + 2, &g_spectate_mode_target); // item_render

    // Note: additional patches are in player_fpgun.cpp
#endif // SPECTATE_MODE_SHOW_WEAPON
}

void multi_spectate_after_full_game_init()
{
    g_spectate_mode_target = rf::local_player;
}

void multi_spectate_player_create_entity_post(rf::Player* player, rf::Entity* entity)
{
    // hide target player from camera after respawn
    if (g_spectate_mode_enabled && player == g_spectate_mode_target) {
        entity->local_player = player;
        // When entering limbo state the game changes camera mode to fixed
        // Make sure we are in first person mode when target entity spawns
        auto cam = rf::local_player->cam;
        if (cam->mode != rf::CAMERA_FIRST_PERSON) {
            rf::camera_enter_first_person(cam);
        }
    }
    // Do not allow spectating in Force Respawn game after spawning for the first time
    if (player == rf::local_player) {
        g_spawned_in_current_level = true;
    }
}

void multi_spectate_level_init()
{
    g_spawned_in_current_level = false;
}

template<typename F>
static void draw_with_shadow(int x, int y, int shadow_dx, int shadow_dy, rf::Color clr, rf::Color shadow_clr, F fun)
{
    rf::gr_set_color(shadow_clr);
    fun(x + shadow_dx, y + shadow_dy);
    rf::gr_set_color(clr);
    fun(x, y);
}

void multi_spectate_render()
{
    if (rf::hud_disabled || rf::gameseq_get_state() != rf::GS_GAMEPLAY || multi_spectate_is_freelook()) {
        return;
    }

    int large_font = hud_get_large_font();
    int large_font_h = rf::gr_get_font_height(large_font);
    int medium_font = hud_get_default_font();
    int medium_font_h = rf::gr_get_font_height(medium_font);

    int hints_x = 20;
    int hints_y = g_game_config.big_hud ? 350 : 200;
    int hints_line_spacing = medium_font_h + 3;
    if (!g_spectate_mode_enabled) {
        if (rf::player_is_dead(rf::local_player)) {
            rf::gr_set_color(0xFF, 0xFF, 0xFF, 0xFF);
            rf::gr_string(hints_x, hints_y, "Press JUMP key to enter Spectate Mode", medium_font);
        }
        return;
    }

    int scr_w = rf::gr_screen_width();
    int scr_h = rf::gr_screen_height();

    int title_x = scr_w / 2;
    int title_y = g_game_config.big_hud ? 250 : 150;
    rf::Color white_clr{255, 255, 255, 255};
    rf::Color shadow_clr{0, 0, 0, 128};
    draw_with_shadow(title_x, title_y, 2, 2, white_clr, shadow_clr, [=](int x, int y) {
        rf::gr_string_aligned(rf::GR_ALIGN_CENTER, x, y, "SPECTATE MODE", large_font);
    });

    rf::gr_set_color(0xFF, 0xFF, 0xFF, 0xFF);
    rf::gr_string(hints_x, hints_y, "Press JUMP key to exit Spectate Mode", medium_font);
    hints_y += hints_line_spacing;
    rf::gr_string(hints_x, hints_y, "Press PRIMARY ATTACK key to switch to the next player", medium_font);
    hints_y += hints_line_spacing;
    rf::gr_string(hints_x, hints_y, "Press SECONDARY ATTACK key to switch to the previous player", medium_font);

    const int bar_w = g_game_config.big_hud ? 800 : 500;
    const int bar_h = 50;
    int bar_x = (scr_w - bar_w) / 2;
    int bar_y = scr_h - 100;
    rf::gr_set_color(0, 0, 0x00, 0x60);
    rf::gr_rect(bar_x, bar_y, bar_w, bar_h);

    rf::gr_set_color(0xFF, 0xFF, 0, 0x80);
    auto str = string_format("Spectating: %s", g_spectate_mode_target->name.c_str());
    rf::gr_string_aligned(rf::GR_ALIGN_CENTER, bar_x + bar_w / 2, bar_y + bar_h / 2 - large_font_h / 2, str.c_str(), large_font);

    rf::Entity* entity = rf::entity_from_handle(g_spectate_mode_target->entity_handle);
    if (!entity) {
        rf::gr_set_color(0xFF, 0xFF, 0xFF, 0xFF);
        static int blood_bm = rf::bm_load("bloodsmear07_A.tga", -1, true);
        int blood_w, blood_h;
        rf::bm_get_dimensions(blood_bm, &blood_w, &blood_h);
        rf::gr_bitmap_scaled(blood_bm, (scr_w - blood_w * 2) / 2, (scr_h - blood_h * 2) / 2, blood_w * 2,
                             blood_h * 2, 0, 0, blood_w, blood_h, false, false, rf::gr_bitmap_clamp_mode);

        rf::Color dead_clr{0xF0, 0x20, 0x10, 0xC0};
        draw_with_shadow(scr_w / 2, scr_h / 2, 2, 2, dead_clr, shadow_clr, [=](int x, int y) {
            rf::gr_string_aligned(rf::GR_ALIGN_CENTER, x, y, "DEAD", large_font);
        });
    }
}
