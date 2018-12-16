#include "stdafx.h"
#include "rf.h"
#include "rfproto.h"
#include "spectate_mode.h"
#include "BuildConfig.h"
#include "utils.h"
#include "scoreboard.h"

#include "hooks/HookCall.h"

using namespace rf;

#if SPECTATE_MODE_ENABLE

static CPlayer *g_SpectateModeTarget;
static rf::Camera *g_OldTargetCamera = NULL;
static bool g_SpectateModeEnabled = false;
static int g_LargeFont = -1, g_MediumFont = -1, g_SmallFont = -1;

auto HandleCtrlInGame_Hook = makeFunHook(HandleCtrlInGame);
auto RenderReticle_Hook = makeFunHook(RenderReticle);
auto PlayerCreateEntity_Hook = makeFunHook(PlayerCreateEntity);
auto RenderScannerViewForLocalPlayers_Hook = makeCallHook(GrResetClip);
auto PlayerFpgunUpdateState_Hook = makeFunHook(PlayerFpgunUpdateState);

static void SetCameraTarget(CPlayer *pPlayer)
{
    // Based on function SetCamera1View
    if (!g_pLocalPlayer || !(g_pLocalPlayer)->pCamera || !pPlayer)
        return;

    rf::Camera *pCamera = (g_pLocalPlayer)->pCamera;
    pCamera->Type = rf::CAM_FIRST_PERSON;
    pCamera->pPlayer = pPlayer;

    g_OldTargetCamera = pPlayer->pCamera;
    pPlayer->pCamera = pCamera; // fix crash 0040D744

    CameraSetFirstPerson(pCamera);
}

void SpectateModeSetTargetPlayer(CPlayer *pPlayer)
{
    if (!pPlayer)
        pPlayer = g_pLocalPlayer;

    if (!g_pLocalPlayer || !(g_pLocalPlayer)->pCamera || !g_SpectateModeTarget || g_SpectateModeTarget == pPlayer)
        return;

    if (g_GameOptions & RF_GO_FORCE_RESPAWN)
    {
        CString strMessage, strPrefix;
        CString_Init(&strMessage, "You cannot use Spectate Mode because Force Respawn option is enabled on this server!");
        CString_InitEmpty(&strPrefix);
        ChatPrint(strMessage, 4, strPrefix);
        return;
    }

    // fix old target
    if (g_SpectateModeTarget && g_SpectateModeTarget != g_pLocalPlayer)
    {
        g_SpectateModeTarget->pCamera = g_OldTargetCamera;
        g_OldTargetCamera = NULL;

#if SPECTATE_MODE_SHOW_WEAPON
        g_SpectateModeTarget->Flags &= ~(1 << 4);
        EntityObj *pEntity = EntityGetFromHandle(g_SpectateModeTarget->hEntity);
        if (pEntity)
            pEntity->pLocalPlayer = NULL;
#endif // SPECTATE_MODE_SHOW_WEAPON
    }

    g_SpectateModeEnabled = (pPlayer != g_pLocalPlayer);
    g_SpectateModeTarget = pPlayer;

    KillLocalPlayer();
    SetCameraTarget(pPlayer);

#if SPECTATE_MODE_SHOW_WEAPON
    pPlayer->Flags |= 1 << 4;
    EntityObj *pEntity = EntityGetFromHandle(pPlayer->hEntity);
    if (pEntity)
    {
        // make sure weapon mesh is loaded now
        PlayerFpgunSetupMesh(pPlayer, pEntity->WeaponInfo.WeaponClsId);
        TRACE("pFpgunMesh %p", pPlayer->pFpgunMesh);

        // Hide target player from camera
        pEntity->pLocalPlayer = pPlayer;
    }
#endif // SPECTATE_MODE_SHOW_WEAPON
}

