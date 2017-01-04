#include "hud.h"
#include "utils.h"
#include "rf.h"

void HudCmdHandler(void)
{
    uint32_t bHide = !*((uint32_t*)0x006379F0);
    
    // g_bHideHud, g_bHideHud2
    WriteMemUInt8((PVOID)0x006A1448, bHide);
    WriteMemUInt8((PVOID)0x006379F0, bHide);
    
    // HudRender2
    WriteMemUInt8((PVOID)0x00476D70, bHide ? ASM_RET : 0x51);
    
    // Powerups textures
    WriteMemInt32((PVOID)0x006FC43C, bHide ? -2 : -1);
    WriteMemInt32((PVOID)0x006FC440, bHide ? -2 : -1);
}

void HudSetupPositionsHook(int Width)
{
    int Height = RfGetHeight();
    POINT *pPosData = NULL;
    
    TRACE("HudSetupPositionsHook(%d)", Width);
    
    switch(Width)
    {
        case 640:
            if(Height == 480)
                pPosData = g_pHudPosData640;
            break;
        case 800:
            if(Height == 600)
                pPosData = g_pHudPosData800;
            break;
        case 1024:
            if(Height == 768)
                pPosData = g_pHudPosData1024;
            break;
        case 1280:
            if(Height == 1024)
                pPosData = g_pHudPosData1280;
            break;
    }
    if(pPosData)
        memcpy(g_pHudPosData, pPosData, HUD_POINTS_COUNT * sizeof(POINT));
    else
    {
        /* We have to scale positions from other resolution here */
        unsigned i;
        
        for(i = 0; i < HUD_POINTS_COUNT; ++i)
        {
            if(g_pHudPosData1024[i].x <= 1024/3)
                g_pHudPosData[i].x = g_pHudPosData1024[i].x;
            else if(g_pHudPosData1024[i].x > 1024/3 && g_pHudPosData1024[i].x < 1024*2/3)
                g_pHudPosData[i].x = g_pHudPosData1024[i].x + Width/2 - 1024/2;
            else
                g_pHudPosData[i].x = g_pHudPosData1024[i].x + Width - 1024;
            
            if(g_pHudPosData1024[i].y <= 768/3)
                g_pHudPosData[i].y = g_pHudPosData1024[i].y;
            else if(g_pHudPosData1024[i].y > 768/3 && g_pHudPosData1024[i].y < 768*2/3)
                g_pHudPosData[i].y = g_pHudPosData1024[i].y + (Height - 768)/2;
            else /* g_pHudPosData1024[i].y >= 768*2/3 */
                g_pHudPosData[i].y = g_pHudPosData1024[i].y + Height - 768;
        }
    }
}

void InitHud(void)
{
    /* Fix HUD on not supported resolutions */
    WriteMemUInt8((PVOID)0x004377C0, ASM_LONG_JMP_REL);
    WriteMemPtr((PVOID)0x004377C1, (PVOID)((ULONG_PTR)HudSetupPositionsHook - (0x004377C0 + 0x5)));
}
