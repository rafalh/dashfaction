#include "hud.h"
#include "scoreboard.h"
#include "hud_internal.h"
#include "../main.h"
#include "../console/console.h"
#include "../rf/hud.h"
#include "../rf/network.h"
#include <patch_common/FunHook.h>
#include <patch_common/CallHook.h>

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

void HudSetupPositions(int width)
{
    int height = rf::GrGetMaxHeight();
    rf::HudPoint* pos_data = nullptr;

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
            rf::HudPoint& src_pt = rf::hud_points_1024[i];
            rf::HudPoint& dst_pt = rf::hud_points[i];

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
FunHook HudSetupPositions_hook{0x004377C0, HudSetupPositions};

void SetBigHud(bool is_big)
{
    SetBigHealthArmorHud(is_big);
    SetBigChatbox(is_big);
    SetBigPersonaMsgHud(is_big);
    SetBigWeaponCycleHud(is_big);
    SetBigScoreboard(is_big);
    SetBigTeamScoresHud(is_big);
    if (is_big) {
        rf::hud_text_font_num = rf::rfpc_large_font_id;
    }
    else {
        rf::hud_text_font_num = rf::rfpc_medium_font_id;
    }
}

DcCommand2 bighud_cmd{
    "bighud",
    []() {
        g_game_config.big_hud = !g_game_config.big_hud;
        SetBigHud(g_game_config.big_hud);
        g_game_config.save();
        rf::DcPrintf("Big HUD is %s", g_game_config.big_hud ? "enabled" : "disabled");
    },
    "Toggle big HUD",
    "bighud",
};

DcCommand2 hud_coords_cmd{
    "d_hud_coords",
    [](int idx, std::optional<int> x, std::optional<int> y) {
        if (x && y) {
            rf::hud_points[idx].x = x.value();
            rf::hud_points[idx].y = y.value();
        }
        rf::DcPrintf("HUD coords[%d]: <%d, %d>", idx, rf::hud_points[idx].x, rf::hud_points[idx].y);

    },
};

void HudScaledBitmap(int bmh, int x, int y, float scale, rf::GrRenderState render_state)
{
    int bm_w, bm_h;
    rf::BmGetBitmapSize(bmh, &bm_w, &bm_h);
    int dst_w = bm_w * scale;
    int dst_h = bm_h * scale;
    rf::GrBitmapStretched(bmh, x, y, dst_w, dst_h, 0, 0, bm_w, bm_h, 0.0f, 0.0f, render_state);
}

void HudRectBorder(int x, int y, int w, int h, int border, rf::GrRenderState state)
{
    // top
    rf::GrRect(x, y, w, border, state);
    // bottom
    rf::GrRect(x, y + h - border, w, border, state);
    // left
    rf::GrRect(x, y + border, border, h - 2 * border, state);
    // right
    rf::GrRect(x + w - border, y + border, border, h - 2 * border, state);
}

std::string HudFitString(std::string_view str, int max_w, int* str_w_out, int font_id)
{
    std::string result{str};
    int str_w, str_h;
    bool has_ellipsis = false;
    rf::GrGetTextWidth(&str_w, &str_h, result.c_str(), -1, font_id);
    while (str_w > max_w) {
        result = result.substr(0, result.size() - (has_ellipsis ? 4 : 1)) + "...";
        has_ellipsis = true;
        rf::GrGetTextWidth(&str_w, &str_h, result.c_str(), -1, font_id);
    }
    if (str_w_out) {
        *str_w_out = str_w;
    }
    return result;
}

CallHook<void(int, int, int, int, int, int, int, int, int, char, char, int)> gr_bitmap_stretched_message_log_hook{
    0x004551F0,
    [](int bm_handle, int dst_x, int dst_y, int dst_w, int dst_h, int src_x, int src_y, int src_w, int src_h,
        char unk_u, char unk_v, int render_state) {

        float scale_x = rf::GrGetMaxWidth() / 640.0f;
        float scale_y = rf::GrGetMaxHeight() / 480.0f;
        dst_w = static_cast<int>(src_w * scale_x);
        dst_h = static_cast<int>(src_h * scale_y);
        dst_x = (rf::GrGetMaxWidth() - dst_w) / 2;
        dst_y = (rf::GrGetMaxHeight() - dst_h) / 2;
        gr_bitmap_stretched_message_log_hook.CallTarget(bm_handle, dst_x, dst_y, dst_w, dst_h,
            src_x, src_y, src_w, src_h, unk_u, unk_v, render_state);

        auto& message_log_entries_clip_h = AddrAsRef<int>(0x006425D4);
        auto& message_log_entries_clip_y = AddrAsRef<int>(0x006425D8);
        auto& message_log_entries_clip_w = AddrAsRef<int>(0x006425DC);
        auto& message_log_entries_clip_x = AddrAsRef<int>(0x006425E0);

        message_log_entries_clip_x = static_cast<int>(dst_x + scale_x * 30);
        message_log_entries_clip_y = static_cast<int>(dst_y + scale_y * 41);
        message_log_entries_clip_w = static_cast<int>(scale_x * 313);
        message_log_entries_clip_h = static_cast<int>(scale_y * 296);
    },
};

FunHook<void()> HudInit_hook{
    0x00437AB0,
    []() {
        HudInit_hook.CallTarget();
        // Init big HUD
        if (!rf::is_dedicated_server) {
            SetBigHud(g_game_config.big_hud);
        }
    },
};

void ApplyHudPatches()
{
    // Fix HUD on not supported resolutions
    HudSetupPositions_hook.Install();

    // Command for hidding the HUD
    hud_render_for_multi_hook.Install();
    hud_cmd.Register();

    // Add some init code
    HudInit_hook.Install();

    // Other commands
    bighud_cmd.Register();
#ifndef NDEBUG
    hud_coords_cmd.Register();
#endif

    // Fix message log rendering in resolutions with ratio different than 4:3
    gr_bitmap_stretched_message_log_hook.Install();

    // Patches from other files
    InstallHealthArmorHudPatches();
    InstallChatboxPatches();
    InstallWeaponCycleHudPatches();
    InstallPersonaMsgHudPatches();
    ApplyTeamScoresHudPatches();
}
