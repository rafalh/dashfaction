#include "debug_internal.h"

#ifndef NDEBUG
#define DEBUG_PERF
#endif

void DebugApplyPatches()
{
    DebugCmdApplyPatches();
    DebugUnresponsiveApplyPatches();
#ifdef DEBUG_PERF
    ProfilerInit();
#endif
}

void DebugInit()
{
    DebugCmdInit();
    DebugUnresponsiveInit();
#ifndef NDEBUG
    RegisterObjDebugCommands();
#endif
}

void DebugRender()
{
    DebugCmdRender();
}

void DebugRenderUI()
{
    DebugCmdRenderUI();
#ifdef DEBUG_PERF
    ProfilerDrawUI();
#endif
#ifndef NDEBUG
    RenderObjDebugUI();
#endif
}

void DebugCleanup()
{
    DebugUnresponsiveCleanup();
}

void DebugDoUpdate()
{
    DebugUnresponsiveDoUpdate();
}
