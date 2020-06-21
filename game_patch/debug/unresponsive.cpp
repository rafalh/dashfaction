#include <patch_common/FunHook.h>
#include "debug_internal.h"
#include "../console/console.h"
#include "crash_handler_stub/WatchDogTimer.h"

WatchDogTimer g_watch_dog_timer;

FunHook<void()> gr_flip_hook{
    0x0050CE20,
    []() {
        // Resetting detector is needed here to handle Bink videos properly
        g_watch_dog_timer.Restart();
        gr_flip_hook.CallTarget();
    },
};

#ifndef NDEBUG
DcCommand2 hang_cmd{
    "d_hang",
    []() {
        int start = rf::TimerGet(1000);
        while (rf::TimerGet(1000) - start < 6000) {}
    },
};
#endif

void DebugUnresponsiveApplyPatches()
{
    gr_flip_hook.Install();
}

void DebugUnresponsiveInit()
{
    g_watch_dog_timer.Start();
#ifndef NDEBUG
    hang_cmd.Register();
#endif
}

void DebugUnresponsiveCleanup()
{
    g_watch_dog_timer.Stop();
}

void DebugUnresponsiveDoUpdate()
{
    g_watch_dog_timer.Restart();
}
