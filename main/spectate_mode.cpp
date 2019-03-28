#include <FunHook2.h>
#include <CallHook2.h>
#include <windef.h>
#include <shlwapi.h>
#include "stdafx.h"
#include "rf.h"
#include "rfproto.h"
#include "spectate_mode.h"
#include "BuildConfig.h"
#include "utils.h"
#include "scoreboard.h"

#if SPECTATE_MODE_ENABLE

namespace rf {

static const auto IsEntityLoopFire = (bool(*)(int entity_handle, signed int weapon_cls_id))0x0041A830;
static const auto EntityIsSwimming = (bool(*)(EntityObj *entity))0x0042A0A0;
static const auto EntityIsFalling = (bool(*)(EntityObj *entit))0x0042A020;

static const auto PlayerFpgunRender = (void(*)(Player*))0x004A2B30;
static const auto PlayerFpgunUpdate = (void(*)(Player*))0x004A2700;
static const auto PlayerFpgunSetupMesh = (void(*)(Player*, int weapon_cls_id))0x004AA230;
static const auto PlayerFpgunUpdateMesh = (void(*)(Player*))0x004AA6D0;
static const auto PlayerRenderRocketLauncherScannerView = (void(*)(Player *player))0x004AEEF0;
static const auto PlayerFpgunSetState = (void(*)(Player *player, int state))0x004AA560;
static const auto PlayerFpgunHasState = (bool(*)(Player *player, int state))0x004A9520;

}

static rf::Player *g_SpectateModeTarget;
static rf::Camera *g_OldTargetCamera = NULL;
static bool g_SpectateModeEnabled = false;
static int g_LargeFont = -1, g_MediumFont = -1, g_SmallFont = -1;

static void SetCameraTarget(rf::Player *player)
{
    // Based on function SetCamera1View
    if (!rf::g_LocalPlayer || !rf::g_LocalPlayer->Camera || !player)
        return;

    rf::Camera *Camera = rf::g_LocalPlayer->Camera;
    Camera->Type = rf::CAM_FIRST_PERSON;
    Camera->Player = player;

    g_OldTargetCamera = player->Camera;
    player->Camera = Camera; // fix crash 0040D744

    rf::CameraSetFirstPerson(Camera);
}

void SpectateModeSetTargetPlayer(rf::Player *player)
{
    if (!player)
        player = rf::g_LocalPlayer;

    if (!rf::g_LocalPlayer || !rf::g_LocalPlayer->Camera || !g_SpectateModeTarget || g_SpectateModeTarget == player)
        return;

    if (rf::g_GameOptions & RF_GO_FORCE_RESPAWN)
    {
        // rf::String msg{ "You cannot use Spectate Mode because Force Respawn option is enabled on this server!" };
        // rf::String prefix;
        // rf::ChatPrint(msg, 4, prefix);
        return;
    }

    // fix old target
    if (g_SpectateModeTarget && g_SpectateModeTarget != rf::g_LocalPlayer)
    {
        g_SpectateModeTarget->Camera = g_OldTargetCamera;
        g_OldTargetCamera = NULL;

#if SPECTATE_MODE_SHOW_WEAPON
        g_SpectateModeTarget->Flags &= ~(1 << 4);
        rf::EntityObj *Entity = rf::EntityGetFromHandle(g_SpectateModeTarget->Entity_handle);
        if (Entity)
            Entity->LocalPlayer = NULL;
#endif // SPECTATE_MODE_SHOW_WEAPON
    }

    g_SpectateModeEnabled = (player != rf::g_LocalPlayer);
    g_SpectateModeTarget = player;

    rf::KillLocalPlayer();
    SetCameraTarget(player);

#if SPECTATE_MODE_SHOW_WEAPON
    player->Flags |= 1 << 4;
    rf::EntityObj *Entity = rf::EntityGetFromHandle(player->Entity_handle);
    if (Entity)
    {
        // make sure weapon mesh is loaded now
        rf::PlayerFpgunSetupMesh(player, Entity->WeaponInfo.WeaponClsId);
        TRACE("FpgunMesh %p", player->FpgunMesh);

        // Hide target player from camera
        Entity->LocalPlayer = player;
    }
#endif // SPECTATE_MODE_SHOW_WEAPON
}

