#include "stdafx.h"
#include "commands.h"
#include "rf.h"
#include "utils.h"
#include "lazyban.h"
#include "main.h"
#include "BuildConfig.h"
#include "spectate_mode.h"
#include "main.h"
#include "misc.h"
#include "packfile.h"
#include "inline_asm.h"
#include <FunHook2.h>
#include <CallHook2.h>

namespace rf {

// Server configuration commands
static const auto DcfKillLimit = (DcCmdHandler)0x0046CBC0;
static const auto DcfTimeLimit = (DcCmdHandler)0x0046CC10;
static const auto DcfGeomodLimit = (DcCmdHandler)0x0046CC70;
static const auto DcfCaptureLimit = (DcCmdHandler)0x0046CCC0;

// Misc commands
static const auto DcfSound = (DcCmdHandler)0x00434590;
static const auto DcfDifficulty = (DcCmdHandler)0x00434EB0;
static const auto DcfMouseSensitivity = (DcCmdHandler)0x0043CE90;
static const auto DcfLevelInfo = (DcCmdHandler)0x0045C210;
static const auto DcfVerifyLevel = (DcCmdHandler)0x0045E1F0;
static const auto DcfPlayerNames = (DcCmdHandler)0x0046CB80;
static const auto DcfClientsCount = (DcCmdHandler)0x0046CD10;
static const auto DcfKickAll = (DcCmdHandler)0x0047B9E0;
static const auto DcfTimedemo = (DcCmdHandler)0x004CC1B0;
static const auto DcfFramerateTest = (DcCmdHandler)0x004CC360;
static const auto DcfSystemInfo = (DcCmdHandler)0x00525A60;
static const auto DcfTrilinearFiltering = (DcCmdHandler)0x0054F050;
static const auto DcfDetailTextures = (DcCmdHandler)0x0054F0B0;

}

// Note: limit should fit in int8_t
constexpr int CMD_LIMIT = 127;

rf::DcCommand *g_CommandsBuffer[CMD_LIMIT];
bool g_DbgGeometryRenderingStats = false;
bool g_DbgStaticLights = false;
bool g_volumetric_lights = true;

