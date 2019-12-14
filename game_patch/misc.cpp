#include "misc.h"
#include "commands.h"
#include "main.h"
#include "rf.h"
#include "stdafx.h"
#include "utils.h"
#include "server/server.h"
#include <common/version.h>
#include <cstddef>
#include <algorithm>
#include <patch_common/CallHook.h>
#include <patch_common/FunHook.h>
#include <patch_common/CodeInjection.h>
#include <patch_common/ShortTypes.h>
#include <common/rfproto.h>

namespace rf
{

static auto& menu_version_label = AddrAsRef<UiGadget>(0x0063C088);
static auto& sound_enabled = AddrAsRef<bool>(0x017543D8);
static auto& hide_enemy_bullets = AddrAsRef<bool>(0x005A24D0);

static auto& EntityIsReloading = AddrAsRef<bool(EntityObj* entity)>(0x00425250);

static auto& CanSave = AddrAsRef<bool()>(0x004B61A0);

static auto& GameSeqPushState = AddrAsRef<int(int state, bool update_parent_state, bool parent_dlg_open)>(0x00434410);
static auto& GameSeqProcessDeferredChange = AddrAsRef<int()>(0x00434310);

static auto& GrIsSphereOutsideView = AddrAsRef<bool(rf::Vector3& pos, float radius)>(0x005186A0);

} // namespace rf

constexpr double PI = 3.14159265358979323846;

constexpr int EGG_ANIM_ENTER_TIME = 2000;
constexpr int EGG_ANIM_LEAVE_TIME = 2000;
constexpr int EGG_ANIM_IDLE_TIME = 3000;

int g_version_click_counter = 0;
int g_egg_anim_start;

// Note: fastcall is used because MSVC does not allow free thiscall functions
using UiLabel_Create2_Type = void __fastcall(rf::UiGadget*, void*, rf::UiGadget*, int, int, int, int, const char*, int);
extern CallHook<UiLabel_Create2_Type> UiLabel_Create2_VersionLabel_hook;
void __fastcall UiLabel_Create2_VersionLabel(rf::UiGadget* self, void* edx, rf::UiGadget* parent, int x, int y, int w,
                                             int h, const char* text, int font_id)
{
    rf::GrGetTextWidth(&w, &h, text, -1, rf::medium_font_id);
    x = 430 - w;
    w += 5;
    h += 2;
    UiLabel_Create2_VersionLabel_hook.CallTarget(self, edx, parent, x, y, w, h, text, font_id);
}
CallHook<UiLabel_Create2_Type> UiLabel_Create2_VersionLabel_hook{0x0044344D, UiLabel_Create2_VersionLabel};

FunHook<void(const char**, const char**)> GetVersionStr_hook{
    0x004B33F0,
    [](const char** version, const char** a2) {
        static const char version_in_menu[] = PRODUCT_NAME_VERSION;
        if (version)
            *version = version_in_menu;
        if (a2)
            *a2 = "";
    },
};

FunHook<int()> MenuUpdate_hook{
    0x00434230,
    []() {
        int menu_id = MenuUpdate_hook.CallTarget();
        if (menu_id == rf::GS_MP_LIMBO) // hide cursor when changing level - hackfixed in RF by changing rendering logic
            rf::SetCursorVisible(false);
        else if (menu_id == rf::GS_MAIN_MENU)
            rf::SetCursorVisible(true);
        return menu_id;
    },
};

CodeInjection gr_direct3d_lock_crash_fix{
    0x0055CE55,
    [](auto& regs) {
        if (regs.eax == 0) {
            regs.esp += 8;
            regs.eip = 0x0055CF23;
        }
    },
};

CallHook<void()> MenuMainProcessMouse_hook{
    0x004437B9,
    []() {
        MenuMainProcessMouse_hook.CallTarget();
        if (rf::MouseWasButtonPressed(0)) {
            int x, y, z;
            rf::MouseGetPos(x, y, z);
            rf::UiGadget* gadgets_to_check[1] = {&rf::menu_version_label};
            int matched = rf::UiGetGadgetFromPos(x, y, gadgets_to_check, std::size(gadgets_to_check));
            if (matched == 0) {
                TRACE("Version clicked");
                ++g_version_click_counter;
                if (g_version_click_counter == 3)
                    g_egg_anim_start = GetTickCount();
            }
        }
    },
};

int LoadEasterEggImage()
{
    HRSRC res_handle = FindResourceA(g_hmodule, MAKEINTRESOURCEA(100), RT_RCDATA);
    if (!res_handle) {
        ERR("FindResourceA failed");
        return -1;
    }
    HGLOBAL res_data_handle = LoadResource(g_hmodule, res_handle);
    if (!res_data_handle) {
        ERR("LoadResource failed");
        return -1;
    }
    void* res_data = LockResource(res_data_handle);
    if (!res_data) {
        ERR("LockResource failed");
        return -1;
    }

    constexpr int easter_egg_size = 128;

    int hbm = rf::BmCreateUserBmap(rf::BMPF_8888, easter_egg_size, easter_egg_size);

    rf::GrLockData lock_data;
    if (!rf::GrLock(hbm, 0, &lock_data, 1))
        return -1;

    rf::BmConvertFormat(lock_data.bits, (rf::BmPixelFormat)lock_data.pixel_format, res_data, rf::BMPF_8888,
                        easter_egg_size * easter_egg_size);
    rf::GrUnlock(&lock_data);

    return hbm;
}

CallHook<void()> MenuMainRender_hook{
    0x00443802,
    []() {
        MenuMainRender_hook.CallTarget();
        if (g_version_click_counter >= 3) {
            static int img = LoadEasterEggImage(); // data.vpp
            if (img == -1)
                return;
            int w, h;
            rf::BmGetBitmapSize(img, &w, &h);
            int anim_delta_time = GetTickCount() - g_egg_anim_start;
            int pos_x = (rf::GrGetMaxWidth() - w) / 2;
            int pos_y = rf::GrGetMaxHeight() - h;
            if (anim_delta_time < EGG_ANIM_ENTER_TIME) {
                float enter_progress = anim_delta_time / static_cast<float>(EGG_ANIM_ENTER_TIME);
                pos_y += h - static_cast<int>(sinf(enter_progress * static_cast<float>(PI) / 2.0f) * h);
            }
            else if (anim_delta_time > EGG_ANIM_ENTER_TIME + EGG_ANIM_IDLE_TIME) {
                int leave_delta = anim_delta_time - (EGG_ANIM_ENTER_TIME + EGG_ANIM_IDLE_TIME);
                float leave_progress = leave_delta / static_cast<float>(EGG_ANIM_LEAVE_TIME);
                pos_y += static_cast<int>((1.0f - cosf(leave_progress * static_cast<float>(PI) / 2.0f)) * h);
                if (leave_delta > EGG_ANIM_LEAVE_TIME)
                    g_version_click_counter = 0;
            }
            rf::GrDrawImage(img, pos_x, pos_y, rf::gr_bitmap_material);
        }
    },
};

