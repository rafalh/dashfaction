#include "config.h"
#include "exports.h"
#include "version.h"
#include "crashdump.h"
#include "autodl.h"
#include "utils.h"
#include "rf.h"
#include "pf.h"
#include "scoreboard.h"
#include "hud.h"
#include "packfile.h"
#include "lazyban.h"
#include "gamma.h"
#include "camera.h"
#include "kill.h"
#include "screenshot.h"
#include "commands.h"
#include "wndproc.h"
#include <stdio.h>
#include <stdint.h>
#include <math.h>

#ifndef M_PI
# define M_PI 3.141592f
#endif

static void ProcessUnreliableGamePacketsHook(const char *pData, int cbData, void *pAddr, void *pPlayer)
{
    RfProcessGamePackets(pData, cbData, pAddr, pPlayer);
    
#ifdef MASK_AS_PF
    ProcessPfPacket(pData, cbData, pAddr, pPlayer);
#endif
}

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
    char szBuf[64];
    
    RfDrawConsoleAndProcessKbdFifo(bServer);
    
    sprintf(szBuf, "FPS: %.1f", *g_fFps);
    GrSetColorRgb(0, 255, 0, 255);
    GrDrawAlignedText(2, RfGetWidth() - 10, 60, szBuf, -1, *((uint32_t*)0x17C7C5C));
    
#ifdef LEVELS_AUTODOWNLOADER
    RenderDownloadProgress();
#endif
}

static void InitGameHook(void)
{
    RfInitGame();
    
    /* Allow modded strings.tbl in ui.vpp */
    ForceFileFromPackfile("strings.tbl", "ui.vpp");
    
    RegisterCommands();
    
    /*int *ptr = HeapAlloc(GetProcessHeap(), 0, 8);
    ptr[2] = 1;
    
    HeapFree(GetProcessHeap(), 0, 0x5324325);*/
}

static void CleanupGameHook(void)
{
    ResetGammaRamp();
    CleanupScreenshot();
    RfCleanupGame();
}

static void HandleNewPlayerPacketHook(BYTE *pData, NET_ADDR *pAddr)
{
    if(*g_pbNetworkGame && !*g_pbLocalNetworkGame && GetForegroundWindow() != *g_phWnd)
        Beep(750, 300);
    RfHandleNewPlayerPacket(pData, pAddr);
}
#if 0
static void RenderEntityHook(CEntity *pEntity)
{ // TODO
    DWORD dwOldLightining;
    GrFlushBuffers();
    IDirect3DDevice8_GetRenderState(*g_ppGrDevice, D3DRS_LIGHTING, &dwOldLightining);
    IDirect3DDevice8_SetRenderState(*g_ppGrDevice, D3DRS_LIGHTING, 1);
    IDirect3DDevice8_SetRenderState(*g_ppGrDevice, D3DRS_AMBIENT, 0xFFFFFFFF);
    
    RfRenderEntity((CObject*)pEntity);
    GrFlushBuffers();
    //IDirect3DDevice8_SetRenderState(*g_ppGrDevice, D3DRS_LIGHTING, dwOldLightining);
}
#endif
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

