#include <windows.h>
#include <d3d8.h>
#include "misc.h"
#include "sound.h"
#include "../console/console.h"
#include "../main.h"
#include "../rf/object.h"
#include "../rf/entity.h"
#include "../rf/trigger.h"
#include "../rf/item.h"
#include "../rf/clutter.h"
#include "../rf/event.h"
#include "../rf/graphics.h"
#include "../rf/player.h"
#include "../rf/weapon.h"
#include "../rf/network.h"
#include "../rf/game_seq.h"
#include "../rf/input.h"
#include "../stdafx.h"
#include "../server/server.h"
#include <common/version.h>
#include <common/BuildConfig.h>
#include <patch_common/CallHook.h>
#include <patch_common/FunHook.h>
#include <patch_common/CodeInjection.h>
#include <patch_common/ShortTypes.h>
#include <cstddef>
#include <regex>

namespace rf
{

static auto& GameSeqPushState = AddrAsRef<int(int state, bool update_parent_state, bool parent_dlg_open)>(0x00434410);
static auto& GameSeqProcessDeferredChange = AddrAsRef<int()>(0x00434310);
auto AnimMeshGetName = AddrAsRef<const char*(rf::AnimMesh* anim_mesh)>(0x00503470);

} // namespace rf

void ApplyCutscenePatches();
void ApplyEventPatches();
void ApplyGlarePatches();
void ApplyLimitsPatches();
void ApplyMainMenuPatches();
void DoLevelSpecificEventHacks(const char* level_filename);
void ApplySaveRestorePatches();
void ApplyWeaponPatches();
void ApplySoundPatches();
void ApplyTriggerPatches();
void RegisterSoundCommands();

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
static rf::CmdLineParam& GetUrlCmdLineParam()
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

