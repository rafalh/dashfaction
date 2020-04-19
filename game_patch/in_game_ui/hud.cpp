#include "hud.h"
#include "../console/console.h"
#include "../rf/graphics.h"
#include <patch_common/ShortTypes.h>
#include <patch_common/AsmWriter.h>
#include <patch_common/FunHook.h>
#include <patch_common/MemUtils.h>

namespace rf
{
constexpr int num_hud_points = 48;
static auto& hud_points_640 = AddrAsRef<POINT[num_hud_points]>(0x00637868);
static auto& hud_points_800 = AddrAsRef<POINT[num_hud_points]>(0x006373D0);
static auto& hud_points_1024 = AddrAsRef<POINT[num_hud_points]>(0x00637230);
static auto& hud_points_1280 = AddrAsRef<POINT[num_hud_points]>(0x00637560);
static auto& hud_points = AddrAsRef<POINT[num_hud_points]>(0x006376E8);
} // namespace rf

FunHook<void()> hud_render_for_multi_hook{
    0x0046ECB0,
    []() {
        if (!rf::is_hud_hidden) {
            hud_render_for_multi_hook.CallTarget();
        }
    },
};

DcCommand2 hud_cmd{
    "hud",
    [](std::optional<bool> hud_opt) {

        // toggle if no parameter passed
        bool hud_visible = hud_opt.value_or(rf::is_hud_hidden);
        rf::is_hud_hidden = !hud_visible;
    },
    "Show and hide HUD",
};

void HudSetupPositionsHook(int width)
{
    int height = rf::GrGetMaxHeight();
    POINT* pos_data = nullptr;

    xlog::trace("HudSetupPositionsHook(%d)", width);

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
        std::copy(pos_data, pos_data + rf::num_hud_points, rf::hud_points);
    }
    else {
        // We have to scale positions from other resolution here
        for (int i = 0; i < rf::num_hud_points; ++i) {
            POINT& src_pt = rf::hud_points_1024[i];
            POINT& dst_pt = rf::hud_points[i];

            if (src_pt.x <= 1024 / 3)
                dst_pt.x = src_pt.x;
            else if (src_pt.x > 1024 / 3 && src_pt.x < 1024 * 2 / 3)
                dst_pt.x = src_pt.x + (width - 1024) / 2;
            else
                dst_pt.x = src_pt.x + width - 1024;

            if (src_pt.y <= 768 / 3)
                dst_pt.y = src_pt.y;
            else if (src_pt.y > 768 / 3 && src_pt.y < 768 * 2 / 3)
                dst_pt.y = src_pt.y + (height - 768) / 2;
            else // hud_points_1024[i].y >= 768*2/3
                dst_pt.y = src_pt.y + height - 768;
        }
    }
}

void InitHud()
{
    // Fix HUD on not supported resolutions
    AsmWriter(0x004377C0).jmp(HudSetupPositionsHook);

    // Command for hidding the HUD
    hud_render_for_multi_hook.Install();
    hud_cmd.Register();
}
