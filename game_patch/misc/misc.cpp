#include <windows.h>
#include <common/version/version.h>
#include <common/config/BuildConfig.h>
#include <xlog/xlog.h>
#include <patch_common/AsmOpcodes.h>
#include <patch_common/AsmWriter.h>
#include <patch_common/CallHook.h>
#include <patch_common/CodeInjection.h>
#include <patch_common/FunHook.h>
#include <cstring>
#include "misc.h"
#include "../sound/sound.h"
#include "../os/console.h"
#include "../main/main.h"
#include "../rf/gr/gr.h"
#include "../rf/player/player.h"
#include "../rf/multi.h"
#include "../rf/gameseq.h"
#include "../rf/os/os.h"
#include "../rf/misc.h"
#include "../rf/vmesh.h"
#include "../rf/level.h"
#include "../rf/file/file.h"
#include "../object/object.h"

void apply_main_menu_patches();
void apply_save_restore_patches();
void apply_sound_patches();
void register_sound_commands();
void player_do_patch();
void player_fpgun_do_patch();
void g_solid_do_patch();
void camera_do_patch();
void ui_apply_patch();
void game_apply_patch();
void character_apply_patch();

struct JoinMpGameData
{
    rf::NetAddr addr;
    std::string password;
};

bool g_in_mp_game = false;
bool g_jump_to_multi_server_list = false;
std::optional<JoinMpGameData> g_join_mp_game_seq_data;

CodeInjection critical_error_hide_main_wnd_patch{
    0x0050BA90,
    []() {
        rf::gr::close();
        if (rf::main_wnd)
            ShowWindow(rf::main_wnd, SW_HIDE);
    },
};

CodeInjection critical_error_log_injection{
    0x0050BAE8,
    [](auto& regs) {
        const char* text = regs.ecx;
        xlog::error("Critical error:\n{}", text);
    },
};

CodeInjection level_read_data_check_restore_status_patch{
    0x00461195,
    [](auto& regs) {
        // check if sr_load_level_state is successful
        if (regs.eax != 0)
            return;
        // check if this is auto-load when changing level
        const char* save_filename = regs.edi;
        if (!std::strcmp(save_filename, "auto.svl"))
            return;
        // manual load failed
        xlog::error("Restoring game state failed");
        char* error_info = *reinterpret_cast<char**>(regs.esp + 0x2B0 + 0xC);
        std::strcpy(error_info, "Save file is corrupted");
        // return to level_read_data failure path
        regs.eip = 0x004608CC;
    },
};

void set_jump_to_multi_server_list(bool jump)
{
    g_jump_to_multi_server_list = jump;
}

void start_join_multi_game_sequence(const rf::NetAddr& addr, const std::string& password)
{
    g_jump_to_multi_server_list = true;
    g_join_mp_game_seq_data = {JoinMpGameData{addr, password}};
}

bool multi_join_game(const rf::NetAddr& addr, const std::string& password)
{
    auto multi_set_current_server_addr = addr_as_ref<void(const rf::NetAddr& addr)>(0x0044B380);
    auto send_join_req_packet = addr_as_ref<void(const rf::NetAddr& addr, rf::String::Pod name, rf::String::Pod password, int max_rate)>(0x0047AA40);

    if (rf::gameseq_get_state() != rf::GS_MULTI_SERVER_LIST) {
        return false;
    }

    rf::String password2{password.c_str()};
    g_join_mp_game_seq_data.reset();
    multi_set_current_server_addr(addr);
    send_join_req_packet(addr, rf::local_player->name, password2, rf::local_player->net_data->max_update_rate);
    return true;
}

FunHook<void(int, int)> rf_init_state_hook{
    0x004B1AC0,
    [](int state, int old_state) {
        rf_init_state_hook.call_target(state, old_state);
        xlog::trace("state {} old_state {} g_jump_to_multi_server_list {}", state, old_state, g_jump_to_multi_server_list);

        bool exiting_game = state == rf::GS_MAIN_MENU &&
            (old_state == rf::GS_END_GAME || old_state == rf::GS_NEW_LEVEL);
        if (exiting_game && g_in_mp_game) {
            g_in_mp_game = false;
            g_jump_to_multi_server_list = true;
        }

        if (state == rf::GS_MAIN_MENU && g_jump_to_multi_server_list) {
            xlog::trace("jump to mp menu!");
            set_sound_enabled(false);
            AddrCaller{0x00443C20}.c_call(); // open_multi_menu
            old_state = state;
            state = rf::gameseq_process_deferred_change();
            rf_init_state_hook.call_target(state, old_state);
        }
        if (state == rf::GS_MULTI_MENU && g_jump_to_multi_server_list) {
            AddrCaller{0x00448B70}.c_call(); // on_mp_join_game_btn_click
            old_state = state;
            state = rf::gameseq_process_deferred_change();
            rf_init_state_hook.call_target(state, old_state);
        }
        if (state == rf::GS_MULTI_SERVER_LIST && g_jump_to_multi_server_list) {
            g_jump_to_multi_server_list = false;
            set_sound_enabled(true);

            if (g_join_mp_game_seq_data) {
                auto addr = g_join_mp_game_seq_data.value().addr;
                auto password = g_join_mp_game_seq_data.value().password;
                g_join_mp_game_seq_data.reset();
                multi_join_game(addr, password);
            }
        }
    },
};

