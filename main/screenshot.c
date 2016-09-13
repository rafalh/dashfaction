#include "screenshot.h"
#include "utils.h"
#include <windows.h>

static BYTE **g_ScreenshotScanlinesBuf = NULL;

void InitScreenshot(void)
{
    /* Fix for screenshots creation when height > 1024 */
    if(GetSystemMetrics(SM_CYSCREEN) > 1024)
    {
        g_ScreenshotScanlinesBuf = malloc(GetSystemMetrics(SM_CYSCREEN) * sizeof(BYTE*));
        WriteMemUInt8((PVOID)0x0055A066, ASM_MOV_ECX);
        WriteMemUInt32((PVOID)0x0055A067, (uint32_t)&g_ScreenshotScanlinesBuf[0]);
        WriteMemUInt8Repeat((PVOID)0x0055A06B, ASM_NOP, 2);
        WriteMemUInt8((PVOID)0x0055A0DF, ASM_MOV_EAX);
        WriteMemUInt32((PVOID)0x0055A0E0, (uint32_t)g_ScreenshotScanlinesBuf);
        WriteMemUInt8Repeat((PVOID)0x0055A0E4, ASM_NOP, 2);
    }
}

void CleanupScreenshot(void)
{
    free(g_ScreenshotScanlinesBuf);
}
