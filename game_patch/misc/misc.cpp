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

CodeInjection CriticalError_hide_main_wnd_patch{
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
    [](rf::GSolid* geom, rf::GRoom* room) {
        int ret = geo_cache_prepare_room_hook.call_target(geom, room);
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
        // check if SaveRestoreLoadAll is successful
        if (regs.eax)
            return;
        // check if this is auto-load when changing level
        const char* save_filename = reinterpret_cast<const char*>(regs.edi);
        if (!strcmp(save_filename, "auto.svl"))
            return;
        // manual load failed
        xlog::error("Restoring game state failed");
        char* error_info = *reinterpret_cast<char**>(regs.esp + 0x2B0 + 0xC);
        strcpy(error_info, "Save file is corrupted");
        // return to RflLoadInternal failure path
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

FunHook<char(int, int, int, int, char)> monitor_create_hook{
    0x00412470,
    [](int clutter_handle, int always_minus_1, int w, int h, char always_1) {
        if (g_game_config.high_monitor_res) {
            constexpr int factor = 2;
            w *= factor;
            h *= factor;
        }
        return monitor_create_hook.call_target(clutter_handle, always_minus_1, w, h, always_1);
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
        auto GlassShardInit = addr_as_ref<void()>(0x00490F60);
        GlassShardInit();
    },
};

FunHook<rf::Object*(int, int, int, rf::ObjectCreateInfo*, int, rf::GRoom*)> obj_create_hook{
    0x00486DA0,
    [](int type, int sub_type, int parent, rf::ObjectCreateInfo* create_info, int flags, rf::GRoom* room) {
        auto obj = obj_create_hook.call_target(type, sub_type, parent, create_info, flags, room);
        if (!obj) {
            xlog::info("Failed to create object (type %d)", type);
        }
        return obj;
    },
};

CodeInjection sort_items_patch{
    0x004593AC,
    [](auto& regs) {
        auto item = reinterpret_cast<rf::Item*>(regs.esi);
        auto vmesh = item->vmesh;
        auto mesh_name = vmesh ? rf::vmesh_get_name(vmesh) : nullptr;
        if (!mesh_name) {
            // Sometimes on level change some objects can stay and have only vmesh destroyed
            return;
        }
        std::string_view mesh_name_sv = mesh_name;

        // HACKFIX: enable alpha sorting for Invulnerability Powerup and Riot Shield
        // Note: material used for alpha-blending is flare_blue1.tga - it uses non-alpha texture
        // so information about alpha-blending cannot be taken from material alone - it must be read from VFX
        static const char* force_alpha_mesh_names[] = {
            "powerup_invuln.vfx",
            "Weapon_RiotShield.V3D",
        };
        for (auto alpha_mesh_name : force_alpha_mesh_names) {
            if (mesh_name_sv == alpha_mesh_name) {
                item->obj_flags |= rf::OF_HAS_ALPHA;
                break;
            }
        }

        rf::Item* current = rf::item_list.next;
        while (current != &rf::item_list) {
            auto current_anim_mesh = current->vmesh;
            auto current_mesh_name = current_anim_mesh ? rf::vmesh_get_name(current_anim_mesh) : nullptr;
            if (current_mesh_name && mesh_name_sv == current_mesh_name) {
                break;
            }
            current = current->next;
        }
        item->next = current;
        item->prev = current->prev;
        item->next->prev = item;
        item->prev->next = item;
        // Set up needed registers
        regs.ecx = regs.esp + 0xC0 - 0xB0; // create_info
        regs.eip = 0x004593D1;
    },
};

CodeInjection sort_clutter_patch{
    0x004109D4,
    [](auto& regs) {
        auto clutter = reinterpret_cast<rf::Clutter*>(regs.esi);
        auto vmesh = clutter->vmesh;
        auto mesh_name = vmesh ? rf::vmesh_get_name(vmesh) : nullptr;
        if (!mesh_name) {
            // Sometimes on level change some objects can stay and have only vmesh destroyed
            return;
        }
        std::string_view mesh_name_sv = mesh_name;

        auto& clutter_list = addr_as_ref<rf::Clutter>(0x005C9360);
        auto current = clutter_list.next;
        while (current != &clutter_list) {
            auto current_anim_mesh = current->vmesh;
            auto current_mesh_name = current_anim_mesh ? rf::vmesh_get_name(current_anim_mesh) : nullptr;
            if (current_mesh_name && mesh_name_sv == current_mesh_name) {
                break;
            }
            if (current_mesh_name && std::string_view{current_mesh_name} == "LavaTester01.v3d") {
                // HACKFIX: place LavaTester01 at the end to fix alpha draw order issues in L5S2 (Geothermal Plant)
                // Note: OF_HAS_ALPHA cannot be used because it causes another draw-order issue when lava goes up
                break;
            }
            current = current->next;
        }
        // insert before current
        clutter->next = current;
        clutter->prev = current->prev;
        clutter->next->prev = clutter;
        clutter->prev->next = clutter;
        // Set up needed registers
        regs.eax = addr_as_ref<bool>(regs.esp + 0xD0 + 0x18); // killable
        regs.ecx = addr_as_ref<i32>(0x005C9358) + 1; // num_clutter_objs
        regs.eip = 0x00410A03;
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

CodeInjection render_corpse_in_monitor_patch{
    0x00412905,
    []() {
        auto PlayerRenderHeldCorpse = addr_as_ref<void(rf::Player* player)>(0x004A2B90);
        PlayerRenderHeldCorpse(rf::local_player);
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

void __fastcall player_execute_action_timestamp_set_new(rf::Timestamp* fire_wait_timer, int, int value)
{
    if (!fire_wait_timer->valid() || fire_wait_timer->time_until() < value) {
        fire_wait_timer->set(value);
    }
}

CallHook<void __fastcall(rf::Timestamp*, int, int)> player_execute_action_timestamp_set_fire_wait_patch{
    {0x004A62C2u, 0x004A6325u},
    &player_execute_action_timestamp_set_new,
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

CallHook<bool(rf::Object*)> entity_check_is_in_liquid_obj_is_player_hook{
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
    // Buffer overflows in RflReadStaticGeometry
    // Note: Buffer size is 1024 but opcode allows only 1 byte size
    //       What is more important bm_load copies texture name to 32 bytes long buffers
    write_mem<i8>(0x004ED612 + 1, 32);
    write_mem<i8>(0x004ED66E + 1, 32);
    write_mem<i8>(0x004ED72E + 1, 32);
    write_mem<i8>(0x004EDB02 + 1, 32);
#endif

    // Increase damage for kill command in Single Player
    write_mem<float>(0x004A4DF5 + 1, 100000.0f);

    // Fix crash in shadows rendering
    write_mem<u8>(0x0054A3C0 + 2, 16);

    // Fix crash in geometry rendering
    geo_cache_prepare_room_hook.install();

    // Remove Sleep calls in TimerInit
    AsmWriter(0x00504A67, 0x00504A82).nop();

    // Use spawnpoint team property in TeamDM game (PF compatible)
    write_mem<u8>(0x00470395 + 4, 0); // change cmp argument: CTF -> DM
    write_mem<u8>(0x0047039A, asm_opcodes::jz_rel_short);  // invert jump condition: jnz -> jz

    // Disable broken optimization of segment vs geometry collision test
    // Fixes hitting objects if mover is in the line of the shot
    AsmWriter(0x00499055).jmp(0x004990B4);

    // Disable Flamethower debug sphere drawing (optimization)
    // It is not visible in game because other things are drawn over it
    AsmWriter(0x0041AE47, 0x0041AE4C).nop();

    // Preserve password case when processing rcon_request command
    write_mem<i8>(0x0046C85A + 1, 1);

    // Add checking if restoring game state from save file failed during level loading
    level_read_data_check_restore_status_patch.install();

    // Open server list menu instead of main menu when leaving multiplayer game
    rf_init_state_hook.install();
    rf_state_is_closed_hook.install();
    multi_after_players_packet_hook.install();

    // Hide main window when displaying critical error message box
    CriticalError_hide_main_wnd_patch.install();

    // Allow undefined mp_character in PlayerCreateEntity
    // Fixes Go_Undercover event not changing player 3rd person character
    AsmWriter(0x004A414F, 0x004A4153).nop();

    // High monitors/mirrors resolution
    monitor_create_hook.install();

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

    // Log error when object cannot be created
    obj_create_hook.install();

    // Use local_player variable for debris distance calculation instead of local_player_entity
    // Fixed debris pool being exhausted when local player is dead
    AsmWriter(0x0042A223, 0x0042A232).mov(asm_regs::ecx, {&rf::local_player});

    // Skip broken code that was supposed to skip particle emulation when particle emitter is in non-rendered room
    // RF code is broken here because level emitters have object handle set to 0 and other emitters are not added to
    // the searched list
    write_mem<u8>(0x00495158, asm_opcodes::jmp_rel_short);

    // Sort objects by anim mesh name to improve rendering performance
    sort_items_patch.install();
    sort_clutter_patch.install();

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

    // Fix ItemCreate null result handling in RFL loading (affects multiplayer only)
    level_load_items_crash_fix.install();

    // Fix col-spheres vs mesh collisions
    vmesh_col_fix.install();

    // Render held corpse in monitor
    render_corpse_in_monitor_patch.install();

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

    // Fix setting fire wait timer when closing weapon switch menu
    // Note: this timer makes sense for weapons that require holding (not clicking) the control to fire (e.g. shotgun)
    player_execute_action_timestamp_set_fire_wait_patch.install();

    // Fix crash when particle emitter allocation fails during entity ignition
    entity_fire_switch_parent_to_corpse_hook.install();

    // Fix buzzing sound when some player is floating in water
    entity_check_is_in_liquid_obj_is_player_hook.install();

    // Fix dedicated server crash when loading level that uses directional light
    level_read_geometry_header_light_add_directional_hook.install();

    // Fix stack corruption when packfile has lower size than expected
    vfile_read_stack_corruption_fix.install();

    // Init cmd line param
    get_url_cmd_line_param();

    // Fix memory corruption when transitioning to 5th level in a sequence and the level has no entry in ponr.tbl
    AsmWriter{0x004B3CAF, 0x004B3CB2}.xor_(asm_regs::ebx, asm_regs::ebx);

    // Apply patches from other files
    apply_event_patches();
    cutscene_apply_patches();
    glare_patches_patches();
    apply_limits_patches();
    apply_main_menu_patches();
    apply_save_restore_patches();
    apply_sound_patches();
    trigger_apply_patches();
    apply_weapon_patches();
    register_sound_commands();
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
