#include "commands.h"
#include "BuildConfig.h"
#include "lazyban.h"
#include "main.h"
#include "misc.h"
#include "packfile.h"
#include "rf.h"
#include "spectate_mode.h"
#include "stdafx.h"
#include "utils.h"
#include <CallHook.h>
#include <FunHook.h>
#include <RegsPatch.h>

namespace rf
{

// Server configuration commands
static const auto DcfKillLimit = reinterpret_cast<DcCmdHandler>(0x0046CBC0);
static const auto DcfTimeLimit = reinterpret_cast<DcCmdHandler>(0x0046CC10);
static const auto DcfGeomodLimit = reinterpret_cast<DcCmdHandler>(0x0046CC70);
static const auto DcfCaptureLimit = reinterpret_cast<DcCmdHandler>(0x0046CCC0);

// Misc commands
static const auto DcfSound = reinterpret_cast<DcCmdHandler>(0x00434590);
static const auto DcfDifficulty = reinterpret_cast<DcCmdHandler>(0x00434EB0);
static const auto DcfMouseSensitivity = reinterpret_cast<DcCmdHandler>(0x0043CE90);
static const auto DcfLevelInfo = reinterpret_cast<DcCmdHandler>(0x0045C210);
static const auto DcfVerifyLevel = reinterpret_cast<DcCmdHandler>(0x0045E1F0);
static const auto DcfPlayerNames = reinterpret_cast<DcCmdHandler>(0x0046CB80);
static const auto DcfClientsCount = reinterpret_cast<DcCmdHandler>(0x0046CD10);
static const auto DcfKickAll = reinterpret_cast<DcCmdHandler>(0x0047B9E0);
static const auto DcfTimedemo = reinterpret_cast<DcCmdHandler>(0x004CC1B0);
static const auto DcfFramerateTest = reinterpret_cast<DcCmdHandler>(0x004CC360);
static const auto DcfSystemInfo = reinterpret_cast<DcCmdHandler>(0x00525A60);
static const auto DcfTrilinearFiltering = reinterpret_cast<DcCmdHandler>(0x0054F050);
static const auto DcfDetailTextures = reinterpret_cast<DcCmdHandler>(0x0054F0B0);

} // namespace rf

// Note: limit should fit in int8_t
constexpr int CMD_LIMIT = 127;

rf::DcCommand* g_commands_buffer[CMD_LIMIT];
bool g_dbg_geometry_rendering_stats = false;
bool g_dbg_static_lights = false;

rf::Player* FindBestMatchingPlayer(const char* name)
{
    rf::Player* found_player;
    int num_found = 0;
    FindPlayer(StringMatcher().Exact(name), [&](rf::Player* player) {
        found_player = player;
        ++num_found;
    });
    if (num_found == 1)
        return found_player;

    num_found = 0;
    FindPlayer(StringMatcher().Infix(name), [&](rf::Player* player) {
        found_player = player;
        ++num_found;
    });

    if (num_found == 1)
        return found_player;
    else if (num_found > 1)
        rf::DcPrintf("Found %d players matching '%s'!", num_found, name);
    else
        rf::DcPrintf("Cannot find player matching '%s'", name);
    return nullptr;
}

#if SPLITSCREEN_ENABLE

DcCommand2 SplitScreenCmd{
    "splitscreen",
    []() {
        if (rf::is_net_game)
            SplitScreenStart(); /* FIXME: set player 2 controls */
        else
            rf::DcPrintf("Works only in multiplayer game!");
    },
    "Starts split screen mode",
};

#endif // SPLITSCREEN_ENABLE

DcCommand2 max_fps_cmd{
    "maxfps",
    [](std::optional<int> limit_opt) {
        if (limit_opt) {
#ifdef NDEBUG
            int new_limit = std::clamp<int>(limit_opt.value(), MIN_FPS_LIMIT, MAX_FPS_LIMIT);
#else
            int new_limit = limit_opt.value();
#endif
            g_game_config.maxFps = new_limit;
            g_game_config.save();
            rf::min_framerate = 1.0f / new_limit;
        }
        else
            rf::DcPrintf("Maximal FPS: %.1f", 1.0f / rf::min_framerate);
    },
    "Sets maximal FPS",
    "maxfps <limit>",
};