FunHook<int(void*, void*)> GeomCachePrepareRoom_hook{
    0x004F0C00,
    [](void* geom, void* room) {
        int ret = GeomCachePrepareRoom_hook.CallTarget(geom, room);
        std::byte** pp_room_geom = (std::byte**)(static_cast<std::byte*>(room) + 4);
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

CodeInjection RflLoadInternal_CheckRestoreStatus_patch{
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

void SetJumpToMultiServerList(bool jump)
{
    g_jump_to_multi_server_list = jump;
}

void StartJoinMpGameSequence(const rf::NwAddr& addr, const std::string& password)
{
    g_jump_to_multi_server_list = true;
    g_join_mp_game_seq_data = {JoinMpGameData{addr, password}};
}

FunHook<void(int, int)> GameEnterState_hook{
    0x004B1AC0,
    [](int state, int old_state) {
        GameEnterState_hook.CallTarget(state, old_state);
        xlog::trace("state %d old_state %d g_jump_to_multi_server_list %d", state, old_state, g_jump_to_multi_server_list);

        bool exiting_game = state == rf::GS_MAIN_MENU &&
            (old_state == rf::GS_EXIT_GAME || old_state == rf::GS_LOADING_LEVEL);
        if (exiting_game && g_in_mp_game) {
            g_in_mp_game = false;
            g_jump_to_multi_server_list = true;
        }

        if (state == rf::GS_MAIN_MENU && g_jump_to_multi_server_list) {
            xlog::trace("jump to mp menu!");
            SetSoundEnabled(false);
            AddrCaller{0x00443C20}.c_call(); // OpenMultiMenu
            old_state = state;
            state = rf::GameSeqProcessDeferredChange();
            GameEnterState_hook.CallTarget(state, old_state);
        }
        if (state == rf::GS_MP_MENU && g_jump_to_multi_server_list) {
            AddrCaller{0x00448B70}.c_call(); // OnMpJoinGameBtnClick
            old_state = state;
            state = rf::GameSeqProcessDeferredChange();
            GameEnterState_hook.CallTarget(state, old_state);
        }
        if (state == rf::GS_MP_SERVER_LIST_MENU && g_jump_to_multi_server_list) {
            g_jump_to_multi_server_list = false;
            SetSoundEnabled(true);

            if (g_join_mp_game_seq_data) {
                auto MultiSetCurrentServerAddr = AddrAsRef<void(const rf::NwAddr& addr)>(0x0044B380);
                auto SendJoinReqPacket = AddrAsRef<void(const rf::NwAddr& addr, rf::String::Pod name, rf::String::Pod password, int max_rate)>(0x0047AA40);

                auto addr = g_join_mp_game_seq_data.value().addr;
                rf::String password{g_join_mp_game_seq_data.value().password.c_str()};
                g_join_mp_game_seq_data.reset();
                MultiSetCurrentServerAddr(addr);
                SendJoinReqPacket(addr, rf::local_player->name, password, rf::local_player->nw_data->max_update_rate);
            }
        }

        if (state == rf::GS_MP_LIMBO) {
            ServerOnLimboStateEnter();
        }
    },
};

FunHook<bool(int)> IsGameStateUiHidden_hook{
    0x004B1DD0,
    [](int state) {
        if (g_jump_to_multi_server_list)
            return true;
        return IsGameStateUiHidden_hook.CallTarget(state);
    },
};

FunHook<void()> MultiAfterPlayersPackets_hook{
    0x00482080,
    []() {
        MultiAfterPlayersPackets_hook.CallTarget();
        g_in_mp_game = true;
    },
};

FunHook<char(int, int, int, int, char)> ClutterInitMonitor_hook{
    0x00412470,
    [](int clutter_handle, int always_minus_1, int w, int h, char always_1) {
        if (g_game_config.high_monitor_res) {
            constexpr int factor = 2;
            w *= factor;
            h *= factor;
        }
        return ClutterInitMonitor_hook.CallTarget(clutter_handle, always_minus_1, w, h, always_1);
    },
};

CodeInjection moving_group_rotate_in_place_keyframe_oob_crashfix{
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
        if (AddrAsRef<u32>(0x0085C760) == 32) {
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

FunHook<void(const char*, int)> strings_tbl_buffer_overflow_fix{
    0x004B0720,
    [](const char* str, int id) {
        if (id < 1000) {
            strings_tbl_buffer_overflow_fix.CallTarget(str, id);
        }
        else {
            xlog::warn("strings.tbl index is out of bounds: %d", id);
        }
    },
};

CodeInjection glass_kill_init_fix{
    0x00435A90,
    []() {
        auto GlassKillInit = AddrAsRef<void()>(0x00490F60);
        GlassKillInit();
    },
};

FunHook<rf::Object*(int, int, int, void*, int, void*)> ObjCreate_hook{
    0x00486DA0,
    [](int type, int sub_type, int parent, void* create_info, int flags, void* room) {
        auto obj = ObjCreate_hook.CallTarget(type, sub_type, parent, create_info, flags, room);
        if (!obj) {
            xlog::warn("Failed to create object (type %d)", type);
        }
        return obj;
    },
};

CodeInjection sort_items_patch{
    0x004593AC,
    [](auto& regs) {
        auto item = reinterpret_cast<rf::ItemObj*>(regs.esi);
        auto anim_mesh = item->anim_mesh;
        auto mesh_name = anim_mesh ? rf::AnimMeshGetName(anim_mesh) : nullptr;
        if (!mesh_name) {
            // Sometimes on level change some objects can stay and have only anim_mesh destroyed
            return;
        }

        // HACKFIX: enable alpha sorting for Invulnerability Powerup
        // Note: material used for alpha-blending is flare_blue1.tga - it uses non-alpha texture
        // so information about alpha-blending cannot be taken from material alone - it must be read from VFX
        if (!strcmp(mesh_name, "powerup_invuln.vfx")) {
            item->obj_flags |= 0x100000; // OF_HAS_ALPHA
        }

        auto& item_obj_list = AddrAsRef<rf::ItemObj>(0x00642DD8);
        rf::ItemObj* current = item_obj_list.next;
        while (current != &item_obj_list) {
            auto current_anim_mesh = current->anim_mesh;
            auto current_mesh_name = current_anim_mesh ? rf::AnimMeshGetName(current_anim_mesh) : nullptr;
            if (current_mesh_name && !strcmp(mesh_name, current_mesh_name)) {
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
        auto clutter = reinterpret_cast<rf::ClutterObj*>(regs.esi);
        auto anim_mesh = clutter->anim_mesh;
        auto mesh_name = anim_mesh ? rf::AnimMeshGetName(anim_mesh) : nullptr;
        if (!mesh_name) {
            // Sometimes on level change some objects can stay and have only anim_mesh destroyed
            return;
        }
        auto& clutter_obj_list = AddrAsRef<rf::ClutterObj>(0x005C9360);
        auto current = clutter_obj_list.next;
        while (current != &clutter_obj_list) {
            auto current_anim_mesh = current->anim_mesh;
            auto current_mesh_name = current_anim_mesh ? rf::AnimMeshGetName(current_anim_mesh) : nullptr;
            if (current_mesh_name && !strcmp(mesh_name, current_mesh_name)) {
                break;
            }
            current = current->next;
        }
        clutter->next = current;
        clutter->prev = current->prev;
        clutter->next->prev = clutter;
        clutter->prev->next = clutter;
        // Set up needed registers
        regs.eax = AddrAsRef<bool>(regs.esp + 0xD0 + 0x18); // killable
        regs.ecx = AddrAsRef<i32>(0x005C9358) + 1; // num_clutter_objs
        regs.eip = 0x00410A03;
    },
};

CodeInjection face_scroll_fix{
    0x004EE1D6,
    [](auto& regs) {
        auto geometry = reinterpret_cast<void*>(regs.ebp);
        auto& scroll_data_vec = StructFieldRef<rf::DynamicArray<void*>>(geometry, 0x2F4);
        auto RflFaceScroll_SetupFaces = reinterpret_cast<void(__thiscall*)(void* self, void* geometry)>(0x004E60C0);
        for (int i = 0; i < scroll_data_vec.Size(); ++i) {
            RflFaceScroll_SetupFaces(scroll_data_vec.Get(i), geometry);
        }
    },
};

CallHook<void(int)> play_bik_file_vram_leak_fix{
    0x00520C79,
    [](int hbm) {
        auto gr_tcache_add_ref = AddrAsRef<void(int hbm)>(0x0050E850);
        auto gr_tcache_remove_ref = AddrAsRef<void(int hbm)>(0x0050E870);
        gr_tcache_add_ref(hbm);
        gr_tcache_remove_ref(hbm);
        play_bik_file_vram_leak_fix.CallTarget(hbm);
    },
};

int DebugPrintHook(char* buf, const char *fmt, ...) {
    va_list vl;
    va_start(vl, fmt);
    int ret = vsprintf(buf, fmt, vl);
    va_end(vl);
    xlog::error("%s", buf);
    return ret;
}

CallHook<int(char*, const char*)> mvf_load_rfa_debug_print_patch{
    0x0053AA73,
    reinterpret_cast<int(*)(char*, const char*)>(DebugPrintHook),
};

CodeInjection rfl_load_items_crash_fix{
    0x0046519F,
    [](auto& regs) {
        if (!regs.eax) {
            regs.eip = 0x004651C6;
        }
    },
};

CodeInjection anim_mesh_col_fix{
    0x00499BCF,
    [](auto& regs) {
        auto stack_frame = regs.esp + 0xC8;
        auto params = reinterpret_cast<void*>(regs.eax);
        // Reset flags field so start_pos/dir always gets transformed into mesh space
        // Note: MeshCollide function adds flag 2 after doing transformation into mesh space
        // If start_pos/dir is being updated for next call, flags must be reset as well
        StructFieldRef<int>(params, 0x4C) = 0;
        // Reset dir field
        StructFieldRef<rf::Vector3>(params, 0x3C) = AddrAsRef<rf::Vector3>(stack_frame - 0xAC);
    },
};

CodeInjection render_corpse_in_monitor_patch{
    0x00412905,
    []() {
        auto PlayerRenderHeldCorpse = AddrAsRef<void(rf::Player* player)>(0x004A2B90);
        PlayerRenderHeldCorpse(rf::local_player);
    },
};

CodeInjection PlayBikFile_infinite_loop_fix{
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

void __fastcall HandleCtrlInGame_TimerSet_New(rf::Timer* fire_wait_timer, int, int value)
{
    if (!fire_wait_timer->IsSet() || fire_wait_timer->GetTimeLeftMs() < value) {
        fire_wait_timer->Set(value);
    }
}

CallHook<void __fastcall(rf::Timer*, int, int)> HandleCtrlInGame_TimerSet_fire_wait_patch{
    {0x004A62C2u, 0x004A6325u},
    &HandleCtrlInGame_TimerSet_New,
};

void MiscAfterLevelLoad(const char* level_filename)
{
    DoLevelSpecificEventHacks(level_filename);
    ClearTriggersForLateJoiners();
}

void MiscInit()
{
    // Window title (client and server)
    WriteMemPtr(0x004B2790, PRODUCT_NAME);
    WriteMemPtr(0x004B27A4, PRODUCT_NAME);

#if NO_CD_FIX
    // No-CD fix
    WriteMem<u8>(0x004B31B6, asm_opcodes::jmp_rel_short);
#endif // NO_CD_FIX

    // Disable thqlogo.bik
    if (g_game_config.fast_start) {
        WriteMem<u8>(0x004B208A, asm_opcodes::jmp_rel_short);
        WriteMem<u8>(0x004B24FD, asm_opcodes::jmp_rel_short);
    }

    // Crash-fix... (probably argument for function is invalid); Page Heap is needed
    WriteMem<u32>(0x0056A28C + 1, 0);

#if 1
    // Buffer overflows in RflReadStaticGeometry
    // Note: Buffer size is 1024 but opcode allows only 1 byte size
    //       What is more important BmLoad copies texture name to 32 bytes long buffers
    WriteMem<i8>(0x004ED612 + 1, 32);
    WriteMem<i8>(0x004ED66E + 1, 32);
    WriteMem<i8>(0x004ED72E + 1, 32);
    WriteMem<i8>(0x004EDB02 + 1, 32);
#endif

    // Increase damage for kill command in Single Player
    WriteMem<float>(0x004A4DF5 + 1, 100000.0f);

    // Fix crash in shadows rendering
    WriteMem<u8>(0x0054A3C0 + 2, 16);

    // Fix crash in geometry rendering
    GeomCachePrepareRoom_hook.Install();

    // Remove Sleep calls in TimerInit
    AsmWriter(0x00504A67, 0x00504A82).nop();

    // Use spawnpoint team property in TeamDM game (PF compatible)
    WriteMem<u8>(0x00470395 + 4, 0); // change cmp argument: CTF -> DM
    WriteMem<u8>(0x0047039A, asm_opcodes::jz_rel_short);  // invert jump condition: jnz -> jz

    // Disable broken optimization of segment vs geometry collision test
    // Fixes hitting objects if mover is in the line of the shot
    AsmWriter(0x00499055).jmp(0x004990B4);

    // Disable Flamethower debug sphere drawing (optimization)
    // It is not visible in game because other things are drawn over it
    AsmWriter(0x0041AE47, 0x0041AE4C).nop();

    // Preserve password case when processing rcon_request command
    WriteMem<i8>(0x0046C85A + 1, 1);

    // Add checking if restoring game state from save file failed during level loading
    RflLoadInternal_CheckRestoreStatus_patch.Install();

    // Open server list menu instead of main menu when leaving multiplayer game
    GameEnterState_hook.Install();
    IsGameStateUiHidden_hook.Install();
    MultiAfterPlayersPackets_hook.Install();

    // Hide main window when displaying critical error message box
    CriticalError_hide_main_wnd_patch.Install();

    // Allow undefined mp_character in PlayerCreateEntity
    // Fixes Go_Undercover event not changing player 3rd person character
    AsmWriter(0x004A414F, 0x004A4153).nop();

    // High monitors/mirrors resolution
    ClutterInitMonitor_hook.Install();

    // Fix crash when skipping cutscene after robot kill in L7S4
    moving_group_rotate_in_place_keyframe_oob_crashfix.Install();

    // Fix crash in LEGO_MP mod caused by XSTR(1000, "RL"); for some reason it does not crash in PF...
    parser_xstr_oob_fix.Install();

    // Fix crashes caused by too many records in tbl files
    ammo_tbl_buffer_overflow_fix.Install();
    clutter_tbl_buffer_overflow_fix.Install();
    strings_tbl_buffer_overflow_fix.Install();

    // Fix killed glass restoration from a save file
    AsmWriter(0x0043604A).nop(5);
    glass_kill_init_fix.Install();

    // Log error when object cannot be created
    ObjCreate_hook.Install();

    // Use local_player variable for debris distance calculation instead of local_entity
    // Fixed debris pool being exhausted when local player is dead
    AsmWriter(0x0042A223, 0x0042A232).mov(asm_regs::ecx, {&rf::local_player});

    // Skip broken code that was supposed to skip particle emulation when particle emitter is in non-rendered room
    // RF code is broken here because level emitters have object handle set to 0 and other emitters are not added to
    // the searched list
    WriteMem<u8>(0x00495158, asm_opcodes::jmp_rel_short);

    // Sort objects by anim mesh name to improve rendering performance
    sort_items_patch.Install();
    sort_clutter_patch.Install();

    // Fix face scroll in levels after version 0xB4
    face_scroll_fix.Install();

    // Increase entity simulation max distance
    // TODO: create a config property for this
    if (g_game_config.disable_lod_models) {
        WriteMem<float>(0x00589548, 100.0f);
    }

    // Fix PlayBikFile texture leak
    play_bik_file_vram_leak_fix.Install();

    // Log error when RFA cannot be loaded
    mvf_load_rfa_debug_print_patch.Install();

    // Fix crash when executing camera2 command in main menu
    AsmWriter(0x0040DCFC).nop(5);

    // Fix ItemCreate null result handling in RFL loading (affects multiplayer only)
    rfl_load_items_crash_fix.Install();

    // Fix col-spheres vs mesh collisions
    anim_mesh_col_fix.Install();

    // Render held corpse in monitor
    render_corpse_in_monitor_patch.Install();

    // Fix possible infinite loop when starting Bink video
    PlayBikFile_infinite_loop_fix.Install();

    // Fix memory leak when trying to play non-existing Bink video
    WriteMem<i32>(0x00520B7E + 2, 0x00520C6E - (0x00520B7E + 6));

    // Fix crash caused by explosion near dying player-controlled entity (entity->local_player is null)
    explosion_crash_fix.Install();

    // If speed reduction in background is not wanted disable that code in RF
    if (!g_game_config.reduced_speed_in_background) {
        WriteMem<u8>(0x004353CC, asm_opcodes::jmp_rel_short);
    }

    // Fix setting fire wait timer when closing weapon switch menu
    // Note: this timer makes sense for weapons that require holding (not clicking) the control to fire (e.g. shotgun)
    HandleCtrlInGame_TimerSet_fire_wait_patch.Install();

    // Init cmd line param
    GetUrlCmdLineParam();

    // Apply patches from other files
    ApplyEventPatches();
    ApplyCutscenePatches();
    ApplyGlarePatches();
    ApplyLimitsPatches();
    ApplyMainMenuPatches();
    ApplySaveRestorePatches();
    ApplySoundPatches();
    ApplyTriggerPatches();
    ApplyWeaponPatches();
    RegisterSoundCommands();
}

void HandleUrlParam()
{
    if (!GetUrlCmdLineParam().IsEnabled()) {
        return;
    }

    auto url = GetUrlCmdLineParam().GetArg();
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

    rf::DcPrintf("Connecting to %s:%d...", host_name.c_str(), port);
    auto host = ntohl(reinterpret_cast<in_addr *>(hp->h_addr_list[0])->S_un.S_addr);

    rf::NwAddr addr{host, port};
    StartJoinMpGameSequence(addr, password);
}

void MiscAfterFullGameInit()
{
    HandleUrlParam();
}
