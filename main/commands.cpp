#include "stdafx.h"
#include "commands.h"
#include "rf.h"
#include "utils.h"
#include "hud.h"
#include "lazyban.h"
#include "main.h"
#include "BuildConfig.h"
#include "spectate_mode.h"
#include "main.h"
#include "misc.h"
#include "packfile.h"
#include "hooks/MemChange.h"

using namespace rf;

// Note: limit should fit in int8_t
constexpr int CMD_LIMIT = 127;

static DcCommand *g_CommandsBuffer[CMD_LIMIT];

auto DcAutoCompleteInput_Hook = makeFunHook(DcAutoCompleteInput);

rf::CPlayer *FindBestMatchingPlayer(const char *pszName)
{
    rf::CPlayer *pFoundPlayer;
    int NumFound = 0;
    FindPlayer(StringMatcher().Exact(pszName), [&](rf::CPlayer *pPlayer)
    {
        pFoundPlayer = pPlayer;
        ++NumFound;
    });
    if (NumFound == 1)
        return pFoundPlayer;

    NumFound = 0;
    FindPlayer(StringMatcher().Infix(pszName), [&](rf::CPlayer *pPlayer)
    {
        pFoundPlayer = pPlayer;
        ++NumFound;
    });

    if (NumFound == 1)
        return pFoundPlayer;
    else if (NumFound > 1)
        rf::DcPrintf("Found %d players matching '%s'!", NumFound,  pszName);
    else
        rf::DcPrintf("Cannot find player matching '%s'", pszName);
    return nullptr;
}


#if SPLITSCREEN_ENABLE

static void SplitScreenCmdHandler(void)
{
    if (g_bDcRun)
    {
        if (g_bNetworkGame)
            SplitScreenStart(); /* FIXME: set player 2 controls */
        else
            DcPrintf("Works only in multiplayer game!");
    }
}

#endif // SPLITSCREEN_ENABLE

static void MaxFpsCmdHandler(void)
{
    if (g_bDcRun)
    {
        rf::DcGetArg(DC_ARG_NONE | DC_ARG_FLOAT, 0);

        if ((g_DcArgType & DC_ARG_FLOAT))
        {
#ifdef NDEBUG
            float newLimit = std::min(std::max(g_fDcArg, (float)MIN_FPS_LIMIT), (float)MAX_FPS_LIMIT);
#else
            float newLimit = g_fDcArg;
#endif
            g_gameConfig.maxFps = (unsigned)newLimit;
            g_gameConfig.save();
            g_fMinFramerate = 1.0f / newLimit;
        }
        else
            DcPrintf("Maximal FPS: %.1f", 1.0f / g_fMinFramerate);
    }
    
    if (g_bDcHelp)
    {
        DcPrint(g_ppszStringsTable[STR_USAGE], NULL);
        DcPrint("     maxfps <limit>", NULL);
    }
}

#ifndef NDEBUG
static void DebugCmdHandler(void)
{
    bool bDbg = !g_pbDbgFlagsArray[0];
    memset((char*)g_pbDbgFlagsArray, bDbg, 9);
    g_bRenderEventIcons = bDbg;
    g_bDbgNetwork = bDbg;
    g_bDbgWeapon = bDbg;
    g_bDbgRenderTriggers = bDbg;
}
#endif

#if SPECTATE_MODE_ENABLE

static void SpectateCmdHandler(void)
{
    if (g_bDcRun)
    {
        if (g_bNetworkGame)
        {
            rf::CPlayer *pPlayer;
            rf::DcGetArg(DC_ARG_NONE | DC_ARG_STR | DC_ARG_FALSE, 0);
            if (g_DcArgType & DC_ARG_FALSE)
                pPlayer = nullptr;
            else if (g_DcArgType & DC_ARG_STR)
                pPlayer = FindBestMatchingPlayer(g_pszDcArg);
            else
                pPlayer = nullptr;
            
            if (pPlayer)
                SpectateModeSetTargetPlayer(pPlayer);
        } else
            DcPrint("Works only in multiplayer game!", NULL);
    }
    
    if (g_bDcHelp)
    {
        DcPrint(g_ppszStringsTable[STR_USAGE], NULL);
        DcPrintf("     spectate <%s>", g_ppszStringsTable[STR_PLAYER_NAME]);
        DcPrintf("     spectate false", g_ppszStringsTable[STR_PLAYER_NAME]);
    }
}