DcCommand2 debug_cmd{
    "debug",
    [](std::optional<std::string> type_opt) {

#ifdef NDEBUG
        if (rf::is_net_game) {
            rf::DcPrintf("This command is disabled in multiplayer!");
            return;
        }
#endif

        auto type = type_opt ? type_opt.value() : "";
        bool handled = false;
        static bool all_flags = false;

        if (type.empty()) {
            all_flags = !all_flags;
            handled = true;
            rf::DcPrintf("All debug flags are %s", all_flags ? "enabled" : "disabled");
        }

        auto handle_flag = [&](bool& flag_ref, const char* flag_name) {
            bool old_flag_value = flag_ref;
            if (type == flag_name) {
                flag_ref = !flag_ref;
                handled = true;
                rf::DcPrintf("Debug flag '%s' is %s", flag_name, flag_ref ? "enabled" : "disabled");
            }
            else if (type.empty()) {
                flag_ref = all_flags;
            }
            return old_flag_value != flag_ref;
        };

        // clang-format off
        auto &dg_thruster                  = AddrAsRef<bool>(0x0062F3AA);
        auto &dg_lights                    = AddrAsRef<bool>(0x0062FE19);
        auto &dg_push_and_climbing_regions = AddrAsRef<bool>(0x0062FE1A);
        auto &dg_geo_regions               = AddrAsRef<bool>(0x0062FE1B);
        auto &dg_glass                     = AddrAsRef<bool>(0x0062FE1C);
        auto &dg_movers                    = AddrAsRef<bool>(0x0062FE1D);
        auto &dg_entity_burn               = AddrAsRef<bool>(0x0062FE1E);
        auto &dg_movement_mode             = AddrAsRef<bool>(0x0062FE1F);
        auto &dg_perfomance                = AddrAsRef<bool>(0x0062FE20);
        auto &dg_perf_bar                  = AddrAsRef<bool>(0x0062FE21);
        auto &dg_waypoints                 = AddrAsRef<bool>(0x0064E39C);
        auto &dg_network                   = AddrAsRef<bool>(0x006FED24);
        auto &dg_particle_stats            = AddrAsRef<bool>(0x007B2758);
        auto &dg_weapon                    = AddrAsRef<bool>(0x007CAB59);
        auto &dg_events                    = AddrAsRef<bool>(0x00856500);
        auto &dg_triggers                  = AddrAsRef<bool>(0x0085683C);
        auto &dg_obj_rendering             = AddrAsRef<bool>(0x009BB5AC);
        // Geometry rendering flags
        auto& render_everything_see_through       = AddrAsRef<bool>(0x009BB594);
        auto& render_rooms_in_different_colors    = AddrAsRef<bool>(0x009BB598);
        auto& render_non_see_through_portal_faces = AddrAsRef<bool>(0x009BB59C);
        auto& render_lightmaps_only               = AddrAsRef<bool>(0x009BB5A4);
        auto& render_no_lightmaps                 = AddrAsRef<bool>(0x009BB5A8);
        // clang-format on

        handle_flag(dg_thruster, "thruster");
        // debug string at the left-top corner
        handle_flag(dg_lights, "light");
        handle_flag(g_dbg_static_lights, "light2");
        handle_flag(dg_push_and_climbing_regions, "push_climb_reg");
        handle_flag(dg_geo_regions, "geo_reg");
        handle_flag(dg_glass, "glass");
        handle_flag(dg_movers, "mover");
        handle_flag(dg_entity_burn, "ignite");
        handle_flag(dg_movement_mode, "movemode");
        handle_flag(dg_perfomance, "perf");
        handle_flag(dg_perf_bar, "perfbar");
        handle_flag(dg_waypoints, "waypoint");
        // network meter in left-top corner
        handle_flag(dg_network, "network");
        handle_flag(dg_particle_stats, "particlestats");
        // debug strings at the left side of the screen
        handle_flag(dg_weapon, "weapon");
        handle_flag(dg_events, "event");
        handle_flag(dg_triggers, "trigger");
        handle_flag(dg_obj_rendering, "objrender");
        handle_flag(g_dbg_geometry_rendering_stats, "roomstats");
        // geometry rendering
        bool geom_rendering_changed = false;
        geom_rendering_changed |= handle_flag(render_everything_see_through, "trans");
        geom_rendering_changed |= handle_flag(render_rooms_in_different_colors, "room");
        geom_rendering_changed |= handle_flag(render_non_see_through_portal_faces, "portal");
        geom_rendering_changed |= handle_flag(render_lightmaps_only, "lightmap");
        geom_rendering_changed |= handle_flag(render_no_lightmaps, "nolightmap");

        // Clear geometry cache (needed for geometry rendering flags)
        if (geom_rendering_changed) {
            auto geom_cache_clear = (void (*)())0x004F0B90;
            geom_cache_clear();
        }

        if (!handled)
            rf::DcPrintf("Invalid debug flag: %s", type.c_str());
    },
    nullptr,
    "debug [thruster | light | light2 | push_climb_reg | geo_reg | glass | mover | ignite | movemode | perf | "
    "perfbar | waypoint | network | particlestats | weapon | event | trigger | objrender | roomstats | trans | "
    "room | portal | lightmap | nolightmap]",
};

