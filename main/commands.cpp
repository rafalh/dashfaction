#include "stdafx.h"
#include "commands.h"
#include "rf.h"
#include "utils.h"
#include "hud.h"
#include "lazyban.h"
#include "main.h"
#include "config.h"
#include "spectate_mode.h"
#include "main.h"
#include "config.h"

#if SPLITSCREEN_ENABLE

static void SplitScreenCmdHandler(void)
{
    if(*g_pbCmdRun)
    {
        if(*g_pbNetworkGame)
            RfSplitScreen(); /* FIXME: set player 2 controls */
        else
            RfConsolePrintf("Works only in multiplayer game!");
    }
}

#endif // SPLITSCREEN_ENABLE

static void MaxFpsCmdHandler(void)
{
    if (*g_pbCmdRun)
    {
        RfCmdGetNextArg(CMD_ARG_NONE | CMD_ARG_FLOAT, 0);

        if ((*g_piCmdArgType & CMD_ARG_FLOAT))
        {
#ifdef NDEBUG
            float newLimit = std::min(std::max(*g_pfCmdArg, (float)MIN_FPS_LIMIT), (float)MAX_FPS_LIMIT);
#else
            float newLimit = *g_pfCmdArg;
#endif
            g_gameConfig.maxFps = (unsigned)newLimit;
            g_gameConfig.save();
            *g_pfMinFramerate = 1.0f / newLimit;
        }
        else
            RfConsolePrintf("Maximal FPS: %.1f", 1.0f / *g_pfMinFramerate);
    }
    
    if (*g_pbCmdHelp)
    {
        RfConsoleWrite(g_ppszStringsTable[886], NULL);
        RfConsoleWrite("     maxfps <limit>", NULL);
    }
}

#ifndef NDEBUG
static void DebugCmdHandler(void)
{
    bool bDbg = !g_pbDbgFlagsArray[0];
    memset((char*)g_pbDbgFlagsArray, bDbg, 9);
    *g_pbRenderEventIcons = bDbg;
    *g_pbDbgNetwork = bDbg;
    *g_pbDbgWeapon = bDbg;
}
#endif

#if SPECTATE_MODE_ENABLE

static void SpectateCmdHandler(void)
{
    if (*g_pbCmdRun)
    {
        if (*g_pbNetworkGame)
        {
            CPlayer *pPlayer;
            RfCmdGetNextArg(CMD_ARG_NONE | CMD_ARG_STR, 0);
            if (*g_piCmdArgType & CMD_ARG_STR)
            {
                pPlayer = FindPlayer(g_pszCmdArg);
                if (!pPlayer)
                    RfConsolePrintf("Cannot find player: %s", g_pszCmdArg);
            }
            else {
                RfConsolePrintf("Expected player name.");
                pPlayer = *g_ppLocalPlayer;
            }
            
            if (pPlayer)
                SpectateModeSetTargetPlayer(pPlayer);
        } else
            RfConsoleWrite("Works only in multiplayer game!", NULL);
    }
    
    if (*g_pbCmdHelp)
    {
        RfConsoleWrite(g_ppszStringsTable[886], NULL);
        RfConsolePrintf("     spectate <%s>", g_ppszStringsTable[835]);
    }
}

#endif // SPECTATE_MODE_ENABLE

#if MULTISAMPLING_SUPPORT
static void AntiAliasingCmdHandler(void)
{
    if (*g_pbCmdRun)
    {
        if (!g_gameConfig.msaa)
            RfConsolePrintf("Anti-aliasing is not supported");
        else
        {
            DWORD Enabled = FALSE;
            IDirect3DDevice8_GetRenderState(*g_ppGrDevice, D3DRS_MULTISAMPLEANTIALIAS, &Enabled);
            Enabled = !Enabled;
            IDirect3DDevice8_SetRenderState(*g_ppGrDevice, D3DRS_MULTISAMPLEANTIALIAS, Enabled);
            RfConsolePrintf("Anti-aliasing is %s", Enabled ? "enabled" : "disabled");
        }
    }
}
#endif // MULTISAMPLING_SUPPORT

#if DIRECTINPUT_SUPPORT
static void InputModeCmdHandler(void)
{
    if (*g_pbCmdRun)
    {
        *g_pbDirectInputDisabled = !*g_pbDirectInputDisabled;
        if (*g_pbDirectInputDisabled)
            RfConsolePrintf("DirectInput is disabled");
        else
            RfConsolePrintf("DirectInput is enabled");
    }
}
#endif // DIRECTINPUT_SUPPORT

#if CAMERA_1_3_COMMANDS

static int CanPlayerFireHook(CPlayer *pPlayer)
{
    if (!(pPlayer->field_10 & 0x10))
        return 0;
    if (*g_pbNetworkGame && (pPlayer->pCamera->Type == RF_CAM_FREE || pPlayer->pCamera->pPlayer != pPlayer))
        return 0;
    return 1;
}

#endif // if CAMERA_1_3_COMMANDS

CCmd g_Commands[] = {
#if SPLITSCREEN_ENABLE
    {"splitscreen", "Starts split screen mode", SplitScreenCmdHandler},
#endif
    {"maxfps", "Sets maximal FPS", MaxFpsCmdHandler},
    {"hud", "Show and hide HUD", HudCmdHandler},
#ifndef NDEBUG
    {"debug", "Switches debugging in RF", DebugCmdHandler},
#endif
#if SPECTATE_MODE_ENABLE
    {"spectate", "Starts spectating mode", SpectateCmdHandler},
#endif
#if MULTISAMPLING_SUPPORT
    { "antialiasing", "Toggles anti-aliasing", AntiAliasingCmdHandler },
#endif
#if DIRECTINPUT_SUPPORT
    { "inputmode", "Toggles input mode", InputModeCmdHandler },
#endif
    {"unban_last", "Unbans last banned player", UnbanLastCmdHandler},
};

void CommandsInit(void)
{
#if CAMERA_1_3_COMMANDS
    /* Enable camera1-3 in multiplayer and hook CanPlayerFire to disable shooting in camera2 */
    WriteMemUInt8Repeat((PVOID)0x00431280, ASM_NOP, 2);
    WriteMemUInt8Repeat((PVOID)0x004312E0, ASM_NOP, 2);
    WriteMemUInt8Repeat((PVOID)0x00431340, ASM_NOP, 2);
    WriteMemUInt8((PVOID)0x004A68D0, ASM_LONG_JMP_REL);
    WriteMemUInt32((PVOID)0x004A68D1, ((ULONG_PTR)CanPlayerFireHook) - (0x004A68D0 + 0x5));
#endif // if CAMERA_1_3_COMMANDS
}

void CommandRegister(CCmd *pCmd)
{
    if (*g_pcCommands < MAX_COMMANDS_COUNT)
        g_ppCommands[(*g_pcCommands)++] = pCmd;
}

void CommandsAfterGameInit()
{
    unsigned i;

    /* Add commands */
    for (i = 0; i < COUNTOF(g_Commands); ++i)
        CommandRegister(&g_Commands[i]);
}
