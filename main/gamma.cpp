#include "stdafx.h"
#include "gamma.h"
#include "rf.h"
#include "utils.h"

#ifndef D3DSGR_NO_CALIBRATION
  #define D3DSGR_NO_CALIBRATION 0x0
#endif

using namespace rf;

static D3DGAMMARAMP g_GammaRamp;
static bool g_GammaRampInitialized = false;

static void SetGammaRamp(D3DGAMMARAMP *pGammaRamp)
{
#if 0 // Note: D3D Gamma Ramp doesn't work in windowed mode
    if (*g_ppGrDevice)
        IDirect3DDevice8_SetGammaRamp(*g_ppGrDevice, D3DSGR_NO_CALIBRATION, pGammaRamp);
#else
    HDC hdc;

    hdc = GetDC(*g_phWnd);
    if (hdc)
    {
        if (!SetDeviceGammaRamp(hdc, pGammaRamp))
            ERR("SetDeviceGammaRamp failed %lu", GetLastError());
        ReleaseDC(*g_phWnd, hdc);
    }
    else
        ERR("GetDC failed");
#endif
}

static void GrUpdateGammaRampHook(void)
{
    for (unsigned i = 0; i < 256; ++i)
    {
        unsigned Val = g_pGrGammaRamp[i] << 8;
        g_GammaRamp.red[i] = Val;
        g_GammaRamp.green[i] = Val;
        g_GammaRamp.blue[i] = Val;
    }

    g_GammaRampInitialized = true;
    SetGammaRamp(&g_GammaRamp);
}

void ResetGammaRamp(void)
{
    D3DGAMMARAMP GammaRamp;

    if (!g_GammaRampInitialized)
        return;
    
    for (unsigned i = 0; i < 256; ++i)
        GammaRamp.red[i] = GammaRamp.green[i] = GammaRamp.blue[i] = i << 8;
    
    SetGammaRamp(&GammaRamp);
}

static void GammaMsgHandler(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_ACTIVATE:
    case WM_ACTIVATEAPP:
        TRACE("WM_ACTIVATE %lx", wParam);
        if (g_GammaRampInitialized)
        {
            if (wParam)
                SetGammaRamp(&g_GammaRamp);
            else
                ResetGammaRamp();
        }
    }
}

void InitGamma(void)
{
    /* Gamma fix */
    AsmWritter(0x00547A60).jmpLong(GrUpdateGammaRampHook);
    
    rf::AddMsgHandler(GammaMsgHandler);
}
