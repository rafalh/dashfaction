#include <windows.h>
#include <d3d8.h>
#include "misc.h"
#include "sound.h"
#include "../console/console.h"
#include "../main.h"
#include "../rf/object.h"
#include "../rf/entity.h"
#include "../rf/corpse.h"
#include "../rf/trigger.h"
#include "../rf/item.h"
#include "../rf/clutter.h"
#include "../rf/event.h"
#include "../rf/graphics.h"
#include "../rf/gr_direct3d.h"
#include "../rf/player.h"
#include "../rf/weapon.h"
#include "../rf/multi.h"
#include "../rf/gameseq.h"
#include "../rf/input.h"
#include "../rf/ui.h"
#include "../rf/particle_emitter.h"
#include "../rf/os.h"
#include "../rf/misc.h"
#include "../server/server.h"
#include <common/version.h>
#include <common/BuildConfig.h>
#include <xlog/xlog.h>
#include <patch_common/AsmOpcodes.h>
#include <patch_common/AsmWriter.h>
#include <patch_common/CallHook.h>
#include <patch_common/CodeInjection.h>
#include <patch_common/FunHook.h>
#include <patch_common/ShortTypes.h>
#include <cstddef>
#include <regex>

void cutscene_apply_patches();
void apply_event_patches();
void glare_patches_patches();
void apply_limits_patches();
void apply_main_menu_patches();
void do_level_specific_event_hacks(const char* level_filename);
void apply_save_restore_patches();
void apply_weapon_patches();
void apply_sound_patches();
void trigger_apply_patches();
void register_sound_commands();
void player_do_patch();
void player_fpgun_do_patch();
void obj_do_patch();
void monitor_do_patch();

struct JoinMpGameData
{
    rf::NwAddr addr;
    std::string password;
};

bool g_in_mp_game = false;
bool g_jump_to_multi_server_list = false;
std::optional<JoinMpGameData> g_join_mp_game_seq_data;

// Note: this must be called from DLL init function
// Note: we can't use global variable because that would lead to crash when launcher loads this DLL to check dependencies
static rf::CmdLineParam& get_url_cmd_line_param()
{
    static rf::CmdLineParam url_param{"-url", "", true};
    return url_param;
}

CodeInjection critical_error_hide_main_wnd_patch{
    0x0050BA90,
    []() {
        if (rf::gr_d3d_device)
            rf::gr_d3d_device->Release();
        if (rf::main_wnd)
            ShowWindow(rf::main_wnd, SW_HIDE);
    },
};

FunHook<int(rf::GSolid*, rf::GRoom*)> geo_cache_prepare_room_hook{
    0x004F0C00,
    [](rf::GSolid* solid, rf::GRoom* room) {
        int ret = geo_cache_prepare_room_hook.call_target(solid, room);
        std::byte** pp_room_geom = (std::byte**)(reinterpret_cast<std::byte*>(room) + 4);
        std::byte* room_geom = *pp_room_geom;
        if (ret == 0 && room_geom) {
            uint32_t room_vert_num = *reinterpret_cast<uint32_t*>(room_geom + 4);
            if (room_vert_num > 8000) {
                static int once = 0;
                if (!(once++))
                    xlog::warn("Not rendering room with %u vertices!", room_vert_num);
                *pp_room_geom = nullptr;
                return -1;
            }
        }
        return ret;
    },
};