void SetPlaySoundEventsVolumeScale(float volume_scale)
{
    volume_scale = std::clamp(volume_scale, 0.0f, 1.0f);
    uintptr_t offsets[] = {
        // Play Sound event
        0x004BA4D8, 0x004BA515, 0x004BA71C, 0x004BA759, 0x004BA609, 0x004BA5F2, 0x004BA63F,
    };
    for (auto offset : offsets) {
        WriteMem<float>(offset + 1, volume_scale);
    }
}

CallHook<void(int, rf::Vector3*, float*, float*, float)> SndConvertVolume3D_AmbientSound_hook{
    0x00505F93,
    [](int game_snd_id, rf::Vector3* sound_pos, float* pan_out, float* volume_out, float volume_in) {
        SndConvertVolume3D_AmbientSound_hook.CallTarget(game_snd_id, sound_pos, pan_out, volume_out, volume_in);
        *volume_out *= g_game_config.level_sound_volume;
    },
};

bool IsHoldingAssaultRifle()
{
    static auto& assault_rifle_cls_id = AddrAsRef<int>(0x00872470);
    rf::EntityObj* entity = rf::EntityGetFromHandle(rf::local_player->entity_handle);
    return entity && entity->ai_info.weapon_cls_id == assault_rifle_cls_id;
}

FunHook<void(rf::Player*, bool, bool)> PlayerLocalFireControl_hook{
    0x004A4E80,
    [](rf::Player* player, bool secondary, bool was_pressed) {
        if (g_game_config.swap_assault_rifle_controls && IsHoldingAssaultRifle())
            secondary = !secondary;
        PlayerLocalFireControl_hook.CallTarget(player, secondary, was_pressed);
    },
};

extern CallHook<bool(rf::ControlConfig*, rf::GameCtrl, bool*)> IsEntityCtrlActive_hook1;
bool IsEntityCtrlActive_New(rf::ControlConfig* control_config, rf::GameCtrl game_ctrl, bool* was_pressed)
{
    if (g_game_config.swap_assault_rifle_controls && IsHoldingAssaultRifle()) {
        if (game_ctrl == rf::GC_PRIMARY_ATTACK)
            game_ctrl = rf::GC_SECONDARY_ATTACK;
        else if (game_ctrl == rf::GC_SECONDARY_ATTACK)
            game_ctrl = rf::GC_PRIMARY_ATTACK;
    }
    return IsEntityCtrlActive_hook1.CallTarget(control_config, game_ctrl, was_pressed);
}
CallHook<bool(rf::ControlConfig*, rf::GameCtrl, bool*)> IsEntityCtrlActive_hook1{0x00430E65, IsEntityCtrlActive_New};
CallHook<bool(rf::ControlConfig*, rf::GameCtrl, bool*)> IsEntityCtrlActive_hook2{0x00430EF7, IsEntityCtrlActive_New};

DcCommand2 swap_assault_rifle_controls_cmd{
    "swap_assault_rifle_controls",
    []() {
        g_game_config.swap_assault_rifle_controls = !g_game_config.swap_assault_rifle_controls;
        g_game_config.save();
        rf::DcPrintf("Swap assault rifle controls: %s",
                     g_game_config.swap_assault_rifle_controls ? "enabled" : "disabled");
    },
    "Swap Assault Rifle controls",
};

CodeInjection CriticalError_hide_main_wnd_patch{
    0x0050BA90,
    []() {
        if (rf::gr_d3d_device)
            rf::gr_d3d_device->Release();
        if (rf::main_wnd)
            ShowWindow(rf::main_wnd, SW_HIDE);
    },
};

bool EntityIsReloading_SwitchWeapon_New(rf::EntityObj* entity)
{
    if (rf::EntityIsReloading(entity))
        return true;

    int weapon_cls_id = entity->ai_info.weapon_cls_id;
    if (weapon_cls_id >= 0) {
        rf::WeaponClass* weapon_cls = &rf::weapon_classes[weapon_cls_id];
        if (entity->ai_info.weapons_ammo[weapon_cls_id] == 0 && entity->ai_info.clip_ammo[weapon_cls->ammo_type] > 0)
            return true;
    }
    return false;
}

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
                    WARN("Not rendering room with %u vertices!", room_vert_num);
                *pp_room_geom = nullptr;
                return -1;
            }
        }
        return ret;
    },
};

struct ServerListEntry
{
    char name[32];
    char level_name[32];
    char mod_name[16];
    int game_type;
    rf::NwAddr addr;
    char current_players;
    char max_players;
    int16_t ping;
    int field_60;
    char field_64;
    int flags;
};
static_assert(sizeof(ServerListEntry) == 0x6C, "invalid size");

FunHook<int(const int&, const int&)> ServerListCmpFunc_hook{
    0x0044A6D0,
    [](const int& index1, const int& index2) {
        auto server_list = AddrAsRef<ServerListEntry*>(0x0063F62C);
        bool has_ping1 = server_list[index1].ping >= 0;
        bool has_ping2 = server_list[index2].ping >= 0;
        if (has_ping1 != has_ping2)
            return has_ping1 ? -1 : 1;
        else
            return ServerListCmpFunc_hook.CallTarget(index1, index2);
    },
};

constexpr int CHAT_MSG_MAX_LEN = 224;

FunHook<void(uint16_t)> ChatSayAddChar_hook{
    0x00444740,
    [](uint16_t key) {
        if (key)
            ChatSayAddChar_hook.CallTarget(key);
    },
};

FunHook<void(const char*, bool)> ChatSayAccept_hook{
    0x00444440,
    [](const char* msg, bool is_team_msg) {
        std::string msg_str{msg};
        if (msg_str.size() > CHAT_MSG_MAX_LEN)
            msg_str.resize(CHAT_MSG_MAX_LEN);
        ChatSayAccept_hook.CallTarget(msg_str.c_str(), is_team_msg);
    },
};

constexpr uint8_t TRIGGER_CLIENT_SIDE = 0x2;
constexpr uint8_t TRIGGER_SOLO = 0x4;
constexpr uint8_t TRIGGER_TELEPORT = 0x8;

