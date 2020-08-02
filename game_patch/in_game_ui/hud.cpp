#include "hud.h"
#include "scoreboard.h"
#include "hud_internal.h"
#include "../main.h"
#include "../console/console.h"
#include "../graphics/graphics.h"
#include "../rf/hud.h"
#include "../rf/network.h"
#include "../rf/fs.h"
#include "../rf/entity.h"
#include "../rf/player.h"
#include "../rf/weapon.h"
#include <patch_common/FunHook.h>
#include <patch_common/CallHook.h>
#include <xlog/xlog.h>
#include <cassert>

float g_hud_ammo_scale = 1.0f;
int g_target_player_name_font = -1;

static int HudTransformValue(int val, int old_max, int new_max)
{
    if (val < old_max / 3) {
        return val;
    }
    else if (val < old_max * 2 / 3) {
        return val - old_max / 2 + new_max / 2;
    }
    else {
        return val - old_max + new_max;
    }
}

static int HudScaleValue(int val, int max, float scale)
{
    if (val < max / 3) {
        return static_cast<int>(std::round(val * scale));
    }
    else if (val < max * 2 / 3) {
        return static_cast<int>(std::round(max / 2.0f + (val - max / 2.0f) * scale));
    }
    else {
        return static_cast<int>(std::round(max + (val - max) * scale));
    }
}

rf::HudPoint HudScaleCoords(rf::HudPoint pt, float scale)
{
    return {
        HudScaleValue(pt.x, rf::GrGetMaxWidth(), scale),
        HudScaleValue(pt.y, rf::GrGetMaxHeight(), scale),
    };
}

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

CallHook<void(int, int, int, rf::GrRenderState)> HudRenderAmmo_GrBitmap_hook{
    {
        // HudRenderAmmoClip
        0x0043A5E9u,
        0x0043A637u,
        0x0043A680u,
        // HudRenderAmmoPower
        0x0043A988u,
        0x0043A9DDu,
        0x0043AA24u,
        // HudRenderAmmoNoClip
        0x0043AE80u,
        0x0043AEC3u,
        0x0043AF0Au,
    },
    [](int bm_handle, int x, int y, rf::GrRenderState render_state) {
        HudScaledBitmap(bm_handle, x, y, g_hud_ammo_scale, render_state);
    },
};

FunHook<void(rf::EntityObj*, int, int, bool)> HudRenderAmmo_hook{
    0x0043A510,
    [](rf::EntityObj *entity, int weapon_cls_id, int offset_y, bool is_inactive) {
        offset_y = static_cast<int>(offset_y * g_hud_ammo_scale);
        HudRenderAmmo_hook.CallTarget(entity, weapon_cls_id, offset_y, is_inactive);
    },
};

CallHook<void(int, int, int, rf::GrRenderState)> RenderReticle_GrBitmap_hook{
    {
        0x0043A499,
        0x0043A4FE,
    },
    [](int bm_handle, int x, int y, rf::GrRenderState render_state) {
        float base_scale = g_game_config.big_hud ? 2.0f : 1.0f;
        float scale = base_scale * g_game_config.reticle_scale;

        x = static_cast<int>((x - rf::GrGetClipWidth() / 2) * scale) + rf::GrGetClipWidth() / 2;
        y = static_cast<int>((y - rf::GrGetClipHeight() / 2) * scale) + rf::GrGetClipHeight() / 2;

        HudScaledBitmap(bm_handle, x, y, scale, render_state);
    },
};

CallHook<void(int, int, int, rf::GrRenderState)> HudRenderPowerUps_GrBitmap_hook{
    {
        0x0047FF2F,
        0x0047FF96,
        0x0047FFFD,
    },
    [](int bm_handle, int x, int y, rf::GrRenderState render_state) {
        float scale = g_game_config.big_hud ? 2.0f : 1.0f;
        x = HudTransformValue(x, 640, rf::GrGetClipWidth());
        x = HudScaleValue(x, rf::GrGetClipWidth(), scale);
        y = HudScaleValue(y, rf::GrGetClipHeight(), scale);
        HudScaledBitmap(bm_handle, x, y, scale, render_state);
    },
};

