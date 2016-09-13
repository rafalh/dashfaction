#include "gamma.h"
#include "rf.h"
#include "utils.h"

#ifndef D3DSGR_NO_CALIBRATION
#define D3DSGR_NO_CALIBRATION 0
#endif

static D3DGAMMARAMP g_GammaRamp;

static void SetGammaRamp(D3DGAMMARAMP *pGammaRamp)
{
    if(0)
        IDirect3DDevice8_SetGammaRamp(*g_ppGrDevice, D3DSGR_NO_CALIBRATION, pGammaRamp);
    else
    {
        HDC hdc;
        
        hdc = GetDC(*g_phWnd);
        if(hdc)
        {
            if(!SetDeviceGammaRamp(hdc, pGammaRamp))
                ERR("SetDeviceGammaRamp failed");
            ReleaseDC(*g_phWnd, hdc);
        }
        else
            ERR("GetDC failed");
    }
}

static void GrUpdateGammaRampHook(void)
{
    unsigned i, Val;
    
    for(i = 0; i < 256; ++i)
    {
        Val = g_pGrGammaRamp[i] << 8;
        g_GammaRamp.red[i] = Val;
        g_GammaRamp.green[i] = Val;
        g_GammaRamp.blue[i] = Val;
    }
    
    SetGammaRamp(&g_GammaRamp);
}

void ResetGammaRamp(void)
{
    D3DGAMMARAMP GammaRamp;
    unsigned i;
    
    for(i = 0; i < 256; ++i)
        GammaRamp.red[i] = GammaRamp.green[i] = GammaRamp.blue[i] = i << 8;
    SetGammaRamp(&GammaRamp);
}

static void GammaMsgHandler(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch(uMsg)
    {
        case WM_SETFOCUS:
            SetGammaRamp(&g_GammaRamp);
            TRACE("WM_SETFOCUS");
            break;
        
        case WM_KILLFOCUS:
            ResetGammaRamp();
            TRACE("WM_KILLFOCUS");
            break;
    }
}

void InitGamma(void)
{
    /* Gamma fix */
    WriteMemUInt8((PVOID)0x00547A60, ASM_LONG_JMP_REL);
    WriteMemUInt32((PVOID)0x00547A61, ((ULONG_PTR)GrUpdateGammaRampHook) - (0x00547A60 + 0x5));
    
    RfAddMsgHandler(GammaMsgHandler);
}
