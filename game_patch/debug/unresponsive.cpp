#include <patch_common/FunHook.h>
#include "debug_internal.h"
#include "../os/console.h"
#include "../rf/os/timer.h"
#include "crash_handler_stub/WatchDogTimer.h"

WatchDogTimer g_watch_dog_timer;

FunHook<void()> gr_flip_hook{
    0x0050CE20,
    []() {
        // Resetting detector is needed here to handle Bink videos properly
        g_watch_dog_timer.restart();
        gr_flip_hook.call_target();
    },
};

#ifndef NDEBUG
ConsoleCommand2 hang_cmd{
    "d_hang",
    []() {
        int start = rf::timer_get(1000);
        while (rf::timer_get(1000) - start < 6000) {
        }
    },
};
#endif

void debug_unresponsive_apply_patches()
{
    gr_flip_hook.install();
}

void debug_unresponsive_init()
{
    g_watch_dog_timer.start();
#ifndef NDEBUG
    hang_cmd.register_cmd();
#endif
}

void debug_unresponsive_cleanup()
{
    g_watch_dog_timer.stop();
}

void debug_unresponsive_do_update()
{
    g_watch_dog_timer.restart();
}
