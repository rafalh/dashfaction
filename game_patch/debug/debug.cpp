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
#ifndef NDEBUG
    ProfilerDrawUI();
    RenderObjDebugUI();
#endif
}