rf::Player* g_trigger_solo_player = nullptr;

void SendTriggerActivatePacket(rf::Player* player, int trigger_uid, int32_t entity_handle)
{
    rfTriggerActivate packet;
    packet.type = RF_TRIGGER_ACTIVATE;
    packet.size = sizeof(packet) - sizeof(rfPacketHeader);
    packet.uid = trigger_uid;
    packet.entity_handle = entity_handle;
    rf::NwSendReliablePacket(player, reinterpret_cast<uint8_t*>(&packet), sizeof(packet), 0);
}

FunHook<void(int, int)> SendTriggerActivatePacketToAllPlayers_hook{
    0x00483190,
    [](int trigger_uid, int entity_handle) {
        if (g_trigger_solo_player)
            SendTriggerActivatePacket(g_trigger_solo_player, trigger_uid, entity_handle);
        else
            SendTriggerActivatePacketToAllPlayers_hook.CallTarget(trigger_uid, entity_handle);
    },
};

FunHook<void(rf::TriggerObj*, int32_t, bool)> TriggerActivate_hook{
    0x004C0220,
    [](rf::TriggerObj* trigger, int32_t h_entity, bool skip_movers) {
        // Check team
        auto player = rf::GetPlayerFromEntityHandle(h_entity);
        auto trigger_name = trigger->_super.name.CStr();
        if (player && trigger->team != -1 && trigger->team != player->blue_team) {
            // rf::DcPrintf("Trigger team does not match: %d vs %d (%s)", trigger->team, Player->blue_team,
            // trigger_name);
            return;
        }

        // Check if this is Solo or Teleport trigger (REDPF feature)
        uint8_t ext_flags = trigger_name[0] == '\xAB' ? trigger_name[1] : 0;
        bool is_solo_trigger = (ext_flags & (TRIGGER_SOLO | TRIGGER_TELEPORT)) != 0;
        if (rf::is_net_game && rf::is_local_net_game && is_solo_trigger && player) {
            // rf::DcPrintf("Solo/Teleport trigger activated %s", trigger_name);
            if (player != rf::local_player) {
                SendTriggerActivatePacket(player, trigger->_super.uid, h_entity);
                return;
            }
            else {
                g_trigger_solo_player = player;
            }
        }

        // Normal activation
        // rf::DcPrintf("trigger normal activation %s %d", trigger_name, ext_flags);
        TriggerActivate_hook.CallTarget(trigger, h_entity, skip_movers);
        g_trigger_solo_player = nullptr;
    },
};

CodeInjection TriggerCheckActivation_patch{
    0x004BFC7D,
    [](auto& regs) {
        auto trigger = reinterpret_cast<rf::TriggerObj*>(regs.eax);
        auto trigger_name = trigger->_super.name.CStr();
        uint8_t ext_flags = trigger_name[0] == '\xAB' ? trigger_name[1] : 0;
        bool is_client_side = (ext_flags & TRIGGER_CLIENT_SIDE) != 0;
        if (is_client_side)
            regs.eip = 0x004BFCDB;
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
        ERR("Restoring game state failed");
        char* error_info = *reinterpret_cast<char**>(regs.esp + 0x2B0 + 0xC);
        strcpy(error_info, "Save file is corrupted");
        // return to RflLoadInternal failure path
        regs.eip = 0x004608CC;
    },
};

static int g_cutscene_bg_sound_sig = -1;
static constexpr rf::GameCtrl default_skip_cutscene_ctrl = rf::GC_MP_STATS;

rf::String GetGameCtrlBindName(int game_ctrl)
{
    auto GetKeyName = AddrAsRef<int(rf::String *out, int key)>(0x0043D930);
    auto GetMouseButtonName = AddrAsRef<int(rf::String *out, int mouse_btn)>(0x0043D970);
    auto ctrl_config = rf::local_player->config.controls.keys[game_ctrl];
    rf::String name;
    if (ctrl_config.scan_codes[0] >= 0) {
        GetKeyName(&name, ctrl_config.scan_codes[0]);
    }
    else if (ctrl_config.mouse_btn_id >= 0) {
        GetMouseButtonName(&name, ctrl_config.mouse_btn_id);
    }
    else {
        return rf::String::Format("?");
    }
    return name;
}

void RenderSkipCutsceneHintText(rf::GameCtrl ctrl)
{
    auto bind_name = GetGameCtrlBindName(ctrl);
    auto& ctrl_name = rf::local_player->config.controls.keys[ctrl].name;
    rf::GrSetColor(255, 255, 255, 255);
    auto msg = rf::String::Format("Press %s (%s) to skip the cutscene", ctrl_name.CStr(), bind_name.CStr());
    auto x = rf::GrGetMaxWidth() / 2;
    auto y = rf::GrGetMaxHeight() - 30;
    rf::GrDrawAlignedText(rf::GR_ALIGN_CENTER, x, y, msg.CStr(), -1, rf::gr_text_material);
}

FunHook<void(bool)> MenuInGameUpdateCutscene_hook{
    0x0045B5E0,
    [](bool dlg_open) {
        bool skip_cutscene = false;
        auto skip_cutscene_ctrl = g_game_config.skip_cutscene_ctrl != -1
            ? static_cast<rf::GameCtrl>(g_game_config.skip_cutscene_ctrl)
            : default_skip_cutscene_ctrl;
        rf::IsEntityCtrlActive(&rf::local_player->config.controls, skip_cutscene_ctrl, &skip_cutscene);

        if (!skip_cutscene) {
            MenuInGameUpdateCutscene_hook.CallTarget(dlg_open);
            RenderSkipCutsceneHintText(skip_cutscene_ctrl);
        }
        else {
            auto& timer_add_delta_time = AddrAsRef<int(int delta_ms)>(0x004FA2D0);
            auto& snd_stop = AddrAsRef<char(int sig)>(0x005442B0);
            auto& destroy_all_paused_sounds = AddrAsRef<void()>(0x005059F0);
            auto& set_all_playing_sounds_paused = AddrAsRef<void(bool paused)>(0x00505C70);

            auto& timer_base = AddrAsRef<int64_t>(0x01751BF8);
            auto& timer_freq = AddrAsRef<int32_t>(0x01751C04);
            auto& frame_time = AddrAsRef<float>(0x005A4014);
            auto& num_shots = StructFieldRef<int>(rf::active_cutscene, 4);
            auto& current_shot_idx = StructFieldRef<int>(rf::active_cutscene, 0x808);
            auto& current_shot_timer = StructFieldRef<rf::Timer>(rf::active_cutscene, 0x810);

            if (g_cutscene_bg_sound_sig != -1) {
                snd_stop(g_cutscene_bg_sound_sig);
                g_cutscene_bg_sound_sig = -1;
            }

            set_all_playing_sounds_paused(true);
            destroy_all_paused_sounds();
            rf::sound_enabled = false;

            while (rf::CutsceneIsActive()) {
                int shot_time_left_ms = current_shot_timer.GetTimeLeftMs();

                if (current_shot_idx == num_shots - 1) {
                    // run last half second with a speed of 10 FPS so all events get properly processed before
                    // going back to normal gameplay
                    if (shot_time_left_ms > 500)
                        shot_time_left_ms -= 500;
                    else
                        shot_time_left_ms = std::min(shot_time_left_ms, 100);
                }
                timer_add_delta_time(shot_time_left_ms);
                frame_time = shot_time_left_ms / 1000.0f;
                timer_base -= static_cast<int64_t>(shot_time_left_ms) * timer_freq / 1000;
                MenuInGameUpdateCutscene_hook.CallTarget(dlg_open);
            }

            rf::sound_enabled = true;
        }
    },
};

