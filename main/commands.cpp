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
#include "inline_asm.h"

using namespace rf;

// Note: limit should fit in int8_t
constexpr int CMD_LIMIT = 127;
constexpr int DC_ARG_ANY = 0xFFFFFFFF;

DcCommand *g_CommandsBuffer[CMD_LIMIT];
bool g_DbgGeometryRenderingStats = false;
bool g_DbgStaticLights = false;

class DcInvalidArgTypeError : public std::exception {};
class DcRequiredArgMissingError : public std::exception {};

static bool ReadArgInternal(int TypeFlag, bool PreserveCase = false)
{
    DcGetArg(DC_ARG_ANY, PreserveCase);
    if (g_DcArgType & TypeFlag)
        return true;
    if (!(g_DcArgType & DC_ARG_NONE))
        throw DcInvalidArgTypeError();
    return false;
}

template<typename T>
T DcReadArg()
{
    auto ValueOpt = DcReadArg<std::optional<T>>();
    if (!ValueOpt)
        throw DcRequiredArgMissingError();
    return ValueOpt.value();
}

template<>
std::optional<int> DcReadArg()
{
    if (!ReadArgInternal(DC_ARG_INT))
        return {};
    return std::optional{g_iDcArg};
}

template<>
std::optional<float> DcReadArg()
{
    if (!ReadArgInternal(DC_ARG_FLOAT))
        return {};
    return std::optional{g_fDcArg};
}

template<>
std::optional<bool> DcReadArg()
{
    if (!ReadArgInternal(DC_ARG_TRUE | DC_ARG_FALSE))
        return {};
    bool Value = (g_DcArgType & DC_ARG_TRUE) == DC_ARG_TRUE;
    return std::optional{Value};
}

template<>
std::optional<std::string> DcReadArg()
{
    if (!ReadArgInternal(DC_ARG_STR))
        return {};
    return std::optional{g_pszDcArg};
}

template<typename... Args>
class DcCommand2;

template<typename... Args>
class DcCommand2<void(Args...)> : public rf::DcCommand
{
private:
    std::function<void(Args...)> m_HandlerFunc;
    const char *m_Usage;

public:
    DcCommand2(const char *Name, void(*HandlerFunc)(Args...) ,
        const char *Description = nullptr, const char *Usage = nullptr) :
        m_HandlerFunc(HandlerFunc), m_Usage(Usage)
    {
        pszCmd = Name;
        pszDescr = Description;
        pfnHandler = (void (*)()) StaticHandler;
    }

    void Register()
    {
        CommandRegister(this);
    }

private:
    static __fastcall void StaticHandler(DcCommand2 *This)
    {
        This->Handler();
    }

    void Handler()
    {
        if (g_bDcRun)
            Run();
        else if (g_bDcHelp)
            Help();
    }

    void Run()
    {
        try
        {
            m_HandlerFunc(DcReadArg<Args>()...);
        }
        catch (DcInvalidArgTypeError e)
        {
            DcPrint("Invalid arg type!", nullptr);
        }
        catch (DcRequiredArgMissingError e)
        {
            DcPrint("Required arg is missing!", nullptr);
        }
    }

    void Help()
    {
        if (m_Usage)
        {
            DcPrint(g_ppszStringsTable[STR_USAGE], nullptr);
            DcPrintf("     %s", m_Usage);
        }
    }
};

#ifdef __cpp_deduction_guides
// deduction guide for lambda functions
template <class T>
DcCommand2(const char *, T)
    -> DcCommand2<typename std::remove_pointer_t<decltype(+std::declval<T>())>>;
template <class T>
DcCommand2(const char *, T, const char *)
    -> DcCommand2<typename std::remove_pointer_t<decltype(+std::declval<T>())>>;
template <class T>
DcCommand2(const char *, T, const char *, const char *)
    -> DcCommand2<typename std::remove_pointer_t<decltype(+std::declval<T>())>>;
#endif


auto DcAutoCompleteInput_Hook = makeFunHook(DcAutoCompleteInput);

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
        if (g_bNetworkGame)
            SplitScreenStart(); /* FIXME: set player 2 controls */
        else
            DcPrintf("Works only in multiplayer game!");
    },
    "Starts split screen mode"
};

#endif // SPLITSCREEN_ENABLE

