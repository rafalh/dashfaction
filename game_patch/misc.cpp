#include "misc.h"
#include "commands.h"
#include "main.h"
#include "rf.h"
#include "stdafx.h"
#include "utils.h"
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

int g_version_label_x, g_version_label_width, g_version_label_height;
int g_version_click_counter = 0;
int g_egg_anim_start;

using UiLabel_Create2_Type = void __fastcall(rf::UiGadget*, void*, rf::UiGadget*, int, int, int, int, const char*, int);
extern CallHook<UiLabel_Create2_Type> UiLabel_Create2_VersionLabel_hook;
void __fastcall UiLabel_Create2_VersionLabel(rf::UiGadget* self, void* edx, rf::UiGadget* parent, int x, int y, int w,
                                             int h, const char* text, int font_id)
{
    x = g_version_label_x;
    w = g_version_label_width;
    h = g_version_label_height;
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
        rf::GrGetTextWidth(&g_version_label_width, &g_version_label_height, version_in_menu, -1, rf::medium_font_id);

        g_version_label_x = 430 - g_version_label_width;
        g_version_label_width = g_version_label_width + 5;
        g_version_label_height = g_version_label_height + 2;
    },
};

FunHook<int()> MenuUpdate_hook{
    0x00434230,
    []() {
        int menu_id = MenuUpdate_hook.CallTarget();
        if (menu_id == rf::GS_MP_LIMBO) // hide cursor when changing level - hackfixed in RF by chaning rendering logic
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
    return entity && entity->weapon_info.weapon_cls_id == assault_rifle_cls_id;
}

FunHook<void(rf::Player*, bool, bool)> PlayerLocalFireControl_hook{
    0x004A4E80,
    [](rf::Player* player, bool secondary, bool was_pressed) {
        if (g_game_config.swap_assault_rifle_controls && IsHoldingAssaultRifle())
            secondary = !secondary;
        PlayerLocalFireControl_hook.CallTarget(player, secondary, was_pressed);
    },
};

extern CallHook<char(rf::ControlConfig*, rf::GameCtrl, bool*)> IsEntityCtrlActive_hook1;
char IsEntityCtrlActive_New(rf::ControlConfig* control_config, rf::GameCtrl game_ctrl, bool* was_pressed)
{
    if (g_game_config.swap_assault_rifle_controls && IsHoldingAssaultRifle()) {
        if (game_ctrl == rf::GC_PRIMARY_ATTACK)
            game_ctrl = rf::GC_SECONDARY_ATTACK;
        else if (game_ctrl == rf::GC_SECONDARY_ATTACK)
            game_ctrl = rf::GC_PRIMARY_ATTACK;
    }
    return IsEntityCtrlActive_hook1.CallTarget(control_config, game_ctrl, was_pressed);
}
CallHook<char(rf::ControlConfig*, rf::GameCtrl, bool*)> IsEntityCtrlActive_hook1{0x00430E65, IsEntityCtrlActive_New};
CallHook<char(rf::ControlConfig*, rf::GameCtrl, bool*)> IsEntityCtrlActive_hook2{0x00430EF7, IsEntityCtrlActive_New};

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
    []([[maybe_unused]] auto& regs) {
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

    int weapon_cls_id = entity->weapon_info.weapon_cls_id;
    if (weapon_cls_id >= 0) {
        rf::WeaponClass* weapon_cls = &rf::weapon_classes[weapon_cls_id];
        if (entity->weapon_info.weapons_ammo[weapon_cls_id] == 0 && entity->weapon_info.clip_ammo[weapon_cls->ammo_type] > 0)
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
    [](X86Regs& regs) {
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

FunHook<void(bool)> MenuInGameUpdateCutscene_hook{
    0x0045B5E0,
    [](bool dlg_open) {
        bool skip_cutscene = false;
        rf::IsEntityCtrlActive(&rf::local_player->config.controls, rf::GC_JUMP, &skip_cutscene);

        if (!skip_cutscene) {
            MenuInGameUpdateCutscene_hook.CallTarget(dlg_open);

            rf::GrSetColor(255, 255, 255, 255);
            rf::GrDrawAlignedText(rf::GR_ALIGN_CENTER, rf::GrGetMaxWidth() / 2, rf::GrGetMaxHeight() - 30,
                                  "Press JUMP key to skip the cutscene", -1, rf::gr_text_material);
        }
        else {
            auto& timer_add_delta_time = AddrAsRef<int(int delta_ms)>(0x004FA2D0);
            auto& snd_stop = AddrAsRef<char(int sig)>(0x005442B0);
            auto& destroy_all_paused_sounds = AddrAsRef<void()>(0x005059F0);
            auto& set_all_playing_sounds_paused = AddrAsRef<void(bool paused)>(0x00505C70);

            auto& timer_base = AddrAsRef<int64_t>(0x01751BF8);
            auto& timer_freq = AddrAsRef<int32_t>(0x01751C04);
            auto& frame_time = AddrAsRef<float>(0x005A4014);
            auto& current_shot_idx = StructFieldRef<int>(rf::active_cutscene, 0x808);
            void* current_shot_timer = reinterpret_cast<char*>(rf::active_cutscene) + 0x810;
            auto& num_shots = StructFieldRef<int>(rf::active_cutscene, 4);

            if (g_cutscene_bg_sound_sig != -1) {
                snd_stop(g_cutscene_bg_sound_sig);
                g_cutscene_bg_sound_sig = -1;
            }

            set_all_playing_sounds_paused(true);
            destroy_all_paused_sounds();
            rf::sound_enabled = false;

            while (rf::CutsceneIsActive()) {
                int shot_time_left_ms = rf::Timer__GetTimeLeftMs(current_shot_timer);

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

CodeInjection CoronaEntityCollisionTestFix{
    0x004152F1,
    [](X86Regs& regs) {
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
    WriteMem<u8>(0x004B31B6, ASM_SHORT_JMP_REL);
#endif // NO_CD_FIX

    // Disable thqlogo.bik
    if (g_game_config.fast_start) {
        WriteMem<u8>(0x004B208A, ASM_SHORT_JMP_REL);
        WriteMem<u8>(0x004B24FD, ASM_SHORT_JMP_REL);
    }

    // Sound loop fix
    WriteMem<u8>(0x00505D08, 0x00505D5B - (0x00505D07 + 0x2));

    // Set initial FPS limit
    WriteMem<float>(0x005094CA, 1.0f / g_game_config.max_fps);

    // Crash-fix... (probably argument for function is invalid); Page Heap is needed
    WriteMem<u32>(0x0056A28C + 1, 0);

    // Crash-fix in case texture has not been created (this happens if GrReadBackbuffer fails)
    gr_direct3d_lock_crash_fix.Install();

    // Dont overwrite player name and prefered weapons when loading saved game
    AsmWritter(0x004B4D99, 0x004B4DA5).nop();
    AsmWritter(0x004B4E0A, 0x004B4E22).nop();

    // Dont filter levels for DM and TeamDM
    WriteMem<u8>(0x005995B0, 0);
    WriteMem<u8>(0x005995B8, 0);

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
    AsmWritter(0x0047C490).ret();
    AsmWritter(0x0047C4AA).ret();
    AsmWritter(0x004B2E15).nop(2);
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
    AsmWritter(0x00477490, 0x004774A4).mov(asm_regs::eax, 0x30); // chatbox border
    AsmWritter(0x00477528, 0x00477535).mov(asm_regs::ebx, 0x40); // chatbox background
    AsmWritter(0x00478E00, 0x00478E14).mov(asm_regs::eax, 0x30); // chat input border
    AsmWritter(0x00478E91, 0x00478E9E).mov(asm_regs::ebx, 0x40); // chat input background

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
    AsmWritter(0x00504A67, 0x00504A82).nop();

    // Use spawnpoint team property in TeamDM game (PF compatible)
    WriteMem<u8>(0x00470395 + 4, 0); // change cmp argument: CTF -> DM
    WriteMem<u8>(0x0047039A, 0x74);  // invert jump condition: jnz -> jz

    // Put not responding servers at the bottom of server list
    ServerListCmpFunc_hook.Install();

    // Disable broken optimization of segment vs geometry collision test
    // Fixes hitting objects if mover is in the line of the shot
    AsmWritter(0x00499055).jmp(0x004990B4);

    // Disable Flamethower debug sphere drawing (optimization)
    // It is not visible in game because other things are drawn over it
    AsmWritter(0x0041AE47, 0x0041AE4C).nop();

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
    AsmWritter(0x004B4B47).jmp(0x004B4B7B);

    // Add checking if restoring game state from save file failed during level loading
    RflLoadInternal_CheckRestoreStatus_patch.Install();

    // Fix creating corrupted saves if cutscene starts in the same frame as quick save button is pressed
    DoQuickSave_hook.Install();

    // Support skipping cutscenes
    MenuInGameUpdateCutscene_hook.Install();
    PlayHardcodedBackgroundMusicForCutscene_hook.Install();

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
    AsmWritter(0x004A414F, 0x004A4153).nop();

    // High monitors/mirrors resolution
    ClutterInitMonitor_hook.Install();

    // Fix font texture leak
    // Original code sets bitmap handle in all fonts to -1 on level unload. On next font usage the font bitmap is reloaded.
    // Note: font bitmaps are dynamic (USERBMAP) so they cannot be found by name unlike normal bitmaps.
    AsmWritter(0x0050E1A8).ret();
    GrLoadFontInternal_fix_texture_ref.Install();

#if 0
    // Fix weapon switch glitch when reloading (should be used on Match Mode)
    AsmWritter(0x004A4B4B).call(EntityIsReloading_SwitchWeapon_New);
    AsmWritter(0x004A4B77).call(EntityIsReloading_SwitchWeapon_New);
#endif
}

void MiscCleanup()
{
}