CallHook<int()> PlayHardcodedBackgroundMusicForCutscene_hook{
    0x0045BB85,
    []() {
        g_cutscene_bg_sound_sig = PlayHardcodedBackgroundMusicForCutscene_hook.CallTarget();
        return g_cutscene_bg_sound_sig;
    },
};

DcCommand2 skip_cutscene_bind_cmd{
    "skip_cutscene_bind",
    [](std::string bind_name) {
        if (bind_name == "default") {
            g_game_config.skip_cutscene_ctrl = -1;
        }
        else {
            auto ConfigFindControlByName = AddrAsRef<int(rf::PlayerConfig&, const char*)>(0x0043D9F0);
            int ctrl = ConfigFindControlByName(rf::local_player->config, bind_name.c_str());
            if (ctrl == -1) {
                rf::DcPrintf("Cannot find control: %s", bind_name.c_str());
            }
            else {
                g_game_config.skip_cutscene_ctrl = ctrl;
                g_game_config.save();
                rf::DcPrintf("Skip Cutscene bind changed to: %s", bind_name.c_str());
            }
        }
    },
    "Changes bind used to skip cutscenes",
    "skip_cutscene_bind ctrl_name",
};

CodeInjection CoronaEntityCollisionTestFix{
    0x004152F1,
    [](auto& regs) {
        auto get_entity_root_bone_pos = AddrAsRef<void(rf::EntityObj*, rf::Vector3&)>(0x48AC70);
        using IntersectLineWithAabbType = bool(rf::Vector3 * aabb1, rf::Vector3 * aabb2, rf::Vector3 * pos1,
                                               rf::Vector3 * pos2, rf::Vector3 * out_pos);
        auto& intersect_line_with_aabb = AddrAsRef<IntersectLineWithAabbType>(0x00508B70);

        rf::EntityObj* entity = reinterpret_cast<rf::EntityObj*>(regs.esi);
        if (!rf::CutsceneIsActive()) {
            return;
        }

        rf::Vector3 root_bone_pos;
        get_entity_root_bone_pos(entity, root_bone_pos);
        rf::Vector3 aabb_min = root_bone_pos - entity->_super.phys_info.radius;
        rf::Vector3 aabb_max = root_bone_pos + entity->_super.phys_info.radius;
        auto corona_pos = reinterpret_cast<rf::Vector3*>(regs.edi);
        auto eye_pos = reinterpret_cast<rf::Vector3*>(regs.ebx);
        auto tmp_vec = reinterpret_cast<rf::Vector3*>(regs.ecx);
        regs.eax = intersect_line_with_aabb(&aabb_min, &aabb_max, corona_pos, eye_pos, tmp_vec);
        regs.eip = 0x004152F6;
    },
};

FunHook<void(rf::Object*, int)> GlareRenderCorona_hook{
    0x00414860,
    [](rf::Object *glare, int player_idx) {
        // check if corona is in view using dynamic radius dedicated for this effect
        // Note: object radius matches volumetric effect size and can be very large so this check helps
        // to speed up rendering
        auto& current_radius = StructFieldRef<float[2]>(glare, 0x2A4);
        if (!rf::GrIsSphereOutsideView(glare->pos, current_radius[player_idx])) {
            GlareRenderCorona_hook.CallTarget(glare, player_idx);
        }
    },
};

FunHook<void()> DoQuickSave_hook{
    0x004B5E20,
    []() {
        if (rf::CanSave())
            DoQuickSave_hook.CallTarget();
    },
};

bool g_in_mp_game = false;
bool g_jump_to_multi_server_list = false;

void SetJumpToMultiServerList(bool jump)
{
    g_jump_to_multi_server_list = jump;
}