#endif // SPECTATE_MODE_ENABLE

#if MULTISAMPLING_SUPPORT
static void AntiAliasingCmdHandler(void)
{
    if (g_bDcRun)
    {
        if (!g_gameConfig.msaa)
            DcPrintf("Anti-aliasing is not supported");
        else
        {
            DWORD Enabled = FALSE;
            IDirect3DDevice8_GetRenderState(g_pGrDevice, D3DRS_MULTISAMPLEANTIALIAS, &Enabled);
            Enabled = !Enabled;
            IDirect3DDevice8_SetRenderState(g_pGrDevice, D3DRS_MULTISAMPLEANTIALIAS, Enabled);
            DcPrintf("Anti-aliasing is %s", Enabled ? "enabled" : "disabled");
        }
    }
}
#endif // MULTISAMPLING_SUPPORT

#if DIRECTINPUT_SUPPORT
static void InputModeCmdHandler(void)
{
    if (g_bDcRun)
    {
        g_bDirectInputDisabled = !g_bDirectInputDisabled;
        if (g_bDirectInputDisabled)
            DcPrintf("DirectInput is disabled");
        else
            DcPrintf("DirectInput is enabled");
    }
}
#endif // DIRECTINPUT_SUPPORT

#if CAMERA_1_3_COMMANDS

static int CanPlayerFireHook(CPlayer *pPlayer)
{
    if (!(pPlayer->Flags & 0x10))
        return 0;
    if (g_bNetworkGame && (pPlayer->pCamera->Type == rf::CAM_FREELOOK || pPlayer->pCamera->pPlayer != pPlayer))
        return 0;
    return 1;
}

#endif // if CAMERA_1_3_COMMANDS

static void MouseSensitivityCmdHandler(void)
{
    if (g_bDcRun)
    {
        rf::DcGetArg(DC_ARG_NONE | DC_ARG_FLOAT, 0);

        if (g_DcArgType & DC_ARG_FLOAT)
        {
            float fValue = g_fDcArg;
            fValue = clamp(fValue, 0.0f, 1.0f);
            (g_pLocalPlayer)->Config.Controls.fMouseSensitivity = fValue;
        }
        DcPrintf("Mouse sensitivity: %.2f", (g_pLocalPlayer)->Config.Controls.fMouseSensitivity);
    }

    if (g_bDcHelp)
    {
        DcPrint(g_ppszStringsTable[STR_USAGE], NULL);
        DcPrint("     ms <value>", NULL);
    }
}

static void VolumeLightsCmdHandler(void)
{
    if (g_bDcRun)
    {
        static MemChange vliCallChange(0x0043233E);
        if (vliCallChange.IsApplied())
            vliCallChange.Revert();
        else
            vliCallChange.Write("\x90\x90\x90\x90\x90", 5);
        DcPrintf("Volumetric lightining is %s.", vliCallChange.IsApplied() ? "disabled" : "enabled");
    }
}

static void LevelSpCmdHandler(void)
{
    if (g_bDcRun)
    {
        rf::DcGetArg(DC_ARG_STR, 0);

        if (g_DcArgType & DC_ARG_STR)
        {
            if (g_bNetworkGame)
            {
                DcPrintf("You cannot use it in multiplayer game!");
                return;
            }
            CString strUnk, strLevel;
            CString_Init(&strUnk, "");
            if (stristr(g_pszDcArg, ".rfl"))
                CString_Init(&strLevel, g_pszDcArg);
            else
            {
                char Buf[256];
                snprintf(Buf, sizeof(Buf), "%s.rfl", g_pszDcArg);
                CString_Init(&strLevel, Buf);
            }
            
            DcPrintf("Loading level.");
            SetNextLevelFilename(strLevel, strUnk);
            SwitchMenu(5, 0);
        }
    }

    if (g_bDcHelp)
    {
        DcPrint(g_ppszStringsTable[STR_USAGE], NULL);
        DcPrint("     <rfl_name>", NULL);
    }
}