DWORD DLL_EXPORT Init(PVOID pParam)
{
    DWORD dwRfThreadId = (DWORD)pParam;
    HANDLE hRfThread;
    
    /* Init crash dump writer before anything else */
    InitCrashDumps();
    
    /* Fix for "At least 8 MB of available video memory" */
    WriteMemUInt8((PVOID)0x005460CD, ASM_SHORT_JMP_REL);
    
#ifdef NO_CD_FIX
    /* No-CD fix */
    WriteMemUInt8((PVOID)0x004B31B6, ASM_SHORT_JMP_REL);
#endif /* NO_CD_FIX */
    
    /* Enable windowed mode */
    //WriteMemUInt8Repeat((PVOID)0x00545ABF, ASM_NOP, 2); // force PRESENT_PARAMETERS::Windowed to TRUE
    //WriteMemUInt32((PVOID)(0x004B29A5 + 6), 0xC8);
    //WriteMemUInt32((PVOID)(0x0050C4E3 + 1), WS_POPUP|WS_SYSMENU);
	//WriteMemUInt8((PVOID)0x00524C98, ASM_SHORT_JMP_REL); // disable window hooks
	//WriteMemUInt32((PVOID)(0x00545B4D + 6), 3); // D3DSWAPEFFECT_DISCARD
	//WriteMemUInt8Repeat((PVOID)(0x00545BA5), ASM_NOP, 6); // PP.Flags = 0

	/* Process messages in the same thread as DX processing */
	WriteMemUInt8Repeat((PVOID)0x00524C48, ASM_NOP, 0x00524C83 - 0x00524C48); // disable msg loop thread
	WriteMemUInt8((PVOID)0x00524C48, ASM_LONG_CALL_REL);
	WriteMemPtr((PVOID)0x00524C49, (PVOID)((ULONG_PTR)0x00524E40 - (0x00524C48 + 0x5))); // CreateMainWindow
	WriteMemPtr((PVOID)0x0050CE21, (PVOID)((ULONG_PTR)GrSwitchBuffersHook - (0x0050CE20 + 0x5)));
	

    //WriteMemUInt8Repeat((PVOID)0x00545017, 0xFF, 4);
    

    //WriteMemUInt8Repeat((PVOID)0x0050CE31, ASM_NOP, 2);
    //WriteMemUInt8Repeat((PVOID)0x00545017, 0xFF, 5);
    
    /* No present */
    //WriteMemUInt8Repeat((PVOID)0x00544FFB, ASM_NOP, 7);
    //WriteMemUInt8Repeat((PVOID)0x00545002, ASM_NOP, 3);
    
    /* Console init string */
    WriteMemPtr((PVOID)0x004B2534, "-- " PRODUCT_NAME " Initializing --\n");
    
#ifdef NO_INTRO
    /* Disable thqlogo.bik */
    WriteMemUInt16((PVOID)0x004B2091, MAKEWORD(ASM_NOP, ASM_NOP));
    WriteMemUInt16((PVOID)0x004B2093, MAKEWORD(ASM_NOP, ASM_NOP));
    WriteMemUInt8((PVOID)0x004B2095, ASM_NOP);
#endif /* NO_INTRO */
    
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
    
    /* Use hardware vertex processing */
    //WriteMemUInt8((PVOID)0x00545BDF, D3DCREATE_HARDWARE_VERTEXPROCESSING);
    
    //WriteMemUInt8((PVOID)0x005450A0, ASM_MOV_EAX32);
    //WriteMemUInt32((PVOID)0x005450A1, 0 /*D3DPOOL_MANAGED*/); // FIXME: D3DPOOL_MANAGED makes game much slower
    //WriteMemUInt8Repeat((PVOID)0x005450A6, ASM_NOP, 27);
    //WriteMemUInt8Repeat((PVOID)0x005450C2, ASM_NOP, 4);
    //WriteMemUInt8Repeat((PVOID)0x005450CB, ASM_NOP, 3);
    
    //WriteMemUInt32((PVOID)0x005450DE, /*D3DUSAGE_DYNAMIC|*/D3DUSAGE_DONOTCLIP|D3DUSAGE_WRITEONLY);
    //WriteMemUInt32((PVOID)0x00545118, /*D3DUSAGE_DYNAMIC|*/D3DUSAGE_DONOTCLIP|D3DUSAGE_WRITEONLY);
    
    //WriteMemUInt8Repeat((PVOID)0x005450CB, ASM_NOP, 3);
    
    /* Sound loop fix */
    WriteMemUInt8((PVOID)0x00505D08, 0x00505D5B - (0x00505D07 + 0x2));
    
    /* nVidia issue fix (make sure D3Dsc is enabled) */
    WriteMemUInt8((PVOID)0x00546154, 1);
    
    /* Load ui.vpp before tables.vpp */
    WriteMemPtr((PVOID)0x0052BC58, "ui.vpp");
    WriteMemPtr((PVOID)0x0052BC67, "tables.vpp");
    
    /* DrawConsoleAndProcssKbdFifo hook */
    WriteMemPtr((PVOID)0x004B2DD4, (PVOID)((ULONG_PTR)DrawConsoleAndProcessKbdFifoHook - (0x004B2DD3 + 0x5)));
    
    /* ProcessGamePackets hook (not reliable only) */
    WriteMemPtr((PVOID)0x00479245, (PVOID)((ULONG_PTR)ProcessUnreliableGamePacketsHook - (0x00479244 + 0x5)));
    
    /* CleanupGame and InitGame hooks */
    WriteMemPtr((PVOID)0x004B27CE, (PVOID)((ULONG_PTR)InitGameHook - (0x004B27CD + 0x5)));
    WriteMemPtr((PVOID)0x004B2822, (PVOID)((ULONG_PTR)CleanupGameHook - (0x004B2821 + 0x5)));
    
    /* Don't use LOD models */
    WriteMemUInt8((PVOID)0x0052FACC, ASM_SHORT_JMP_REL);
    
    /* Improve SimultaneousPing */
    *g_pSimultaneousPing = 32;
    
    /* Set FPS limit to 60 */
    WriteMemFloat((PVOID)0x005094CA, 1.0f / 60.0f);
    
    /* Improve FOV for wide screens */
    float w = (float)GetSystemMetrics(SM_CXSCREEN);
	float h = (float)GetSystemMetrics(SM_CYSCREEN);
    float fFov = 2 * atanf((w/h)*tanf(45.0f / 180.0f * M_PI) / 1.33f) / M_PI * 180.0f;
    WriteMemFloat((PVOID)0x004A6B4B, fFov);
    TRACE("FOV: %f", fFov);
    
    /* If server forces player character, don't save it in settings */
    WriteMemUInt8Repeat((PVOID)0x004755C1, ASM_NOP, 6);
    WriteMemUInt8Repeat((PVOID)0x004755C7, ASM_NOP, 6);
    
    /* Show valid info for servers with incompatible version */
    WriteMemUInt8((PVOID)0x0047B3CB, ASM_SHORT_JMP_REL);
    
    /* Beep when new player joins */
    WriteMemPtr((PVOID)0x0059E158, HandleNewPlayerPacketHook);
    
    /* Crash-fix... (probably argument for function is invalid); Page Heap is needed */
    WriteMemUInt32((PVOID)(0x0056A28C + 1), 0);
    
    //WriteMemUInt32((PVOID)0x00488BE9, ((ULONG_PTR)RenderEntityHook) - (0x00488BE8 + 0x5));
    
    /* Don't use Direct Input */
    //WriteMemUInt8Repeat((PVOID)0x0051E02F, 0x90, 5 + 3 + 2);
    
	/* Allow ports < 1023 (especially 0 - any port) */
	WriteMemUInt8Repeat((PVOID)0x00528F24, 0x90, 2);
	/* Default port: 0 */
	WriteMemUInt16((PVOID)0x0059CDE4, 0);
	
	

    /* Init modules */
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
    InitCamera();
    
    /* Resume RF thread */
    hRfThread = OpenThread(THREAD_SUSPEND_RESUME, FALSE, dwRfThreadId);
    if (!hRfThread)
    {
        MessageBox(0, "OpenThread failed!", 0, 0);
        return 0; /* fail */
    }
    ResumeThread(hRfThread);
    CloseHandle(hRfThread);
    
    return 1; /* success */
}

BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD fdwReason, LPVOID lpvReserved)
{
    DisableThreadLibraryCalls(hInstance);
    return TRUE;
}