void DebugRender3d()
{
    const auto dbg_waypoints = AddrAsRef<void()>(0x00468F00);
    const auto dbg_internal_lights = AddrAsRef<void()>(0x004DB830);

    dbg_waypoints();
    if (g_dbg_static_lights)
        dbg_internal_lights();
}

void DebugRender2d()
{
    const auto dbg_rendering_stats = AddrAsRef<void()>(0x004D36B0);
    const auto dbg_particle_stats = AddrAsRef<void()>(0x004964E0);
    if (g_dbg_geometry_rendering_stats)
        dbg_rendering_stats();
    dbg_particle_stats();
}

#if SPECTATE_MODE_ENABLE

DcCommand2 SpectateCmd{
    "spectate",
    [](std::optional<std::string> player_name) {
        if (rf::is_net_game) {
            rf::Player* player;
            if (player_name && player_name.value() == "false")
                player = nullptr;
            else if (player_name)
                player = FindBestMatchingPlayer(player_name.value().c_str());
            else
                player = nullptr;

            if (player)
                SpectateModeSetTargetPlayer(player);
        }
        else
            rf::DcPrint("Works only in multiplayer game!", nullptr);
    },
    "Starts spectating mode", "spectate <player_name/false>"
    // rf::DcPrintf("     spectate <%s>", rf::strings::player_name);
    // rf::DcPrintf("     spectate false");
};

#endif // SPECTATE_MODE_ENABLE

#if MULTISAMPLING_SUPPORT
DcCommand2 AntiAliasingCmd{
    "antialiasing",
    []() {
        if (!g_game_config.msaa)
            rf::DcPrintf("Anti-aliasing is not supported");
        else {
            DWORD enabled = 0;
            rf::gr_d3d_device->GetRenderState(D3DRS_MULTISAMPLEANTIALIAS, &enabled);
            enabled = !enabled;
            rf::gr_d3d_device->SetRenderState(D3DRS_MULTISAMPLEANTIALIAS, enabled);
            rf::DcPrintf("Anti-aliasing is %s", enabled ? "enabled" : "disabled");
        }
    },
    "Toggles anti-aliasing",
};
#endif // MULTISAMPLING_SUPPORT

#if DIRECTINPUT_SUPPORT
DcCommand2 input_mode_cmd{
    "inputmode",
    []() {
        rf::direct_input_disabled = !rf::direct_input_disabled;
        if (rf::direct_input_disabled)
            rf::DcPrintf("DirectInput is disabled");
        else
            rf::DcPrintf("DirectInput is enabled");
    },
    "Toggles input mode",
};
#endif // DIRECTINPUT_SUPPORT

#if CAMERA_1_3_COMMANDS

static int CanPlayerFireHook(rf::Player* player)
{
    if (!(player->flags & 0x10))
        return 0;
    if (rf::is_net_game && (player->camera->type == rf::CAM_FREELOOK || player->camera->player != player))
        return 0;
    return 1;
}

