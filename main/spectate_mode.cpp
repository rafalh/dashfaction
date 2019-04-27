#include "spectate_mode.h"
#include "BuildConfig.h"
#include "rf.h"
#include "rfproto.h"
#include "scoreboard.h"
#include "stdafx.h"
#include "utils.h"
#include <CallHook.h>
#include <FunHook.h>
#include <shlwapi.h>
#include <windef.h>

namespace rf
{

static auto& IsEntityLoopFire = AddrAsRef<bool(int entity_handle, signed int weapon_cls_id)>(0x0041A830);
static auto& EntityIsSwimming = AddrAsRef<bool(EntityObj* entity)>(0x0042A0A0);
static auto& EntityIsFalling = AddrAsRef<bool(EntityObj* entit)>(0x0042A020);

static auto& PlayerFpgunRender = AddrAsRef<void(Player*)>(0x004A2B30);
static auto& PlayerFpgunUpdate = AddrAsRef<void(Player*)>(0x004A2700);
static auto& PlayerFpgunSetupMesh = AddrAsRef<void(Player*, int weapon_cls_id)>(0x004AA230);
static auto& PlayerFpgunUpdateMesh = AddrAsRef<void(Player*)>(0x004AA6D0);
static auto& PlayerRenderRocketLauncherScannerView = AddrAsRef<void(Player* player)>(0x004AEEF0);
static auto& PlayerFpgunSetState = AddrAsRef<void(Player* player, int state)>(0x004AA560);
static auto& PlayerFpgunHasState = AddrAsRef<bool(Player* player, int state)>(0x004A9520);

} // namespace rf

static rf::Player* g_spectate_mode_target;
static rf::Camera* g_old_target_camera = nullptr;
static bool g_spectate_mode_enabled = false;

static void SetCameraTarget(rf::Player* player)
{
    // Based on function SetCamera1View
    if (!rf::local_player || !rf::local_player->camera || !player)
        return;

    rf::Camera* camera = rf::local_player->camera;
    camera->type = rf::CAM_FIRST_PERSON;
    camera->player = player;

    g_old_target_camera = player->camera;
    player->camera = camera; // fix crash 0040D744

    rf::CameraSetFirstPerson(camera);
}

void SpectateModeSetTargetPlayer(rf::Player* player)
{
    if (!player)
        player = rf::local_player;

    if (!rf::local_player || !rf::local_player->camera || !g_spectate_mode_target || g_spectate_mode_target == player)
        return;

    if (rf::game_options & RF_GO_FORCE_RESPAWN) {
        rf::String msg{"You cannot use Spectate Mode because Force Respawn option is enabled on this server!"};
        rf::String prefix;
        rf::ChatPrint(msg, rf::ChatMsgColor::white_white, prefix);
        return;
    }

    // fix old target
    if (g_spectate_mode_target && g_spectate_mode_target != rf::local_player) {
        g_spectate_mode_target->camera = g_old_target_camera;
        g_old_target_camera = nullptr;

#if SPECTATE_MODE_SHOW_WEAPON
        g_spectate_mode_target->flags &= ~(1 << 4);
        rf::EntityObj* entity = rf::EntityGetFromHandle(g_spectate_mode_target->entity_handle);
        if (entity)
            entity->local_player = nullptr;
#endif // SPECTATE_MODE_SHOW_WEAPON
    }

    g_spectate_mode_enabled = (player != rf::local_player);
    g_spectate_mode_target = player;

    rf::KillLocalPlayer();
    SetCameraTarget(player);

#if SPECTATE_MODE_SHOW_WEAPON
    player->flags |= 1 << 4;
    rf::EntityObj* entity = rf::EntityGetFromHandle(player->entity_handle);
    if (entity) {
        // make sure weapon mesh is loaded now
        rf::PlayerFpgunSetupMesh(player, entity->weapon_info.weapon_cls_id);
        TRACE("FpgunMesh %p", player->fpgun_mesh);

        // Hide target player from camera
        entity->local_player = player;
    }
#endif // SPECTATE_MODE_SHOW_WEAPON
}

static void SpectateNextPlayer(bool dir, bool try_alive_players_first = false)
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
        if (try_alive_players_first && rf::IsPlayerEntityInvalid(new_target))
            continue;
        if (new_target != rf::local_player) {
            SpectateModeSetTargetPlayer(new_target);
            return;
        }
    }

    if (try_alive_players_first)
        SpectateNextPlayer(dir, false);
}