static void SpectateNextPlayer(bool bDir, bool bTryAlivePlayersFirst = false)
{
    CPlayer *pNewTarget;
    if (g_SpectateModeEnabled)
        pNewTarget = g_SpectateModeTarget;
    else
        pNewTarget = g_pLocalPlayer;
    while (true)
    {
        pNewTarget = bDir ? pNewTarget->pNext : pNewTarget->pPrev;
        if (!pNewTarget || pNewTarget == g_SpectateModeTarget)
            break; // nothing found
        if (bTryAlivePlayersFirst && IsPlayerEntityInvalid(pNewTarget))
            continue;
        if (pNewTarget != g_pLocalPlayer)
        {
            SpectateModeSetTargetPlayer(pNewTarget);
            return;
        }
    }

    if (bTryAlivePlayersFirst)
        SpectateNextPlayer(bDir, false);
}

static void HandleCtrlInGameHook(CPlayer *pPlayer, EGameCtrl KeyId, char WasPressed)
{
    if (g_SpectateModeEnabled)
    {
        if (KeyId == GC_PRIMARY_ATTACK || KeyId == GC_SLIDE_RIGHT)
        {
            if (WasPressed)
                SpectateNextPlayer(true);
            return; // dont allow spawn
        }
        else if (KeyId == GC_SECONDARY_ATTACK || KeyId == GC_SLIDE_LEFT)
        {
            if (WasPressed)
                SpectateNextPlayer(false);
            return;
        }
        else if (KeyId == GC_JUMP)
        {
            if (WasPressed)
                SpectateModeSetTargetPlayer(nullptr);
            return;
        }
    }
    else if (!g_SpectateModeEnabled)
    {
        if (KeyId == GC_JUMP && WasPressed && IsPlayerEntityInvalid(g_pLocalPlayer))
        {
            SpectateModeSetTargetPlayer(g_pLocalPlayer);
            SpectateNextPlayer(true, true);
            return;
        }
    }
    
    HandleCtrlInGame_Hook.callTrampoline(pPlayer, KeyId, WasPressed);
}

static bool IsPlayerEntityInvalidHook(CPlayer *pPlayer)
{
    if (g_SpectateModeEnabled)
        return false;
    else
        return IsPlayerEntityInvalid(pPlayer);
}

static bool IsPlayerDyingHook(CPlayer *pPlayer)
{
    if (g_SpectateModeEnabled)
        return false;
    else
        return IsPlayerDying(pPlayer);
}

void SpectateModeOnDestroyPlayer(CPlayer *pPlayer)
{
    if (g_SpectateModeTarget == pPlayer)
        SpectateNextPlayer(true);
    if (g_SpectateModeTarget == pPlayer)
        SpectateModeSetTargetPlayer(nullptr);
}

static void RenderReticle_New(CPlayer *pPlayer)
{
    if (GetCurrentMenuId() == MENU_MP_LIMBO)
        return;
    if (g_SpectateModeEnabled)
        RenderReticle_Hook.callTrampoline(g_SpectateModeTarget);
    else
        RenderReticle_Hook.callTrampoline(pPlayer);
}


EntityObj *PlayerCreateEntity_New(CPlayer *pPlayer, int ClassId, const CVector3 *pPos, const CMatrix3 *pRotMatrix, int MpCharacter)
{
    // hide target player from camera after respawn
    EntityObj *pEntity = PlayerCreateEntity_Hook.callTrampoline(pPlayer, ClassId, pPos, pRotMatrix, MpCharacter);
    if (pEntity && pPlayer == g_SpectateModeTarget)
        pEntity->pLocalPlayer = pPlayer;

    return pEntity;
}

void RenderScannerViewForLocalPlayers_GrResetClip_New()
{
    if (g_SpectateModeEnabled)
        PlayerRenderRocketLauncherScannerView(g_SpectateModeTarget);
    GrResetClip();
}

#if SPECTATE_MODE_SHOW_WEAPON