#endif // if CAMERA_1_3_COMMANDS

DcCommand2 ms_cmd{
    "ms",
    [](std::optional<float> value_opt) {
        if (value_opt) {
            float value = value_opt.value();
            value = std::clamp(value, 0.0f, 1.0f);
            rf::local_player->config.controls.mouse_sensitivity = value;
        }
        rf::DcPrintf("Mouse sensitivity: %.2f", rf::local_player->config.controls.mouse_sensitivity);
    },
    "Sets mouse sensitivity",
    "ms <value>",
};

CallHook<void(bool)> GlareRenderAllCorona_hook{
    0x0043233E,
    [](bool reflections) {
        if (g_game_config.glares)
            GlareRenderAllCorona_hook.CallTarget(reflections);
    },
};

DcCommand2 vli_cmd{
    "vli",
    []() {
        g_game_config.glares = !g_game_config.glares;
        g_game_config.save();
        rf::DcPrintf("Volumetric lightining is %s.", g_game_config.glares ? "enabled" : "disabled");
    },
    "Toggles volumetric lightining",
};

DcCommand2 level_sounds_cmd{
    "levelsounds",
    [](std::optional<float> volume) {
        if (volume) {
            float vol_scale = std::clamp(volume.value(), 0.0f, 1.0f);
            SetPlaySoundEventsVolumeScale(vol_scale);

            g_game_config.levelSoundVolume = vol_scale;
            g_game_config.save();
        }
        rf::DcPrintf("Level sound volume: %.1f", g_game_config.levelSoundVolume);
    },
    "Sets level sounds volume scale",
    "levelsounds <volume>",
};

DcCommand2 player_count_cmd{
    "playercount",
    []() {
        if (!rf::is_net_game)
            return;

        int player_count = 0;
        rf::Player* player = rf::player_list;
        while (player) {
            if (player != rf::player_list)
                ++player_count;
            player = player->next;
            if (player == rf::player_list)
                break;
        }
        rf::DcPrintf("Player count: %d\n", player_count);
    },
    "Get player count",
};

DcCommand2 find_level_cmd{
    "findlevel",
    [](std::string pattern) {
        PackfileFindMatchingFiles(StringMatcher().Infix(pattern).Suffix(".rfl"), [](const char* name) {
            // Print all matching filenames
            rf::DcPrintf("%s\n", name);
        });
    },
    "Find a level by a filename fragment",
    "findlevel <query>",
};

DcCommandAlias find_map_cmd{
    "findmap",
    find_level_cmd,
};

auto& level_cmd = AddrAsRef<rf::DcCommand>(0x00637078);
DcCommandAlias map_cmd{
    "map",
    level_cmd,
};

void DcShowCmdHelp(rf::DcCommand* cmd)
{
    rf::dc_run = 0;
    rf::dc_help = 1;
    rf::dc_status = 0;
    auto handler = reinterpret_cast<void(__fastcall*)(rf::DcCommand*)>(cmd->func);
    handler(cmd);
}

int DcAutoCompleteGetComponent(int offset, std::string& result)
{
    const char *begin = rf::dc_cmd_line + offset, *end = nullptr, *next;
    if (begin[0] == '"') {
        ++begin;
        end = strchr(begin, '"');
        next = end ? strchr(end, ' ') : nullptr;
    }
    else
        end = next = strchr(begin, ' ');

    if (!end)
        end = rf::dc_cmd_line + rf::dc_cmd_line_len;

    size_t len = end - begin;
    result.assign(begin, len);

    return next ? next + 1 - rf::dc_cmd_line : -1;
}

void DcAutoCompletePutComponent(int offset, const std::string& component, bool finished)
{
    bool quote = component.find(' ') != std::string::npos;
    int max_len = std::size(rf::dc_cmd_line);
    int len;
    if (quote) {
        len = snprintf(rf::dc_cmd_line + offset, max_len - offset, "\"%s\"", component.c_str());
    }
    else {
        len = snprintf(rf::dc_cmd_line + offset, max_len - offset, "%s", component.c_str());
    }
    rf::dc_cmd_line_len = offset + len;
    if (finished)
        rf::dc_cmd_line_len += snprintf(rf::dc_cmd_line + rf::dc_cmd_line_len, max_len - rf::dc_cmd_line_len, " ");
}

