#include <cassert>
#include <patch_common/FunHook.h>
#include <patch_common/CallHook.h>
#include <patch_common/AsmWriter.h>
#include <xlog/xlog.h>
#include "hud.h"
#include "multi_scoreboard.h"
#include "hud_internal.h"
#include "../main/main.h"
#include "../os/console.h"
#include "../rf/gr/gr_font.h"
#include "../rf/hud.h"
#include "../rf/multi.h"
#include "../rf/file.h"
#include "../rf/entity.h"
#include "../rf/player.h"
#include "../rf/weapon.h"
#include "../rf/gameseq.h"
#include "multi_spectate.h"

int g_target_player_name_font = -1;

int hud_transform_value(int val, int old_max, int new_max)
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

int hud_scale_value(int val, int max, float scale)
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

rf::HudPoint hud_scale_coords(rf::HudPoint pt, float scale)
{
    return {
        hud_scale_value(pt.x, rf::gr_screen_width(), scale),
        hud_scale_value(pt.y, rf::gr_screen_height(), scale),
    };
}

FunHook<void()> hud_render_for_multi_hook{
    0x0046ECB0,
    []() {
        if (!rf::hud_disabled) {
            hud_render_for_multi_hook.call_target();
        }
    },
};

ConsoleCommand2 hud_cmd{
    "hud",
    [](std::optional<bool> hud_opt) {

        // toggle if no parameter passed
        bool hud_visible = hud_opt.value_or(rf::hud_disabled);
        rf::hud_disabled = !hud_visible;
    },
    "Show and hide HUD",
};

void hud_setup_positions(int width)
{
    int height = rf::gr_screen_height();
    rf::HudPoint* pos_data = nullptr;

    xlog::trace("hud_setup_positionsHook(%d)", width);

    switch (width) {
    case 640:
        if (height == 480)
            pos_data = rf::hud_coords_640;
        break;
    case 800:
        if (height == 600)
            pos_data = rf::hud_coords_800;
        break;
    case 1024:
        if (height == 768)
            pos_data = rf::hud_coords_1024;
        break;
    case 1280:
        if (height == 1024)
            pos_data = rf::hud_coords_1280;
        break;
    }
    if (pos_data) {
        std::copy(pos_data, pos_data + rf::num_hud_coords, rf::hud_coords);
    }
    else {
        // We have to scale positions from other resolution here
        for (int i = 0; i < rf::num_hud_coords; ++i) {
            rf::HudPoint& src_pt = rf::hud_coords_1024[i];
            rf::HudPoint& dst_pt = rf::hud_coords[i];

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
            else // hud_coords_1024[i].y >= 768*2/3
                dst_pt.y = src_pt.y + height - 768;
        }
    }
}
FunHook hud_setup_positions_hook{0x004377C0, hud_setup_positions};

void set_big_countdown_counter(bool is_big)
{
    float scale = is_big ? 2.0f : 1.0f;
    rf::hud_coords[rf::hud_countdown_timer] = hud_scale_coords(rf::hud_coords[rf::hud_countdown_timer], scale);
}

void set_big_hud(bool is_big)
{
    hud_status_set_big(is_big);
    multi_hud_chat_set_big(is_big);
    hud_personas_set_big(is_big);
    weapon_select_set_big(is_big);
    multi_scoreboard_set_big(is_big);
    multi_hud_set_big(is_big);
    rf::hud_text_font_num = hud_get_default_font();
    g_target_player_name_font = hud_get_default_font();

    hud_setup_positions(rf::gr_screen_width());
    hud_weapons_set_big(is_big);
    set_big_countdown_counter(is_big);

    // TODO: Message Log - Note: it remembers text height in save files so method of recalculation is needed
    //write_mem<i8>(0x004553DB + 1, is_big ? 127 : 70);
}

