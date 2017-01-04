#include "stdafx.h"
#include "config.h"
#include "exports.h"
#include "version.h"
#include "sharedoptions.h"
#include "crashdump.h"
#include "autodl.h"
#include "utils.h"
#include "rf.h"
#include "scoreboard.h"
#include "hud.h"
#include "packfile.h"
#include "lazyban.h"
#include "gamma.h"
#include "kill.h"
#include "screenshot.h"
#include "commands.h"
#include "spectate.h"
#include "wndproc.h"
#include "graphics.h"
#include "network.h"

SHARED_OPTIONS g_Options;

static void ProcessWaitingMessages()
{
    MSG msg;
    while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}

static void DrawConsoleAndProcessKbdFifoHook(BOOL bServer)
{
    RfDrawConsoleAndProcessKbdFifo(bServer);
    
    GraphicsDrawFpsCounter();
    
#ifdef LEVELS_AUTODOWNLOADER
    RenderDownloadProgress();
#endif
#if SPECTATE_ENABLE
    DrawSpectateModeUI();
#endif
}

static void InitGameHook(void)
{
    RfInitGame();

    GraphicsAfterGameInit();

#if DIRECTINPUT_SUPPORT
    *g_pbDirectInputDisabled = !DIRECTINPUT_ENABLED;
#endif

    /* Allow modded strings.tbl in ui.vpp */
    ForceFileFromPackfile("strings.tbl", "ui.vpp");
    
    CommandsAfterGameInit();
}

static void CleanupGameHook(void)
{
    ResetGammaRamp();
    CleanupScreenshot();
    RfCleanupGame();
}

CPlayer *FindPlayer(const char *pszName)
{
    CPlayer *pPlayer = *g_ppPlayersList;
    while(pPlayer)
    {
        if(stristr(pPlayer->strName.psz, pszName))
            return pPlayer;
        pPlayer = pPlayer->pNext;
        if(pPlayer == *g_ppPlayersList)
            break;
    }
    return NULL;
}

static void GrSwitchBuffersHook(void)
{
    // We disabled msg loop thread so we have to process them somewhere
    ProcessWaitingMessages();
}

void  __declspec(naked) CrashFix0055CE59()
{
    _asm
    {
        shl   edi, 5
        lea   edx, [esp + 0x38 - 0x28]
        mov   eax, [eax + edi]
        test  eax, eax
        jz    CrashFix0055CE59_label1
        //jmp CrashFix0055CE59_label1
        push  0
        push  0
        push  edx
        push  0
        push  eax
        mov   ecx, 0x0055CE59
        jmp   ecx
    CrashFix0055CE59_label1:
        mov   ecx, 0x0055CF23
        jmp   ecx
    }
}