template<typename T, typename F>
void DcAutoCompletePrintSuggestions(T& suggestions, F mapping_fun)
{
    for (auto item : suggestions) {
        rf::DcPrintf("%s\n", mapping_fun(item));
    }
}

void DcAutoCompleteUpdateCommonPrefix(std::string& common_prefix, const std::string& value, bool& first,
                                      bool case_sensitive = false)
{
    if (first) {
        first = false;
        common_prefix = value;
    }
    if (common_prefix.size() > value.size())
        common_prefix.resize(value.size());
    for (size_t i = 0; i < common_prefix.size(); ++i) {
        if ((case_sensitive && common_prefix[i] != value[i]) || tolower(common_prefix[i]) != tolower(value[i])) {
            common_prefix.resize(i);
            break;
        }
    }
}

void DcAutoCompleteLevel(int offset)
{
    std::string level_name;
    DcAutoCompleteGetComponent(offset, level_name);
    if (level_name.size() < 3)
        return;

    bool first = true;
    std::string common_prefix;
    std::vector<std::string> matches;
    PackfileFindMatchingFiles(StringMatcher().Prefix(level_name).Suffix(".rfl"), [&](const char* name) {
        auto ext = strrchr(name, '.');
        auto name_len = ext ? ext - name : strlen(name);
        std::string name_without_ext(name, name_len);
        matches.push_back(name_without_ext);
        DcAutoCompleteUpdateCommonPrefix(common_prefix, name_without_ext, first);
    });

    if (matches.size() == 1)
        DcAutoCompletePutComponent(offset, matches[0], true);
    else if (!matches.empty()) {
        DcAutoCompletePrintSuggestions(matches, [](std::string& name) { return name.c_str(); });
        DcAutoCompletePutComponent(offset, common_prefix, false);
    }
}

void DcAutoCompletePlayer(int offset)
{
    std::string player_name;
    DcAutoCompleteGetComponent(offset, player_name);
    if (player_name.size() < 1)
        return;

    bool first = true;
    std::string common_prefix;
    std::vector<rf::Player*> matching_players;
    FindPlayer(StringMatcher().Prefix(player_name), [&](rf::Player* player) {
        matching_players.push_back(player);
        DcAutoCompleteUpdateCommonPrefix(common_prefix, player->name.CStr(), first);
    });

    if (matching_players.size() == 1)
        DcAutoCompletePutComponent(offset, matching_players[0]->name.CStr(), true);
    else if (!matching_players.empty()) {
        DcAutoCompletePrintSuggestions(matching_players, [](rf::Player* player) {
            // Print player names
            return player->name.CStr();
        });
        DcAutoCompletePutComponent(offset, common_prefix, false);
    }
}

void DcAutoCompleteCommand(int offset)
{
    std::string cmd_name;
    int next_offset = DcAutoCompleteGetComponent(offset, cmd_name);
    if (cmd_name.size() < 2)
        return;

    bool first = true;
    std::string common_prefix;

    std::vector<rf::DcCommand*> matching_cmds;
    for (unsigned i = 0; i < rf::dc_num_commands; ++i) {
        rf::DcCommand* cmd = g_commands_buffer[i];
        if (!strnicmp(cmd->cmd_name, cmd_name.c_str(), cmd_name.size()) &&
            (next_offset == -1 || !cmd->cmd_name[cmd_name.size()])) {
            matching_cmds.push_back(cmd);
            DcAutoCompleteUpdateCommonPrefix(common_prefix, cmd->cmd_name, first);
        }
    }

    if (next_offset != -1) {
        if (matching_cmds.size() != 1)
            return;
        if (!stricmp(cmd_name.c_str(), "level") || !stricmp(cmd_name.c_str(), "levelsp"))
            DcAutoCompleteLevel(next_offset);
        else if (!stricmp(cmd_name.c_str(), "kick") || !stricmp(cmd_name.c_str(), "ban"))
            DcAutoCompletePlayer(next_offset);
        else if (!stricmp(cmd_name.c_str(), "rcon"))
            DcAutoCompleteCommand(next_offset);
        else
            DcShowCmdHelp(matching_cmds[0]);
    }
    else if (matching_cmds.size() > 1) {
        for (auto* cmd : matching_cmds) {
            if (cmd->descr)
                rf::DcPrintf("%s - %s", cmd->cmd_name, cmd->descr);
            else
                rf::DcPrintf("%s", cmd->cmd_name);
        }
        DcAutoCompletePutComponent(offset, common_prefix, false);
    }
    else if (matching_cmds.size() == 1)
        DcAutoCompletePutComponent(offset, matching_cmds[0]->cmd_name, true);
}