CodeInjection level_read_data_check_restore_status_patch{
    0x00461195,
    [](auto& regs) {
        // check if sr_load_level_state is successful
        if (regs.eax)
            return;
        // check if this is auto-load when changing level
        const char* save_filename = reinterpret_cast<const char*>(regs.edi);
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

void start_join_multi_game_sequence(const rf::NwAddr& addr, const std::string& password)
{
    g_jump_to_multi_server_list = true;
    g_join_mp_game_seq_data = {JoinMpGameData{addr, password}};
}

FunHook<void(int, int)> rf_init_state_hook{
    0x004B1AC0,
    [](int state, int old_state) {
        rf_init_state_hook.call_target(state, old_state);
        xlog::trace("state %d old_state %d g_jump_to_multi_server_list %d", state, old_state, g_jump_to_multi_server_list);

        bool exiting_game = state == rf::GS_MAIN_MENU &&
            (old_state == rf::GS_EXIT_GAME || old_state == rf::GS_LOADING_LEVEL);
        if (exiting_game && g_in_mp_game) {
            g_in_mp_game = false;
            g_jump_to_multi_server_list = true;
        }

        if (state == rf::GS_MAIN_MENU && g_jump_to_multi_server_list) {
            xlog::trace("jump to mp menu!");
            set_sound_enabled(false);
            AddrCaller{0x00443C20}.c_call(); // OpenMultiMenu
            old_state = state;
            state = rf::gameseq_process_deferred_change();
            rf_init_state_hook.call_target(state, old_state);
        }
        if (state == rf::GS_MULTI_MENU && g_jump_to_multi_server_list) {
            AddrCaller{0x00448B70}.c_call(); // OnMpJoinGameBtnClick
            old_state = state;
            state = rf::gameseq_process_deferred_change();
            rf_init_state_hook.call_target(state, old_state);
        }
        if (state == rf::GS_MULTI_SERVER_LIST && g_jump_to_multi_server_list) {
            g_jump_to_multi_server_list = false;
            set_sound_enabled(true);

            if (g_join_mp_game_seq_data) {
                auto MultiSetCurrentServerAddr = addr_as_ref<void(const rf::NwAddr& addr)>(0x0044B380);
                auto SendJoinReqPacket = addr_as_ref<void(const rf::NwAddr& addr, rf::String::Pod name, rf::String::Pod password, int max_rate)>(0x0047AA40);

                auto addr = g_join_mp_game_seq_data.value().addr;
                rf::String password{g_join_mp_game_seq_data.value().password.c_str()};
                g_join_mp_game_seq_data.reset();
                MultiSetCurrentServerAddr(addr);
                SendJoinReqPacket(addr, rf::local_player->name, password, rf::local_player->nw_data->max_update_rate);
            }
        }

        if (state == rf::GS_MULTI_LIMBO) {
            server_on_limbo_state_enter();
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
            xlog::warn("XSTR index is out of bounds: %d!", regs.edi);
            regs.edi = -1;
        }
    }
};

CodeInjection ammo_tbl_buffer_overflow_fix{
    0x004C218E,
    [](auto& regs) {
        if (addr_as_ref<u32>(0x0085C760) == 32) {
            xlog::warn("ammo.tbl limit of 32 definitions has been reached!");
            regs.eip = 0x004C21B8;
        }
    },
};

CodeInjection clutter_tbl_buffer_overflow_fix{
    0x0040F49E,
    [](auto& regs) {
        if (regs.ecx == 450) {
            xlog::warn("clutter.tbl limit of 450 definitions has been reached!");
            regs.eip = 0x0040F4B0;
        }
    },
};

FunHook<void(const char*, int)> localize_add_string_bof_fix{
    0x004B0720,
    [](const char* str, int id) {
        if (id < 1000) {
            localize_add_string_bof_fix.call_target(str, id);
        }
        else {
            xlog::warn("strings.tbl index is out of bounds: %d", id);
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

CodeInjection face_scroll_fix{
    0x004EE1D6,
    [](auto& regs) {
        auto geometry = reinterpret_cast<void*>(regs.ebp);
        auto& scroll_data_vec = struct_field_ref<rf::VArray<void*>>(geometry, 0x2F4);
        auto GTextureMover_SetupFaces = reinterpret_cast<void(__thiscall*)(void* self, void* geometry)>(0x004E60C0);
        for (int i = 0; i < scroll_data_vec.size(); ++i) {
            GTextureMover_SetupFaces(scroll_data_vec[i], geometry);
        }
    },
};

CallHook<void(int)> play_bik_file_vram_leak_fix{
    0x00520C79,
    [](int hbm) {
        auto gr_tcache_add_ref = addr_as_ref<void(int hbm)>(0x0050E850);
        auto gr_tcache_remove_ref = addr_as_ref<void(int hbm)>(0x0050E870);
        gr_tcache_add_ref(hbm);
        gr_tcache_remove_ref(hbm);
        play_bik_file_vram_leak_fix.call_target(hbm);
    },
};

int debug_print_hook(char* buf, const char *fmt, ...) {
    va_list vl;
    va_start(vl, fmt);
    int ret = vsprintf(buf, fmt, vl);
    va_end(vl);
    xlog::error("%s", buf);
    return ret;
}

CallHook<int(char*, const char*)> skeleton_pagein_debug_print_patch{
    0x0053AA73,
    reinterpret_cast<int(*)(char*, const char*)>(debug_print_hook),
};

CodeInjection level_load_items_crash_fix{
    0x0046519F,
    [](auto& regs) {
        if (!regs.eax) {
            regs.eip = 0x004651C6;
        }
    },
};

CodeInjection vmesh_col_fix{
    0x00499BCF,
    [](auto& regs) {
        auto stack_frame = regs.esp + 0xC8;
        auto params = reinterpret_cast<void*>(regs.eax);
        // Reset flags field so start_pos/dir always gets transformed into mesh space
        // Note: MeshCollide function adds flag 2 after doing transformation into mesh space
        // If start_pos/dir is being updated for next call, flags must be reset as well
        struct_field_ref<int>(params, 0x4C) = 0;
        // Reset dir field
        struct_field_ref<rf::Vector3>(params, 0x3C) = addr_as_ref<rf::Vector3>(stack_frame - 0xAC);
    },
};

CodeInjection play_bik_file_infinite_loop_fix{
    0x00520BEE,
    [](auto& regs) {
        if (!regs.eax) {
            // pop edi
            regs.edi = *reinterpret_cast<int*>(regs.esp);
            regs.esp += 4;
            regs.eip = 0x00520C6E;
        }
    },
};

CodeInjection explosion_crash_fix{
    0x00436594,
    [](auto& regs) {
        if (!regs.edx) {
            regs.esp += 4;
            regs.eip = 0x004365EC;
        }
    },
};

FunHook<void(rf::EntityFireInfo&, int)> entity_fire_switch_parent_to_corpse_hook{
    0x0042F510,
    [](rf::EntityFireInfo& burn_info, int corpse_handle) {
        auto corpse = rf::corpse_from_handle(corpse_handle);
        burn_info.parent_handle = corpse_handle;
        rf::entity_fire_init_bones(&burn_info, corpse);
        for (auto& emitter_ptr : burn_info.emitters) {
            if (emitter_ptr) {
                emitter_ptr->parent_handle = corpse_handle;
            }
        }
        burn_info.field_2C = 1;
        burn_info.field_30 = 0.0f;
        corpse->corpse_flags |= 0x200;
    },
};

CallHook<bool(rf::Object*)> entity_update_liquid_status_obj_is_player_hook{
    {
        0x004292E3,
        0x0042932A,
        0x004293F4,
    },
    [](rf::Object* obj) {
        return obj == rf::local_player_entity;
    },
};

CallHook<void(rf::Vector3*, float, float, float, float, bool, int, int)> level_read_geometry_header_light_add_directional_hook{
    0x004619E1,
    [](rf::Vector3 *dir, float intensity, float r, float g, float b, bool is_dynamic, int casts_shadow, int dropoff_type) {
        auto LightingIsEnabled = addr_as_ref<bool()>(0x004DB8B0);
        if (LightingIsEnabled()) {
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

CallHook<bool(const rf::Vector3&, const rf::Vector3&, rf::PhysicsData*, rf::PCollisionOut*)> entity_maybe_stop_crouching_collide_spheres_world_hook{
    0x00428AB9,
    [](const rf::Vector3& p1, const rf::Vector3& p2, rf::PhysicsData* pd, rf::PCollisionOut* collision) {
        // Temporarly disable collisions with liquid faces
        auto collision_flags = pd->collision_flags;
        pd->collision_flags &= ~0x1000;
        bool result = entity_maybe_stop_crouching_collide_spheres_world_hook.call_target(p1, p2, pd, collision);
        pd->collision_flags = collision_flags;
        return result;
    },
};

void misc_after_level_load(const char* level_filename)
{
    do_level_specific_event_hacks(level_filename);
    clear_triggers_for_late_joiners();
}

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

#if 1
    // Buffer overflows in solid_read
    // Note: Buffer size is 1024 but opcode allows only 1 byte size
    //       What is more important bm_load copies texture name to 32 bytes long buffers
    write_mem<i8>(0x004ED612 + 1, 32);
    write_mem<i8>(0x004ED66E + 1, 32);
    write_mem<i8>(0x004ED72E + 1, 32);
    write_mem<i8>(0x004EDB02 + 1, 32);
#endif

    // Fix crash in shadows rendering
    write_mem<u8>(0x0054A3C0 + 2, 16);

    // Fix crash in geometry rendering
    geo_cache_prepare_room_hook.install();

    // Remove Sleep calls in timer_init
    AsmWriter(0x00504A67, 0x00504A82).nop();

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

    // Fix crash when skipping cutscene after robot kill in L7S4
    mover_rotating_keyframe_oob_crashfix.install();

    // Fix crash in LEGO_MP mod caused by XSTR(1000, "RL"); for some reason it does not crash in PF...
    parser_xstr_oob_fix.install();

    // Fix crashes caused by too many records in tbl files
    ammo_tbl_buffer_overflow_fix.install();
    clutter_tbl_buffer_overflow_fix.install();
    localize_add_string_bof_fix.install();

    // Fix killed glass restoration from a save file
    AsmWriter(0x0043604A).nop(5);
    glass_shard_level_init_fix.install();

    // Use local_player variable for debris distance calculation instead of local_player_entity
    // Fixed debris pool being exhausted when local player is dead
    AsmWriter(0x0042A223, 0x0042A232).mov(asm_regs::ecx, {&rf::local_player});

    // Skip broken code that was supposed to skip particle emulation when particle emitter is in non-rendered room
    // RF code is broken here because level emitters have object handle set to 0 and other emitters are not added to
    // the searched list
    write_mem<u8>(0x00495158, asm_opcodes::jmp_rel_short);

    // Fix face scroll in levels after version 0xB4
    face_scroll_fix.install();

    // Increase entity simulation max distance
    // TODO: create a config property for this
    if (g_game_config.disable_lod_models) {
        write_mem<float>(0x00589548, 100.0f);
    }

    // Fix PlayBikFile texture leak
    play_bik_file_vram_leak_fix.install();

    // Log error when RFA cannot be loaded
    skeleton_pagein_debug_print_patch.install();

    // Fix crash when executing camera2 command in main menu
    AsmWriter(0x0040DCFC).nop(5);

    // Fix item_create null result handling in RFL loading (affects multiplayer only)
    level_load_items_crash_fix.install();

    // Fix col-spheres vs mesh collisions
    vmesh_col_fix.install();

    // Fix possible infinite loop when starting Bink video
    play_bik_file_infinite_loop_fix.install();

    // Fix memory leak when trying to play non-existing Bink video
    write_mem<i32>(0x00520B7E + 2, 0x00520C6E - (0x00520B7E + 6));

    // Fix crash caused by explosion near dying player-controlled entity (entity->local_player is null)
    explosion_crash_fix.install();

    // If speed reduction in background is not wanted disable that code in RF
    if (!g_game_config.reduced_speed_in_background) {
        write_mem<u8>(0x004353CC, asm_opcodes::jmp_rel_short);
    }

    // Fix crash when particle emitter allocation fails during entity ignition
    entity_fire_switch_parent_to_corpse_hook.install();

    // Fix buzzing sound when some player is floating in water
    entity_update_liquid_status_obj_is_player_hook.install();

    // Fix dedicated server crash when loading level that uses directional light
    level_read_geometry_header_light_add_directional_hook.install();

    // Fix stack corruption when packfile has lower size than expected
    vfile_read_stack_corruption_fix.install();

    // Fix entity staying in crouched state after entering liquid
    entity_maybe_stop_crouching_collide_spheres_world_hook.install();

    // Init cmd line param
    get_url_cmd_line_param();

    // Apply patches from other files
    apply_event_patches();
    cutscene_apply_patches();
    glare_patches_patches();
    obj_do_patch();
    apply_main_menu_patches();
    apply_save_restore_patches();
    apply_sound_patches();
    trigger_apply_patches();
    apply_weapon_patches();
    player_do_patch();
    player_fpgun_do_patch();
    monitor_do_patch();
    register_sound_commands();


    static CodeInjection entity_maybe_stop_crouching_injection{
        0x00428A60,
        []() {
            xlog::warn("entity_maybe_stop_crouching");
        },
    };
    entity_maybe_stop_crouching_injection.install();
    static CodeInjection entity_crouch_injection{
        0x004289D0,
        []() {
            xlog::warn("entity_crouch");
        },
    };
    entity_crouch_injection.install();
    static CodeInjection entity_make_swim_injection{
        0x00428270,
        []() {
            xlog::warn("entity_make_swim");
        },
    };
    entity_make_swim_injection.install();
}

void handle_url_param()
{
    if (!get_url_cmd_line_param().found()) {
        return;
    }

    auto url = get_url_cmd_line_param().get_arg();
    std::regex e("^rf://([\\w\\.-]+):(\\d+)/?(?:\\?password=(.*))?$");
    std::cmatch cm;
    if (!std::regex_match(url, cm, e)) {
        xlog::warn("Unsupported URL: %s", url);
        return;
    }

    auto host_name = cm[1].str();
    auto port = static_cast<u16>(std::stoi(cm[2].str()));
    auto password = cm[3].str();

    auto hp = gethostbyname(host_name.c_str());
    if (!hp) {
        xlog::warn("URL host lookup failed");
        return;
    }

    if (hp->h_addrtype != AF_INET) {
        xlog::warn("Unsupported address type (only IPv4 is supported)");
        return;
    }

    rf::console_printf("Connecting to %s:%d...", host_name.c_str(), port);
    auto host = ntohl(reinterpret_cast<in_addr *>(hp->h_addr_list[0])->S_un.S_addr);

    rf::NwAddr addr{host, port};
    start_join_multi_game_sequence(addr, password);
}

void misc_after_full_game_init()
{
    handle_url_param();
}
