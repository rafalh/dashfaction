#include <common/config/BuildConfig.h>
#include "console.h"
#include "../main/main.h"
#include "../rf/multi.h"
#include "../rf/player/player.h"
#include "../rf/level.h"
#include "../misc/misc.h"
#include "../misc/vpackfile.h"
#include <common/utils/list-utils.h>
#include <algorithm>
#include <patch_common/CallHook.h>
#include <patch_common/FunHook.h>
#include <patch_common/AsmWriter.h>

ConsoleCommand2 dot_cmd{
    ".",
    [](std::string pattern) {
        for (i32 i = 0; i < rf::console::num_commands; ++i) {
            rf::console::Command* cmd = g_commands_buffer[i];
            if (string_contains_ignore_case(cmd->name, pattern)) {
                rf::console::print("{}", cmd->name);
            }
        }
    },
    "Find a console variable or command",
    ". <query>",
};

ConsoleCommand2 vli_cmd{
    "vli",
    []() {
        g_game_config.glares = !g_game_config.glares;
        g_game_config.save();
        rf::console::print("Volumetric lightining is {}.", g_game_config.glares ? "enabled" : "disabled");
    },
    "Toggles volumetric lightining",
};

ConsoleCommand2 player_count_cmd{
    "playercount",
    []() {
        if (!rf::is_multi)
            return;

        auto player_list = SinglyLinkedList{rf::player_list};
        auto player_count = std::distance(player_list.begin(), player_list.end());
        rf::console::print("Player count: {}\n", player_count);
    },
    "Get player count",
};

ConsoleCommand2 find_level_cmd{
    "findlevel",
    [](std::string pattern) {
        vpackfile_find_matching_files(StringMatcher().infix(pattern).suffix(".rfl"), [](const char* name) {
            // Print all matching filenames
            rf::console::print("{}\n", name);
        });
    },
    "Find a level by a filename fragment",
    "findlevel <query>",
};

DcCommandAlias find_map_cmd{
    "findmap",
    find_level_cmd,
};

auto& level_cmd = addr_as_ref<rf::console::Command>(0x00637078);
DcCommandAlias map_cmd{
    "map",
    level_cmd,
};

ConsoleCommand2 level_info_cmd{
    "level_info",
    []() {
        if (rf::level.flags & rf::LEVEL_LOADED) {
            rf::console::print("Filename: {}", rf::level.filename);
            rf::console::print("Name: {}", rf::level.name);
            rf::console::print("Author: {}", rf::level.author);
            rf::console::print("Date: {}", rf::level.level_date);
        } else {
            rf::console::print("No level loaded!");
        }
    },
    "Shows information about the current level",
};

DcCommandAlias map_info_cmd{
    "map_info",
    level_info_cmd,
};

ConsoleCommand2 server_password_cmd{
    "server_password",
    [](std::optional<std::string> new_password) {
        if (!rf::is_multi || !rf::is_server) {
            rf::console::print("This command can only be run as a server!");
            return;
        }            

        if (new_password) {
            rf::netgame.password = new_password.value().c_str();
            rf::console::print("Server password set to: {}", rf::netgame.password);
        }
        else {
            rf::netgame.password = "";
            rf::console::print("Server password removed.");
        }
    },
    "Set or remove the server password.",
    "server_password <password>",
};

// only allow verify_level if a level is loaded (avoid a crash if command is run in menu)
FunHook<void()> verify_level_cmd_hook{
    0x0045E1F0,
    []() {
        if (rf::level.flags & rf::LEVEL_LOADED) {
            verify_level_cmd_hook.call_target();
        } else {
            rf::console::print("No level loaded!");
        }
    }
};

static void register_builtin_command(const char* name, const char* description, uintptr_t addr)
{
    static std::vector<std::unique_ptr<rf::console::Command>> builtin_commands;
    auto cmd = std::make_unique<rf::console::Command>();
    rf::console::Command::init(cmd.get(), name, description, reinterpret_cast<rf::console::CommandFuncPtr>(addr));
    builtin_commands.push_back(std::move(cmd));
}

void console_commands_apply_patches()
{
    // Allow 'level' command outside of multiplayer game
    AsmWriter(0x00434FEC, 0x00434FF2).nop();
}