FunHook<bool(int)> rf_state_is_closed_hook{
    0x004B1DD0,
    [](int state) {
        if (g_jump_to_multi_server_list)
            return true;
        return rf_state_is_closed_hook.call_target(state);
    },
};

FunHook<void()> multi_after_players_packet_hook{
    0x00482080,
    []() {
        multi_after_players_packet_hook.call_target();
        g_in_mp_game = true;
    },
};

CodeInjection mover_rotating_keyframe_oob_crashfix{
    0x0046A559,
    [](auto& regs) {
        float& unk_time = *reinterpret_cast<float*>(regs.esi + 0x308);
        unk_time = 0;
        regs.eip = 0x0046A89D;
    }
};

CodeInjection parser_xstr_oob_fix{
    0x0051212E,
    [](auto& regs) {
        if (regs.edi >= 1000) {
            xlog::warn("XSTR index is out of bounds: {}!", static_cast<int>(regs.edi));
            regs.edi = -1;
        }
    }
};

CodeInjection ammo_tbl_buffer_overflow_fix{
    0x004C218E,
    [](auto& regs) {
        constexpr int max_ammo_types = 32;
        auto num_ammo_types = addr_as_ref<int>(0x0085C760);
        if (num_ammo_types == max_ammo_types) {
            xlog::warn("ammo.tbl limit of {} definitions has been reached!", max_ammo_types);
            regs.eip = 0x004C21B8;
        }
    },
};

CodeInjection clutter_tbl_buffer_overflow_fix{
    0x0040F49E,
    [](auto& regs) {
        constexpr int max_clutter_types = 450;
        if (regs.ecx == max_clutter_types) {
            xlog::warn("clutter.tbl limit of {} definitions has been reached!", max_clutter_types);
            regs.eip = 0x0040F4B0;
        }
    },
};

CodeInjection vclip_tbl_buffer_overflow_fix{
    0x004C1401,
    [](auto& regs) {
        constexpr int max_vclips = 64;
        auto num_vclips = addr_as_ref<int>(0x008568AC);
        if (num_vclips == max_vclips) {
            xlog::warn("vclip.tbl limit of {} definitions has been reached!", max_vclips);
            regs.eip = 0x004C1420;
        }
    },
};

CodeInjection items_tbl_buffer_overflow_fix{
    0x00458AD1,
    [](auto& regs) {
        constexpr int max_item_types = 96;
        auto num_item_types = addr_as_ref<int>(0x00644EA0);
        if (num_item_types == max_item_types) {
            xlog::warn("items.tbl limit of {} definitions has been reached!", max_item_types);
            regs.eip = 0x00458AFB;
        }
    },
};

CodeInjection explosion_tbl_buffer_overflow_fix{
    0x0048E0F3,
    [](auto& regs) {
        constexpr int max_explosion_types = 12;
        auto num_explosion_types = addr_as_ref<int>(0x0075EC44);
        if (num_explosion_types == max_explosion_types) {
            xlog::warn("explosion.tbl limit of {} definitions has been reached!", max_explosion_types);
            regs.eip = 0x0048E112;
        }
    },
};

CodeInjection weapons_tbl_primary_buffer_overflow_fix{
    0x004C6855,
    [](auto& regs) {
        constexpr int max_weapon_types = 64;
        auto num_weapon_types = addr_as_ref<int>(0x00872448);
        if (num_weapon_types == max_weapon_types) {
            xlog::warn("weapons.tbl limit of {} definitions has been reached!", max_weapon_types);
            regs.eip = 0x004C68D9;
        }
    },
};

CodeInjection weapons_tbl_secondary_buffer_overflow_fix{
    0x004C68AD,
    [](auto& regs) {
        constexpr int max_weapon_types = 64;
        auto num_weapon_types = addr_as_ref<int>(0x00872448);
        if (num_weapon_types == max_weapon_types) {
            xlog::warn("weapons.tbl limit of {} definitions has been reached!", max_weapon_types);
            regs.eip = 0x004C68D9;
        }
    },
};

