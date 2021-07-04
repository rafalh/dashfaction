#include <windows.h>
#include <d3d8.h>
#include <xlog/xlog.h>
#include <patch_common/AsmWriter.h>
#include "../gr.h"
#include "../../rf/os/os.h"
#include "../../rf/gr/gr.h"
#include "../../rf/gr/gr_direct3d.h"

#ifndef D3DSGR_NO_CALIBRATION
#define D3DSGR_NO_CALIBRATION 0x0
#endif

static D3DGAMMARAMP g_gamma_ramp;
static bool g_gamma_ramp_initialized = false;

static bool set_gamma_ramp_via_d3d(D3DGAMMARAMP* gamma_ramp)
{
    // Avoid crash before D3D is initialized or in dedicated server mode
    if (!rf::gr::d3d::device) {
        return true;
    }
    // D3D Gamma Ramp doesn't work in windowed mode
    if (rf::gr::d3d::pp.Windowed) {
        return false;
    }
    if (!(rf::gr::d3d::device_caps.Caps2 & D3DCAPS2_FULLSCREENGAMMA)) {
        xlog::info("Swap chain does not support gamma ramps");
        return false;
    }
    rf::gr::d3d::device->SetGammaRamp(D3DSGR_NO_CALIBRATION, gamma_ramp);
    return true;
}

static bool set_gamma_ramp_via_gdi(D3DGAMMARAMP* gamma_ramp)
{
    //HDC hdc = GetDC(rf::main_wnd);
    HDC hdc = CreateDCA("DISPLAY", nullptr, nullptr, nullptr);
    if (!hdc) {
        xlog::warn("CreateDCA failed, error %lu", GetLastError());
        return false;
    }
    int cmcap = GetDeviceCaps(hdc, COLORMGMTCAPS);
    if (cmcap != CM_GAMMA_RAMP) {
        static bool once = false;
        if (!once) {
            xlog::warn("Display device does not support gamma ramps (cmcap %x)", cmcap);
            once = true;
        }
        // try it anyway - Wine returns 0 here but SetDeviceGammaRamp actually work
    }
    SetLastError(0);
    bool result = SetDeviceGammaRamp(hdc, gamma_ramp);
    //ReleaseDC(rf::main_wnd, hdc);
    DeleteDC(hdc);

    if (!result) {
        if (cmcap == CM_GAMMA_RAMP) {
            xlog::info("SetDeviceGammaRamp failed, error %lu", GetLastError());
        }
        return false;
    }
    return true;
}

static void set_gamma_ramp(D3DGAMMARAMP* gamma_ramp)
{
    bool d3d_result = set_gamma_ramp_via_d3d(gamma_ramp);
    if (!d3d_result) {
        set_gamma_ramp_via_gdi(gamma_ramp);
    }
}

static void gr_d3d_update_gamma_ramp_hook()
{
    for (unsigned i = 0; i < 256; ++i) {
        unsigned val = rf::gr::gamma_ramp[i] << 8;
        g_gamma_ramp.red[i] = val;
        g_gamma_ramp.green[i] = val;
        g_gamma_ramp.blue[i] = val;
    }

    g_gamma_ramp_initialized = true;
    set_gamma_ramp(&g_gamma_ramp);
}

void gr_d3d_gamma_reset()
{
    D3DGAMMARAMP gamma_ramp;

    if (!g_gamma_ramp_initialized)
        return;

    for (unsigned i = 0; i < 256; ++i) {
        gamma_ramp.red[i] = gamma_ramp.green[i] = gamma_ramp.blue[i] = i << 8;
    }

    set_gamma_ramp(&gamma_ramp);
}

static void gamma_msg_handler(UINT msg, WPARAM w_param, [[maybe_unused]] LPARAM l_param)
{
    switch (msg) {
    case WM_ACTIVATE:
    case WM_ACTIVATEAPP:
        xlog::trace("WM_ACTIVATE %x", w_param);
        if (g_gamma_ramp_initialized) {
            if (w_param)
                set_gamma_ramp(&g_gamma_ramp);
            else
                gr_d3d_gamma_reset();
        }
    }
}

void gr_d3d_gamma_init()
{
    rf::os_add_msg_handler(gamma_msg_handler);
}

void gr_d3d_gamma_apply_patch()
{
    // Add gamma support for windowed mode
    AsmWriter(0x00547A60).jmp(gr_d3d_update_gamma_ramp_hook);

}