static void LevelSoundsCmdHandler(void)
{
    if (g_bDcRun)
    {
        rf::DcGetArg(DC_ARG_FLOAT | DC_ARG_NONE, 0);

        if ((g_DcArgType & DC_ARG_FLOAT))
        {
            float fVolScale = clamp(g_fDcArg, 0.0f, 1.0f);
            SetPlaySoundEventsVolumeScale(fVolScale);

            g_gameConfig.levelSoundVolume = fVolScale;
            g_gameConfig.save();
        }
        DcPrintf("Level sound volume: %.1f", g_gameConfig.levelSoundVolume);
    }

    if (g_bDcHelp)
    {
        DcPrint(g_ppszStringsTable[STR_USAGE], NULL);
        DcPrint("     <volume>", NULL);
    }
}

static void DcfFindMap()
{
    if (g_bDcRun)
    {
        rf::DcGetArg(DC_ARG_STR, 0);

        if (g_DcArgType & DC_ARG_STR)
        {
            PackfileFindMatchingFiles(StringMatcher().Infix(g_pszDcArg).Suffix(".rfl"), [](const char *pszName)
            {
                DcPrintf("%s\n", pszName);
            });
        }
    }

    if (g_bDcHelp)
    {
        DcPrint(g_ppszStringsTable[STR_USAGE], NULL);
        DcPrint("     <query>", NULL);
    }
}

DcCommand g_Commands[] = {
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
    { "unban_last", "Unbans last banned player", UnbanLastCmdHandler },
    { "ms", "Sets mouse sensitivity", MouseSensitivityCmdHandler },
    { "vli", "Toggles volumetric lightining", VolumeLightsCmdHandler },
    { "levelsp", "Loads single player level", LevelSpCmdHandler },
    { "levelsounds", "Sets level sounds volume scale", LevelSoundsCmdHandler },
};

void DcShowCmdHelp(DcCommand *pCmd)
{
    g_bDcRun = 0;
    g_bDcHelp = 1;
    g_bDcStatus = 0;
    pCmd->pfnHandler();
}

int DcAutoCompleteGetComponent(int Offset, std::string &Result)
{
    const char *pszBegin = g_szDcCmdLine + Offset, *pszEnd = nullptr, *pszNext;
    if (pszBegin[0] == '"')
    {
        ++pszBegin;
        pszEnd = strchr(pszBegin, '"');
        pszNext = pszEnd ? strchr(pszEnd, ' ') : nullptr;
    }
    else
        pszEnd = pszNext = strchr(pszBegin, ' ');

    if (!pszEnd)
        pszEnd = g_szDcCmdLine + g_cchDcCmdLineLen;
    
    size_t Len = pszEnd - pszBegin;
    Result.assign(pszBegin, Len);

    return pszNext ? pszNext + 1 - g_szDcCmdLine : -1;
}

void DcAutoCompletePutComponent(int Offset, const std::string &Component, bool Finished)
{
    bool Quote = Component.find(' ') != std::string::npos;
    if (Quote)
        g_cchDcCmdLineLen = Offset + sprintf(g_szDcCmdLine + Offset, "\"%s\"", Component.c_str());
    else
        g_cchDcCmdLineLen = Offset + sprintf(g_szDcCmdLine + Offset, "%s", Component.c_str());
    if (Finished)
        g_cchDcCmdLineLen += sprintf(g_szDcCmdLine + g_cchDcCmdLineLen, " ");
}

template<typename T, typename F>
void DcAutoCompletePrintSuggestions(T &Suggestions, F MappingFun)
{
    for (auto Item : Suggestions)
        DcPrintf("%s\n", MappingFun(Item));
}

void DcAutoCompleteUpdateCommonPrefix(std::string &CommonPrefix, const std::string &Value, bool &First, bool CaseSensitive = false)
{
    if (First)
    {
        First = false;
        CommonPrefix = Value;
    }
    if (CommonPrefix.size() > Value.size())
        CommonPrefix.resize(Value.size());
    for (size_t i = 0; i < CommonPrefix.size(); ++i)
        if ((CaseSensitive && CommonPrefix[i] != Value[i]) || tolower(CommonPrefix[i]) != tolower(Value[i]))
        {
            CommonPrefix.resize(i);
            break;
        }
}