static void SpectateNextPlayer(bool dir, bool try_alive_players_first = false)
{
    rf::Player *NewTarget;
    if (g_SpectateModeEnabled)
        NewTarget = g_SpectateModeTarget;
    else
        NewTarget = rf::g_LocalPlayer;
    while (true)
    {
        NewTarget = dir ? NewTarget->Next : NewTarget->Prev;
        if (!NewTarget || NewTarget == g_SpectateModeTarget)
            break; // nothing found
        if (try_alive_players_first && rf::IsPlayerEntityInvalid(NewTarget))
            continue;
        if (NewTarget != rf::g_LocalPlayer)
        {
            SpectateModeSetTargetPlayer(NewTarget);
            return;
        }
    }

    if (try_alive_players_first)
        SpectateNextPlayer(dir, false);
}

FunHook2<void(rf::Player*, rf::GameCtrl, bool)> HandleCtrlInGame_Hook{
    0x004A6210,
    [](rf::Player* player, rf::GameCtrl key_id, bool was_pressed) {
        if (g_SpectateModeEnabled) {
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
        else if (!g_SpectateModeEnabled) {
            if (key_id == rf::GC_JUMP && was_pressed && rf::IsPlayerEntityInvalid(rf::g_LocalPlayer)) {
                SpectateModeSetTargetPlayer(rf::g_LocalPlayer);
                SpectateNextPlayer(true, true);
                return;
            }
        }

        HandleCtrlInGame_Hook.CallTarget(player, key_id, was_pressed);
    }
};

bool IsPlayerEntityInvalid_New(rf::Player* player) {
    if (g_SpectateModeEnabled)
        return false;
    else
        return rf::IsPlayerEntityInvalid(player);
}

CallHook2<bool(rf::Player*)> IsPlayerEntityInvalid_RedBars_Hook{ 0x00432A52, IsPlayerEntityInvalid_New };
CallHook2<bool(rf::Player*)> IsPlayerEntityInvalid_Scoreboard_Hook{ 0x00437BEE, IsPlayerEntityInvalid_New };
CallHook2<bool(rf::Player*)> IsPlayerEntityInvalid_Scoreboard2_Hook{ 0x00437C25, IsPlayerEntityInvalid_New };

static bool IsPlayerDying_New(rf::Player* player)
{
    if (g_SpectateModeEnabled)
        return false;
    else
        return rf::IsPlayerDying(player);
}

CallHook2 IsPlayerDying_RedBars_Hook{ 0x00432A5F, IsPlayerDying_New };
CallHook2 IsPlayerDying_Scoreboard_Hook{ 0x00437C01, IsPlayerDying_New };
CallHook2 IsPlayerDying_Scoreboard2_Hook{ 0x00437C36, IsPlayerDying_New };

void SpectateModeOnDestroyPlayer(rf::Player *player)
{
    if (g_SpectateModeTarget == player)
        SpectateNextPlayer(true);
    if (g_SpectateModeTarget == player)
        SpectateModeSetTargetPlayer(nullptr);
}

FunHook2<void(rf::Player*)> RenderReticle_Hook{
    0x0043A2C0,
    [](rf::Player* player) {
        if (rf::GameSeqGetState() == rf::GS_MP_LIMBO)
            return;
        if (g_SpectateModeEnabled)
            RenderReticle_Hook.CallTarget(g_SpectateModeTarget);
        else
            RenderReticle_Hook.CallTarget(player);
    }
};

FunHook2<rf::EntityObj*(rf::Player*, int, const rf::Vector3*, const rf::Matrix3*, int)> PlayerCreateEntity_Hook{
    0x004A4130,
    [](rf::Player* player, int cls_id, const rf::Vector3* pos, const rf::Matrix3* orient, int mp_character) {
        // hide target player from camera after respawn
        rf::EntityObj* entity = PlayerCreateEntity_Hook.CallTarget(player, cls_id, pos, orient, mp_character);
        if (entity && player == g_SpectateModeTarget)
            entity->LocalPlayer = player;

        return entity;
    }
};

CallHook2<void()> GrResetClip_RenderScannerViewForLocalPlayers_Hook{
    0x00431890,
    []() {
        if (g_SpectateModeEnabled)
            rf::PlayerRenderRocketLauncherScannerView(g_SpectateModeTarget);
        GrResetClip_RenderScannerViewForLocalPlayers_Hook.CallTarget();
    }
};

#if SPECTATE_MODE_SHOW_WEAPON

static void PlayerFpgunRender_New(rf::Player *player)
{
    if (g_SpectateModeEnabled)
    {
        rf::EntityObj *Entity = rf::EntityGetFromHandle(g_SpectateModeTarget->Entity_handle);

        // HACKFIX: RF uses function PlayerSetRemoteChargeVisible for local player only
        g_SpectateModeTarget->WeaponInfo.RemoteChargeVisible = (Entity && Entity->WeaponInfo.WeaponClsId == rf::g_RemoteChargeClsId);

        if (g_SpectateModeTarget != rf::g_LocalPlayer && Entity)
        {
            static rf::Vector3 OldVel;
            rf::Vector3 VelDiff = Entity->_Super.PhysInfo.Vel - OldVel;
            OldVel = Entity->_Super.PhysInfo.Vel;

            if (VelDiff.y > 0.1f)
                Entity->EntityFlags |= 2; // jump
        }


        if (g_SpectateModeTarget->WeaponInfo.InScopeView)
            g_SpectateModeTarget->WeaponInfo.fScopeZoom = 2.0f;
        rf::g_LocalPlayer->WeaponInfo.InScopeView = g_SpectateModeTarget->WeaponInfo.InScopeView;
        rf::g_LocalPlayer->WeaponInfo.fScopeZoom = g_SpectateModeTarget->WeaponInfo.fScopeZoom;

        rf::PlayerFpgunUpdateMesh(g_SpectateModeTarget);
        rf::PlayerFpgunRender(g_SpectateModeTarget);
    }
    else
        rf::PlayerFpgunRender(player);
}

FunHook2<void(rf::Player*)> PlayerFpgunUpdateState_Hook{
    0x004AA3A0,
    [](rf::Player* player) {
        PlayerFpgunUpdateState_Hook.CallTarget(player);
        if (player != rf::g_LocalPlayer) {
            rf::EntityObj* entity = rf::EntityGetFromHandle(player->Entity_handle);
            if (entity) {
                float fHorzSpeedPow2 = entity->_Super.PhysInfo.Vel.x * entity->_Super.PhysInfo.Vel.x
                    + entity->_Super.PhysInfo.Vel.z * entity->_Super.PhysInfo.Vel.z;
                int state = 0;
                if (rf::IsEntityLoopFire(entity->_Super.Handle, entity->WeaponInfo.WeaponClsId))
                    state = 2;
                else if (rf::EntityIsSwimming(entity) || rf::EntityIsFalling(entity))
                    state = 0;
                else if (fHorzSpeedPow2 > 0.2f)
                    state = 1;
                if (!rf::PlayerFpgunHasState(player, state))
                    rf::PlayerFpgunSetState(player, state);
            }
        }
    }
};

#endif // SPECTATE_MODE_SHOW_WEAPON

void SpectateModeInit()
{
    IsPlayerDying_RedBars_Hook.Install();
    IsPlayerDying_Scoreboard_Hook.Install();
    IsPlayerDying_Scoreboard2_Hook.Install();

    IsPlayerEntityInvalid_RedBars_Hook.Install();
    IsPlayerEntityInvalid_Scoreboard_Hook.Install();
    IsPlayerEntityInvalid_Scoreboard2_Hook.Install();

    HandleCtrlInGame_Hook.Install();
    RenderReticle_Hook.Install();
    PlayerCreateEntity_Hook.Install();

    // Note: HUD rendering doesn't make sense because life and armor isn't synced

#if SPECTATE_MODE_SHOW_WEAPON
    WriteMem<i32>(0x0043285D + 1, (uintptr_t)PlayerFpgunRender_New - (0x0043285D + 0x5));
    AsmWritter(0x004AB1B8).nop(6); // PlayerFpgunRenderInternal
    AsmWritter(0x004AA23E).nop(6); // PlayerFpgunSetupMesh
    AsmWritter(0x004AE0DF).nop(2); // PlayerFpgunLoadMesh

    AsmWritter(0x004A938F).nop(6); // PlayerFpgunSetAction
    WriteMem<u8>(0x004A952C, ASM_SHORT_JMP_REL); // PlayerFpgunHasState
    AsmWritter(0x004AA56D).nop(6); // PlayerFpgunSetState
    AsmWritter(0x004AA6E7).nop(6); // PlayerFpgunUpdateMesh
    AsmWritter(0x004AE384).nop(6); // PlayerFpgunPrepareWeapon
    WriteMem<u8>(0x004ACE2C, ASM_SHORT_JMP_REL); // GetZoomValue

    WriteMemPtr(0x0048857E + 2, &g_SpectateModeTarget); // RenderObjects
    WriteMemPtr(0x00488598 + 1, &g_SpectateModeTarget); // RenderObjects
    WriteMemPtr(0x00421889 + 2, &g_SpectateModeTarget); // EntityRender
    WriteMemPtr(0x004218A2 + 2, &g_SpectateModeTarget); // EntityRender

    GrResetClip_RenderScannerViewForLocalPlayers_Hook.Install();
    PlayerFpgunUpdateState_Hook.Install();
#endif // SPECTATE_MODE_SHOW_WEAPON
}

void SpectateModeAfterFullGameInit()
{
    g_SpectateModeTarget = rf::g_LocalPlayer;
}

void SpectateModeDrawUI()
{
    if (!g_SpectateModeEnabled)
    {
        if (rf::IsPlayerEntityInvalid(rf::g_LocalPlayer))
        {
            rf::GrSetColor(0xFF, 0xFF, 0xFF, 0xFF);
            rf::GrDrawAlignedText(rf::GR_ALIGN_LEFT, 20, 200, "Press JUMP key to enter Spectate Mode", -1, rf::g_GrTextMaterial);
        }
        return;
    }

    if (g_LargeFont == -1)
        g_LargeFont = rf::GrLoadFont("rfpc-large.vf", -1);
    if (g_MediumFont == -1)
        g_MediumFont = rf::GrLoadFont("rfpc-medium.vf", -1);
    if (g_SmallFont == -1)
        g_SmallFont = rf::GrLoadFont("rfpc-small.vf", -1);

    const unsigned cx = 500, cy = 50;
    unsigned cxScr = rf::GrGetMaxWidth(), cySrc = rf::GrGetMaxHeight();
    unsigned x = (cxScr - cx) / 2;
    unsigned y = cySrc - 100;
    unsigned cyFont = rf::GrGetFontHeight(-1);

    rf::GrSetColor(0, 0, 0, 0x80);
    rf::GrDrawAlignedText(rf::GR_ALIGN_CENTER, cxScr / 2 + 2, 150 + 2, "SPECTATE MODE", g_LargeFont, rf::g_GrTextMaterial);
    rf::GrSetColor(0xFF, 0xFF, 0xFF, 0xFF);
    rf::GrDrawAlignedText(rf::GR_ALIGN_CENTER, cxScr / 2, 150, "SPECTATE MODE", g_LargeFont, rf::g_GrTextMaterial);

    rf::GrSetColor(0xFF, 0xFF, 0xFF, 0xFF);
    rf::GrDrawAlignedText(rf::GR_ALIGN_LEFT, 20, 200, "Press JUMP key to exit Spectate Mode", g_MediumFont, rf::g_GrTextMaterial);
    rf::GrDrawAlignedText(rf::GR_ALIGN_LEFT, 20, 215, "Press PRIMARY ATTACK key to switch to the next player", g_MediumFont, rf::g_GrTextMaterial);
    rf::GrDrawAlignedText(rf::GR_ALIGN_LEFT, 20, 230, "Press SECONDARY ATTACK key to switch to the previous player", g_MediumFont, rf::g_GrTextMaterial);

    rf::GrSetColor(0, 0, 0x00, 0x60);
    rf::GrDrawRect(x, y, cx, cy, rf::g_GrRectMaterial);

    char Buf[256];
    rf::GrSetColor(0xFF, 0xFF, 0, 0x80);
    snprintf(Buf, sizeof(Buf), "Spectating: %s", g_SpectateModeTarget->strName.CStr());
    rf::GrDrawAlignedText(rf::GR_ALIGN_CENTER, x + cx / 2, y + cy / 2 - cyFont / 2 - 5, Buf, g_LargeFont, rf::g_GrTextMaterial);

    rf::EntityObj *Entity = rf::EntityGetFromHandle(g_SpectateModeTarget->Entity_handle);
    if (!Entity)
    {
        rf::GrSetColor(0xFF, 0xFF, 0xFF, 0xFF);
        static int BloodBm = rf::BmLoad("bloodsmear07_A.tga", -1, true);
        int BloodW, BloodH;
        rf::BmGetBitmapSize(BloodBm, &BloodW, &BloodH);
        rf::GrDrawBitmapStretched(BloodBm, (cxScr - BloodW*2) / 2, (cySrc - BloodH*2) / 2, BloodW * 2 , BloodH * 2,
            0, 0, BloodW, BloodH, 0.0f, 0.0f, rf::g_GrBitmapMaterial);

        rf::GrSetColor(0, 0, 0, 0x80);
        rf::GrDrawAlignedText(rf::GR_ALIGN_CENTER, cxScr / 2 + 2, cySrc / 2 + 2, "DEAD", g_LargeFont, rf::g_GrTextMaterial);
        rf::GrSetColor(0xF0, 0x20, 0x10, 0xC0);
        rf::GrDrawAlignedText(rf::GR_ALIGN_CENTER, cxScr / 2, cySrc / 2, "DEAD", g_LargeFont, rf::g_GrTextMaterial);
    }
}

bool SpectateModeIsActive()
{
    return g_SpectateModeEnabled;
}

#endif // SPECTATE_MODE_ENABLE
