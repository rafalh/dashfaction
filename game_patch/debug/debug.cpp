#include "debug_internal.h"
#include <patch_common/FunHook.h>
#include <xlog/xlog.h>

#ifndef NDEBUG
#define DEBUG_PERF
#endif

FunHook<int(size_t)> callnewh_hook{
    0x0057A212,
    [](size_t size) {
        xlog::error("Failed to allocate %u bytes", size);
        return callnewh_hook.CallTarget(size);
    },
};

void DebugApplyPatches()
{
    // Log error when memory allocation fails
    callnewh_hook.Install();

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