FunHook<void(int, int)> GameEnterState_hook{
    0x004B1AC0,
    [](int state, int old_state) {
        GameEnterState_hook.CallTarget(state, old_state);
        TRACE("state %d old_state %d g_jump_to_multi_server_list %d", state, old_state, g_jump_to_multi_server_list);

        bool exiting_game = state == rf::GS_MAIN_MENU &&
            (old_state == rf::GS_EXIT_GAME || old_state == rf::GS_LOADING_LEVEL);
        if (exiting_game && g_in_mp_game) {
            g_in_mp_game = false;
            g_jump_to_multi_server_list = true;
        }

        if (state == rf::GS_MAIN_MENU && g_jump_to_multi_server_list) {
            TRACE("jump to mp menu!");
            rf::sound_enabled = false;
            CallAddr(0x00443C20); // OpenMultiMenu
            old_state = state;
            state = rf::GameSeqProcessDeferredChange();
            GameEnterState_hook.CallTarget(state, old_state);
        }
        if (state == rf::GS_MP_MENU && g_jump_to_multi_server_list) {
            CallAddr(0x00448B70); // OnMpJoinGameBtnClick
            old_state = state;
            state = rf::GameSeqProcessDeferredChange();
            GameEnterState_hook.CallTarget(state, old_state);
        }
        if (state == rf::GS_MP_SERVER_LIST_MENU && g_jump_to_multi_server_list) {
            g_jump_to_multi_server_list = false;
            rf::sound_enabled = true;
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

DcCommand2 show_enemy_bullets_cmd{
    "show_enemy_bullets",
    []() {
        g_game_config.show_enemy_bullets = !g_game_config.show_enemy_bullets;
        g_game_config.save();
        rf::hide_enemy_bullets = !g_game_config.show_enemy_bullets;
        rf::DcPrintf("Enemy bullets are %s", g_game_config.show_enemy_bullets ? "enabled" : "disabled");
    },
    "Toggles enemy bullets visibility",
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

CodeInjection GrLoadFontInternal_fix_texture_ref{
    0x0051F429,
    [](auto& regs) {
        auto gr_tcache_add_ref = AddrAsRef<void(int bm_handle)>(0x0050E850);
        gr_tcache_add_ref(regs.eax);
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
            WARN("XSTR index is out of bounds: %d!", regs.edi);
            regs.edi = -1;
        }
    }
};

CodeInjection ammo_tbl_buffer_overflow_fix{
    0x004C218E,
    [](auto& regs) {
        if (AddrAsRef<u32>(0x0085C760) == 32) {
            WARN("ammo.tbl limit of 32 definitions has been reached!");
            regs.eip = 0x004C21B8;
        }
    },
};

CodeInjection clutter_tbl_buffer_overflow_fix{
    0x0040F49E,
    [](auto& regs) {
        if (regs.ecx == 450) {
            WARN("clutter.tbl limit of 450 definitions has been reached!");
            regs.eip = 0x0040F4B0;
        }
    },
};

CodeInjection weapons_tbl_buffer_overflow_fix_1{
    0x004C6855,
    [](auto& regs) {
        if (AddrAsRef<u32>(0x00872448) == 64) {
            WARN("weapons.tbl limit of 64 definitions has been reached!");
            regs.eip = 0x004C6881;
        }
    },
};

CodeInjection weapons_tbl_buffer_overflow_fix_2{
    0x004C68AD,
    [](auto& regs) {
        if (AddrAsRef<u32>(0x00872448) == 64) {
            WARN("weapons.tbl limit of 64 definitions has been reached!");
            regs.eip = 0x004C68D9;
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
            WARN("strings.tbl index is out of bounds: %d", id);
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
struct LevelSaveData
{
  char unk[0x18708];
  uint8_t num_goal_create_events;
  uint8_t num_alarm_siren_events;
  uint8_t num_when_dead_events;
  uint8_t num_cyclic_timer_events;
  uint8_t num_make_invulnerable_events;
  uint8_t num_other_events;
  uint8_t num_emitters;
  uint8_t num_decals;
  uint8_t num_entities;
  uint8_t num_items;
  uint16_t num_clutter;
  uint8_t num_triggers;
  uint8_t num_keyframes;
  uint8_t num_push_regions;
  uint8_t num_persistent_goals;
  uint8_t num_weapons;
  uint8_t num_blood_smears;
  uint8_t num_corpse;
  uint8_t num_geomod_craters;
  uint8_t num_killed_room_ids;
  uint8_t num_dead_entities;
  uint8_t num_deleted_events;
  char field_1871F;
};
static_assert(sizeof(LevelSaveData) == 0x18720);

template<typename T>
T validate_save_file_num(T value, T limit, const char* what)
{
    if (value > limit) {
        WARN("Save file is corrupted: expected up to %d %s but got %d", limit, what, value);
        return limit;
    }
    return value;
}

FunHook<void(LevelSaveData*)> SaveRestoreDeserializeAllObjects_hook{
    0x004B4FB0,
    [](LevelSaveData *data) {
        data->num_goal_create_events = validate_save_file_num<uint8_t>(data->num_goal_create_events, 8, "goal_create events");
        data->num_alarm_siren_events = validate_save_file_num<uint8_t>(data->num_alarm_siren_events, 8, "alarm_siren events");
        data->num_when_dead_events = validate_save_file_num<uint8_t>(data->num_when_dead_events, 20, "when_dead events");
        data->num_cyclic_timer_events = validate_save_file_num<uint8_t>(data->num_cyclic_timer_events, 12, "cyclic_timer events");
        data->num_make_invulnerable_events = validate_save_file_num<uint8_t>(data->num_make_invulnerable_events, 8, "make_invulnerable events");
        data->num_other_events = validate_save_file_num<uint8_t>(data->num_other_events, 32, "other events");
        data->num_emitters = validate_save_file_num<uint8_t>(data->num_emitters, 16, "emitters");
        data->num_decals = validate_save_file_num<uint8_t>(data->num_decals, 64, "decals");
        data->num_entities = validate_save_file_num<uint8_t>(data->num_entities, 64, "entities");
        data->num_items = validate_save_file_num<uint8_t>(data->num_items, 64, "items");
        data->num_clutter = validate_save_file_num<uint16_t>(data->num_clutter, 512, "clutters");
        data->num_triggers = validate_save_file_num<uint8_t>(data->num_triggers, 96, "triggers");
        data->num_keyframes = validate_save_file_num<uint8_t>(data->num_keyframes, 128, "keyframes");
        data->num_push_regions = validate_save_file_num<uint8_t>(data->num_push_regions, 32, "push regions");
        data->num_persistent_goals = validate_save_file_num<uint8_t>(data->num_persistent_goals, 10, "persistent goals");
        data->num_weapons = validate_save_file_num<uint8_t>(data->num_weapons, 8, "weapons");
        data->num_blood_smears = validate_save_file_num<uint8_t>(data->num_blood_smears, 16, "blood smears");
        data->num_corpse = validate_save_file_num<uint8_t>(data->num_corpse, 32, "corpses");
        data->num_geomod_craters = validate_save_file_num<uint8_t>(data->num_geomod_craters, 128, "geomod craters");
        data->num_killed_room_ids = validate_save_file_num<uint8_t>(data->num_killed_room_ids, 128, "killed rooms");
        data->num_dead_entities = validate_save_file_num<uint8_t>(data->num_dead_entities, 64, "dead entities");
        data->num_deleted_events = validate_save_file_num<uint8_t>(data->num_deleted_events, 32, "deleted events");
        SaveRestoreDeserializeAllObjects_hook.CallTarget(data);
    },
};

FunHook<rf::Object*(int, int, int, void*, int, void*)> ObjCreate_hook{
    0x00486DA0,
    [](int type, int sub_type, int parent, void* create_info, int flags, void* room) {
        auto obj = ObjCreate_hook.CallTarget(type, sub_type, parent, create_info, flags, room);
        if (!obj) {
            WARN("Failed to create object (type %d)", type);
        }
        return obj;
    },
};

FunHook<void(rf::WeaponObj *weapon)> WeaponMoveOne_hook{
    0x004C69A0,
    [](rf::WeaponObj* weapon) {
        WeaponMoveOne_hook.CallTarget(weapon);
        auto& level_aabb_min = StructFieldRef<rf::Vector3>(rf::rfl_static_geometry, 0x48);
        auto& level_aabb_max = StructFieldRef<rf::Vector3>(rf::rfl_static_geometry, 0x54);
        float margin = weapon->_super.anim_mesh ? 275.0f : 10.0f;
        bool has_gravity_flag = weapon->_super.phys_info.flags & 1;
        bool check_y_axis = !(has_gravity_flag || weapon->weapon_cls->thrust_lifetime > 0.0f);
        auto& pos = weapon->_super.pos;
        if (pos.x < level_aabb_min.x - margin || pos.x > level_aabb_max.x + margin
        || pos.z < level_aabb_min.z - margin || pos.z > level_aabb_max.z + margin
        || (check_y_axis && (pos.y < level_aabb_min.y - margin || pos.y > level_aabb_max.y + margin))) {
            // Weapon is outside the level - delete it
            rf::ObjQueueDelete(&weapon->_super);
        }
    },
};

#ifdef BIG_WEAPON_POOL
CallHook<void*(size_t)> weapon_pool_alloc_zero_dynamic_mem{
    0x0048B5C6,
    [](size_t size) {
        auto result = weapon_pool_alloc_zero_dynamic_mem.CallTarget(size);
        if (result) {
            memset(result, 0, size);
        }
        return result;
    },
};
#endif

int TryToFreeDsChannel(float volume)
{
    int num_music = 0;
    int num_looping = 0;
    int num_long = 0;
    int chnl_id = -1;
    for (size_t i = 0; i < std::size(rf::snd_channels); ++i) {
        // Skip music and looping sounds
        if (rf::snd_channels[i].flags & rf::SCHF_MUSIC) {
            ++num_music;
            continue;
        }
        if (rf::snd_channels[i].flags & rf::SCHF_LOOPING) {
            ++num_looping;
            continue;
        }
        // Skip long sounds
        float duration = rf::SndDsEstimateDuration(rf::snd_channels[i].snd_ds_id);
        if (duration > 10.0f) {
            ++num_long;
            continue;
        }
        // Select channel with the lowest volume
        if (chnl_id < 0 || rf::snd_channels[chnl_id].volume < rf::snd_channels[i].volume) {
            chnl_id = static_cast<int>(i);
        }
    }
    // Free channel with the lowest volume and use it for new sound
    if (chnl_id > 0 && rf::snd_channels[chnl_id].volume <= volume) {
        INFO("Freeing sound channel %d to make place for a new sound (volume %.4f duration %.1f)", chnl_id,
            rf::snd_channels[chnl_id].volume, rf::SndDsEstimateDuration(rf::snd_channels[chnl_id].snd_ds_id));
        rf::SndDsCloseChannel(chnl_id);
    }
    else {
        WARN("Failed to allocate a sound channel: volume %.2f, num_music %d, num_looping %d, num_long %d", volume,
            num_music, num_looping, num_long);
    }
    return chnl_id;
}

CodeInjection snd_ds_play_3d_no_free_slots_fix{
    0x005222DF,
    [](auto& regs) {
        auto stack_frame = regs.esp + 0x14;
        auto volume_arg = AddrAsRef<float>(stack_frame + 0x18);
        if (regs.eax == -1) {
            regs.eax = TryToFreeDsChannel(volume_arg);
        }
    },
};

CodeInjection snd_ds_play_no_free_slots_fix{
    0x0052255C,
    [](auto& regs) {
        auto stack_frame = regs.esp + 0x10;
        auto volume_arg = AddrAsRef<float>(stack_frame + 0x8);
        if (regs.eax == -1) {
            regs.eax = TryToFreeDsChannel(volume_arg);
        }
    },
};

CodeInjection PlaySound_no_free_slots_fix{
    0x005055E3,
    [](auto& regs) {
        auto stack_frame = regs.esp + 0x10;
        auto type_arg = AddrAsRef<int>(stack_frame + 0x8);
        auto vol_scale_arg = AddrAsRef<float>(stack_frame + 0x10);
        auto& snd_vol_scale_per_type = AddrAsRef<float[4]>(0x01753C18);
        // Try to free a slot
        int num_looping = 0;
        int num_long = 0;
        int best_idx = -1;
        float best_vol_scale = 0.0f;
        for (size_t i = 0; i < std::size(rf::level_sounds); ++i) {
            auto& lvl_snd = rf::level_sounds[i];
            // Skip looping sounds
            if (rf::game_sounds[lvl_snd.game_snd_id].is_looping) {
                TRACE("Skipping sound %d because it is looping", i);
                ++num_looping;
                continue;
            }
            // Skip long sounds
            float duration = rf::SndGetDuration(lvl_snd.game_snd_id);
            if (duration > 10.0f) {
                TRACE("Skipping sound %d because of duration: %f", i, duration);
                ++num_long;
                continue;
            }
            // Find sound with the lowest volume
            float current_vol_scale = snd_vol_scale_per_type[lvl_snd.type] * lvl_snd.vol_scale;
            if (best_idx == -1 || current_vol_scale < best_vol_scale) {
                best_idx = static_cast<int>(i);
                best_vol_scale = current_vol_scale;
            }
        }
        float new_sound_vol_scale = snd_vol_scale_per_type[type_arg] * vol_scale_arg;
        if (best_idx >= 0 && best_vol_scale <= new_sound_vol_scale) {
            // Free the selected slot and use it for a new sound
            auto& best_lvl_snd = rf::level_sounds[best_idx];
            INFO("Freeing level sound %d to make place for a new sound (volume %.4f duration %.1f)", best_idx,
                best_vol_scale, rf::SndGetDuration(best_lvl_snd.game_snd_id));
            rf::SndStop(best_lvl_snd.sig);
            rf::ClearLevelSound(&best_lvl_snd);
            regs.ebx = best_idx;
            regs.esi = reinterpret_cast<int32_t>(&best_lvl_snd);
            regs.eip = 0x005055EB;
        }
        else {
            WARN("Failed to allocate a level sound: volume %.2f num_looping %d num_long %d ",
                new_sound_vol_scale, num_looping, num_long);
        }
    },
};

auto AnimMeshGetName = AddrAsRef<const char*(rf::AnimMesh* anim_mesh)>(0x00503470);

CodeInjection sort_items_patch{
    0x004593AC,
    [](auto& regs) {
        auto item = reinterpret_cast<rf::ItemObj*>(regs.esi);
        auto anim_mesh = item->_super.anim_mesh;
        auto mesh_name = anim_mesh ? AnimMeshGetName(anim_mesh) : nullptr;
        if (!mesh_name) {
            // Sometimes on level change some objects can stay and have only anim_mesh destroyed
            return;
        }

        // HACKFIX: enable alpha sorting for Invulnerability Powerup
        // Note: material used for alpha-blending is flare_blue1.tga - it uses non-alpha texture
        // so information about alpha-blending cannot be taken from material alone - it must be read from VFX
        if (!strcmp(mesh_name, "powerup_invuln.vfx")) {
            item->_super.flags |= 0x100000; // OF_HAS_ALPHA
        }

        auto& item_obj_list = AddrAsRef<rf::ItemObj>(0x00642DD8);
        rf::ItemObj* current = item_obj_list.next;
        while (current != &item_obj_list) {
            auto current_anim_mesh = current->_super.anim_mesh;
            auto current_mesh_name = current_anim_mesh ? AnimMeshGetName(current_anim_mesh) : nullptr;
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
        auto anim_mesh = clutter->_super.anim_mesh;
        auto mesh_name = anim_mesh ? AnimMeshGetName(anim_mesh) : nullptr;
        if (!mesh_name) {
            // Sometimes on level change some objects can stay and have only anim_mesh destroyed
            return;
        }
        auto& clutter_obj_list = AddrAsRef<rf::ClutterObj>(0x005C9360);
        auto current = clutter_obj_list.next;
        while (current != &clutter_obj_list) {
            auto current_anim_mesh = current->_super.anim_mesh;
            auto current_mesh_name = current_anim_mesh ? AnimMeshGetName(current_anim_mesh) : nullptr;
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

CodeInjection quick_save_mem_leak_fix{
    0x004B603F,
    []() {
        AddrAsRef<bool>(0x00856054) = true;
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

CallHook<int(void*)> AiNavClear_on_load_level_event_crash_fix{
    0x004BBD99,
    [](void* nav) {
        // Clear NavPoint pointers before level load
        StructFieldRef<void*>(nav, 0x114) = nullptr;
        StructFieldRef<void*>(nav, 0x118) = nullptr;
        return AiNavClear_on_load_level_event_crash_fix.CallTarget(nav);
    },
};

void MiscInit()
{
    // Console init string
    WriteMemPtr(0x004B2534, "-- " PRODUCT_NAME " Initializing --\n");

    // Version in Main Menu
    UiLabel_Create2_VersionLabel_hook.Install();
    GetVersionStr_hook.Install();

    // Window title (client and server)
    WriteMemPtr(0x004B2790, PRODUCT_NAME);
    WriteMemPtr(0x004B27A4, PRODUCT_NAME);

    // Console background color
    WriteMem<u32>(0x005098D1, CONSOLE_BG_A); // Alpha
    WriteMem<u8>(0x005098D6, CONSOLE_BG_B);  // Blue
    WriteMem<u8>(0x005098D8, CONSOLE_BG_G);  // Green
    WriteMem<u8>(0x005098DA, CONSOLE_BG_R);  // Red

#ifdef NO_CD_FIX
    // No-CD fix
    WriteMem<u8>(0x004B31B6, asm_opcodes::jmp_rel_short);
#endif // NO_CD_FIX

    // Disable thqlogo.bik
    if (g_game_config.fast_start) {
        WriteMem<u8>(0x004B208A, asm_opcodes::jmp_rel_short);
        WriteMem<u8>(0x004B24FD, asm_opcodes::jmp_rel_short);
    }

    // Sound loop fix
    WriteMem<u8>(0x00505D07 + 1, 0x00505D5B - (0x00505D07 + 2));

    // Set initial FPS limit
    WriteMem<float>(0x005094CA, 1.0f / g_game_config.max_fps);

    // Crash-fix... (probably argument for function is invalid); Page Heap is needed
    WriteMem<u32>(0x0056A28C + 1, 0);

    // Crash-fix in case texture has not been created (this happens if GrReadBackbuffer fails)
    gr_direct3d_lock_crash_fix.Install();

    // Dont overwrite player name and prefered weapons when loading saved game
    AsmWriter(0x004B4D99, 0x004B4DA5).nop();
    AsmWriter(0x004B4E0A, 0x004B4E22).nop();

#if 1
    // Buffer overflows in RflReadStaticGeometry
    // Note: Buffer size is 1024 but opcode allows only 1 byte size
    //       What is more important BmLoad copies texture name to 32 bytes long buffers
    WriteMem<i8>(0x004ED612 + 1, 32);
    WriteMem<i8>(0x004ED66E + 1, 32);
    WriteMem<i8>(0x004ED72E + 1, 32);
    WriteMem<i8>(0x004EDB02 + 1, 32);
#endif

#if 1 // Version Easter Egg
    MenuMainProcessMouse_hook.Install();
    MenuMainRender_hook.Install();
#endif

    // Fix console rendering when changing level
    AsmWriter(0x0047C490).ret();
    AsmWriter(0x0047C4AA).ret();
    AsmWriter(0x004B2E15).nop(2);
    MenuUpdate_hook.Install();

    // Increase damage for kill command in Single Player
    WriteMem<float>(0x004A4DF5 + 1, 100000.0f);

    // Fix keyboard layout
    uint8_t kbd_layout = 0;
    if (MapVirtualKeyA(0x10, MAPVK_VSC_TO_VK) == 'A')
        kbd_layout = 2; // AZERTY
    else if (MapVirtualKeyA(0x15, MAPVK_VSC_TO_VK) == 'Z')
        kbd_layout = 3; // QWERTZ
    INFO("Keyboard layout: %u", kbd_layout);
    WriteMem<u8>(0x004B14B4 + 1, kbd_layout);

    // Level sounds
    SetPlaySoundEventsVolumeScale(g_game_config.level_sound_volume);
    SndConvertVolume3D_AmbientSound_hook.Install();

    // Chat color alpha
    using namespace asm_regs;
    AsmWriter(0x00477490, 0x004774A4).mov(eax, 0x30); // chatbox border
    AsmWriter(0x00477528, 0x00477535).mov(ebx, 0x40); // chatbox background
    AsmWriter(0x00478E00, 0x00478E14).mov(eax, 0x30); // chat input border
    AsmWriter(0x00478E91, 0x00478E9E).mov(ebx, 0x40); // chat input background

    // Show enemy bullets
    rf::hide_enemy_bullets = !g_game_config.show_enemy_bullets;
    show_enemy_bullets_cmd.Register();

    // Swap Assault Rifle fire controls
    PlayerLocalFireControl_hook.Install();
    IsEntityCtrlActive_hook1.Install();
    IsEntityCtrlActive_hook2.Install();
    swap_assault_rifle_controls_cmd.Register();

    // Fix crash in shadows rendering
    WriteMem<u8>(0x0054A3C0 + 2, 16);

    // Fix crash in geometry rendering
    GeomCachePrepareRoom_hook.Install();

    // Remove Sleep calls in TimerInit
    AsmWriter(0x00504A67, 0x00504A82).nop();

    // Use spawnpoint team property in TeamDM game (PF compatible)
    WriteMem<u8>(0x00470395 + 4, 0); // change cmp argument: CTF -> DM
    WriteMem<u8>(0x0047039A, asm_opcodes::jz_rel_short);  // invert jump condition: jnz -> jz

    // Put not responding servers at the bottom of server list
    ServerListCmpFunc_hook.Install();

    // Disable broken optimization of segment vs geometry collision test
    // Fixes hitting objects if mover is in the line of the shot
    AsmWriter(0x00499055).jmp(0x004990B4);

    // Disable Flamethower debug sphere drawing (optimization)
    // It is not visible in game because other things are drawn over it
    AsmWriter(0x0041AE47, 0x0041AE4C).nop();

    // Fix game beeping every frame if chat input buffer is full
    ChatSayAddChar_hook.Install();

    // Change chat input limit to 224 (RF can support 255 safely but PF kicks if message is longer than 224)
    WriteMem<i32>(0x0044474A + 1, CHAT_MSG_MAX_LEN);

    // Add chat message limit for say/teamsay commands
    ChatSayAccept_hook.Install();

    // Preserve password case when processing rcon_request command
    WriteMem<i8>(0x0046C85A + 1, 1);

    // Solo/Teleport triggers handling + filtering by team ID
    TriggerActivate_hook.Install();
    SendTriggerActivatePacketToAllPlayers_hook.Install();

    // Client-side trigger flag handling
    TriggerCheckActivation_patch.Install();

    // Fix crash when loading savegame with missing player entity data
    AsmWriter(0x004B4B47).jmp(0x004B4B7B);

    // Add checking if restoring game state from save file failed during level loading
    RflLoadInternal_CheckRestoreStatus_patch.Install();

    // Fix creating corrupted saves if cutscene starts in the same frame as quick save button is pressed
    DoQuickSave_hook.Install();

    // Support skipping cutscenes
    MenuInGameUpdateCutscene_hook.Install();
    PlayHardcodedBackgroundMusicForCutscene_hook.Install();
    skip_cutscene_bind_cmd.Register();

    // Open server list menu instead of main menu when leaving multiplayer game
    GameEnterState_hook.Install();
    IsGameStateUiHidden_hook.Install();
    MultiAfterPlayersPackets_hook.Install();

    // Fix glares/coronas being visible through characters
    CoronaEntityCollisionTestFix.Install();

    // Corona rendering optimization
    GlareRenderCorona_hook.Install();

    // Hide main window when displaying critical error message box
    CriticalError_hide_main_wnd_patch.Install();

    // Allow undefined mp_character in PlayerCreateEntity
    // Fixes Go_Undercover event not changing player 3rd person character
    AsmWriter(0x004A414F, 0x004A4153).nop();

    // High monitors/mirrors resolution
    ClutterInitMonitor_hook.Install();

    // Fix font texture leak
    // Original code sets bitmap handle in all fonts to -1 on level unload. On next font usage the font bitmap is reloaded.
    // Note: font bitmaps are dynamic (USERBMAP) so they cannot be found by name unlike normal bitmaps.
    AsmWriter(0x0050E1A8).ret();
    GrLoadFontInternal_fix_texture_ref.Install();

    // Fix crash when skipping cutscene after robot kill in L7S4
    moving_group_rotate_in_place_keyframe_oob_crashfix.Install();

    // Fix crash in LEGO_MP mod caused by XSTR(1000, "RL"); for some reason it does not crash in PF...
    parser_xstr_oob_fix.Install();

    // Fix crashes caused by too many records in tbl files
    ammo_tbl_buffer_overflow_fix.Install();
    clutter_tbl_buffer_overflow_fix.Install();
    weapons_tbl_buffer_overflow_fix_1.Install();
    weapons_tbl_buffer_overflow_fix_2.Install();
    strings_tbl_buffer_overflow_fix.Install();

    // Fix killed glass restoration from a save file
    AsmWriter(0x0043604A).nop(5);
    glass_kill_init_fix.Install();

#if 0
    // Fix weapon switch glitch when reloading (should be used on Match Mode)
    AsmWriter(0x004A4B4B).call(EntityIsReloading_SwitchWeapon_New);
    AsmWriter(0x004A4B77).call(EntityIsReloading_SwitchWeapon_New);
#endif

    // Fix crash caused by buffer overflows in level save/load code
    SaveRestoreDeserializeAllObjects_hook.Install();

    // Log error when object cannot be created
    ObjCreate_hook.Install();

    // Delete weapons (projectiles) that reach bounding box of the level
    WeaponMoveOne_hook.Install();

    // Increase weapon (projectile) limit from 50 to 100
    // Disabled by default to not cause any trouble because of total object limit (e.g. big limit would block respawning)
#ifdef BIG_WEAPON_POOL
    const i8 weapon_pool_size = 100;
    WriteMem<u8>(0x0048B5BB, asm_opcodes::jmp_rel_short);
    WriteMem<i8>(0x00487271 + 6, weapon_pool_size);
    //WriteMem<u8>(0x0048B59B, asm_opcodes::jmp_rel_short); // uncomment to always use dynamic allocation (it would cause a leak)
    weapon_pool_alloc_zero_dynamic_mem.Install();
#endif

    // Use local_player variable for debris distance calculation instead of local_entity
    // Fixed debris pool being exhausted when local player is dead
    AsmWriter(0x0042A223, 0x0042A232).mov(asm_regs::ecx, {&rf::local_player});

    // Delete sounds with lowest volume when there is no free slot for a new sound
    snd_ds_play_3d_no_free_slots_fix.Install();
    snd_ds_play_no_free_slots_fix.Install();
    PlaySound_no_free_slots_fix.Install();
    //WriteMem<u8>(0x005055AB, asm_opcodes::jmp_rel_short); // never free level sounds, uncomment to test

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

    // Fix memory leak on quick save
    quick_save_mem_leak_fix.Install();

    // Fix PlayBikFile texture leak
    play_bik_file_vram_leak_fix.Install();

    // Fix crash after level change (Load_Level event) caused by NavPoint pointers in AiNav not being cleared for entities
    // being taken from the previous level
    AiNavClear_on_load_level_event_crash_fix.Install();
}

void MiscCleanup()
{
}
