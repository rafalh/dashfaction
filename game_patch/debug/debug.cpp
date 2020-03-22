#include "debug_internal.h"

void DebugApplyPatches()
{
    DebugCmdApplyPatches();
    DebugUnresponsiveApplyPatches();
#ifndef NDEBUG
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
#ifndef NDEBUG
    ProfilerDrawUI();
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
