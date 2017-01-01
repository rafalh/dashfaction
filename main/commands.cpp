#include "commands.h"
#include "rf.h"
#include "utils.h"
#include "hud.h"
#include "lazyban.h"
#include "main.h"
#include "config.h"
#include "sharedoptions.h"
#include "spectate.h"

extern SHARED_OPTIONS g_Options;

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
    if(*g_pbCmdRun)
    {
        RfCmdGetNextArg(CMD_ARG_NONE | CMD_ARG_FLOAT, 0);
        
        if((*g_piCmdArgType & CMD_ARG_FLOAT) && *g_pfCmdArg >= 5.0f)
            *g_pfMinFramerate = 1.0f / *g_pfCmdArg;
        else
            RfConsolePrintf("Maximal FPS: %.1f", 1.0f / *g_pfMinFramerate);
    }
    
    if(*g_pbCmdHelp)
    {
        RfConsoleWrite(g_ppszStringsTable[886], NULL);
        RfConsoleWrite("     maxfps <limit>", NULL);
    }
}

static void DebugCmdHandler(void)
{
    int bDbg = !(*(char*)0x0062FE19);
    memset((char*)0x0062FE19, bDbg, 9);
    *(char*)0x00856500 = bDbg;
    *(int*)0x006FED24 = bDbg;
    *g_pDbgWeapon = bDbg;
}

typedef void (*PFN_SET_PLAYER_WEAPON_ACTION)(CPlayer *pPlayer, unsigned iAction);
static const PFN_SET_PLAYER_WEAPON_ACTION RfSetPlayerWeaponAction = (PFN_SET_PLAYER_WEAPON_ACTION)0x004A9380;

typedef void (*PFN_TEST)(CPlayer *pPlayer, unsigned iWeaponCls);
static const PFN_TEST RfShoot = (PFN_TEST)0x0041AE70;

#ifndef NDEBUG

static void TestCmdHandler(void)
{
    if(*g_pbCmdRun)
    {
        int *ptr = (int*)HeapAlloc(GetProcessHeap(), 0, 8);
        ptr[10] = 1;
        RfConsolePrintf("test done %p", ptr);
    }
    
    if(*g_pbCmdHelp)
        RfConsoleWrite("     test <n>", NULL);
}

#endif // DEBUG

#if SPECTATE_ENABLE

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
                SetSpectateModeTarget(pPlayer);
        } else
            RfConsoleWrite("Works only in multiplayer game!", NULL);
    }
    
    if(*g_pbCmdHelp)
    {
        RfConsoleWrite(g_ppszStringsTable[886], NULL);
        RfConsolePrintf("     spectate <%s>", g_ppszStringsTable[835]);
    }
}

#endif // SPECTATE_ENABLE

#if MULTISAMPLING_SUPPORT
static void AntiAliasingCmdHandler(void)
{
	if (*g_pbCmdRun)
	{
		if (!g_Options.bMultiSampling)
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
#endif

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
#endif

CCmd g_Commands[] = {
#if SPLITSCREEN_ENABLE
    {"splitscreen", "Starts split screen mode", SplitScreenCmdHandler},
#endif
    {"maxfps", "Sets maximal FPS", MaxFpsCmdHandler},
    {"hud", "Show and hide HUD", HudCmdHandler},
    {"debug", "Switches debugging in RF", DebugCmdHandler},
#if SPECTATE_ENABLE
    {"spectate", "Starts spectating mode", SpectateCmdHandler},
#endif
#if MULTISAMPLING_SUPPORT
	{ "antialiasing", "Toggles anti-aliasing", AntiAliasingCmdHandler },
#endif
#if DIRECTINPUT_SUPPORT
    { "inputmode", "Toggles input mode", InputModeCmdHandler },
#endif
    {"unban_last", "Unbans last banned player", UnbanLastCmdHandler},
#ifndef NDEBUG
    {"test", "Test command", TestCmdHandler},
#endif
};

void RegisterCommands(void)
{
    unsigned i;
    
    /* Add commands */
    for(i = 0; i < COUNTOF(g_Commands); ++i)
    {
        if(*g_pcCommands < MAX_COMMANDS_COUNT)
            g_ppCommands[(*g_pcCommands)++] = &g_Commands[i];
    }
}