CodeInjection pc_multi_tbl_buffer_overflow_fix{
    0x00475B50,
    [](auto& regs) {
        constexpr int max_multi_characters = 31;
        auto num_multi_characters = addr_as_ref<int>(0x006C9C60);
        if (num_multi_characters == max_multi_characters) {
            xlog::warn("pc_multi.tbl limit of {} definitions has been reached!", max_multi_characters);
            regs.eip = 0x0047621F;
        }
    },
};

CodeInjection emitters_tbl_buffer_overflow_fix{
    0x00496E76,
    [](auto& regs) {
        constexpr int max_emitter_types = 64;
        auto num_emitter_types = addr_as_ref<int>(0x006C9C60);
        if (num_emitter_types == max_emitter_types) {
            xlog::warn("emitters.tbl limit of {} definitions has been reached!", max_emitter_types);
            regs.eip = 0x00496F1A;
        }
    },
};

FunHook<void(const char*, int)> lcl_add_message_bof_fix{
    0x004B0720,
    [](const char* str, int id) {
        constexpr int xstr_size = 1000;
        if (id < xstr_size) {
            lcl_add_message_bof_fix.call_target(str, id);
        }
        else {
            xlog::warn("strings.tbl index is out of bounds: {}", id);
        }
    },
};

CodeInjection glass_shard_level_init_fix{
    0x00435A90,
    []() {
        auto glass_shard_level_init = addr_as_ref<void()>(0x00490F60);
        glass_shard_level_init();
    },
};

int debug_print_hook(char* buf, const char *fmt, ...) {
    va_list vl;
    va_start(vl, fmt);
    int ret = std::vsprintf(buf, fmt, vl);
    va_end(vl);
    xlog::warn("{}", buf);
    return ret;
}

CallHook<int(char*, const char*)> skeleton_pagein_debug_print_patch{
    0x0053AA73,
    reinterpret_cast<int(*)(char*, const char*)>(debug_print_hook),
};

CodeInjection level_load_items_crash_fix{
    0x0046519F,
    [](auto& regs) {
        void* item = regs.eax;
        if (item == nullptr) {
            regs.eip = 0x004651C6;
        }
    },
};

CodeInjection vmesh_col_fix{
    0x00499BCF,
    [](auto& regs) {
        auto stack_frame = regs.esp + 0xC8;
        auto& col_in = addr_as_ref<rf::VMeshCollisionInput>(regs.eax);
        // Reset flags field so start_pos/dir always gets transformed into mesh space
        // Note: MeshCollide function adds flag 2 after doing transformation into mesh space
        // If start_pos/dir is being updated for next call, flags must be reset as well
        col_in.flags = 0;
        // Reset dir field
        col_in.dir = addr_as_ref<rf::Vector3>(stack_frame - 0xAC);
    },
};

CodeInjection explosion_crash_fix{
    0x00436594,
    [](auto& regs) {
        rf::Player* player = regs.edx;
        if (player == nullptr) {
            regs.esp += 4;
            regs.eip = 0x004365EC;
        }
    },
};

CallHook<void(rf::Vector3*, float, float, float, float, bool, int, int)> level_read_geometry_header_light_add_directional_hook{
    0x004619E1,
    [](rf::Vector3 *dir, float intensity, float r, float g, float b, bool is_dynamic, int casts_shadow, int dropoff_type) {
        if (rf::gr::lighting_enabled()) {
            level_read_geometry_header_light_add_directional_hook.call_target(dir, intensity, r, g, b, is_dynamic, casts_shadow, dropoff_type);
        }
    },
};

CodeInjection vfile_read_stack_corruption_fix{
    0x0052D0E0,
    [](auto& regs) {
        regs.esi = regs.eax;
    },
};

CodeInjection game_set_file_paths_injection{
    0x004B1810,
    []() {
        if (rf::mod_param.found()) {
            std::string mod_dir = "mods\\";
            mod_dir += rf::mod_param.get_arg();
            rf::file_add_path(mod_dir.c_str(), ".bik" "bluebeard.bty", false);
        }
    },
};

CallHook level_init_pre_console_output_hook{
    0x00435ABB,
    []() {
        rf::console::print("-- Level Initializing: {} --", rf::level_filename_to_load);
    },
};

