#include <common/BuildConfig.h>
#include "console.h"
#include "../main.h"
#include "../rf/network.h"
#include "../rf/player.h"
#include "../misc/misc.h"
#include "../misc/packfile.h"
#include "../utils/list-utils.h"
#include <algorithm>
#include <patch_common/CallHook.h>
#include <patch_common/AsmWriter.h>

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

DcCommand2 player_count_cmd{
    "playercount",
    []() {
        if (!rf::is_multi)
            return;

        auto player_list = SinglyLinkedList{rf::player_list};
        auto player_count = std::distance(player_list.begin(), player_list.end());
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

static void RegisterBuiltInCommand(const char* name, const char* description, uintptr_t addr)
{
    static std::vector<std::unique_ptr<rf::DcCommand>> builtin_commands;
    auto cmd = std::make_unique<rf::DcCommand>();
    rf::DcCommand::Init(cmd.get(), name, description, reinterpret_cast<rf::DcCmdHandler>(addr));
    builtin_commands.push_back(std::move(cmd));
}

void ConsoleCommandsApplyPatches()
{
    // vli command support
    GlareRenderAllCorona_hook.Install();

    // Allow 'level' command outside of multiplayer game
    AsmWriter(0x00434FEC, 0x00434FF2).nop();
}

void ConsoleCommandsInit()
{
    // Register RF builtin commands disabled in PC build

    // Server configuration commands
    RegisterBuiltInCommand("kill_limit", "Sets kill limit", 0x0046CBC0);
    RegisterBuiltInCommand("time_limit", "Sets time limit", 0x0046CC10);
    RegisterBuiltInCommand("geomod_limit", "Sets geomod limit", 0x0046CC70);
    RegisterBuiltInCommand("capture_limit", "Sets capture limit", 0x0046CCC0);

    // Misc commands
    RegisterBuiltInCommand("sound", "Toggle sound", 0x00434590);
    RegisterBuiltInCommand("difficulty", "Set game difficulty", 0x00434EB0);
    // RegisterBuiltInCommand("ms", "Set mouse sensitivity", 0x0043CE90);
    RegisterBuiltInCommand("level_info", "Show level info", 0x0045C210);
    RegisterBuiltInCommand("verify_level", "Verify level", 0x0045E1F0);
    RegisterBuiltInCommand("player_names", "Toggle player names on HUD", 0x0046CB80);
    RegisterBuiltInCommand("clients_count", "Show number of connected clients", 0x0046CD10);
    RegisterBuiltInCommand("kick_all", "Kick all clients", 0x0047B9E0);
    RegisterBuiltInCommand("timedemo", "Start timedemo", 0x004CC1B0);
    RegisterBuiltInCommand("frameratetest", "Start frame rate test", 0x004CC360);
    RegisterBuiltInCommand("system_info", "Show system information", 0x00525A60);
    RegisterBuiltInCommand("trilinear_filtering", "Toggle trilinear filtering", 0x0054F050);
    RegisterBuiltInCommand("detail_textures", "Toggle detail textures", 0x0054F0B0);

#ifdef DEBUG
    RegisterBuiltInCommand("drop_fixed_cam", "Drop a fixed camera", 0x0040D220);
    RegisterBuiltInCommand("orbit_cam", "Orbit camera around current target", 0x0040D2A0);
    RegisterBuiltInCommand("drop_clutter", "Drop any clutter", 0x0040F0A0);
    RegisterBuiltInCommand("glares_toggle", "toggle glares", 0x00414830);
    RegisterBuiltInCommand("set_vehicle_bounce", "set the elasticity of vehicle collisions", 0x004184B0);
    RegisterBuiltInCommand("set_mass", "set the mass of the targeted object", 0x004184F0);
    RegisterBuiltInCommand("set_skin", "Set the skin of the current player target", 0x00418610);
    RegisterBuiltInCommand("set_life", "Set the life of the player's current target", 0x00418680);
    RegisterBuiltInCommand("set_armor", "Set the armor of the player's current target", 0x004186E0);
    RegisterBuiltInCommand("drop_entity", "Drop any entity", 0x00418740);
    RegisterBuiltInCommand("set_weapon", "set the current weapon for the targeted entity", 0x00418AA0);
    RegisterBuiltInCommand("jump_height", "set the height the player can jump", 0x00418B80);
    RegisterBuiltInCommand("fall_factor", nullptr, 0x004282F0);
    RegisterBuiltInCommand("toggle_crouch", nullptr, 0x00430C50);
    RegisterBuiltInCommand("player_step", nullptr, 0x00433DB0);
    RegisterBuiltInCommand("difficulty", nullptr, 0x00434EB0);
    RegisterBuiltInCommand("mouse_cursor", "Sets the mouse cursor", 0x00435210);
    RegisterBuiltInCommand("fogme", "Fog everything", 0x004352E0);
    RegisterBuiltInCommand("mouse_look", nullptr, 0x0043CF30);
    RegisterBuiltInCommand("drop_item", "Drop any item", 0x00458530);
    RegisterBuiltInCommand("set_georegion_hardness", "Set default hardness for geomods", 0x004663E0);
    RegisterBuiltInCommand("make_myself", nullptr, 0x00475040);
    RegisterBuiltInCommand("splitscreen", nullptr, 0x00480AA0);
    RegisterBuiltInCommand("splitscreen_swap", "Swap splitscreen players", 0x00480AB0);
    RegisterBuiltInCommand("splitscreen_bot_test", "Start a splitscreen game in mk_circuits3.rfl", 0x00480AE0);
    RegisterBuiltInCommand("max_plankton", "set the max number of plankton bits", 0x00497FC0);
    RegisterBuiltInCommand("pcollide", "Toggle if player collides with the world", 0x004A0F60);
    RegisterBuiltInCommand("teleport", "Teleport to an x,y,z", 0x004A0FC0);
    RegisterBuiltInCommand("body", "Set player entity type", 0x004A11C0);
    RegisterBuiltInCommand("invulnerable", "Make self invulnerable", 0x004A12A0);
    RegisterBuiltInCommand("freelook", "Toggle between freelook and first person", 0x004A1340);
    RegisterBuiltInCommand("hide_target", "Hide current target, unhide if already hidden", 0x004A13D0);
    RegisterBuiltInCommand("set_primary", "Set default player primary weapon", 0x004A1430);
    RegisterBuiltInCommand("set_secondary", "Set default player secondary weapon", 0x004A14B0);
    RegisterBuiltInCommand("set_view", "Set camera view from entity with specified UID", 0x004A1540);
    RegisterBuiltInCommand("set_ammo", nullptr, 0x004A15C0);
    RegisterBuiltInCommand("endgame", "force endgame", 0x004A1640);
    RegisterBuiltInCommand("irc", "Set the color range of the infrared characters", 0x004AECE0);
    RegisterBuiltInCommand("irc_close", "Set the close color of the infrared characters", 0x004AEDB0);
    RegisterBuiltInCommand("irc_far", "Set the far color of the infrared characters", 0x004AEE20);
    // RegisterBuiltInCommand("pools", nullptr, 0x004B1050);
    RegisterBuiltInCommand("savegame", "Save the current game", 0x004B3410);
    RegisterBuiltInCommand("loadgame", "Load a game", 0x004B34C0);
    RegisterBuiltInCommand("show_obj_times", "Set the number of portal objects to show times for", 0x004D3250);
    // Some commands does not use dc_exec variables and were not added e.g. 004D3290
    RegisterBuiltInCommand("save_commands", "Print out console commands to the text file console.txt", 0x00509920);
    RegisterBuiltInCommand("script", "Runs a console script file (.vcs)", 0x0050B7D0);
    RegisterBuiltInCommand("screen_res", nullptr, 0x0050E400);
    RegisterBuiltInCommand("wfar", nullptr, 0x005183D0);
    RegisterBuiltInCommand("play_bik", nullptr, 0x00520A70);
#endif // DEBUG

    // Custom Dash Faction commands
    vli_cmd.Register();
    player_count_cmd.Register();
    find_level_cmd.Register();
    find_map_cmd.Register();
    map_cmd.Register();
}
