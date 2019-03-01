#include "stdafx.h"
#include "gamma.h"
#include "rf.h"
#include "utils.h"

#ifndef D3DSGR_NO_CALIBRATION
  #define D3DSGR_NO_CALIBRATION 0x0
#endif

static D3DGAMMARAMP g_gamma_ramp;
static bool g_gamma_ramp_initialized = false;

namespace rf {
static const auto gr_gamma_ramp = (uint32_t*)0x017C7C68;
}

static void SetGammaRamp(D3DGAMMARAMP *gamma_ramp)
{
#if 0 // Note: D3D Gamma Ramp doesn't work in windowed mode
    if (g_pGrDevice)
        g_pGrDevice->SetGammaRamp(D3DSGR_NO_CALIBRATION, gamma_ramp);
#else
    HDC hdc = GetDC(rf::g_hWnd);
    if (hdc) {
        if (!SetDeviceGammaRamp(hdc, gamma_ramp))
            ERR("SetDeviceGammaRamp failed %lu", GetLastError());
        ReleaseDC(rf::g_hWnd, hdc);
    } else
        ERR("GetDC failed");
#endif
}

static void GrUpdateGammaRampHook()
{
    for (unsigned i = 0; i < 256; ++i) {
        unsigned Val = rf::gr_gamma_ramp[i] << 8;
        g_gamma_ramp.red[i] = Val;
        g_gamma_ramp.green[i] = Val;
        g_gamma_ramp.blue[i] = Val;
    }

    g_gamma_ramp_initialized = true;
    SetGammaRamp(&g_gamma_ramp);
}

void ResetGammaRamp()
{
    D3DGAMMARAMP gamma_ramp;

    if (!g_gamma_ramp_initialized)
        return;
    
    for (unsigned i = 0; i < 256; ++i)
        gamma_ramp.red[i] = gamma_ramp.green[i] = gamma_ramp.blue[i] = i << 8;
    
    SetGammaRamp(&gamma_ramp);
}

static void GammaMsgHandler(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg) {
    case WM_ACTIVATE:
    case WM_ACTIVATEAPP:
        TRACE("WM_ACTIVATE %lx", wParam);
        if (g_gamma_ramp_initialized) {
            if (wParam)
                SetGammaRamp(&g_gamma_ramp);
            else
                ResetGammaRamp();
        }
    }
}

void InitGamma()
{
    /* Gamma fix */
    AsmWritter(0x00547A60).jmpLong(GrUpdateGammaRampHook);
    
    rf::AddMsgHandler(GammaMsgHandler);
}