FunHook<void()> DcAutoCompleteInput_hook{
    0x0050A620,
    []() {
        // autocomplete on offset 0
        DcAutoCompleteCommand(0);
    },
};

RegsPatch DcRunCmd_CallHandlerPatch{
    0x00509DBB,
    [](auto& regs) {
        // Make sure command pointer is in ecx register to support thiscall handlers
        regs.ecx = regs.eax;
    },
};

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
    ASSERT(rf::dc_num_commands == 0);
    WriteMemPtr(0x005099AC + 1, g_commands_buffer);
    WriteMem<u8>(0x00509A78 + 2, CMD_LIMIT);
    WriteMemPtr(0x00509A8A + 1, g_commands_buffer);
    WriteMemPtr(0x00509AB0 + 3, g_commands_buffer);
    WriteMemPtr(0x00509AE1 + 3, g_commands_buffer);
    WriteMemPtr(0x00509AF5 + 3, g_commands_buffer);
    WriteMemPtr(0x00509C8F + 1, g_commands_buffer);
    WriteMemPtr(0x00509DB4 + 3, g_commands_buffer);
    WriteMemPtr(0x00509E6F + 1, g_commands_buffer);
    WriteMemPtr(0x0050A648 + 4, g_commands_buffer);
    WriteMemPtr(0x0050A6A0 + 3, g_commands_buffer);

    DcRunCmd_CallHandlerPatch.Install();

    // Better console autocomplete
    DcAutoCompleteInput_hook.Install();

    // vli command support
    GlareRenderAllCorona_hook.Install();

    // Allow 'level' command outside of multiplayer game
    AsmWritter(0x00434FEC, 0x00434FF2).nop();
}

void CommandRegister(rf::DcCommand* cmd)
{
    if (rf::dc_num_commands < CMD_LIMIT)
        g_commands_buffer[rf::dc_num_commands++] = cmd;
    else
        ASSERT(false);
}

void CommandsAfterGameInit()
{
    // Register some unused builtin commands
    DC_REGISTER_CMD(kill_limit, "Sets kill limit", rf::DcfKillLimit);
    DC_REGISTER_CMD(time_limit, "Sets time limit", rf::DcfTimeLimit);
    DC_REGISTER_CMD(geomod_limit, "Sets geomod limit", rf::DcfGeomodLimit);
    DC_REGISTER_CMD(capture_limit, "Sets capture limit", rf::DcfCaptureLimit);

    DC_REGISTER_CMD(sound, "Toggle sound", rf::DcfSound);
    DC_REGISTER_CMD(difficulty, "Set game difficulty", rf::DcfDifficulty);
    // DC_REGISTER_CMD(ms, "Set mouse sensitivity", rf::DcfMouseSensitivity);
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

    max_fps_cmd.Register();
    ms_cmd.Register();
    vli_cmd.Register();
    level_sounds_cmd.Register();
    player_count_cmd.Register();
    find_level_cmd.Register();
    find_map_cmd.Register();
    map_cmd.Register();

#if SPECTATE_MODE_ENABLE
    SpectateCmd.Register();
#endif
#if SPLITSCREEN_ENABLE
    SplitScreenCmd.Register();
#endif
#if DIRECTINPUT_SUPPORT
    input_mode_cmd.Register();
#endif
#if MULTISAMPLING_SUPPORT
    AntiAliasingCmd.Register();
#endif
    debug_cmd.Register();
}