void console_commands_init()
{
    // Register RF builtin commands disabled in PC build

    // Server configuration commands
    register_builtin_command("kill_limit", "Sets kill limit", 0x0046CBC0);
    register_builtin_command("time_limit", "Sets time limit", 0x0046CC10);
    register_builtin_command("geomod_limit", "Sets geomod limit", 0x0046CC70);
    register_builtin_command("capture_limit", "Sets capture limit", 0x0046CCC0);

    // Misc commands
    register_builtin_command("sound", "Toggle sound", 0x00434590);
    register_builtin_command("difficulty", "Set game difficulty", 0x00434EB0);
    // register_builtin_command("ms", "Set mouse sensitivity", 0x0043CE90);
    // register_builtin_command("level_info", "Show level info", 0x0045C210);
    register_builtin_command("verify_level", "Verify level", 0x0045E1F0);
    register_builtin_command("player_names", "Toggle player names on HUD", 0x0046CB80);
    register_builtin_command("clients_count", "Show number of connected clients", 0x0046CD10);
    register_builtin_command("kick_all", "Kick all clients", 0x0047B9E0);
    register_builtin_command("timedemo", "Start timedemo", 0x004CC1B0);
    register_builtin_command("frameratetest", "Start frame rate test", 0x004CC360);
    register_builtin_command("system_info", "Show system information", 0x00525A60);
    register_builtin_command("trilinear_filtering", "Toggle trilinear filtering", 0x0054F050);
    register_builtin_command("detail_textures", "Toggle detail textures", 0x0054F0B0);

#ifdef DEBUG
    register_builtin_command("drop_fixed_cam", "Drop a fixed camera", 0x0040D220);
    register_builtin_command("orbit_cam", "Orbit camera around current target", 0x0040D2A0);
    register_builtin_command("drop_clutter", "Drop any clutter", 0x0040F0A0);
    register_builtin_command("glares_toggle", "toggle glares", 0x00414830);
    register_builtin_command("set_vehicle_bounce", "set the elasticity of vehicle collisions", 0x004184B0);
    register_builtin_command("set_mass", "set the mass of the targeted object", 0x004184F0);
    register_builtin_command("set_skin", "Set the skin of the current player target", 0x00418610);
    register_builtin_command("set_life", "Set the life of the player's current target", 0x00418680);
    register_builtin_command("set_armor", "Set the armor of the player's current target", 0x004186E0);
    register_builtin_command("drop_entity", "Drop any entity", 0x00418740);
    register_builtin_command("set_weapon", "set the current weapon for the targeted entity", 0x00418AA0);
    register_builtin_command("jump_height", "set the height the player can jump", 0x00418B80);
    register_builtin_command("fall_factor", nullptr, 0x004282F0);
    register_builtin_command("toggle_crouch", nullptr, 0x00430C50);
    register_builtin_command("player_step", nullptr, 0x00433DB0);
    register_builtin_command("difficulty", nullptr, 0x00434EB0);
    register_builtin_command("mouse_cursor", "Sets the mouse cursor", 0x00435210);
    register_builtin_command("fogme", "Fog everything", 0x004352E0);
    register_builtin_command("mouse_look", nullptr, 0x0043CF30);
    register_builtin_command("drop_item", "Drop any item", 0x00458530);
    register_builtin_command("set_georegion_hardness", "Set default hardness for geomods", 0x004663E0);
    register_builtin_command("make_myself", nullptr, 0x00475040);
    register_builtin_command("splitscreen", nullptr, 0x00480AA0);
    register_builtin_command("splitscreen_swap", "Swap splitscreen players", 0x00480AB0);
    register_builtin_command("splitscreen_bot_test", "Start a splitscreen game in mk_circuits3.rfl", 0x00480AE0);
    register_builtin_command("max_plankton", "set the max number of plankton bits", 0x00497FC0);
    register_builtin_command("pcollide", "Toggle if player collides with the world", 0x004A0F60);
    register_builtin_command("teleport", "Teleport to an x,y,z", 0x004A0FC0);
    register_builtin_command("body", "Set player entity type", 0x004A11C0);
    register_builtin_command("invulnerable", "Make self invulnerable", 0x004A12A0);
    register_builtin_command("freelook", "Toggle between freelook and first person", 0x004A1340);
    register_builtin_command("hide_target", "Hide current target, unhide if already hidden", 0x004A13D0);
    register_builtin_command("set_primary", "Set default player primary weapon", 0x004A1430);
    register_builtin_command("set_secondary", "Set default player secondary weapon", 0x004A14B0);
    register_builtin_command("set_view", "Set camera view from entity with specified UID", 0x004A1540);
    register_builtin_command("set_ammo", nullptr, 0x004A15C0);
    register_builtin_command("endgame", "force endgame", 0x004A1640);
    register_builtin_command("irc", "Set the color range of the infrared characters", 0x004AECE0);
    register_builtin_command("irc_close", "Set the close color of the infrared characters", 0x004AEDB0);
    register_builtin_command("irc_far", "Set the far color of the infrared characters", 0x004AEE20);
    // register_builtin_command("pools", nullptr, 0x004B1050);
    register_builtin_command("savegame", "Save the current game", 0x004B3410);
    register_builtin_command("loadgame", "Load a game", 0x004B34C0);
    register_builtin_command("show_obj_times", "Set the number of portal objects to show times for", 0x004D3250);
    // Some commands does not use console_exec variables and were not added e.g. 004D3290
    register_builtin_command("save_commands", "Print out console commands to the text file console.txt", 0x00509920);
    register_builtin_command("script", "Runs a console script file (.vcs)", 0x0050B7D0);
    register_builtin_command("screen_res", nullptr, 0x0050E400);
    register_builtin_command("wfar", nullptr, 0x005183D0);
    register_builtin_command("play_bik", nullptr, 0x00520A70);
#endif // DEBUG

    // Custom Dash Faction commands
    dot_cmd.register_cmd();
    vli_cmd.register_cmd();
    player_count_cmd.register_cmd();
    find_level_cmd.register_cmd();
    find_map_cmd.register_cmd();
    map_cmd.register_cmd();
    level_info_cmd.register_cmd();
    map_info_cmd.register_cmd();
    server_password_cmd.register_cmd();
    verify_level_cmd_hook.install();
}