rf::Player *FindBestMatchingPlayer(const char *pszName)
{
    rf::Player *pFoundPlayer;
    int NumFound = 0;
    FindPlayer(StringMatcher().Exact(pszName), [&](rf::Player *pPlayer)
    {
        pFoundPlayer = pPlayer;
        ++NumFound;
    });
    if (NumFound == 1)
        return pFoundPlayer;

    NumFound = 0;
    FindPlayer(StringMatcher().Infix(pszName), [&](rf::Player *pPlayer)
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

DcCommand2 SplitScreenCmd{ "splitscreen",
    []() {
        if (rf::g_bNetworkGame)
            SplitScreenStart(); /* FIXME: set player 2 controls */
        else
            rf::DcPrintf("Works only in multiplayer game!");
    },
    "Starts split screen mode"
};

#endif // SPLITSCREEN_ENABLE

DcCommand2 MaxFpsCmd{
    "maxfps",
    [](std::optional<float> LimitOpt) {

        if (LimitOpt) {
#ifdef NDEBUG
            float newLimit = std::min(std::max(LimitOpt.value(), (float)MIN_FPS_LIMIT), (float)MAX_FPS_LIMIT);
#else
            float newLimit = LimitOpt.value();
#endif
            g_game_config.maxFps = (unsigned)newLimit;
            g_game_config.save();
            rf::g_fMinFramerate = 1.0f / newLimit;
        }
        else
            rf::DcPrintf("Maximal FPS: %.1f", 1.0f / rf::g_fMinFramerate);
    },
    "Sets maximal FPS",
    "maxfps <limit>"
};

DcCommand2 DebugCmd{
    "debug",
    [](std::optional<std::string> TypeOpt) {

#ifdef NDEBUG
        if (rf::g_bNetworkGame) {
            rf::DcPrintf("This command disabled in multiplayer!", nullptr);
            return;
        }
#endif

        auto Type = TypeOpt ? TypeOpt.value() : "";
        bool Handled = false;
        static bool AllFlags = false;

        if (Type.empty()) {
            AllFlags = !AllFlags;
            Handled = true;
            rf::DcPrintf("All debug flags are %s", AllFlags ? "enabled" : "disabled");
        }

        auto HandleFlag = [&](bool &FlagRef, const char *FlagName) {
            bool OldFlagValue = FlagRef;
            if (Type == FlagName) {
                FlagRef = !FlagRef;
                Handled = true;
                rf::DcPrintf("Debug flag '%s' is %s", FlagName, FlagRef ? "enabled" : "disabled");
            }
            else if (Type.empty()) {
                FlagRef = AllFlags;
            }
            return OldFlagValue != FlagRef;
        };

        auto &bDbgThruster               = AddrAsRef<bool>(0x0062F3AA);
        auto &bDbgLights                 = AddrAsRef<bool>(0x0062FE19);
        auto &bDbgPushAndClimbingRegions = AddrAsRef<bool>(0x0062FE1A);
        auto &bDbgGeoRegions             = AddrAsRef<bool>(0x0062FE1B);
        auto &bDbgGlass                  = AddrAsRef<bool>(0x0062FE1C);
        auto &bDbgMovers                 = AddrAsRef<bool>(0x0062FE1D);
        auto &bDbgEntityBurn             = AddrAsRef<bool>(0x0062FE1E);
        auto &bDbgMovementMode           = AddrAsRef<bool>(0x0062FE1F);
        auto &bDbgPerfomance             = AddrAsRef<bool>(0x0062FE20);
        auto &bDbgPerfBar                = AddrAsRef<bool>(0x0062FE21);
        auto &bDbgWaypoints              = AddrAsRef<bool>(0x0064E39C);
        auto &bDbgNetwork                = AddrAsRef<bool>(0x006FED24);
        auto &bDbgParticleStats          = AddrAsRef<bool>(0x007B2758);
        auto &bDbgWeapon                 = AddrAsRef<bool>(0x007CAB59);
        auto &bDbgEvents                 = AddrAsRef<bool>(0x00856500);
        auto &bDbgTriggers               = AddrAsRef<bool>(0x0085683C);
        auto &bDbgObjRendering           = AddrAsRef<bool>(0x009BB5AC);
        // Geometry rendering flags
        auto &RenderEverythingSeeThrough     = AddrAsRef<bool>(0x009BB594);
        auto &RenderRoomsInDifferentColors   = AddrAsRef<bool>(0x009BB598);
        auto &RenderNonSeeThroughPortalFaces = AddrAsRef<bool>(0x009BB59C);
        auto &RenderLightmapsOnly            = AddrAsRef<bool>(0x009BB5A4);
        auto &RenderNoLightmaps              = AddrAsRef<bool>(0x009BB5A8);

        HandleFlag(bDbgThruster, "thruster");
        // debug string at the left-top corner
        HandleFlag(bDbgLights, "light");
        HandleFlag(g_DbgStaticLights, "light2");
        HandleFlag(bDbgPushAndClimbingRegions, "push_climb_reg");
        HandleFlag(bDbgGeoRegions, "geo_reg");
        HandleFlag(bDbgGlass, "glass");
        HandleFlag(bDbgMovers, "mover");
        HandleFlag(bDbgEntityBurn, "ignite");
        HandleFlag(bDbgMovementMode, "movemode");
        HandleFlag(bDbgPerfomance, "perf");
        HandleFlag(bDbgPerfBar, "perfbar");
        HandleFlag(bDbgWaypoints, "waypoint");
        // network meter in left-top corner
        HandleFlag(bDbgNetwork, "network");
        HandleFlag(bDbgParticleStats, "particlestats");
        // debug strings at the left side of the screen
        HandleFlag(bDbgWeapon, "weapon");
        HandleFlag(bDbgEvents, "event");
        HandleFlag(bDbgTriggers, "trigger");
        HandleFlag(bDbgObjRendering, "objrender");
        HandleFlag(g_DbgGeometryRenderingStats, "roomstats");
        // geometry rendering
        bool GeomRenderingChanged = false;
        GeomRenderingChanged |= HandleFlag(RenderEverythingSeeThrough, "trans");
        GeomRenderingChanged |= HandleFlag(RenderRoomsInDifferentColors, "room");
        GeomRenderingChanged |= HandleFlag(RenderNonSeeThroughPortalFaces, "portal");
        GeomRenderingChanged |= HandleFlag(RenderLightmapsOnly, "lightmap");
        GeomRenderingChanged |= HandleFlag(RenderNoLightmaps, "nolightmap");

        // Clear geometry cache (needed for geometry rendering flags)
        if (GeomRenderingChanged) {
            auto GeomCacheClear = (void(*)())0x004F0B90;
            GeomCacheClear();
        }

        if (!Handled)
            rf::DcPrintf("Invalid debug flag: %s", Type.c_str());
    },
    nullptr,
    "debug [thruster | light | light2 | push_climb_reg | geo_reg | glass | mover | ignite | movemode | perf | perfbar | "
    "waypoint | network | particlestats | weapon | event | trigger | objrender | roomstats | "
    "trans | room | portal | lightmap | nolightmap]"
};

void DebugRender3d()
{
    const auto DbgWaypoints       = (void(*)()) 0x00468F00;
    const auto DbgInternalLights  = (void(*)()) 0x004DB830;

    DbgWaypoints();
    if (g_DbgStaticLights)
        DbgInternalLights();
}

void DebugRender2d()
{
    const auto DbgRenderingStats  = (void(*)()) 0x004D36B0;
    const auto DbgParticleStats   = (void(*)()) 0x004964E0;
    if (g_DbgGeometryRenderingStats)
        DbgRenderingStats();
    DbgParticleStats();
}

#if SPECTATE_MODE_ENABLE

DcCommand2 SpectateCmd { "spectate",
    [](std::optional<std::string> PlayerName) {
        if (rf::g_bNetworkGame) {
            rf::Player *pPlayer;
            if (PlayerName && PlayerName.value() == "false")
                pPlayer = nullptr;
            else if (PlayerName)
                pPlayer = FindBestMatchingPlayer(PlayerName.value().c_str());
            else
                pPlayer = nullptr;

            if (pPlayer)
                SpectateModeSetTargetPlayer(pPlayer);
        } else
            rf::DcPrint("Works only in multiplayer game!", NULL);
    },
    "Starts spectating mode",
    "spectate <player_name/false>"
    // rf::DcPrintf("     spectate <%s>", rf::strings::player_name);
    // rf::DcPrintf("     spectate false");
};

#endif // SPECTATE_MODE_ENABLE

#if MULTISAMPLING_SUPPORT
DcCommand2 AntiAliasingCmd{ "antialiasing",
    []() {
        if (!g_game_config.msaa)
            rf::DcPrintf("Anti-aliasing is not supported");
        else
        {
            DWORD Enabled = 0;
            rf::g_pGrDevice->GetRenderState(D3DRS_MULTISAMPLEANTIALIAS, &Enabled);
            Enabled = !Enabled;
            rf::g_pGrDevice->SetRenderState(D3DRS_MULTISAMPLEANTIALIAS, Enabled);
            rf::DcPrintf("Anti-aliasing is %s", Enabled ? "enabled" : "disabled");
        }
    },
    "Toggles anti-aliasing"
};
#endif // MULTISAMPLING_SUPPORT

#if DIRECTINPUT_SUPPORT
DcCommand2 InputModeCmd{
    "inputmode",
    []() {
        rf::g_bDirectInputDisabled = !rf::g_bDirectInputDisabled;
        if (rf::g_bDirectInputDisabled)
            rf::DcPrintf("DirectInput is disabled");
        else
            rf::DcPrintf("DirectInput is enabled");
    },
    "Toggles input mode"
};
#endif // DIRECTINPUT_SUPPORT

#if CAMERA_1_3_COMMANDS

static int CanPlayerFireHook(rf::Player *pPlayer)
{
    if (!(pPlayer->Flags & 0x10))
        return 0;
    if (rf::g_bNetworkGame && (pPlayer->pCamera->Type == rf::CAM_FREELOOK || pPlayer->pCamera->pPlayer != pPlayer))
        return 0;
    return 1;
}

#endif // if CAMERA_1_3_COMMANDS

DcCommand2 MouseSensitivityCmd{
    "ms",
    [](std::optional<float> Value) {
        if (Value) {
            float fValue = Value.value();
            fValue = clamp(fValue, 0.0f, 1.0f);
            rf::g_pLocalPlayer->Config.Controls.fMouseSensitivity = fValue;
        }
        rf::DcPrintf("Mouse sensitivity: %.2f", rf::g_pLocalPlayer->Config.Controls.fMouseSensitivity);
    },
    "Sets mouse sensitivity",
    "ms <value>"
};

CallHook2<void()> CoronaRenderAll_Hook{
    0x0043233E,
    []() {
        if (g_volumetric_lights)
            CoronaRenderAll_Hook.CallTarget();
    }
};

DcCommand2 VolumeLightsCmd{
    "vli",
    []() {
        g_volumetric_lights = !g_volumetric_lights;
        rf::DcPrintf("Volumetric lightining is %s.", g_volumetric_lights ? "disabled" : "enabled");
    },
    "Toggles volumetric lightining"
};

DcCommand2 LevelSpCmd{
    "levelsp",
    [](std::string LevelFilename) {
        if (rf::g_bNetworkGame) {
            rf::DcPrintf("You cannot use it in multiplayer game!");
            return;
        }
        if (LevelFilename.find(".rfl") == std::string::npos)
            LevelFilename += ".rfl";

        rf::String unk_str;
        rf::String level_str(LevelFilename.c_str());

        rf::DcPrintf("Loading level.");
        rf::SetNextLevelFilename(level_str, unk_str);
        rf::GameSeqSetState(rf::GS_LOADING_LEVEL, false);
    },
    "Loads single player level",
    "levelsp <rfl_name>"
};

DcCommand2 LevelSoundsCmd{
    "levelsounds",
    [](std::optional<float> Volume) {
        if (Volume) {
            float fVolScale = clamp(Volume.value(), 0.0f, 1.0f);
            SetPlaySoundEventsVolumeScale(fVolScale);

            g_game_config.levelSoundVolume = fVolScale;
            g_game_config.save();
        }
        rf::DcPrintf("Level sound volume: %.1f", g_game_config.levelSoundVolume);
    },
    "Sets level sounds volume scale",
    "levelsounds <volume>"
};

DcCommand2 PlayerCountCmd{
    "playercount",
    []() {
        if (!rf::g_bNetworkGame)
            return;

        int PlayerCount = 0;
        rf::Player *pPlayer = rf::g_pPlayersList;
        while (pPlayer)
        {
            if (pPlayer != rf::g_pPlayersList)
                ++PlayerCount;
            pPlayer = pPlayer->pNext;
            if (pPlayer == rf::g_pPlayersList)
                break;
        }
        rf::DcPrintf("Player count: %d\n", PlayerCount);
    },
    "Get player count"
};

DcCommand2 FindMapCmd{
    "findmap",
    [](std::string Pattern) {
        PackfileFindMatchingFiles(StringMatcher().Infix(Pattern).Suffix(".rfl"), [](const char *pszName)
        {
            rf::DcPrintf("%s\n", pszName);
        });
    },
    "Find map by filename fragment",
    "findmap <query>"
};

void DcShowCmdHelp(rf::DcCommand *pCmd)
{
    rf::g_bDcRun = 0;
    rf::g_bDcHelp = 1;
    rf::g_bDcStatus = 0;
    pCmd->pfnHandler();
}

int DcAutoCompleteGetComponent(int Offset, std::string &Result)
{
    const char *pszBegin = rf::g_szDcCmdLine + Offset, *pszEnd = nullptr, *pszNext;
    if (pszBegin[0] == '"')
    {
        ++pszBegin;
        pszEnd = strchr(pszBegin, '"');
        pszNext = pszEnd ? strchr(pszEnd, ' ') : nullptr;
    }
    else
        pszEnd = pszNext = strchr(pszBegin, ' ');

    if (!pszEnd)
        pszEnd = rf::g_szDcCmdLine + rf::g_cchDcCmdLineLen;

    size_t Len = pszEnd - pszBegin;
    Result.assign(pszBegin, Len);

    return pszNext ? pszNext + 1 - rf::g_szDcCmdLine : -1;
}

void DcAutoCompletePutComponent(int Offset, const std::string &Component, bool Finished)
{
    bool Quote = Component.find(' ') != std::string::npos;
    if (Quote)
        rf::g_cchDcCmdLineLen = Offset + sprintf(rf::g_szDcCmdLine + Offset, "\"%s\"", Component.c_str());
    else
        rf::g_cchDcCmdLineLen = Offset + sprintf(rf::g_szDcCmdLine + Offset, "%s", Component.c_str());
    if (Finished)
        rf::g_cchDcCmdLineLen += sprintf(rf::g_szDcCmdLine + rf::g_cchDcCmdLineLen, " ");
}

template<typename T, typename F>
void DcAutoCompletePrintSuggestions(T &Suggestions, F MappingFun)
{
    for (auto Item : Suggestions)
        rf::DcPrintf("%s\n", MappingFun(Item));
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
    std::vector<rf::Player*> MatchingPlayers;
    FindPlayer(StringMatcher().Prefix(PlayerName), [&](rf::Player *pPlayer)
    {
        MatchingPlayers.push_back(pPlayer);
        DcAutoCompleteUpdateCommonPrefix(CommonPrefix, pPlayer->strName.CStr(), First);
    });

    if (MatchingPlayers.size() == 1)
        DcAutoCompletePutComponent(Offset, MatchingPlayers[0]->strName.CStr(), true);
    else
    {
        DcAutoCompletePrintSuggestions(MatchingPlayers, [](rf::Player *pPlayer) {
            return pPlayer->strName.CStr();
        });
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

    std::vector<rf::DcCommand*> MatchingCmds;
    for (unsigned i = 0; i < rf::g_DcNumCommands; ++i)
    {
        rf::DcCommand *pCmd = g_CommandsBuffer[i];
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
            rf::DcPrintf("%s - %s", pCmd->pszCmd, pCmd->pszDescr);
        DcAutoCompletePutComponent(Offset, CommonPrefix, false);
    }
    else if (MatchingCmds.size() == 1)
        DcAutoCompletePutComponent(Offset, MatchingCmds[0]->pszCmd, true);
}

FunHook2<void()> DcAutoCompleteInput_Hook{
    0x0050A620,
    []() {
        DcAutoCompleteCommand(0);
    }
};

ASM_FUNC(DcRunCmd_CallHandlerPatch,
    ASM_I  mov   ecx, ASM_SYM(g_CommandsBuffer)[edi*4]
    ASM_I  call  dword ptr [ecx+8]
    ASM_I  push  0x00509DBE
    ASM_I  ret
)

void CommandsInit()
{
#if CAMERA_1_3_COMMANDS
    /* Enable camera1-3 in multiplayer and hook CanPlayerFire to disable shooting in camera2 */
    AsmWritter(0x00431280).nop(2);
    AsmWritter(0x004312E0).nop(2);
    AsmWritter(0x00431340).nop(2);
    AsmWritter(0x004A68D0).jmp(CanPlayerFireHook);
#endif // if CAMERA_1_3_COMMANDS

    // Change limit of commands
    ASSERT(rf::g_DcNumCommands == 0);
    WriteMemPtr(0x005099AC + 1, g_CommandsBuffer);
    WriteMem<u8>(0x00509A78 + 2, CMD_LIMIT);
    WriteMemPtr(0x00509A8A + 1, g_CommandsBuffer);
    WriteMemPtr(0x00509AB0 + 3, g_CommandsBuffer);
    WriteMemPtr(0x00509AE1 + 3, g_CommandsBuffer);
    WriteMemPtr(0x00509AF5 + 3, g_CommandsBuffer);
    WriteMemPtr(0x00509C8F + 1, g_CommandsBuffer);
    WriteMemPtr(0x00509DB4 + 3, g_CommandsBuffer);
    WriteMemPtr(0x00509E6F + 1, g_CommandsBuffer);
    WriteMemPtr(0x0050A648 + 4, g_CommandsBuffer);
    WriteMemPtr(0x0050A6A0 + 3, g_CommandsBuffer);

    AsmWritter(0x00509DB4).jmp(DcRunCmd_CallHandlerPatch);

    // Better console autocomplete
    DcAutoCompleteInput_Hook.Install();

    // vli command support
    CoronaRenderAll_Hook.Install();
}

void CommandRegister(rf::DcCommand *pCmd)
{
    if (rf::g_DcNumCommands < CMD_LIMIT)
        g_CommandsBuffer[rf::g_DcNumCommands++] = pCmd;
    else
        ASSERT(false);
}

void CommandsAfterGameInit()
{
    unsigned i;

    // Register some unused builtin commands
    DC_REGISTER_CMD(kill_limit, "Sets kill limit", rf::DcfKillLimit);
    DC_REGISTER_CMD(time_limit, "Sets time limit", rf::DcfTimeLimit);
    DC_REGISTER_CMD(geomod_limit, "Sets geomod limit", rf::DcfGeomodLimit);
    DC_REGISTER_CMD(capture_limit, "Sets capture limit", rf::DcfCaptureLimit);

    DC_REGISTER_CMD(sound, "Toggle sound", rf::DcfSound);
    DC_REGISTER_CMD(difficulty, "Set game difficulty", rf::DcfDifficulty);
    //DC_REGISTER_CMD(ms, "Set mouse sensitivity", rf::DcfMouseSensitivity);
    DC_REGISTER_CMD(level_info, "Show level info", rf::DcfLevelInfo);
    DC_REGISTER_CMD(verify_level, "Verify level", rf::DcfVerifyLevel);
    DC_REGISTER_CMD(player_names, "Toggle player names on HUD", rf::DcfPlayerNames);
    DC_REGISTER_CMD(clients_count, "Show number of connected clients", rf::DcfClientsCount);
    DC_REGISTER_CMD(kick_all, "Kick all clients", rf::DcfKickAll);
    DC_REGISTER_CMD(timedemo, "Start timedemo", rf::DcfTimedemo);
    DC_REGISTER_CMD(frameratetest, "Start frame rate test", rf::DcfFramerateTest);
    DC_REGISTER_CMD(system_info, "Show system information", rf::DcfSystemInfo);
    DC_REGISTER_CMD(trilinear_filtering, "Toggle trilinear filtering", rf::DcfTrilinearFiltering);
    DC_REGISTER_CMD(detail_textures, "Toggle detail textures", rf::DcfDetailTextures);

    MaxFpsCmd.Register();
    MouseSensitivityCmd.Register();
    VolumeLightsCmd.Register();
    LevelSpCmd.Register();
    LevelSoundsCmd.Register();
    PlayerCountCmd.Register();
    FindMapCmd.Register();

#if SPECTATE_MODE_ENABLE
    SpectateCmd.Register();
#endif
#if SPLITSCREEN_ENABLE
    SplitScreenCmd.Register();
#endif
#if DIRECTINPUT_SUPPORT
    InputModeCmd.Register();
#endif
#if MULTISAMPLING_SUPPORT
    AntiAliasingCmd.Register();
#endif
    DebugCmd.Register();
}
