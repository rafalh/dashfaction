#include "stdafx.h"
#include "rf.h"
#include "rfproto.h"
#include "spectate_mode.h"
#include "BuildConfig.h"
#include "utils.h"
#include "scoreboard.h"

#include "hooks/HookCall.h"

#if SPECTATE_MODE_ENABLE

static rf::Player *g_SpectateModeTarget;
static rf::Camera *g_OldTargetCamera = NULL;
static bool g_SpectateModeEnabled = false;
static int g_LargeFont = -1, g_MediumFont = -1, g_SmallFont = -1;

auto HandleCtrlInGame_Hook = makeFunHook(rf::HandleCtrlInGame);
auto RenderReticle_Hook = makeFunHook(rf::RenderReticle);
auto PlayerCreateEntity_Hook = makeFunHook(rf::PlayerCreateEntity);
auto RenderScannerViewForLocalPlayers_Hook = makeCallHook(rf::GrResetClip);
auto PlayerFpgunUpdateState_Hook = makeFunHook(rf::PlayerFpgunUpdateState);

static void SetCameraTarget(rf::Player *pPlayer)
{
    // Based on function SetCamera1View
    if (!rf::g_pLocalPlayer || !rf::g_pLocalPlayer->pCamera || !pPlayer)
        return;

    rf::Camera *pCamera = rf::g_pLocalPlayer->pCamera;
    pCamera->Type = rf::CAM_FIRST_PERSON;
    pCamera->pPlayer = pPlayer;

    g_OldTargetCamera = pPlayer->pCamera;
    pPlayer->pCamera = pCamera; // fix crash 0040D744

    rf::CameraSetFirstPerson(pCamera);
}

void SpectateModeSetTargetPlayer(rf::Player *pPlayer)
{
    if (!pPlayer)
        pPlayer = rf::g_pLocalPlayer;

    if (!rf::g_pLocalPlayer || !rf::g_pLocalPlayer->pCamera || !g_SpectateModeTarget || g_SpectateModeTarget == pPlayer)
        return;

    if (rf::g_GameOptions & RF_GO_FORCE_RESPAWN)
    {
        rf::String strMessage, strPrefix;
        rf::String::Init(&strMessage, "You cannot use Spectate Mode because Force Respawn option is enabled on this server!");
        rf::String::InitEmpty(&strPrefix);
        rf::ChatPrint(strMessage, 4, strPrefix);
        return;
    }

    // fix old target
    if (g_SpectateModeTarget && g_SpectateModeTarget != rf::g_pLocalPlayer)
    {
        g_SpectateModeTarget->pCamera = g_OldTargetCamera;
        g_OldTargetCamera = NULL;

#if SPECTATE_MODE_SHOW_WEAPON
        g_SpectateModeTarget->Flags &= ~(1 << 4);
        rf::EntityObj *pEntity = rf::EntityGetFromHandle(g_SpectateModeTarget->hEntity);
        if (pEntity)
            pEntity->pLocalPlayer = NULL;
#endif // SPECTATE_MODE_SHOW_WEAPON
    }

    g_SpectateModeEnabled = (pPlayer != rf::g_pLocalPlayer);
    g_SpectateModeTarget = pPlayer;

    rf::KillLocalPlayer();
    SetCameraTarget(pPlayer);

#if SPECTATE_MODE_SHOW_WEAPON
    pPlayer->Flags |= 1 << 4;
    rf::EntityObj *pEntity = rf::EntityGetFromHandle(pPlayer->hEntity);
    if (pEntity)
    {
        // make sure weapon mesh is loaded now
        rf::PlayerFpgunSetupMesh(pPlayer, pEntity->WeaponInfo.WeaponClsId);
        TRACE("pFpgunMesh %p", pPlayer->pFpgunMesh);

        // Hide target player from camera
        pEntity->pLocalPlayer = pPlayer;
    }
#endif // SPECTATE_MODE_SHOW_WEAPON
}