void DcAutoCompleteLevel(int Offset)
{
    std::string LevelName;
    DcAutoCompleteGetComponent(Offset, LevelName);
    if (LevelName.size() < 3)
        return;

    bool First = true;
    std::string CommonPrefix;
    std::vector<std::string> Matches;
    PackfileFindMatchingFiles(StringMatcher().Prefix(LevelName).Suffix(".rfl"), [&](const char *pszName)
    {
        auto pszExt = strrchr(pszName, '.');
        auto NameLen = pszExt ? pszExt - pszName : strlen(pszName);
        std::string Name(pszName, NameLen);
        Matches.push_back(Name);
        DcAutoCompleteUpdateCommonPrefix(CommonPrefix, Name, First);
    });

    if (Matches.size() == 1)
        DcAutoCompletePutComponent(Offset, Matches[0], true);
    else
    {
        DcAutoCompletePrintSuggestions(Matches, [](std::string &Name) { return Name.c_str(); });
        DcAutoCompletePutComponent(Offset, CommonPrefix, false);
    }
}

void DcAutoCompletePlayer(int Offset)
{
    std::string PlayerName;
    DcAutoCompleteGetComponent(Offset, PlayerName);
    if (PlayerName.size() < 1)
        return;

    bool First = true;
    std::string CommonPrefix;
    std::vector<rf::CPlayer*> MatchingPlayers;
    FindPlayer(StringMatcher().Prefix(PlayerName), [&](rf::CPlayer *pPlayer)
    {
        MatchingPlayers.push_back(pPlayer);
        DcAutoCompleteUpdateCommonPrefix(CommonPrefix, pPlayer->strName.psz, First);
    });

    if (MatchingPlayers.size() == 1)
        DcAutoCompletePutComponent(Offset, MatchingPlayers[0]->strName.psz, true);
    else
    {
        DcAutoCompletePrintSuggestions(MatchingPlayers, [](rf::CPlayer *pPlayer) { return pPlayer->strName.psz; });
        DcAutoCompletePutComponent(Offset, CommonPrefix, false);
    }
}

void DcAutoCompleteCommand(int Offset)
{
    std::string Cmd;
    int NextOffset = DcAutoCompleteGetComponent(Offset, Cmd);
    if (Cmd.size() < 2)
        return;

    bool First = true;
    std::string CommonPrefix;

    std::vector<DcCommand*> MatchingCmds;
    for (unsigned i = 0; i < g_DcNumCommands; ++i)
    {
        DcCommand *pCmd = g_CommandsBuffer[i];
        if (!strnicmp(pCmd->pszCmd, Cmd.c_str(), Cmd.size()) && (NextOffset == -1 || !pCmd->pszCmd[Cmd.size()]))
        {
            MatchingCmds.push_back(pCmd);
            DcAutoCompleteUpdateCommonPrefix(CommonPrefix, pCmd->pszCmd, First);
        }
    }

    if (NextOffset != -1)
    {
        if (MatchingCmds.size() != 1)
            return;
        if (!stricmp(Cmd.c_str(), "level") || !stricmp(Cmd.c_str(), "levelsp"))
            DcAutoCompleteLevel(NextOffset);
        else if (!stricmp(Cmd.c_str(), "kick") || !stricmp(Cmd.c_str(), "ban"))
            DcAutoCompletePlayer(NextOffset);
        else if (!stricmp(Cmd.c_str(), "rcon"))
            DcAutoCompleteCommand(NextOffset);
        else
            DcShowCmdHelp(MatchingCmds[0]);
    }
    else if (MatchingCmds.size() > 1)
    {
        for (auto *pCmd : MatchingCmds)
            DcPrintf("%s - %s", pCmd->pszCmd, pCmd->pszDescr);
        DcAutoCompletePutComponent(Offset, CommonPrefix, false);
    }
    else if (MatchingCmds.size() == 1)
        DcAutoCompletePutComponent(Offset, MatchingCmds[0]->pszCmd, true);
}