FunHook<void(rf::Player*, rf::GameCtrl, bool)> HandleCtrlInGame_hook{
    0x004A6210,
    [](rf::Player* player, rf::GameCtrl key_id, bool was_pressed) {
        if (g_spectate_mode_enabled) {
            if (key_id == rf::GC_PRIMARY_ATTACK || key_id == rf::GC_SLIDE_RIGHT) {
                if (was_pressed)
                    SpectateNextPlayer(true);
                return; // dont allow spawn
            }
            else if (key_id == rf::GC_SECONDARY_ATTACK || key_id == rf::GC_SLIDE_LEFT) {
                if (was_pressed)
                    SpectateNextPlayer(false);
                return;
            }
            else if (key_id == rf::GC_JUMP) {
                if (was_pressed)
                    SpectateModeSetTargetPlayer(nullptr);
                return;
            }
        }
        else if (!g_spectate_mode_enabled) {
            if (key_id == rf::GC_JUMP && was_pressed && rf::IsPlayerEntityInvalid(rf::local_player)) {
                SpectateModeSetTargetPlayer(rf::local_player);
                SpectateNextPlayer(true, true);
                return;
            }
        }

        HandleCtrlInGame_hook.CallTarget(player, key_id, was_pressed);
    },
};

bool IsPlayerEntityInvalid_New(rf::Player* player)
{
    if (g_spectate_mode_enabled)
        return false;
    else
        return rf::IsPlayerEntityInvalid(player);
}

CallHook<bool(rf::Player*)> IsPlayerEntityInvalid_RedBars_hook{0x00432A52, IsPlayerEntityInvalid_New};
CallHook<bool(rf::Player*)> IsPlayerEntityInvalid_Scoreboard_hook{0x00437BEE, IsPlayerEntityInvalid_New};
CallHook<bool(rf::Player*)> IsPlayerEntityInvalid_Scoreboard2_hook{0x00437C25, IsPlayerEntityInvalid_New};

static bool IsPlayerDying_New(rf::Player* player)
{
    if (g_spectate_mode_enabled)
        return false;
    else
        return rf::IsPlayerDying(player);
}

CallHook IsPlayerDying_RedBars_hook{0x00432A5F, IsPlayerDying_New};
CallHook IsPlayerDying_Scoreboard_hook{0x00437C01, IsPlayerDying_New};
CallHook IsPlayerDying_Scoreboard2_hook{0x00437C36, IsPlayerDying_New};

void SpectateModeOnDestroyPlayer(rf::Player* player)
{
    if (g_spectate_mode_target == player)
        SpectateNextPlayer(true);
    if (g_spectate_mode_target == player)
        SpectateModeSetTargetPlayer(nullptr);
}

FunHook<void(rf::Player*)> RenderReticle_hook{
    0x0043A2C0,
    [](rf::Player* player) {
        if (rf::GameSeqGetState() == rf::GS_MP_LIMBO)
            return;
        if (g_spectate_mode_enabled)
            RenderReticle_hook.CallTarget(g_spectate_mode_target);
        else
            RenderReticle_hook.CallTarget(player);
    },
};

FunHook<rf::EntityObj*(rf::Player*, int, const rf::Vector3*, const rf::Matrix3*, int)> PlayerCreateEntity_hook{
    0x004A4130,
    [](rf::Player* player, int cls_id, const rf::Vector3* pos, const rf::Matrix3* orient, int mp_character) {
        // hide target player from camera after respawn
        rf::EntityObj* entity = PlayerCreateEntity_hook.CallTarget(player, cls_id, pos, orient, mp_character);
        if (entity && player == g_spectate_mode_target)
            entity->local_player = player;

        return entity;
    },
};

CallHook<void()> GrResetClip_RenderScannerViewForLocalPlayers_hook{
    0x00431890,
    []() {
        if (g_spectate_mode_enabled)
            rf::PlayerRenderRocketLauncherScannerView(g_spectate_mode_target);
        GrResetClip_RenderScannerViewForLocalPlayers_hook.CallTarget();
    },
};

#if SPECTATE_MODE_SHOW_WEAPON

