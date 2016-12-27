#include "config.h"
#include "exports.h"
#include "version.h"
#include "sharedoptions.h"
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

SHARED_OPTIONS g_Options;

static void ProcessUnreliableGamePacketsHook(const char *pData, int cbData, void *pAddr, void *pPlayer)
{
    RfProcessGamePackets(pData, cbData, pAddr, pPlayer);
    
#if MASK_AS_PF
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

static void SetTextureMinMagFilter(D3DTEXTUREFILTERTYPE FilterType)
{
	uintptr_t Addresses[] = {
		// InitD3D
		0x00546283, 0x00546295,
		0x005462A9, 0x005462BD,
		// GrSetMaterial
		0x0054F2E8, 0x0054F2FC,
		0x0054F438, 0x0054F44C,
		0x0054F567, 0x0054F57B,
		0x0054F62D, 0x0054F641,
		0x0054F709, 0x0054F71D,
		0x0054F800, 0x0054F814,
		0x0054F909, 0x0054F91D,
		0x0054FA1A, 0x0054FA2E,
		0x0054FB22, 0x0054FB36,
		0x0054FBFE, 0x0054FC12,
		0x0054FD06, 0x0054FD1A,
		0x0054FD90, 0x0054FDA4,
		0x0054FE98, 0x0054FEAC,
		0x0054FF74, 0x0054FF88,
		0x0055007C, 0x00550090,
		0x005500C6, 0x005500DA,
		0x005501A2, 0x005501B6,
	};
	unsigned i;
	for (i = 0; i < sizeof(Addresses) / sizeof(Addresses[0]); ++i)
		WriteMemUInt8((PVOID)(Addresses[i] + 1), (uint8_t)FilterType);
}

static void InitGameHook(void)
{
    RfInitGame();
    
#if 1
	/* Fix aspect ratio for widescreen */
	*((float*)0x017C7BD8) = 1.0f;

	/* Fix FOV for widescreen */
	float w = (float)*g_pRfWndWidth;
	float h = (float)*g_pRfWndHeight;
	float fFov = 90.0f * (w / h) / (4.0f / 3.0f); // default is 90
	WriteMemFloat((PVOID)(0x004A34C7 + 6), fFov);
	INFO("FOV: %f", fFov);
#endif

#if ANISOTROPIC_FILTERING
	/* Anisotropic texture filtering */
	if (g_pRfGrDeviceCaps->MaxAnisotropy > 0)
	{
		DWORD AnisotropyLevel = min(g_pRfGrDeviceCaps->MaxAnisotropy, 16);
		SetTextureMinMagFilter(D3DTEXF_ANISOTROPIC);
		IDirect3DDevice8_SetTextureStageState(*g_ppGrDevice, 0, D3DTSS_MAXANISOTROPY, AnisotropyLevel);
		IDirect3DDevice8_SetTextureStageState(*g_ppGrDevice, 1, D3DTSS_MAXANISOTROPY, AnisotropyLevel);
		INFO("Anisotropic Filtering enabled (level: %d)", AnisotropyLevel);
	}
#endif

    /* Allow modded strings.tbl in ui.vpp */
    ForceFileFromPackfile("strings.tbl", "ui.vpp");
    
    RegisterCommands();
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

static void SetupPP(void)
{
	D3DPRESENT_PARAMETERS *pPP = (D3DPRESENT_PARAMETERS*)0x01CFCA18;
	memset(pPP, 0, sizeof(*pPP));

	D3DFORMAT *pFormat = (D3DFORMAT*)0x005A135C;
	INFO("D3D Format: %ld", *pFormat);

	pPP->Flags = D3DPRESENTFLAG_LOCKABLE_BACKBUFFER;

	/*int i, Format;
	for (Format = 0; Format < 120; ++Format)
	{
		for (i = 2; i < 8; ++i)
			if (SUCCEEDED(IDirect3D8_CheckDeviceMultiSampleType(*g_ppDirect3D, *g_pAdapterIdx, D3DDEVTYPE_HAL, Format, FALSE, i)))
				WARN("AA (format %d %d samples) supported", Format, i);
	}*/
	
#if MULTISAMPLING_SUPPORT
	if (g_Options.bMultiSampling && *pFormat > 0)
	{
		HRESULT hr = IDirect3D8_CheckDeviceMultiSampleType(*g_ppDirect3D, *g_pAdapterIdx, D3DDEVTYPE_HAL, *pFormat, FALSE, D3DMULTISAMPLE_4_SAMPLES);
		if (SUCCEEDED(hr))
		{
			INFO("Enabling Anti-Aliasing (4x MSAA)...");
			pPP->MultiSampleType = D3DMULTISAMPLE_4_SAMPLES;
		}
		else
		{
			WARN("MSAA not supported (0x%x)...", hr);
			g_Options.bMultiSampling = FALSE;
		}
	}
#endif
}


extern "C" DWORD DLL_EXPORT Init(SHARED_OPTIONS *pOptions)
{
    DWORD dwRfThreadId = pOptions->dwRfThreadId;
    HANDLE hRfThread;
    
	g_Options = *pOptions;

    /* Init crash dump writer before anything else */
    InitCrashDumps();
    
    if (!SetProcessDEPPolicy(PROCESS_DEP_ENABLE))
        WARN("SetProcessDEPPolicy failed (error %ld)", GetLastError());

    /* Fix for "At least 8 MB of available video memory" */
    WriteMemUInt8((PVOID)0x005460CD, ASM_SHORT_JMP_REL);
    
#ifdef NO_CD_FIX
    /* No-CD fix */
    WriteMemUInt8((PVOID)0x004B31B6, ASM_SHORT_JMP_REL);
#endif /* NO_CD_FIX */
    
#if USE_WINDOWED_MODE
	/* Enable windowed mode */
    //WriteMemUInt32((PVOID)(0x004B29A5 + 6), 0xC8);
    //WriteMemUInt32((PVOID)(0x0050C4E3 + 1), WS_POPUP|WS_SYSMENU);
#endif

	//WriteMemUInt8((PVOID)0x00524C98, ASM_SHORT_JMP_REL); // disable window hooks

#if MULTISAMPLING_SUPPORT
	if (pOptions->bMultiSampling)
		WriteMemUInt32((PVOID)(0x00545B4D + 6), D3DSWAPEFFECT_DISCARD);
#endif

	WriteMemUInt8Repeat((PVOID)0x00545AA7, ASM_NOP, 0x00545AB5 - 0x00545AA7);
	WriteMemUInt8((PVOID)0x00545AA7, ASM_LONG_CALL_REL);
	WriteMemPtr((PVOID)0x00545AA8, (PVOID)((ULONG_PTR)SetupPP - (0x00545AA7 + 0x5)));
	WriteMemUInt8Repeat((PVOID)(0x00545BA5), ASM_NOP, 6); // dont set PP.Flags

	/* Process messages in the same thread as DX processing (alternative: D3DCREATE_MULTITHREADED) */
	WriteMemUInt8Repeat((PVOID)0x00524C48, ASM_NOP, 0x00524C83 - 0x00524C48); // disable msg loop thread
	WriteMemUInt8((PVOID)0x00524C48, ASM_LONG_CALL_REL);
	WriteMemPtr((PVOID)0x00524C49, (PVOID)((ULONG_PTR)0x00524E40 - (0x00524C48 + 0x5))); // CreateMainWindow
	WriteMemPtr((PVOID)0x0050CE21, (PVOID)((ULONG_PTR)GrSwitchBuffersHook - (0x0050CE20 + 0x5)));

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
