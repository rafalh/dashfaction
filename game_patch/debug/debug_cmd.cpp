#include "debug_internal.h"
#include "../multi/multi.h"
#include "../rf/multi.h"
#include "../rf/geometry.h"
#include "../os/console.h"
#include <patch_common/MemUtils.h>
#include <patch_common/CodeInjection.h>

struct DebugFlagDesc
{
    bool& ref;
    const char* name;
    bool clear_geometry_cache = false;
    bool allow_multi = false;
};

bool g_dbg_geometry_rendering_stats = false;
bool g_dbg_static_lights = false;

DebugFlagDesc g_debug_flags[] = {
    {addr_as_ref<bool>(0x0062F3AA), "thruster"},
    // debug string at the left-top corner
    {addr_as_ref<bool>(0x0062FE19), "light"},
    {g_dbg_static_lights, "light2"},
    {addr_as_ref<bool>(0x0062FE1A), "push_climb_reg"},
    {addr_as_ref<bool>(0x0062FE1B), "geo_reg"},
    {addr_as_ref<bool>(0x0062FE1C), "glass"},
    {addr_as_ref<bool>(0x0062FE1D), "mover"},
    {addr_as_ref<bool>(0x0062FE1E), "ignite"},
    {addr_as_ref<bool>(0x0062FE1F), "movemode"},
    {addr_as_ref<bool>(0x0062FE20), "perf"},
    {addr_as_ref<bool>(0x0062FE21), "perfbar"},
    {addr_as_ref<bool>(0x0064E39C), "waypoint"},
    // network meter in left-top corner
    {addr_as_ref<bool>(0x006FED24), "network", false, true},
    {addr_as_ref<bool>(0x007B2758), "particlestats"},
    // debug strings at the left side of the screen
    {addr_as_ref<bool>(0x007CAB59), "weapon"},
    {addr_as_ref<bool>(0x00856500), "event"},
    {addr_as_ref<bool>(0x0085683C), "trigger"},
    {addr_as_ref<bool>(0x009BB5AC), "objrender"},
    {g_dbg_geometry_rendering_stats, "roomstats"},
    // geometry rendering
    {addr_as_ref<bool>(0x009BB594), "trans", true}, // transparent_faces
    {addr_as_ref<bool>(0x009BB598), "room", true, true}, // show_rooms
    {addr_as_ref<bool>(0x009BB59C), "portal", true, true},     // show_portals
    {addr_as_ref<bool>(0x009BB5A4), "lightmap", true, true}, // show_lightmaps
    {addr_as_ref<bool>(0x009BB5A8), "nolightmap", true, true}, // fullbright
    {addr_as_ref<bool>(0x009BB5B0), "show_invisible_faces", true, true},
};

ConsoleCommand2 debug_cmd{
    "debug",
    [](std::string type) {
        for (auto& dbg_flag : g_debug_flags) {
            if (type == dbg_flag.name) {
#ifdef NDEBUG
                if (!dbg_flag.allow_multi && rf::is_multi) {
                    rf::console::printf("This command is disabled in multiplayer!");
                    return;
                }
                const auto& server_info_opt = get_df_server_info();
                if ((!server_info_opt)||(server_info_opt && !(server_info_opt.value().allow_client_render_modes))) {
                    rf::console::printf("Server has not allowed this command!");
                    return;
                }
#endif
                dbg_flag.ref = !dbg_flag.ref;
                rf::console::printf("Debug flag '%s' is %s", dbg_flag.name, dbg_flag.ref ? "enabled" : "disabled");
                if (dbg_flag.clear_geometry_cache) {
                    rf::g_cache_clear();
                }
                return;
            }
        }

        rf::console::printf("Invalid debug flag: %s", type.c_str());
    },
    nullptr,
    "debug [thruster | light | light2 | push_climb_reg | geo_reg | glass | mover | ignite | movemode | perf |\n"
    "perfbar | waypoint | network | particlestats | weapon | event | trigger | objrender | roomstats | trans |\n"
    "room | portal | lightmap | nolightmap | show_invisible_faces]",
};

void debug_cmd_multi_init()
{
    bool clear_geom_cache = false;
    for (auto& dbg_flag : g_debug_flags) {
        if (dbg_flag.ref && dbg_flag.clear_geometry_cache)
            clear_geom_cache = true;
        dbg_flag.ref = false;
    }
    if (clear_geom_cache)
        rf::g_cache_clear();
}

void debug_cmd_render()
{
    const auto dbg_waypoints = addr_as_ref<void()>(0x00468F00);
    const auto dbg_internal_lights = addr_as_ref<void()>(0x004DB830);

    dbg_waypoints();
    if (g_dbg_static_lights)
        dbg_internal_lights();
}

void debug_cmd_render_ui()
{
    const auto dbg_rendering_stats = addr_as_ref<void()>(0x004D36B0);
    const auto dbg_particle_stats = addr_as_ref<void()>(0x004964E0);
    if (g_dbg_geometry_rendering_stats)
        dbg_rendering_stats();
    dbg_particle_stats();
}

void debug_cmd_init()
{
    debug_cmd.register_cmd();
}