ConsoleCommand2 bighud_cmd{
    "bighud",
    []() {
        g_game_config.big_hud = !g_game_config.big_hud;
        set_big_hud(g_game_config.big_hud);
        g_game_config.save();
        rf::console_printf("Big HUD is %s", g_game_config.big_hud ? "enabled" : "disabled");
    },
    "Toggle big HUD",
    "bighud",
};

#ifdef DEBUG
ConsoleCommand2 hud_coords_cmd{
    "d_hud_coords",
    [](int idx, std::optional<int> x, std::optional<int> y) {
        if (x && y) {
            rf::hud_coords[idx].x = x.value();
            rf::hud_coords[idx].y = y.value();
        }
        rf::console_printf("HUD coords[%d]: <%d, %d>", idx, rf::hud_coords[idx].x, rf::hud_coords[idx].y);

    },
};
#endif

struct ScaledBitmapInfo {
    int bmh = -1;
    float scale = 2.0f;
};

const ScaledBitmapInfo& hud_get_scaled_bitmap_info(int bmh)
{
    static std::unordered_map<int, ScaledBitmapInfo> scaled_bm_cache;
    // Use bitmap with "_1" suffix instead of "_0" if it exists
    auto it = scaled_bm_cache.find(bmh);
    if (it == scaled_bm_cache.end()) {
        std::string filename = rf::bm_get_filename(bmh);
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

        xlog::trace("loading high res bm %s", filename.c_str());
        ScaledBitmapInfo scaled_bm_info;
        rf::File file;
        if (rf::file_exists(filename.c_str())) {
            scaled_bm_info.bmh = rf::bm_load(filename.c_str(), -1, false);
        }
        xlog::trace("loaded high res bm %s: %d", filename.c_str(), scaled_bm_info.bmh);
        if (scaled_bm_info.bmh != -1) {
            rf::gr_tcache_add_ref(scaled_bm_info.bmh);
            int bm_w, bm_h;
            rf::bm_get_dimensions(bmh, &bm_w, &bm_h);
            int scaled_bm_w, scaled_bm_h;
            rf::bm_get_dimensions(scaled_bm_info.bmh, &scaled_bm_w, &scaled_bm_h);
            scaled_bm_info.scale = static_cast<float>(scaled_bm_w) / static_cast<float>(bm_w);

        }

        it = scaled_bm_cache.insert({bmh, scaled_bm_info}).first;
    }
    return it->second;
}

void hud_preload_scaled_bitmap(int bmh)
{
    hud_get_scaled_bitmap_info(bmh);
}

void hud_scaled_bitmap(int bmh, int x, int y, float scale, rf::GrMode mode)
{
    if (scale > 1.0f) {
        const auto& scaled_bm_info = hud_get_scaled_bitmap_info(bmh);
        if (scaled_bm_info.bmh != -1) {
            bmh = scaled_bm_info.bmh;
            scale /= scaled_bm_info.scale;
        }
    }

    if (scale == 1.0f) {
        rf::gr_bitmap(bmh, x, y, mode);
    }
    else {
        // Get bitmap size and scale it
        int bm_w, bm_h;
        rf::bm_get_dimensions(bmh, &bm_w, &bm_h);
        int dst_w = static_cast<int>(std::round(bm_w * scale));
        int dst_h = static_cast<int>(std::round(bm_h * scale));
        rf::gr_bitmap_scaled(bmh, x, y, dst_w, dst_h, 0, 0, bm_w, bm_h, false, false, mode);
    }
}

void hud_rect_border(int x, int y, int w, int h, int border, rf::GrMode state)
{
    // top
    rf::gr_rect(x, y, w, border, state);
    // bottom
    rf::gr_rect(x, y + h - border, w, border, state);
    // left
    rf::gr_rect(x, y + border, border, h - 2 * border, state);
    // right
    rf::gr_rect(x + w - border, y + border, border, h - 2 * border, state);
}