static void PlayerFpgunRender_New(CPlayer *pPlayer)
{
    if (g_SpectateModeEnabled)
    {
        EntityObj *pEntity = EntityGetFromHandle(g_SpectateModeTarget->hEntity);

        // HACKFIX: RF uses function PlayerSetRemoteChargeVisible for local player only
        g_SpectateModeTarget->WeaponInfo.RemoteChargeVisible = (pEntity && pEntity->WeaponInfo.WeaponClsId == g_RemoteChargeClsId);

        if (g_SpectateModeTarget != g_pLocalPlayer && pEntity)
        {
            static CVector3 vOldVel;
            CVector3 vVelDiff = pEntity->_Super.PhysInfo.vVel - vOldVel;
            vOldVel = pEntity->_Super.PhysInfo.vVel;

            if (vVelDiff.y > 0.1f)
                pEntity->EntityFlags |= 2; // jump
        }


        if (g_SpectateModeTarget->WeaponInfo.bInScopeView)
            g_SpectateModeTarget->WeaponInfo.fScopeZoom = 2.0f;
        (g_pLocalPlayer)->WeaponInfo.bInScopeView = g_SpectateModeTarget->WeaponInfo.bInScopeView;
        (g_pLocalPlayer)->WeaponInfo.fScopeZoom = g_SpectateModeTarget->WeaponInfo.fScopeZoom;

        PlayerFpgunUpdateMesh(g_SpectateModeTarget);
        PlayerFpgunRender(g_SpectateModeTarget);
    }
    else
        PlayerFpgunRender(pPlayer);
}

void PlayerFpgunUpdateState_New(CPlayer *pPlayer)
{
    PlayerFpgunUpdateState_Hook.callTrampoline(pPlayer);
    if (pPlayer != g_pLocalPlayer)
    {
        EntityObj *pEntity = EntityGetFromHandle(pPlayer->hEntity);
        if (pEntity)
        {
            float fHorzSpeedPow2 = pEntity->_Super.PhysInfo.vVel.x * pEntity->_Super.PhysInfo.vVel.x
                + pEntity->_Super.PhysInfo.vVel.z * pEntity->_Super.PhysInfo.vVel.z;
            int State = 0;
            if (IsEntityLoopFire(pEntity->_Super.Handle, pEntity->WeaponInfo.WeaponClsId))
                State = 2;
            else if (EntityIsSwimming(pEntity) || EntityIsFalling(pEntity))
                State = 0;
            else if (fHorzSpeedPow2 > 0.2f)
                State = 1;
            if (!PlayerFpgunHasState(pPlayer, State))
                PlayerFpgunSetState(pPlayer, State);
        }
    }
}

#endif // SPECTATE_MODE_SHOW_WEAPON

void SpectateModeInit()
{
    static HookCall<IsPlayerEntityInvalid_Type> IsPlayerEntityInvalid_RedBars_Hookable(0x00432A52, IsPlayerEntityInvalid);
    IsPlayerEntityInvalid_RedBars_Hookable.Hook(IsPlayerEntityInvalidHook);

    static HookCall<IsPlayerDying_Type> IsPlayerDying_RedBars_Hookable(0x00432A5F, IsPlayerDying);
    IsPlayerDying_RedBars_Hookable.Hook(IsPlayerDyingHook);

    static HookCall<IsPlayerEntityInvalid_Type> IsPlayerEntityInvalid_Scoreboard_Hookable(0x00437BEE, IsPlayerEntityInvalid);
    IsPlayerEntityInvalid_Scoreboard_Hookable.Hook(IsPlayerEntityInvalidHook);

    static HookCall<IsPlayerDying_Type> IsPlayerDying_Scoreboard_Hookable(0x00437C01, IsPlayerDying);
    IsPlayerDying_Scoreboard_Hookable.Hook(IsPlayerDyingHook);

    static HookCall<IsPlayerEntityInvalid_Type> IsPlayerEntityInvalid_Scoreboard_Hookable2(0x00437C25, IsPlayerEntityInvalid);
    IsPlayerEntityInvalid_Scoreboard_Hookable2.Hook(IsPlayerEntityInvalidHook);

    static HookCall<IsPlayerDying_Type> IsPlayerDying_Scoreboard_Hookable2(0x00437C36, IsPlayerDying);
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
    g_SpectateModeTarget = g_pLocalPlayer;
}