extern "C" DWORD DLL_EXPORT Init(SHARED_OPTIONS *pOptions)
{
    g_Options = *pOptions;

    /* Init crash dump writer before anything else */
    InitCrashDumps();
    
    if (!SetProcessDEPPolicy(PROCESS_DEP_ENABLE))
        WARN("SetProcessDEPPolicy failed (error %ld)", GetLastError());

    /* Process messages in the same thread as DX processing (alternative: D3DCREATE_MULTITHREADED) */
    WriteMemUInt8Repeat((PVOID)0x00524C48, ASM_NOP, 0x00524C83 - 0x00524C48); // disable msg loop thread
    WriteMemUInt8((PVOID)0x00524C48, ASM_LONG_CALL_REL);
    WriteMemPtr((PVOID)0x00524C49, (PVOID)((ULONG_PTR)0x00524E40 - (0x00524C48 + 0x5))); // CreateMainWindow
    WriteMemPtr((PVOID)0x0050CE21, (PVOID)((ULONG_PTR)GrSwitchBuffersHook - (0x0050CE20 + 0x5)));

    /* Console init string */
    WriteMemPtr((PVOID)0x004B2534, "-- " PRODUCT_NAME " Initializing --\n");

    /* Version in Main Menu */
    WriteMemUInt32((PVOID)0x005A18A8, VER_MAJOR);
    WriteMemUInt32((PVOID)0x005A18AC, VER_MINOR);
    WriteMemPtr((PVOID)0x004B33D1, PRODUCT_NAME " %u.%02u");
    WriteMemUInt32((PVOID)0x00443444, 300); // X coord
    WriteMemUInt8((PVOID)0x0044343D, 127); // width (127 is max)
    
    /* Window title (client and server) */
    WriteMemPtr((PVOID)0x004B2790, PRODUCT_NAME);
    WriteMemPtr((PVOID)0x004B27A4, PRODUCT_NAME);
    
    /* Console background color */
    WriteMemUInt32((PVOID)0x005098D1, CONSOLE_BG_A); // Alpha
    WriteMemUInt8((PVOID)0x005098D6, CONSOLE_BG_B); // Blue
    WriteMemUInt8((PVOID)0x005098D8, CONSOLE_BG_G); // Green
    WriteMemUInt8((PVOID)0x005098DA, CONSOLE_BG_R); // Red

#ifdef NO_CD_FIX
    /* No-CD fix */
    WriteMemUInt8((PVOID)0x004B31B6, ASM_SHORT_JMP_REL);
#endif /* NO_CD_FIX */

#ifdef NO_INTRO
    /* Disable thqlogo.bik */
    WriteMemUInt16((PVOID)0x004B2091, MAKEWORD(ASM_NOP, ASM_NOP));
    WriteMemUInt16((PVOID)0x004B2093, MAKEWORD(ASM_NOP, ASM_NOP));
    WriteMemUInt8((PVOID)0x004B2095, ASM_NOP);
#endif /* NO_INTRO */
    
    /* Sound loop fix */
    WriteMemUInt8((PVOID)0x00505D08, 0x00505D5B - (0x00505D07 + 0x2));
    
    /* DrawConsoleAndProcssKbdFifo hook */
    WriteMemPtr((PVOID)0x004B2DD4, (PVOID)((ULONG_PTR)DrawConsoleAndProcessKbdFifoHook - (0x004B2DD3 + 0x5)));

    /* CleanupGame and InitGame hooks */
    WriteMemPtr((PVOID)0x004B27CE, (PVOID)((ULONG_PTR)InitGameHook - (0x004B27CD + 0x5)));
    WriteMemPtr((PVOID)0x004B2822, (PVOID)((ULONG_PTR)CleanupGameHook - (0x004B2821 + 0x5)));

    /* Set FPS limit to 60 */
    WriteMemFloat((PVOID)0x005094CA, 1.0f / 60.0f);
    
    /* Crash-fix... (probably argument for function is invalid); Page Heap is needed */
    WriteMemUInt32((PVOID)(0x0056A28C + 1), 0);

    /* Crash-fix for pdm04 (FIXME: monitors in game doesnt work) */
    WriteMemUInt8((PVOID)0x0055CE47, ASM_LONG_JMP_REL);
    WriteMemPtr((PVOID)0x0055CE48, (PVOID)((ULONG_PTR)CrashFix0055CE59 - (0x0055CE48 + 0x4)));
    WriteMemUInt8((PVOID)0x00412370, ASM_RET); // disable function calling GrLock without checking for success (no idea what it does)
    
    /* Switch UI language */
    WriteMemUInt8((PVOID)(0x004B27D2 + 1), (uint8_t)GetInstalledGameLang());

#if DIRECTINPUT_SUPPORT
    *g_pbDirectInputDisabled = 0;
#endif

    /* Init modules */
    GraphicsInit();
    NetworkInit();
    InitGamma();
    InitWndProc();
    InitScreenshot();
    InitHud();
#ifdef LEVELS_AUTODOWNLOADER
    InitAutodownloader();
#endif
    InitScoreboard();
    InitLazyban();
    InitKill();
    VfsApplyHooks(); /* Use new VFS without file count limit */
    InitSpectateMode();
    CommandsInit();

    return 1; /* success */
}

BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD fdwReason, LPVOID lpvReserved)
{
    DisableThreadLibraryCalls(hInstance);
    return TRUE;
}
