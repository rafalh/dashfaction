#include "stdafx.h"
#include "graphics.h"
#include "utils.h"
#include "rf.h"
#include "config.h"
#include "main.h"

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

void GrSetViewMatrixLastCallHook()
{
    constexpr auto GrUnkAfterWFarUpdateInternal = (void(*)())0x00546A40;
    float w = (float)*g_pRfWndWidth;
    float h = (float)*g_pRfWndHeight;
    float fFovMod = (4.0f / 3.0f) / (w / h);
    g_pGrScaleVec->x *= fFovMod;
    g_GrViewMatrix->rows[0].x *= fFovMod;
    g_GrViewMatrix->rows[0].y *= fFovMod;
    g_GrViewMatrix->rows[0].z *= fFovMod;
    GrUnkAfterWFarUpdateInternal();
}

void DisplayD3DDeviceError(HRESULT hr)
{
    ERR("D3D CreateDevice failed (hr 0x%X)", hr);

    char buf[1024];
    sprintf(buf, "Failed to create Direct3D device object - error 0x%X.\n"
        "A critical error has occurred and the program cannot continue.\n"
        "Press OK to exit the program", hr);
    MessageBoxA(nullptr, buf, "Error!", MB_OK | MB_ICONERROR);
    ExitProcess(-1);
}

void  __declspec(naked) GrCreateD3DDeviceError_00545BEF()
{
    _asm
    {
        push eax
        call DisplayD3DDeviceError
        add esp, 4
    }
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
    if (g_gameConfig.msaa && *pFormat > 0)
    {
        HRESULT hr = IDirect3D8_CheckDeviceMultiSampleType(*g_ppDirect3D, *g_pAdapterIdx, D3DDEVTYPE_HAL, *pFormat, FALSE, D3DMULTISAMPLE_4_SAMPLES);
        if (SUCCEEDED(hr))
        {
            INFO("Enabling Anti-Aliasing (%ux MSAA)...", g_gameConfig.msaa);
            pPP->MultiSampleType = (D3DMULTISAMPLE_TYPE)g_gameConfig.msaa;
        }
        else
        {
            WARN("MSAA not supported (0x%x)...", hr);
            g_gameConfig.msaa = 0;
        }
    }
#endif
}

void GraphicsInit()
{
    /* Fix for "At least 8 MB of available video memory" */
    WriteMemUInt8((PVOID)0x005460CD, ASM_SHORT_JMP_REL);

#if WINDOWED_MODE_SUPPORT
    if (g_gameConfig.wndMode != GameConfig::FULLSCREEN)
    {
        /* Enable windowed mode */
        WriteMemUInt32((PVOID)(0x004B29A5 + 6), 0xC8);
        if (g_gameConfig.wndMode == GameConfig::STRETCHED)
            WriteMemUInt32((PVOID)(0x0050C4E3 + 1), WS_POPUP|WS_SYSMENU);
    }
#endif

    //WriteMemUInt8((PVOID)0x00524C98, ASM_SHORT_JMP_REL); // disable window hooks

#if MULTISAMPLING_SUPPORT
    if (g_gameConfig.msaa)
    {
        WriteMemUInt32((PVOID)(0x00545B4D + 6), D3DSWAPEFFECT_DISCARD); // full screen
        WriteMemUInt32((PVOID)(0x00545AE3 + 6), D3DSWAPEFFECT_DISCARD); // windowed
    }
#endif

    WriteMemUInt8Repeat((PVOID)0x00545AA7, ASM_NOP, 0x00545AB5 - 0x00545AA7);
    WriteMemUInt8((PVOID)0x00545AA7, ASM_LONG_CALL_REL);
    WriteMemPtr((PVOID)0x00545AA8, (PVOID)((ULONG_PTR)SetupPP - (0x00545AA7 + 0x5)));
    WriteMemUInt8Repeat((PVOID)(0x00545BA5), ASM_NOP, 6); // dont set PP.Flags

    /* nVidia issue fix (make sure D3Dsc is enabled) */
    WriteMemUInt8((PVOID)0x00546154, 1);

#if WIDESCREEN_FIX
    /* Fix FOV for widescreen */
    WriteMemPtr((PVOID)(0x005473E4 + 1), (PVOID)((ULONG_PTR)GrSetViewMatrixLastCallHook - (0x005473E4 + 0x5)));
    WriteMemFloat((PVOID)0x00596140, 200.0f); // reduce default wfar value to fix Assault Rifle rendering on wide screen
#endif

    /* Don't use LOD models */
    if (g_gameConfig.disableLodModels)
        WriteMemUInt8((PVOID)0x0052FACC, ASM_SHORT_JMP_REL);

    // Better error message in case of device creation error
    WriteMemUInt8((PVOID)0x00545BEF, ASM_LONG_JMP_REL);
    WriteMemPtr((PVOID)(0x00545BEF + 1), (PVOID)((ULONG_PTR)GrCreateD3DDeviceError_00545BEF - (0x00545BEF + 0x5)));
    

    /* Use hardware vertex processing */
    //WriteMemUInt8((PVOID)0x00545BDF, D3DCREATE_HARDWARE_VERTEXPROCESSING);

    //WriteMemUInt8((PVOID)0x005450A0, ASM_MOV_EAX32);
    //WriteMemUInt32((PVOID)0x005450A1, 0 /*D3DPOOL_MANAGED*/); // FIXME: D3DPOOL_MANAGED makes game much slower
    //WriteMemUInt8Repeat((PVOID)0x005450A6, ASM_NOP, 27);
    //WriteMemUInt8Repeat((PVOID)0x005450C2, ASM_NOP, 4);
    //WriteMemUInt8Repeat((PVOID)0x005450CB, ASM_NOP, 3);

    //WriteMemUInt32((PVOID)0x005450DE, /*D3DUSAGE_DYNAMIC|*/D3DUSAGE_DONOTCLIP|D3DUSAGE_WRITEONLY);
    //WriteMemUInt32((PVOID)0x00545118, /*D3DUSAGE_DYNAMIC|*/D3DUSAGE_DONOTCLIP|D3DUSAGE_WRITEONLY);

    //WriteMemUInt32((PVOID)0x00488BE9, ((ULONG_PTR)RenderEntityHook) - (0x00488BE8 + 0x5));
}

void GraphicsAfterGameInit()
{
#if ANISOTROPIC_FILTERING
    /* Anisotropic texture filtering */
    if (g_pRfGrDeviceCaps->MaxAnisotropy > 0 && g_gameConfig.anisotropicFiltering)
    {
        DWORD AnisotropyLevel = std::min(g_pRfGrDeviceCaps->MaxAnisotropy, 16ul);
        SetTextureMinMagFilter(D3DTEXF_ANISOTROPIC);
        IDirect3DDevice8_SetTextureStageState(*g_ppGrDevice, 0, D3DTSS_MAXANISOTROPY, AnisotropyLevel);
        IDirect3DDevice8_SetTextureStageState(*g_ppGrDevice, 1, D3DTSS_MAXANISOTROPY, AnisotropyLevel);
        INFO("Anisotropic Filtering enabled (level: %d)", AnisotropyLevel);
    }
#endif
}

void GraphicsDrawFpsCounter()
{
    char szBuf[32];
    sprintf(szBuf, "FPS: %.1f", *g_fFps);
    GrSetColorRgb(0, 255, 0, 255);
    GrDrawAlignedText(2, RfGetWidth() - 10, 60, szBuf, -1, *((uint32_t*)0x17C7C5C));
}