void misc_init()
{
    // Window title (client and server)
    write_mem_ptr(0x004B2790, PRODUCT_NAME);
    write_mem_ptr(0x004B27A4, PRODUCT_NAME);

#if NO_CD_FIX
    // No-CD fix
    write_mem<u8>(0x004B31B6, asm_opcodes::jmp_rel_short);
#endif // NO_CD_FIX

    // Disable thqlogo.bik
    if (g_game_config.fast_start) {
        write_mem<u8>(0x004B208A, asm_opcodes::jmp_rel_short);
        write_mem<u8>(0x004B24FD, asm_opcodes::jmp_rel_short);
    }

    // Crash-fix... (probably argument for function is invalid); Page Heap is needed
    write_mem<u32>(0x0056A28C + 1, 0);

    // Fix crash in shadows rendering
    write_mem<u8>(0x0054A3C0 + 2, 16);

    // Disable broken optimization of segment vs geometry collision test
    // Fixes hitting objects if mover is in the line of the shot
    AsmWriter(0x00499055).jmp(0x004990B4);

    // Disable Flamethower debug sphere drawing (optimization)
    // It is not visible in game because other things are drawn over it
    AsmWriter(0x0041AE47, 0x0041AE4C).nop();

    // Add checking if restoring game state from save file failed during level loading
    level_read_data_check_restore_status_patch.install();

    // Open server list menu instead of main menu when leaving multiplayer game
    rf_init_state_hook.install();
    rf_state_is_closed_hook.install();
    multi_after_players_packet_hook.install();

    // Hide main window when displaying critical error message box
    critical_error_hide_main_wnd_patch.install();

    // Log critical error message
    critical_error_log_injection.install();

    // Fix crash when skipping cutscene after robot kill in L7S4
    mover_rotating_keyframe_oob_crashfix.install();

    // Fix crash in LEGO_MP mod caused by XSTR(1000, "RL"); for some reason it does not crash in PF...
    parser_xstr_oob_fix.install();

    // Fix crashes caused by too many records in tbl files
    ammo_tbl_buffer_overflow_fix.install();
    clutter_tbl_buffer_overflow_fix.install();
    vclip_tbl_buffer_overflow_fix.install();
    items_tbl_buffer_overflow_fix.install();
    explosion_tbl_buffer_overflow_fix.install();
    weapons_tbl_primary_buffer_overflow_fix.install();
    weapons_tbl_secondary_buffer_overflow_fix.install();
    pc_multi_tbl_buffer_overflow_fix.install();
    emitters_tbl_buffer_overflow_fix.install();
    lcl_add_message_bof_fix.install();

    // Fix killed glass restoration from a save file
    AsmWriter(0x0043604A).nop(5);
    glass_shard_level_init_fix.install();

    // Log error when RFA cannot be loaded
    skeleton_pagein_debug_print_patch.install();

    // Fix item_create null result handling in RFL loading (affects multiplayer only)
    level_load_items_crash_fix.install();

    // Fix col-spheres vs mesh collisions
    vmesh_col_fix.install();

    // Fix crash caused by explosion near dying player-controlled entity (entity->local_player is null)
    explosion_crash_fix.install();

    // If speed reduction in background is not wanted disable that code in RF
    if (!g_game_config.reduced_speed_in_background) {
        write_mem<u8>(0x004353CC, asm_opcodes::jmp_rel_short);
    }

    // Fix dedicated server crash when loading level that uses directional light
    level_read_geometry_header_light_add_directional_hook.install();

    // Fix stack corruption when packfile has lower size than expected
    vfile_read_stack_corruption_fix.install();

    // Improve parse error message
    // For some reason RF replaces all characters with code lower than 0x20 (space) by character with code 0x16 (SYN)
    // in "Found this text" and "Prior text" sections. This makes all new line characters (CRLF) and tabs to be
    // replaced by ugly squeres. After this change only zero character is replaced.
    write_mem<char>(0x00512389 + 2, '\1');
    write_mem<char>(0x005123B6 + 2, '\1');

    // Do not render the level twice when Message Log is open (GS_MESSAGE_LOG game state is marked as transparent)
    AsmWriter{0x0045514E}.nop(5);
    AsmWriter{0x0045515B}.nop(5);

    // Add support for Bink videos in mods
    game_set_file_paths_injection.install();

    // Add level name to "-- Level Initializing --" message
    level_init_pre_console_output_hook.install();

    // Apply patches from other files
    apply_main_menu_patches();
    apply_save_restore_patches();
    apply_sound_patches();
    player_do_patch();
    player_fpgun_do_patch();
    g_solid_do_patch();
    register_sound_commands();
    camera_do_patch();
    ui_apply_patch();
    game_apply_patch();
    character_apply_patch();
}