static void PlayerFpgunRender_New(rf::Player* player)
{
    if (g_spectate_mode_enabled) {
        rf::EntityObj* entity = rf::EntityGetFromHandle(g_spectate_mode_target->entity_handle);

        // HACKFIX: RF uses function PlayerSetRemoteChargeVisible for local player only
        g_spectate_mode_target->weapon_info.remote_charge_visible =
            (entity && entity->weapon_info.weapon_cls_id == rf::remote_charge_cls_id);

        if (g_spectate_mode_target != rf::local_player && entity) {
            static rf::Vector3 old_vel;
            rf::Vector3 vel_diff = entity->_super.phys_info.vel - old_vel;
            old_vel = entity->_super.phys_info.vel;

            if (vel_diff.y > 0.1f)
                entity->entity_flags |= 2; // jump
        }

        if (g_spectate_mode_target->weapon_info.in_scope_view)
            g_spectate_mode_target->weapon_info.scope_zoom = 2.0f;
        rf::local_player->weapon_info.in_scope_view = g_spectate_mode_target->weapon_info.in_scope_view;
        rf::local_player->weapon_info.scope_zoom = g_spectate_mode_target->weapon_info.scope_zoom;

        rf::PlayerFpgunUpdateMesh(g_spectate_mode_target);
        rf::PlayerFpgunRender(g_spectate_mode_target);
    }
    else
        rf::PlayerFpgunRender(player);
}

FunHook<void(rf::Player*)> PlayerFpgunUpdateState_hook{
    0x004AA3A0,
    [](rf::Player* player) {
        PlayerFpgunUpdateState_hook.CallTarget(player);
        if (player != rf::local_player) {
            rf::EntityObj* entity = rf::EntityGetFromHandle(player->entity_handle);
            if (entity) {
                float horz_speed_pow2 = entity->_super.phys_info.vel.x * entity->_super.phys_info.vel.x +
                                          entity->_super.phys_info.vel.z * entity->_super.phys_info.vel.z;
                int state = 0;
                if (rf::IsEntityLoopFire(entity->_super.handle, entity->weapon_info.weapon_cls_id))
                    state = 2;
                else if (rf::EntityIsSwimming(entity) || rf::EntityIsFalling(entity))
                    state = 0;
                else if (horz_speed_pow2 > 0.2f)
                    state = 1;
                if (!rf::PlayerFpgunHasState(player, state))
                    rf::PlayerFpgunSetState(player, state);
            }
        }
    },
};

#endif // SPECTATE_MODE_SHOW_WEAPON

void SpectateModeInit()
{
    IsPlayerDying_RedBars_hook.Install();
    IsPlayerDying_Scoreboard_hook.Install();
    IsPlayerDying_Scoreboard2_hook.Install();

    IsPlayerEntityInvalid_RedBars_hook.Install();
    IsPlayerEntityInvalid_Scoreboard_hook.Install();
    IsPlayerEntityInvalid_Scoreboard2_hook.Install();

    HandleCtrlInGame_hook.Install();
    RenderReticle_hook.Install();
    PlayerCreateEntity_hook.Install();

    // Note: HUD rendering doesn't make sense because life and armor isn't synced

#if SPECTATE_MODE_SHOW_WEAPON

    AsmWritter(0x0043285D).call(PlayerFpgunRender_New);
    AsmWritter(0x004AB1B8).nop(6); // PlayerFpgunRenderInternal
    AsmWritter(0x004AA23E).nop(6); // PlayerFpgunSetupMesh
    AsmWritter(0x004AE0DF).nop(2); // PlayerFpgunLoadMesh

    AsmWritter(0x004A938F).nop(6);               // PlayerFpgunSetAction
    WriteMem<u8>(0x004A952C, ASM_SHORT_JMP_REL); // PlayerFpgunHasState
    AsmWritter(0x004AA56D).nop(6);               // PlayerFpgunSetState
    AsmWritter(0x004AA6E7).nop(6);               // PlayerFpgunUpdateMesh
    AsmWritter(0x004AE384).nop(6);               // PlayerFpgunPrepareWeapon
    WriteMem<u8>(0x004ACE2C, ASM_SHORT_JMP_REL); // GetZoomValue

    WriteMemPtr(0x0048857E + 2, &g_spectate_mode_target); // RenderObjects
    WriteMemPtr(0x00488598 + 1, &g_spectate_mode_target); // RenderObjects
    WriteMemPtr(0x00421889 + 2, &g_spectate_mode_target); // EntityRender
    WriteMemPtr(0x004218A2 + 2, &g_spectate_mode_target); // EntityRender

    GrResetClip_RenderScannerViewForLocalPlayers_hook.Install();
    PlayerFpgunUpdateState_hook.Install();
#endif // SPECTATE_MODE_SHOW_WEAPON
}