void DcAutoCompleteInput_New()
{
    DcAutoCompleteCommand(0);
}

void CommandsInit(void)
{
#if CAMERA_1_3_COMMANDS
    /* Enable camera1-3 in multiplayer and hook CanPlayerFire to disable shooting in camera2 */
    WriteMemUInt8(0x00431280, ASM_NOP, 2);
    WriteMemUInt8(0x004312E0, ASM_NOP, 2);
    WriteMemUInt8(0x00431340, ASM_NOP, 2);
    WriteMemUInt8(0x004A68D0, ASM_LONG_JMP_REL);
    WriteMemUInt32(0x004A68D0 + 1, (uintptr_t)CanPlayerFireHook - (0x004A68D0 + 0x5));
#endif // if CAMERA_1_3_COMMANDS

    // Change limit of commands
    ASSERT(g_DcNumCommands == 0);
    WriteMemPtr(0x005099AC + 1, g_CommandsBuffer);
    WriteMemUInt8(0x00509A78 + 2, CMD_LIMIT);
    WriteMemPtr(0x00509A8A + 1, g_CommandsBuffer);
    WriteMemPtr(0x00509AB0 + 3, g_CommandsBuffer);
    WriteMemPtr(0x00509AE1 + 3, g_CommandsBuffer);
    WriteMemPtr(0x00509AF5 + 3, g_CommandsBuffer);
    WriteMemPtr(0x00509C8F + 1, g_CommandsBuffer);
    WriteMemPtr(0x00509DB4 + 3, g_CommandsBuffer);
    WriteMemPtr(0x00509E6F + 1, g_CommandsBuffer);
    WriteMemPtr(0x0050A648 + 4, g_CommandsBuffer);
    WriteMemPtr(0x0050A6A0 + 3, g_CommandsBuffer);

    // Better console autocomplete
    DcAutoCompleteInput_Hook.hook(DcAutoCompleteInput_New);
}

void CommandRegister(DcCommand *pCmd)
{
    if (g_DcNumCommands < CMD_LIMIT)
        g_CommandsBuffer[(g_DcNumCommands)++] = pCmd;
    else
        ASSERT(false);

    // Register some unused builtin commands
    DC_REGISTER_CMD(kill_limit, "Sets kill limit", DcfKillLimit);
    DC_REGISTER_CMD(time_limit, "Sets time limit", DcfTimeLimit);
    DC_REGISTER_CMD(geomod_limit, "Sets geomod limit", DcfGeomodLimit);
    DC_REGISTER_CMD(capture_limit, "Sets capture limit", DcfCaptureLimit);

    DC_REGISTER_CMD(sound, "Toggle sound", DcfSound);
    DC_REGISTER_CMD(difficulty, "Set game difficulty", DcfDifficulty);
    //DC_REGISTER_CMD(ms, "Set mouse sensitivity", DcfMouseSensitivity);
    DC_REGISTER_CMD(level_info, "Show level info", DcfLevelInfo);
    DC_REGISTER_CMD(verify_level, "Verify level", DcfVerifyLevel);
    DC_REGISTER_CMD(player_names, "Toggle player names on HUD", DcfPlayerNames);
    DC_REGISTER_CMD(clients_count, "Show number of connected clients", DcfClientsCount);
    DC_REGISTER_CMD(kick_all, "Kick all clients", DcfKickAll);
    DC_REGISTER_CMD(timedemo, "Start timedemo", DcfTimedemo);
    DC_REGISTER_CMD(frameratetest, "Start frame rate test", DcfFramerateTest);
    DC_REGISTER_CMD(system_info, "Show system information", DcfSystemInfo);
    DC_REGISTER_CMD(trilinear_filtering, "Toggle trilinear filtering", DcfTrilinearFiltering);
    DC_REGISTER_CMD(detail_textures, "Toggle detail textures", DcfDetailTextures);

    // New commands
    DC_REGISTER_CMD(findmap, "Find map by filename fragment", DcfFindMap);

    
}

void CommandsAfterGameInit()
{
    unsigned i;

    /* Add commands */
    for (i = 0; i < COUNTOF(g_Commands); ++i)
        CommandRegister(&g_Commands[i]);
}