static void SpectateNextPlayer(bool bDir, bool bTryAlivePlayersFirst = false)
{
    rf::Player *pNewTarget;
    if (g_SpectateModeEnabled)
        pNewTarget = g_SpectateModeTarget;
    else
        pNewTarget = rf::g_pLocalPlayer;
    while (true)
    {
        pNewTarget = bDir ? pNewTarget->pNext : pNewTarget->pPrev;
        if (!pNewTarget || pNewTarget == g_SpectateModeTarget)
            break; // nothing found
        if (bTryAlivePlayersFirst && rf::IsPlayerEntityInvalid(pNewTarget))
            continue;
        if (pNewTarget != rf::g_pLocalPlayer)
        {
            SpectateModeSetTargetPlayer(pNewTarget);
            return;
        }
    }

    if (bTryAlivePlayersFirst)
        SpectateNextPlayer(bDir, false);
}

static void HandleCtrlInGameHook(rf::Player *pPlayer, rf::GameCtrl KeyId, bool WasPressed)
{
    if (g_SpectateModeEnabled)
    {
        if (KeyId == rf::GC_PRIMARY_ATTACK || KeyId == rf::GC_SLIDE_RIGHT)
        {
            if (WasPressed)
                SpectateNextPlayer(true);
            return; // dont allow spawn
        }
        else if (KeyId == rf::GC_SECONDARY_ATTACK || KeyId == rf::GC_SLIDE_LEFT)
        {
            if (WasPressed)
                SpectateNextPlayer(false);
            return;
        }
        else if (KeyId == rf::GC_JUMP)
        {
            if (WasPressed)
                SpectateModeSetTargetPlayer(nullptr);
            return;
        }
    }
    else if (!g_SpectateModeEnabled)
    {
        if (KeyId == rf::GC_JUMP && WasPressed && rf::IsPlayerEntityInvalid(rf::g_pLocalPlayer))
        {
            SpectateModeSetTargetPlayer(rf::g_pLocalPlayer);
            SpectateNextPlayer(true, true);
            return;
        }
    }
    
    HandleCtrlInGame_Hook.callTrampoline(pPlayer, KeyId, WasPressed);
}

static bool IsPlayerEntityInvalidHook(rf::Player *pPlayer)
{
    if (g_SpectateModeEnabled)
        return false;
    else
        return rf::IsPlayerEntityInvalid(pPlayer);
}

static bool IsPlayerDyingHook(rf::Player *pPlayer)
{
    if (g_SpectateModeEnabled)
        return false;
    else
        return rf::IsPlayerDying(pPlayer);
}

void SpectateModeOnDestroyPlayer(rf::Player *pPlayer)
{
    if (g_SpectateModeTarget == pPlayer)
        SpectateNextPlayer(true);
    if (g_SpectateModeTarget == pPlayer)
        SpectateModeSetTargetPlayer(nullptr);
}

static void RenderReticle_New(rf::Player *pPlayer)
{
    if (rf::GetCurrentMenuId() == rf::MENU_MP_LIMBO)
        return;
    if (g_SpectateModeEnabled)
        RenderReticle_Hook.callTrampoline(g_SpectateModeTarget);
    else
        RenderReticle_Hook.callTrampoline(pPlayer);
}


rf::EntityObj *PlayerCreateEntity_New(rf::Player *pPlayer, int ClassId, const rf::Vector3 *pPos, const rf::Matrix3 *pRotMatrix, int MpCharacter)
{
    // hide target player from camera after respawn
    rf::EntityObj *pEntity = PlayerCreateEntity_Hook.callTrampoline(pPlayer, ClassId, pPos, pRotMatrix, MpCharacter);
    if (pEntity && pPlayer == g_SpectateModeTarget)
        pEntity->pLocalPlayer = pPlayer;

    return pEntity;
}

void RenderScannerViewForLocalPlayers_GrResetClip_New()
{
    if (g_SpectateModeEnabled)
        rf::PlayerRenderRocketLauncherScannerView(g_SpectateModeTarget);
    rf::GrResetClip();
}

#if SPECTATE_MODE_SHOW_WEAPON