void SpectateModeAfterFullGameInit()
{
    g_spectate_mode_target = rf::local_player;
}

void SpectateModeDrawUI()
{
    if (!g_spectate_mode_enabled) {
        if (rf::IsPlayerEntityInvalid(rf::local_player)) {
            rf::GrSetColor(0xFF, 0xFF, 0xFF, 0xFF);
            rf::GrDrawAlignedText(rf::GR_ALIGN_LEFT, 20, 200, "Press JUMP key to enter Spectate Mode", -1,
                                  rf::gr_text_material);
        }
        return;
    }

    static int large_font = -1;
    if (large_font == -1)
        large_font = rf::GrLoadFont("rfpc-large.vf", -1);
    static int medium_font = -1;
    if (medium_font == -1)
        medium_font = rf::GrLoadFont("rfpc-medium.vf", -1);
    static int small_font = -1;
    if (small_font == -1)
        small_font = rf::GrLoadFont("rfpc-small.vf", -1);

    const unsigned cx = 500, cy = 50;
    unsigned cx_scr = rf::GrGetMaxWidth(), cy_src = rf::GrGetMaxHeight();
    unsigned x = (cx_scr - cx) / 2;
    unsigned y = cy_src - 100;
    unsigned cy_font = rf::GrGetFontHeight(-1);

    rf::GrSetColor(0, 0, 0, 0x80);
    rf::GrDrawAlignedText(rf::GR_ALIGN_CENTER, cx_scr / 2 + 2, 150 + 2, "SPECTATE MODE", large_font,
                          rf::gr_text_material);
    rf::GrSetColor(0xFF, 0xFF, 0xFF, 0xFF);
    rf::GrDrawAlignedText(rf::GR_ALIGN_CENTER, cx_scr / 2, 150, "SPECTATE MODE", large_font, rf::gr_text_material);

    rf::GrSetColor(0xFF, 0xFF, 0xFF, 0xFF);
    rf::GrDrawAlignedText(rf::GR_ALIGN_LEFT, 20, 200, "Press JUMP key to exit Spectate Mode", medium_font,
                          rf::gr_text_material);
    rf::GrDrawAlignedText(rf::GR_ALIGN_LEFT, 20, 215, "Press PRIMARY ATTACK key to switch to the next player",
                          medium_font, rf::gr_text_material);
    rf::GrDrawAlignedText(rf::GR_ALIGN_LEFT, 20, 230, "Press SECONDARY ATTACK key to switch to the previous player",
                          medium_font, rf::gr_text_material);

    rf::GrSetColor(0, 0, 0x00, 0x60);
    rf::GrDrawRect(x, y, cx, cy, rf::gr_rect_material);

    rf::GrSetColor(0xFF, 0xFF, 0, 0x80);
    auto str = StringFormat("Spectating: %s", g_spectate_mode_target->name.CStr());
    rf::GrDrawAlignedText(rf::GR_ALIGN_CENTER, x + cx / 2, y + cy / 2 - cy_font / 2 - 5, str.c_str(), large_font,
                          rf::gr_text_material);

    rf::EntityObj* entity = rf::EntityGetFromHandle(g_spectate_mode_target->entity_handle);
    if (!entity) {
        rf::GrSetColor(0xFF, 0xFF, 0xFF, 0xFF);
        static int blood_bm = rf::BmLoad("bloodsmear07_A.tga", -1, true);
        int blood_w, blood_h;
        rf::BmGetBitmapSize(blood_bm, &blood_w, &blood_h);
        rf::GrDrawBitmapStretched(blood_bm, (cx_scr - blood_w * 2) / 2, (cy_src - blood_h * 2) / 2, blood_w * 2,
                                  blood_h * 2, 0, 0, blood_w, blood_h, 0.0f, 0.0f, rf::gr_bitmap_material);

        rf::GrSetColor(0, 0, 0, 0x80);
        rf::GrDrawAlignedText(rf::GR_ALIGN_CENTER, cx_scr / 2 + 2, cy_src / 2 + 2, "DEAD", large_font,
                              rf::gr_text_material);
        rf::GrSetColor(0xF0, 0x20, 0x10, 0xC0);
        rf::GrDrawAlignedText(rf::GR_ALIGN_CENTER, cx_scr / 2, cy_src / 2, "DEAD", large_font, rf::gr_text_material);
    }
}

bool SpectateModeIsActive()
{
    return g_spectate_mode_enabled;
}
