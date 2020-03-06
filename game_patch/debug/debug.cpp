#include "debug_internal.h"

void DebugApplyPatches()
{
    DebugCmdApplyPatches();
#ifndef NDEBUG
    ProfilerInit();
#endif
}

void DebugInit()
{
    DebugCmdInit();
}

void DebugRender()
{
    DebugCmdRender();
}

void DebugRenderUI()
{
    DebugCmdRenderUI();
#ifndef NDEBUG
    ProfilerDrawUI();
#endif
}