static void PlayerFpgunRender_New(rf::Player *pPlayer)
{
    if (g_SpectateModeEnabled)
    {
        rf::EntityObj *pEntity = rf::EntityGetFromHandle(g_SpectateModeTarget->hEntity);

        // HACKFIX: RF uses function PlayerSetRemoteChargeVisible for local player only
        g_SpectateModeTarget->WeaponInfo.RemoteChargeVisible = (pEntity && pEntity->WeaponInfo.WeaponClsId == rf::g_RemoteChargeClsId);

        if (g_SpectateModeTarget != rf::g_pLocalPlayer && pEntity)
        {
            static rf::Vector3 vOldVel;
            rf::Vector3 vVelDiff = pEntity->_Super.PhysInfo.vVel - vOldVel;
            vOldVel = pEntity->_Super.PhysInfo.vVel;

            if (vVelDiff.y > 0.1f)
                pEntity->EntityFlags |= 2; // jump
        }


        if (g_SpectateModeTarget->WeaponInfo.bInScopeView)
            g_SpectateModeTarget->WeaponInfo.fScopeZoom = 2.0f;
        rf::g_pLocalPlayer->WeaponInfo.bInScopeView = g_SpectateModeTarget->WeaponInfo.bInScopeView;
        rf::g_pLocalPlayer->WeaponInfo.fScopeZoom = g_SpectateModeTarget->WeaponInfo.fScopeZoom;

        rf::PlayerFpgunUpdateMesh(g_SpectateModeTarget);
        rf::PlayerFpgunRender(g_SpectateModeTarget);
    }
    else
        rf::PlayerFpgunRender(pPlayer);
}

void PlayerFpgunUpdateState_New(rf::Player *pPlayer)
{
    PlayerFpgunUpdateState_Hook.callTrampoline(pPlayer);
    if (pPlayer != rf::g_pLocalPlayer)
    {
        rf::EntityObj *pEntity = rf::EntityGetFromHandle(pPlayer->hEntity);
        if (pEntity)
        {
            float fHorzSpeedPow2 = pEntity->_Super.PhysInfo.vVel.x * pEntity->_Super.PhysInfo.vVel.x
                + pEntity->_Super.PhysInfo.vVel.z * pEntity->_Super.PhysInfo.vVel.z;
            int State = 0;
            if (rf::IsEntityLoopFire(pEntity->_Super.Handle, pEntity->WeaponInfo.WeaponClsId))
                State = 2;
            else if (rf::EntityIsSwimming(pEntity) || rf::EntityIsFalling(pEntity))
                State = 0;
            else if (fHorzSpeedPow2 > 0.2f)
                State = 1;
            if (!rf::PlayerFpgunHasState(pPlayer, State))
                rf::PlayerFpgunSetState(pPlayer, State);
        }
    }
}

#endif // SPECTATE_MODE_SHOW_WEAPON