DcCommand2 MaxFpsCmd{ "maxfps",
    [](std::optional<float> LimitOpt) {

        if (LimitOpt)
        {
#ifdef NDEBUG
            float newLimit = std::min(std::max(LimitOpt.value(), (float)MIN_FPS_LIMIT), (float)MAX_FPS_LIMIT);
#else
            float newLimit = LimitOpt.value();
#endif
            g_gameConfig.maxFps = (unsigned)newLimit;
            g_gameConfig.save();
            g_fMinFramerate = 1.0f / newLimit;
        }
        else
            DcPrintf("Maximal FPS: %.1f", 1.0f / g_fMinFramerate);
    },
    "Sets maximal FPS",
    "maxfps <limit>"
};

DcCommand2 DebugCmd{ "debug",
    [](std::optional<std::string> TypeOpt) {

#ifdef NDEBUG
        if (g_bNetworkGame)
        {
            DcPrintf("This command disabled in multiplayer!", nullptr);
            return;
        }
#endif

        auto Type = TypeOpt ? TypeOpt.value() : "";
        bool Handled = false;
        static bool AllFlags = false;

        if (Type.empty())
        {
            AllFlags = !AllFlags;
            Handled = true;
            DcPrintf("All debug flags are %s", AllFlags ? "enabled" : "disabled");
        }

        auto HandleFlag = [&](bool &FlagRef, const char *FlagName) {
            bool OldFlagValue = FlagRef;
            if (Type == FlagName)
            {
                FlagRef = !FlagRef;
                Handled = true;
                DcPrintf("Debug flag '%s' is %s", FlagName, FlagRef ? "enabled" : "disabled");
            } else if (Type.empty()) {
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
        if (GeomRenderingChanged)
        {
            auto GeomCacheClear = (void(*)())0x004F0B90;
            GeomCacheClear();
        }

        if (!Handled)
            DcPrintf("Invalid debug flag: %s", Type.c_str());
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
        if (g_bNetworkGame)
        {
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
            DcPrint("Works only in multiplayer game!", NULL);
    },
    "Starts spectating mode",
    "spectate <player_name/false>"
    // DcPrintf("     spectate <%s>", g_ppszStringsTable[STR_PLAYER_NAME]);
    // DcPrintf("     spectate false");
};

#endif // SPECTATE_MODE_ENABLE

#if MULTISAMPLING_SUPPORT
DcCommand2 AntiAliasingCmd{ "antialiasing",
    []() {
        if (!g_gameConfig.msaa)
            DcPrintf("Anti-aliasing is not supported");
        else
        {
            DWORD Enabled = FALSE;
            g_pGrDevice->GetRenderState(D3DRS_MULTISAMPLEANTIALIAS, &Enabled);
            Enabled = !Enabled;
            g_pGrDevice->SetRenderState(D3DRS_MULTISAMPLEANTIALIAS, Enabled);
            DcPrintf("Anti-aliasing is %s", Enabled ? "enabled" : "disabled");
        }
    },
    "Toggles anti-aliasing"
};
#endif // MULTISAMPLING_SUPPORT

#if DIRECTINPUT_SUPPORT
DcCommand2 InputModeCmd{ "inputmode",
    []() {
        g_bDirectInputDisabled = !g_bDirectInputDisabled;
        if (g_bDirectInputDisabled)
            DcPrintf("DirectInput is disabled");
        else
            DcPrintf("DirectInput is enabled");
    },
    "Toggles input mode"
};
#endif // DIRECTINPUT_SUPPORT

#if CAMERA_1_3_COMMANDS

static int CanPlayerFireHook(rf::Player *pPlayer)
{
    if (!(pPlayer->Flags & 0x10))
        return 0;
    if (g_bNetworkGame && (pPlayer->pCamera->Type == rf::CAM_FREELOOK || pPlayer->pCamera->pPlayer != pPlayer))
        return 0;
    return 1;
}

#endif // if CAMERA_1_3_COMMANDS

DcCommand2 MouseSensitivityCmd{ "ms",
    [](std::optional<float> Value) {
        if (Value) {
            float fValue = g_fDcArg;
            fValue = clamp(fValue, 0.0f, 1.0f);
            g_pLocalPlayer->Config.Controls.fMouseSensitivity = fValue;
        }
        DcPrintf("Mouse sensitivity: %.2f", g_pLocalPlayer->Config.Controls.fMouseSensitivity);
    },
    "Sets mouse sensitivity",
    "ms <value>"
};

DcCommand2 VolumeLightsCmd{ "vli",
    []() {
        static MemChange vliCallChange(0x0043233E);
        if (vliCallChange.IsApplied())
            vliCallChange.Revert();
        else
            vliCallChange.Write("\x90\x90\x90\x90\x90", 5);
        DcPrintf("Volumetric lightining is %s.", vliCallChange.IsApplied() ? "disabled" : "enabled");
    },
    "Toggles volumetric lightining"
};

DcCommand2 LevelSpCmd{ "levelsp",
    [](std::string LevelFilename) {
        if (g_bNetworkGame)
        {
            DcPrintf("You cannot use it in multiplayer game!");
            return;
        }
        if (LevelFilename.find(".rfl") == std::string::npos)
            LevelFilename += ".rfl";

        rf::String strUnk, strLevel;
        rf::String::Init(&strUnk, "");
        rf::String::Init(&strLevel, LevelFilename.c_str());

        DcPrintf("Loading level.");
        SetNextLevelFilename(strLevel, strUnk);
        SwitchMenu(5, 0);
    },
    "Loads single player level",
    "levelsp <rfl_name>"
};

DcCommand2 LevelSoundsCmd{ "levelsounds",
    [](std::optional<float> Volume) {
        if (Volume)
        {
            float fVolScale = clamp(Volume.value(), 0.0f, 1.0f);
            SetPlaySoundEventsVolumeScale(fVolScale);

            g_gameConfig.levelSoundVolume = fVolScale;
            g_gameConfig.save();
        }
        DcPrintf("Level sound volume: %.1f", g_gameConfig.levelSoundVolume);
    },
    "Sets level sounds volume scale",
    "levelsounds <volume>"
};

DcCommand2 PlayerCountCmd{ "playercount",
    []() {
        if (!g_bNetworkGame) return;
        int PlayerCount = 0;
        rf::Player *pPlayer = g_pPlayersList;
        while (pPlayer)
        {
            if (pPlayer != g_pPlayersList)
                ++PlayerCount;
            pPlayer = pPlayer->pNext;
            if (pPlayer == g_pPlayersList)
                break;
        }
        DcPrintf("Player count: %d\n", PlayerCount);
    },
    "Get player count"
};

DcCommand2 FindMapCmd{ "findmap",
    [](std::string Pattern) {
        PackfileFindMatchingFiles(StringMatcher().Infix(Pattern).Suffix(".rfl"), [](const char *pszName)
        {
            DcPrintf("%s\n", pszName);
        });
    },
    "Find map by filename fragment",
    "findmap <query>"
};

DcCommand g_Commands[] = {
    {"hud", "Show and hide HUD", HudCmdHandler},
    { "unban_last", "Unbans last banned player", UnbanLastCmdHandler },
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
    std::vector<rf::Player*> MatchingPlayers;
    FindPlayer(StringMatcher().Prefix(PlayerName), [&](rf::Player *pPlayer)
    {
        MatchingPlayers.push_back(pPlayer);
        DcAutoCompleteUpdateCommonPrefix(CommonPrefix, pPlayer->strName.psz, First);
    });

    if (MatchingPlayers.size() == 1)
        DcAutoCompletePutComponent(Offset, MatchingPlayers[0]->strName.psz, true);
    else
    {
        DcAutoCompletePrintSuggestions(MatchingPlayers, [](rf::Player *pPlayer) { return pPlayer->strName.psz; });
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

ASM_FUNC(DcRunCmd_CallHandlerPatch,
    ASM_I  mov   ecx, ASM_SYM(g_CommandsBuffer)[edi*4]
    ASM_I  call  dword ptr [ecx+8]
    ASM_I  push  0x00509DBE
    ASM_I  ret
)

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

    AsmWritter(0x00509DB4).jmpLong(DcRunCmd_CallHandlerPatch);

    // Better console autocomplete
    DcAutoCompleteInput_Hook.hook(DcAutoCompleteInput_New);
}

void CommandRegister(DcCommand *pCmd)
{
    if (g_DcNumCommands < CMD_LIMIT)
        g_CommandsBuffer[(g_DcNumCommands)++] = pCmd;
    else
        ASSERT(false);
}

void CommandsAfterGameInit()
{
    unsigned i;

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

    /* Add commands */
    for (i = 0; i < COUNTOF(g_Commands); ++i)
        CommandRegister(&g_Commands[i]);

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