void SpectateModeDrawUI()
{
    if (!g_SpectateModeEnabled)
    {
        if (IsPlayerEntityInvalid(g_pLocalPlayer))
        {
            GrSetColor(0xFF, 0xFF, 0xFF, 0xFF);
            GrDrawAlignedText(GR_ALIGN_LEFT, 20, 200, "Press JUMP key to enter Spectate Mode", -1, g_GrTextMaterial);
        }
        return;
    }
    
    if (g_LargeFont == -1)
        g_LargeFont = GrLoadFont("rfpc-large.vf", -1);
    if (g_MediumFont == -1)
        g_MediumFont = GrLoadFont("rfpc-medium.vf", -1);
    if (g_SmallFont == -1)
        g_SmallFont = GrLoadFont("rfpc-small.vf", -1);

    const unsigned cx = 500, cy = 50;
    unsigned cxScr = GrGetMaxWidth(), cySrc = GrGetMaxHeight();
    unsigned x = (cxScr - cx) / 2;
    unsigned y = cySrc - 100;
    unsigned cyFont = GrGetFontHeight(-1);

    GrSetColor(0, 0, 0, 0x80);
    GrDrawAlignedText(GR_ALIGN_CENTER, cxScr / 2 + 2, 150 + 2, "SPECTATE MODE", g_LargeFont, g_GrTextMaterial);
    GrSetColor(0xFF, 0xFF, 0xFF, 0xFF);
    GrDrawAlignedText(GR_ALIGN_CENTER, cxScr / 2, 150, "SPECTATE MODE", g_LargeFont, g_GrTextMaterial);

    GrSetColor(0xFF, 0xFF, 0xFF, 0xFF);
    GrDrawAlignedText(GR_ALIGN_LEFT, 20, 200, "Press JUMP key to exit Spectate Mode", g_MediumFont, g_GrTextMaterial);
    GrDrawAlignedText(GR_ALIGN_LEFT, 20, 215, "Press PRIMARY ATTACK key to switch to the next player", g_MediumFont, g_GrTextMaterial);
    GrDrawAlignedText(GR_ALIGN_LEFT, 20, 230, "Press SECONDARY ATTACK key to switch to the previous player", g_MediumFont, g_GrTextMaterial);

    GrSetColor(0, 0, 0x00, 0x60);
    GrDrawRect(x, y, cx, cy, g_GrRectMaterial);

    char szBuf[256];
    GrSetColor(0xFF, 0xFF, 0, 0x80);
    snprintf(szBuf, sizeof(szBuf), "Spectating: %s", CString_CStr(&g_SpectateModeTarget->strName));
    GrDrawAlignedText(GR_ALIGN_CENTER, x + cx / 2, y + cy / 2 - cyFont / 2 - 5, szBuf, g_LargeFont, g_GrTextMaterial);

    EntityObj *pEntity = EntityGetFromHandle(g_SpectateModeTarget->hEntity);
    if (!pEntity)
    {
        GrSetColor(0xFF, 0xFF, 0xFF, 0xFF);
        static int BloodBm = BmLoad("bloodsmear07_A.tga", 0, 0);
        int BloodW, BloodH;
        BmGetBitmapSize(BloodBm, &BloodW, &BloodH);
        GrDrawBitmapStretched(BloodBm, (cxScr - BloodW*2) / 2, (cySrc - BloodH*2) / 2, BloodW * 2 , BloodH * 2, 
            0, 0, BloodW, BloodH, 0.0f, 0.0f, g_GrBitmapMaterial);

        GrSetColor(0, 0, 0, 0x80);
        GrDrawAlignedText(GR_ALIGN_CENTER, cxScr / 2 + 2, cySrc / 2 + 2, "DEAD", g_LargeFont, g_GrTextMaterial);
        GrSetColor(0xF0, 0x20, 0x10, 0xC0);
        GrDrawAlignedText(GR_ALIGN_CENTER, cxScr / 2, cySrc / 2, "DEAD", g_LargeFont, g_GrTextMaterial);
    }
}

bool SpectateModeIsActive()
{
    return g_SpectateModeEnabled;
}

#endif // SPECTATE_MODE_ENABLE