void SpectateModeInit()
{
    static HookCall<rf::IsPlayerEntityInvalid_Type> IsPlayerEntityInvalid_RedBars_Hookable(0x00432A52, rf::IsPlayerEntityInvalid);
    IsPlayerEntityInvalid_RedBars_Hookable.Hook(IsPlayerEntityInvalidHook);

    static HookCall<rf::IsPlayerDying_Type> IsPlayerDying_RedBars_Hookable(0x00432A5F, rf::IsPlayerDying);
    IsPlayerDying_RedBars_Hookable.Hook(IsPlayerDyingHook);

    static HookCall<rf::IsPlayerEntityInvalid_Type> IsPlayerEntityInvalid_Scoreboard_Hookable(0x00437BEE, rf::IsPlayerEntityInvalid);
    IsPlayerEntityInvalid_Scoreboard_Hookable.Hook(IsPlayerEntityInvalidHook);

    static HookCall<rf::IsPlayerDying_Type> IsPlayerDying_Scoreboard_Hookable(0x00437C01, rf::IsPlayerDying);
    IsPlayerDying_Scoreboard_Hookable.Hook(IsPlayerDyingHook);

    static HookCall<rf::IsPlayerEntityInvalid_Type> IsPlayerEntityInvalid_Scoreboard_Hookable2(0x00437C25, rf::IsPlayerEntityInvalid);
    IsPlayerEntityInvalid_Scoreboard_Hookable2.Hook(IsPlayerEntityInvalidHook);

    static HookCall<rf::IsPlayerDying_Type> IsPlayerDying_Scoreboard_Hookable2(0x00437C36, rf::IsPlayerDying);
    IsPlayerDying_Scoreboard_Hookable2.Hook(IsPlayerDyingHook);
    
    HandleCtrlInGame_Hook.hook(HandleCtrlInGameHook);
    RenderReticle_Hook.hook(RenderReticle_New);
    PlayerCreateEntity_Hook.hook(PlayerCreateEntity_New);

    // Note: HUD rendering doesn't make sense because life and armor isn't synced

#if SPECTATE_MODE_SHOW_WEAPON
    WriteMemInt32(0x0043285D + 1, (uintptr_t)PlayerFpgunRender_New - (0x0043285D + 0x5));
    WriteMemUInt8(0x004AB1B8, ASM_NOP, 6); // PlayerFpgunRenderInternal
    WriteMemUInt8(0x004AA23E, ASM_NOP, 6); // PlayerFpgunSetupMesh
    WriteMemUInt8(0x004AE0DF, ASM_NOP, 2); // PlayerFpgunLoadMesh

    WriteMemUInt8(0x004A938F, ASM_NOP, 6); // PlayerFpgunSetAction
    WriteMemUInt8(0x004A952C, ASM_SHORT_JMP_REL); // PlayerFpgunHasState
    WriteMemUInt8(0x004AA56D, ASM_NOP, 6); // PlayerFpgunSetState
    WriteMemUInt8(0x004AA6E7, ASM_NOP, 6); // PlayerFpgunUpdateMesh
    WriteMemUInt8(0x004AE384, ASM_NOP, 6); // PlayerFpgunPrepareWeapon
    WriteMemUInt8(0x004ACE2C, ASM_SHORT_JMP_REL); // GetZoomValue

    WriteMemPtr(0x0048857E + 2, &g_SpectateModeTarget); // RenderObjects
    WriteMemPtr(0x00488598 + 1, &g_SpectateModeTarget); // RenderObjects
    WriteMemPtr(0x00421889 + 2, &g_SpectateModeTarget); // EntityRender
    WriteMemPtr(0x004218A2 + 2, &g_SpectateModeTarget); // EntityRender

    RenderScannerViewForLocalPlayers_Hook.hook(0x00431890, RenderScannerViewForLocalPlayers_GrResetClip_New);
    PlayerFpgunUpdateState_Hook.hook(PlayerFpgunUpdateState_New);
#endif // SPECTATE_MODE_SHOW_WEAPON
    
}

void SpectateModeAfterFullGameInit()
{
    g_SpectateModeTarget = rf::g_pLocalPlayer;
}

void SpectateModeDrawUI()
{
    if (!g_SpectateModeEnabled)
    {
        if (rf::IsPlayerEntityInvalid(rf::g_pLocalPlayer))
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

    char szBuf[256];
    rf::GrSetColor(0xFF, 0xFF, 0, 0x80);
    snprintf(szBuf, sizeof(szBuf), "Spectating: %s",rf::String::CStr(&g_SpectateModeTarget->strName));
    rf::GrDrawAlignedText(rf::GR_ALIGN_CENTER, x + cx / 2, y + cy / 2 - cyFont / 2 - 5, szBuf, g_LargeFont, rf::g_GrTextMaterial);

    rf::EntityObj *pEntity = rf::EntityGetFromHandle(g_SpectateModeTarget->hEntity);
    if (!pEntity)
    {
        rf::GrSetColor(0xFF, 0xFF, 0xFF, 0xFF);
        static int BloodBm = rf::BmLoad("bloodsmear07_A.tga", 0, 0);
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
