#include "stdafx.h"
#include "graphics.h"
#include "utils.h"
#include "rf.h"
#include "main.h"

using namespace rf;

static float g_GrClippedGeomOffsetX = -0.5;
static float g_GrClippedGeomOffsetY = -0.5;

static void SetTextureMinMagFilter(D3DTEXTUREFILTERTYPE FilterType)
{
    uintptr_t Addresses[] = {
        // GrInitD3D
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

static void GrSetViewMatrix_FovFix(float fFovScale, float fWFarFactor)
{ 
    constexpr float fRefAspectRatio = 4.0f / 3.0f;
    constexpr float fMaxWideAspectRatio = 21.0f / 9.0f; // biggest aspect ratio currently in market

    // g_pGrScreen->fAspect == ScrW / ScrH * 0.75 (1.0 for 4:3 monitors, 1.2 for 16:10) - looks like Pixel Aspect Ratio
    // We use here MaxWidth and MaxHeight to calculate proper FOV for windowed mode

    float fViewportAspectRatio = (float)g_pGrScreen->ViewportWidth / (float)g_pGrScreen->ViewportHeight;
    float fAspectRatio = (float)g_pGrScreen->MaxWidth / (float)g_pGrScreen->MaxHeight;
    float fScaleX = 1.0f;
    float fScaleY = fRefAspectRatio * fViewportAspectRatio / fAspectRatio; // this is how RF does it and is needed for working scanner

    if (fAspectRatio <= fMaxWideAspectRatio) // never make X scale too high in windowed mode
        fScaleX *= fRefAspectRatio / fAspectRatio;
    else
    {
        fScaleX *= fRefAspectRatio / fMaxWideAspectRatio;
        fScaleY *= fAspectRatio / fMaxWideAspectRatio;
    }

    g_pGrScaleVec->x = fWFarFactor / fFovScale * fScaleX;
    g_pGrScaleVec->y = fWFarFactor / fFovScale * fScaleY;
    g_pGrScaleVec->z = fWFarFactor;

    g_GrClippedGeomOffsetX = g_pGrScreen->OffsetX - 0.5f;
    g_GrClippedGeomOffsetY = g_pGrScreen->OffsetY - 0.5f;
    *(float*)0x01818B54 -= 0.5f; // viewport center x
    *(float*)0x01818B5C -= 0.5f; // viewport center y
}

void NAKED GrSetViewMatrix_00547344()
{
    _asm
    {
        mov eax, [esp + 10h + 14h]
        sub esp, 8
        fstp [esp + 4]
        mov [esp + 0], eax
        call GrSetViewMatrix_FovFix
        fld [esp + 4]
        add esp, 8
        mov eax, 00547366h
        jmp eax
    }
}

void DisplayD3DDeviceError(HRESULT hr)
{
    ERR("D3D CreateDevice failed (hr 0x%X - %s)", hr, getDxErrorStr(hr));

    char buf[1024];
    sprintf(buf, "Failed to create Direct3D device object - error 0x%X (%s).\n"
        "A critical error has occurred and the program cannot continue.\n"
        "Press OK to exit the program", hr, getDxErrorStr(hr));
    MessageBoxA(nullptr, buf, "Error!", MB_OK | MB_ICONERROR);
    ExitProcess(-1);
}

void NAKED GrCreateD3DDeviceError_00545BEF()
{
    _asm
    {
        push eax
        call DisplayD3DDeviceError
        add esp, 4
    }
}

void GrClearZBuffer_SetRect(D3DRECT *pClearRect)
{
    pClearRect->x1 = g_pGrScreen->OffsetX + g_pGrScreen->ClipLeft;
    pClearRect->y1 = g_pGrScreen->OffsetY + g_pGrScreen->ClipTop;
    pClearRect->x2 = g_pGrScreen->OffsetX + g_pGrScreen->ClipRight + 1;
    pClearRect->y2 = g_pGrScreen->OffsetY + g_pGrScreen->ClipBottom + 1;
}

void NAKED GrClearZBuffer_005509C4()
{
    _asm
    {
        lea eax, [esp + 14h - 10h] // Rect
        push eax
        call GrClearZBuffer_SetRect
        add esp, 4
        mov eax, 005509F0h
        jmp eax
    }
}

static void SetupPP(void)
{
    D3DPRESENT_PARAMETERS *pPP = (D3DPRESENT_PARAMETERS*)0x01CFCA18;
    memset(pPP, 0, sizeof(*pPP));

    D3DFORMAT *pFormat = (D3DFORMAT*)0x005A135C;
    INFO("D3D Format: %ld", *pFormat);

    pPP->Flags = D3DPRESENTFLAG_LOCKABLE_BACKBUFFER;

#if MULTISAMPLING_SUPPORT
    if (g_gameConfig.msaa && *pFormat > 0)
    {
        // Make sure selected MSAA mode is available
        HRESULT hr = IDirect3D8_CheckDeviceMultiSampleType(*g_ppDirect3D, *g_pAdapterIdx, D3DDEVTYPE_HAL, *pFormat,
            g_gameConfig.wndMode == GameConfig::WINDOWED, (D3DMULTISAMPLE_TYPE)g_gameConfig.msaa);
        if (SUCCEEDED(hr))
        {
            INFO("Enabling Anti-Aliasing (%ux MSAA)...", g_gameConfig.msaa);
            pPP->MultiSampleType = (D3DMULTISAMPLE_TYPE)g_gameConfig.msaa;
        }
        else
        {
            WARN("MSAA not supported (0x%x)...", hr);
            g_gameConfig.msaa = D3DMULTISAMPLE_NONE;
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

    // Replace memset call to SetupPP
    WriteMemUInt8Repeat((PVOID)0x00545AA7, ASM_NOP, 0x00545AB5 - 0x00545AA7);
    WriteMemUInt8((PVOID)0x00545AA7, ASM_LONG_CALL_REL);
    WriteMemPtr((PVOID)(0x00545AA7 + 1), (PVOID)((ULONG_PTR)SetupPP - (0x00545AA7 + 0x5)));
    WriteMemUInt8Repeat((PVOID)(0x00545BA5), ASM_NOP, 6); // dont set PP.Flags

    /* nVidia issue fix (make sure D3Dsc is enabled) */
    WriteMemUInt8((PVOID)0x00546154, 1);

#if WIDESCREEN_FIX
    /* Fix FOV for widescreen */
    WriteMemUInt8((PVOID)0x00547344, ASM_LONG_JMP_REL);
    WriteMemPtr((PVOID)(0x00547344 + 1), (PVOID)((ULONG_PTR)GrSetViewMatrix_00547344 - (0x00547344 + 0x5)));
    WriteMemFloat((PVOID)0x0058A29C, 0.0003f); // factor related to near plane, default is 0.000588f
#endif

    /* Don't use LOD models */
    if (g_gameConfig.disableLodModels)
        WriteMemUInt8((PVOID)0x0052FACC, ASM_SHORT_JMP_REL);

    // Better error message in case of device creation error
    WriteMemUInt8((PVOID)0x00545BEF, ASM_LONG_JMP_REL);
    WriteMemPtr((PVOID)(0x00545BEF + 1), (PVOID)((ULONG_PTR)GrCreateD3DDeviceError_00545BEF - (0x00545BEF + 0x5)));

    // Fix rendering of right and bottom edges of viewport
    WriteMemUInt8((PVOID)0x00431D9F, ASM_SHORT_JMP_REL);
    WriteMemUInt8((PVOID)0x00431F6B, ASM_SHORT_JMP_REL);
    WriteMemUInt8((PVOID)0x004328CF, ASM_SHORT_JMP_REL);
    WriteMemUInt8((PVOID)0x0043298F, ASM_LONG_JMP_REL);
    WriteMemPtr((PVOID)(0x0043298F + 1), (PVOID)((ULONG_PTR)0x004329DC - (0x0043298F + 0x5)));
    WriteMemUInt8((PVOID)0x005509C4, ASM_LONG_JMP_REL);
    WriteMemPtr((PVOID)(0x005509C4 + 1), (PVOID)((ULONG_PTR)GrClearZBuffer_005509C4 - (0x005509C4 + 0x5)));

    // Left and top viewport edge fix for MSAA (RF does similar thing in GrDrawTextureD3D)
    WriteMemUInt8((PVOID)0x005478C6, ASM_FADD);
    WriteMemUInt8((PVOID)0x005478D7, ASM_FADD);
    WriteMemPtr((PVOID)(0x005478C6 + 2), &g_GrClippedGeomOffsetX);
    WriteMemPtr((PVOID)(0x005478D7 + 2), &g_GrClippedGeomOffsetY);
}

void GraphicsAfterGameInit()
{
#if ANISOTROPIC_FILTERING
    /* Anisotropic texture filtering */
    if (g_pGrDeviceCaps->MaxAnisotropy > 0 && g_gameConfig.anisotropicFiltering)
    {
        DWORD AnisotropyLevel = std::min(g_pGrDeviceCaps->MaxAnisotropy, 16ul);
        SetTextureMinMagFilter(D3DTEXF_ANISOTROPIC);
        IDirect3DDevice8_SetTextureStageState(*g_ppGrDevice, 0, D3DTSS_MAXANISOTROPY, AnisotropyLevel);
        IDirect3DDevice8_SetTextureStageState(*g_ppGrDevice, 1, D3DTSS_MAXANISOTROPY, AnisotropyLevel);
        INFO("Anisotropic Filtering enabled (level: %d)", AnisotropyLevel);
    }
#endif
}

void GraphicsDrawFpsCounter()
{
    if (g_gameConfig.fpsCounter)
    {
        char szBuf[32];
        sprintf(szBuf, "FPS: %.1f", *g_fFps);
        GrSetColorRgb(0, 255, 0, 255);
        GrDrawAlignedText(2, RfGetWidth() - 10, 60, szBuf, -1, *g_pGrTextMaterial);
    }
}