std::string hud_fit_string(std::string_view str, int max_w, int* str_w_out, int font_id)
{
    std::string result{str};
    int str_w, str_h;
    bool has_ellipsis = false;
    rf::gr_get_string_size(&str_w, &str_h, result.c_str(), -1, font_id);
    while (str_w > max_w) {
        result = result.substr(0, result.size() - (has_ellipsis ? 4 : 1)) + "...";
        has_ellipsis = true;
        rf::gr_get_string_size(&str_w, &str_h, result.c_str(), -1, font_id);
    }
    if (str_w_out) {
        *str_w_out = str_w;
    }
    return result;
}

const char* hud_get_default_font_name(bool big)
{
    if (big) {
        return "regularfont.ttf:17";
    }
    else {
        return "rfpc-medium.vf";
    }
}

const char* hud_get_bold_font_name(bool big)
{
    if (big) {
        return "boldfont.ttf:26";
    }
    else {
        return "rfpc-large.vf";
    }
}

int hud_get_default_font()
{
    if (g_game_config.big_hud) {
        static int font = -2;
        if (font == -2) {
            font = rf::gr_load_font(hud_get_default_font_name(true));
        }
        return font;
    }
    else {
        static int font = -2;
        if (font == -2) {
            font = rf::gr_load_font(hud_get_default_font_name(false));
        }
        return font;
    }
}

int hud_get_large_font()
{
    if (g_game_config.big_hud) {
        static int font = -2;
        if (font == -2) {
            font = rf::gr_load_font(hud_get_bold_font_name(true));
            if (font == -1) {
                xlog::error("Failed to load boldfont!");
            }
        }
        return font;
    }
    else {
        static int font = -2;
        if (font == -2) {
            font = rf::gr_load_font(hud_get_bold_font_name(false));
        }
        return font;
    }
}

FunHook<void()> hud_init_hook{
    0x00437AB0,
    []() {
        hud_init_hook.call_target();
        // Init big HUD
        if (!rf::is_dedicated_server) {
            set_big_hud(g_game_config.big_hud);
        }
    },
};

CallHook hud_msg_render_gr_get_font_height_hook{
    0x004382DB,
    []([[ maybe_unused ]] int font_no) {
        // Fix wrong font number being used causing line spacing to be invalid
        return rf::gr_get_font_height(rf::hud_text_font_num);
    },
};

void hud_render_00437BC0()
{
    if (!rf::is_multi || !rf::local_player)
        return;

    bool scoreboard_control_pressed = rf::control_config_check_pressed(&rf::local_player->settings.controls, rf::CA_MP_STATS, 0);
    bool is_player_dead = rf::player_is_dead(rf::local_player) || rf::player_is_dying(rf::local_player);
    bool limbo = rf::gameseq_get_state() == rf::GS_MULTI_LIMBO;
    bool show_scoreboard = scoreboard_control_pressed || (!multi_spectate_is_spectating() && is_player_dead) || limbo;

    scoreboard_maybe_render(show_scoreboard);
}

void hud_apply_patches()
{
    // Fix HUD on not supported resolutions
    hud_setup_positions_hook.install();

    // Command for hidding the HUD
    hud_render_for_multi_hook.install();
    hud_cmd.register_cmd();

    // Add some init code
    hud_init_hook.install();

    // Other commands
    bighud_cmd.register_cmd();
#ifndef NDEBUG
    hud_coords_cmd.register_cmd();
#endif

    // Big HUD support
    hud_msg_render_gr_get_font_height_hook.install();

    write_mem_ptr(0x004780D2 + 1, &g_target_player_name_font);
    write_mem_ptr(0x004780FC + 2, &g_target_player_name_font);

    // Change scoreboard rendering logic
    AsmWriter(0x00437BC0).call(hud_render_00437BC0).jmp(0x00437C24);
    AsmWriter(0x00437D40).jmp(0x00437D5C);

    // Patches from other files
    hud_status_apply_patches();
    hud_weapons_apply_patches();
    hud_personas_apply_patches();
    weapon_select_apply_patches();
    multi_hud_chat_apply_patches();
    multi_hud_apply_patches();
    message_log_apply_patch();
}