FunHook<void()> RenderLevelInfo_hook{
    0x00477180,
    []() {
        RunWithDefaultFont(HudGetDefaultFont(), [&]() {
            RenderLevelInfo_hook.CallTarget();
        });
    },
};

void SetBigAmmo(bool is_big)
{
    rf::HudItem ammo_hud_items[] = {
        rf::hud_ammo_bar,
        rf::hud_ammo_signal,
        rf::hud_ammo_icon,
        rf::hud_ammo_in_clip_text_ul_region_coord,
        rf::hud_ammo_in_clip_text_width_and_height,
        rf::hud_ammo_in_inv_text_ul_region_coord,
        rf::hud_ammo_in_inv_text_width_and_height,
        rf::hud_ammo_bar_position_no_clip,
        rf::hud_ammo_signal_position_no_clip,
        rf::hud_ammo_icon_position_no_clip,
        rf::hud_ammo_in_inv_ul_region_coord_no_clip,
        rf::hud_ammo_in_inv_text_width_and_height_no_clip,
        rf::hud_ammo_in_clip_ul_coord,
        rf::hud_ammo_in_clip_width_and_height,
    };
    g_hud_ammo_scale = is_big ? 1.875f : 1.0f;
    for (auto item_num : ammo_hud_items) {
        rf::hud_points[item_num] = HudScaleCoords(rf::hud_points[item_num], g_hud_ammo_scale);
    }
    rf::hud_ammo_font = rf::GrLoadFont(is_big ? "biggerfont.vf" : "bigfont.vf");
}

void SetBigCountdownCounter(bool is_big)
{
    float scale = is_big ? 2.0f : 1.0f;
    rf::hud_points[rf::hud_countdown_timer] = HudScaleCoords(rf::hud_points[rf::hud_countdown_timer], scale);
}

