#include "stdafx.h"
#include "hud.h"
#include "utils.h"
#include "rf.h"

namespace rf {
constexpr int num_hud_points = 48;
static const auto hud_points_640 = reinterpret_cast<POINT*>(0x00637868);
static const auto hud_points_800 = reinterpret_cast<POINT*>(0x006373D0);
static const auto hud_points_1024 = reinterpret_cast<POINT*>(0x00637230);
static const auto hud_points_1280 = reinterpret_cast<POINT*>(0x00637560);
static const auto hud_points = reinterpret_cast<POINT*>(0x006376E8);
}

void HudCmdHandler()
{
    static auto &hud_hidden = AddrAsRef<bool>(0x006379F0);
    static auto &hud_hidden2 = AddrAsRef<bool>(0x006A1448);

    // toggle
    hud_hidden = !hud_hidden;
    hud_hidden2 = hud_hidden;

    // HudRender2
    WriteMemUInt8(0x00476D70, hud_hidden ? ASM_RET : 0x51);

    // Powerups textures
    WriteMemInt32(0x006FC43C, hud_hidden ? -2 : -1);
    WriteMemInt32(0x006FC440, hud_hidden ? -2 : -1);
}

void HudSetupPositionsHook(int width)
{
    int height = rf::GrGetMaxHeight();
    POINT* pos_data = nullptr;
    
    TRACE("HudSetupPositionsHook(%d)", width);
    
    switch (width) {
        case 640:
            if (height == 480)
                pos_data = rf::hud_points_640;
            break;
        case 800:
            if (height == 600)
                pos_data = rf::hud_points_800;
            break;
        case 1024:
            if (height == 768)
                pos_data = rf::hud_points_1024;
            break;
        case 1280:
            if (height == 1024)
                pos_data = rf::hud_points_1280;
            break;
    }
    if (pos_data) {
        memcpy(rf::hud_points, pos_data, rf::num_hud_points * sizeof(POINT));
    }
    else {
        // We have to scale positions from other resolution here
        for (unsigned i = 0; i < rf::num_hud_points; ++i) {
            if (rf::hud_points_1024[i].x <= 1024/3)
                rf::hud_points[i].x = rf::hud_points_1024[i].x;
            else if (rf::hud_points_1024[i].x > 1024/3 && rf::hud_points_1024[i].x < 1024*2/3)
                rf::hud_points[i].x = rf::hud_points_1024[i].x + width/2 - 1024/2;
            else
                rf::hud_points[i].x = rf::hud_points_1024[i].x + width - 1024;
            
            if (rf::hud_points_1024[i].y <= 768/3)
                rf::hud_points[i].y = rf::hud_points_1024[i].y;
            else if (rf::hud_points_1024[i].y > 768/3 && rf::hud_points_1024[i].y < 768*2/3)
                rf::hud_points[i].y = rf::hud_points_1024[i].y + (height - 768)/2;
            else // hud_points_1024[i].y >= 768*2/3
                rf::hud_points[i].y = rf::hud_points_1024[i].y + height - 768;
        }
    }
}

void InitHud()
{
    // Fix HUD on not supported resolutions
    AsmWritter(0x004377C0).jmpLong(HudSetupPositionsHook);
}
