#include <windows.h>
#include <patch_common/FunHook.h>
#include <patch_common/AsmWriter.h>
#include "../rf/os/timer.h"

static LARGE_INTEGER g_qpc_frequency;

FunHook<int(int)> timer_get_hook{
    0x00504AB0,
    [](int scale) {
        // get QPC current value
        LARGE_INTEGER current_qpc_value;
        QueryPerformanceCounter(&current_qpc_value);
        // make sure time never goes backward
        if (current_qpc_value.QuadPart < rf::timer_last_value) {
            current_qpc_value.QuadPart = rf::timer_last_value;
        }
        rf::timer_last_value = current_qpc_value.QuadPart;
        // Make sure we count from game start
        current_qpc_value.QuadPart -= rf::timer_base;
        // Multiply with unit scale (eg. ms/us) before division
        current_qpc_value.QuadPart *= scale;
        // Divide by frequency using 64 bits and then cast to 32 bits
        // Note: sign of result does not matter because it is used only for deltas
        return static_cast<int>(current_qpc_value.QuadPart / g_qpc_frequency.QuadPart);
    },
};

void timer_apply_patch()
{
    // Remove Sleep calls in timer_init
    AsmWriter(0x00504A67, 0x00504A82).nop();

    // Fix timer_get handling of frequency greater than 2MHz (sign bit is set in 32 bit dword)
    QueryPerformanceFrequency(&g_qpc_frequency);
    timer_get_hook.install();
}