void SetBigHud(bool is_big)
{
    SetBigHealthArmorHud(is_big);
    SetBigChatbox(is_big);
    SetBigPersonaMsgHud(is_big);
    SetBigWeaponCycleHud(is_big);
    SetBigScoreboard(is_big);
    SetBigTeamScoresHud(is_big);
    rf::hud_text_font_num = HudGetDefaultFont();
    g_target_player_name_font = HudGetDefaultFont();

    HudSetupPositions(rf::GrGetMaxWidth());
    SetBigAmmo(is_big);
    SetBigCountdownCounter(is_big);

    // TODO: Message Log - Note: it remembers text height in save files so method of recalculation is needed
    //WriteMem<i8>(0x004553DB + 1, is_big ? 127 : 70);
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

DcCommand2 reticle_scale_cmd{
    "reticle_scale",
    [](std::optional<float> scale_opt) {
        if (scale_opt) {
            g_game_config.reticle_scale = scale_opt.value();
            g_game_config.save();
        }
        rf::DcPrintf("Reticle scale %.4f", g_game_config.reticle_scale.value());
    },
    "Sets/gets reticle scale",
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
    struct ScaledBmInfo {
        int bmh = -1;
        float scale = 2.0f;
    };
    static std::unordered_map<int, ScaledBmInfo> hi_res_bm_map;
    if (scale > 1.0f) {
        // Use bitmap with "_1" suffix instead of "_0" if it exists
        auto it = hi_res_bm_map.find(bmh);
        if (it == hi_res_bm_map.end()) {
            std::string filename = rf::BmGetFilename(bmh);
            auto ext_pos = filename.rfind('.');
            if (ext_pos != std::string::npos) {
                if (ext_pos >= 2 && filename.compare(ext_pos - 2, 2, "_0") == 0) {
                    // ends with "_0" - replace '0' by '1'
                    filename[ext_pos - 1] = '1';
                }
                else {
                    // does not end with "_0" - append "_1"
                    filename.insert(ext_pos, "_1");
                    assert(filename.size() < 32);
                }
            }

            xlog::info("loading high res bm %s", filename.c_str());
            ScaledBmInfo scaled_bm_info;
            rf::File file;
            if (rf::FsFileExists(filename.c_str())) {
                scaled_bm_info.bmh = rf::BmLoad(filename.c_str(), -1, false);
            }
            xlog::trace("loaded high res bm %s: %d", filename.c_str(), scaled_bm_info.bmh);
            if (scaled_bm_info.bmh != -1) {
                int bm_w, bm_h;
                rf::BmGetBitmapSize(bmh, &bm_w, &bm_h);
                int scaled_bm_w, scaled_bm_h;
                rf::BmGetBitmapSize(scaled_bm_info.bmh, &scaled_bm_w, &scaled_bm_h);
                scaled_bm_info.scale = static_cast<float>(scaled_bm_w) / static_cast<float>(bm_w);
            }

            it = hi_res_bm_map.insert({bmh, scaled_bm_info}).first;
        }
        ScaledBmInfo& scaled_bm_info = it->second;
        if (scaled_bm_info.bmh != -1) {
            bmh = scaled_bm_info.bmh;
            scale /= scaled_bm_info.scale;
        }
    }

    if (scale == 1.0f) {
        rf::GrBitmap(bmh, x, y, render_state);
    }
    else {
        // Get bitmap size and scale it
        int bm_w, bm_h;
        rf::BmGetBitmapSize(bmh, &bm_w, &bm_h);
        int dst_w = static_cast<int>(std::round(bm_w * scale));
        int dst_h = static_cast<int>(std::round(bm_h * scale));
        rf::GrBitmapStretched(bmh, x, y, dst_w, dst_h, 0, 0, bm_w, bm_h, 0.0f, 0.0f, render_state);
    }
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

const char* HudGetDefaultFontName(bool big)
{
    if (big) {
        return "regularfont.ttf:17";
    }
    else {
        return "rfpc-medium.vf";
    }
}

const char* HudGetBoldFontName(bool big)
{
    if (big) {
        return "boldfont.ttf:26";
    }
    else {
        return "rfpc-large.vf";
    }
}

int HudGetDefaultFont()
{
    if (g_game_config.big_hud) {
        static int font = -2;
        if (font == -2) {
            font = rf::GrLoadFont(HudGetDefaultFontName(true));
        }
        return font;
    }
    else {
        static int font = -2;
        if (font == -2) {
            font = rf::GrLoadFont(HudGetDefaultFontName(false));
        }
        return font;
    }
}

int HudGetLargeFont()
{
    if (g_game_config.big_hud) {
        static int font = -2;
        if (font == -2) {
            font = rf::GrLoadFont(HudGetBoldFontName(true));
            if (font == -1) {
                xlog::error("Failed to load boldfont!");
            }
        }
        return font;
    }
    else {
        static int font = -2;
        if (font == -2) {
            font = rf::GrLoadFont(HudGetBoldFontName(false));
        }
        return font;
    }
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

CallHook HudRenderStatusMsg_GrGetFontHeight_hook{
    0x004382DB,
    []([[ maybe_unused ]] int font_no) {
        // Fix wrong font number being used causing line spacing to be invalid
        return rf::GrGetFontHeight(rf::hud_text_font_num);
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
    reticle_scale_cmd.Register();
#ifndef NDEBUG
    hud_coords_cmd.Register();
#endif

    // Fix message log rendering in resolutions with ratio different than 4:3
    gr_bitmap_stretched_message_log_hook.Install();

    // Big HUD support
    HudRenderAmmo_GrBitmap_hook.Install();
    HudRenderAmmo_hook.Install();
    RenderReticle_GrBitmap_hook.Install();
    HudRenderStatusMsg_GrGetFontHeight_hook.Install();
    HudRenderPowerUps_GrBitmap_hook.Install();
    RenderLevelInfo_hook.Install();

    WriteMemPtr(0x004780D2 + 1, &g_target_player_name_font);
    WriteMemPtr(0x004780FC + 2, &g_target_player_name_font);

    // Patches from other files
    InstallHealthArmorHudPatches();
    InstallChatboxPatches();
    InstallWeaponCycleHudPatches();
    InstallPersonaMsgHudPatches();
    ApplyTeamScoresHudPatches();
}

bool IsDoubleAmmoHud()
{
    if (rf::is_multi) {
        return false;
    }
    auto entity = rf::EntityGetByHandle(rf::local_player->entity_handle);
    if (!entity) {
        return false;
    }
    auto weapon_cls_id = entity->ai_info.weapon_cls_id;
    return weapon_cls_id == rf::machine_pistol_cls_id || weapon_cls_id == rf::machine_pistol_special_cls_id;
}
