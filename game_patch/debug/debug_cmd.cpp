#include "debug_internal.h"
#include "../rf/network.h"
#include "../console/console.h"
#include <patch_common/MemUtils.h>
#include <patch_common/CodeInjection.h>

struct DebugFlagDesc
{
    bool& ref;
    const char* name;
    bool clear_geometry_cache = false;
};

bool g_dbg_geometry_rendering_stats = false;
bool g_dbg_static_lights = false;

DebugFlagDesc g_debug_flags[] = {
    {AddrAsRef<bool>(0x0062F3AA), "thruster"},
    // debug string at the left-top corner
    {AddrAsRef<bool>(0x0062FE19), "light"},
    {g_dbg_static_lights, "light2"},
    {AddrAsRef<bool>(0x0062FE1A), "push_climb_reg"},
    {AddrAsRef<bool>(0x0062FE1B), "geo_reg"},
    {AddrAsRef<bool>(0x0062FE1C), "glass"},
    {AddrAsRef<bool>(0x0062FE1D), "mover"},
    {AddrAsRef<bool>(0x0062FE1E), "ignite"},
    {AddrAsRef<bool>(0x0062FE1F), "movemode"},
    {AddrAsRef<bool>(0x0062FE20), "perf"},
    {AddrAsRef<bool>(0x0062FE21), "perfbar"},
    {AddrAsRef<bool>(0x0064E39C), "waypoint"},
    // network meter in left-top corner
    {AddrAsRef<bool>(0x006FED24), "network"},
    {AddrAsRef<bool>(0x007B2758), "particlestats"},
    // debug strings at the left side of the screen
    {AddrAsRef<bool>(0x007CAB59), "weapon"},
    {AddrAsRef<bool>(0x00856500), "event"},
    {AddrAsRef<bool>(0x0085683C), "trigger"},
    {AddrAsRef<bool>(0x009BB5AC), "objrender"},
    {g_dbg_geometry_rendering_stats, "roomstats"},
    // geometry rendering
    {AddrAsRef<bool>(0x009BB594), "trans", true}, // transparent_faces
    {AddrAsRef<bool>(0x009BB598), "room", true}, // show_rooms
    {AddrAsRef<bool>(0x009BB59C), "portal", true}, // show_portals
    {AddrAsRef<bool>(0x009BB5A4), "lightmap", true}, // show_lightmaps
    {AddrAsRef<bool>(0x009BB5A8), "nolightmap", true}, // fullbright
    {AddrAsRef<bool>(0x009BB5B0), "show_invisible_faces", true},
};

DcCommand2 debug_cmd{
    "debug",
    [](std::string type) {

#ifdef NDEBUG
        if (rf::is_multi) {
            rf::DcPrintf("This command is disabled in multiplayer!");
            return;
        }
#endif

        for (auto& dbg_flag : g_debug_flags) {
            if (type == dbg_flag.name) {
                dbg_flag.ref = !dbg_flag.ref;
                rf::DcPrintf("Debug flag '%s' is %s", dbg_flag.name, dbg_flag.ref ? "enabled" : "disabled");
                if (dbg_flag.clear_geometry_cache) {
                    rf::GeomClearCache();
                }
                return;
            }
        }

        rf::DcPrintf("Invalid debug flag: %s", type.c_str());
    },
    nullptr,
    "debug [thruster | light | light2 | push_climb_reg | geo_reg | glass | mover | ignite | movemode | perf |\n"
    "perfbar | waypoint | network | particlestats | weapon | event | trigger | objrender | roomstats | trans |\n"
    "room | portal | lightmap | nolightmap | show_invisible_faces]",
};

void DisableAllDebugFlags()
{
    bool clear_geom_cache = false;
    for (auto& dbg_flag : g_debug_flags) {
        if (dbg_flag.ref && dbg_flag.clear_geometry_cache)
            clear_geom_cache = true;
        dbg_flag.ref = false;
    }
    if (clear_geom_cache)
        rf::GeomClearCache();
}

CodeInjection MpInit_disable_debug_flags_patch{
    0x0046D5B0,
    []() {
        DisableAllDebugFlags();
    },
};

void DebugCmdRender()
{
    const auto dbg_waypoints = AddrAsRef<void()>(0x00468F00);
    const auto dbg_internal_lights = AddrAsRef<void()>(0x004DB830);

    dbg_waypoints();
    if (g_dbg_static_lights)
        dbg_internal_lights();
}

void DebugCmdRenderUI()
{
    const auto dbg_rendering_stats = AddrAsRef<void()>(0x004D36B0);
    const auto dbg_particle_stats = AddrAsRef<void()>(0x004964E0);
    if (g_dbg_geometry_rendering_stats)
        dbg_rendering_stats();
    dbg_particle_stats();
}

void DebugCmdApplyPatches()
{
    // Reset debug flags when entering multiplayer
    MpInit_disable_debug_flags_patch.Install();
}

void DebugCmdInit()
{
    debug_cmd.Register();
}
