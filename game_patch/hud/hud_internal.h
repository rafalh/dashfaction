#pragma once

#include <string>
#include "../rf/gr/gr.h"

namespace rf
{
    struct HudPoint;
}

void hud_status_apply_patches();
void hud_status_set_big(bool is_big);
void hud_personas_apply_patches();
void hud_personas_set_big(bool is_big);
void hud_weapons_apply_patches();
void hud_weapons_set_big(bool is_big);
void weapon_select_apply_patches();
void weapon_select_set_big(bool is_big);
void multi_hud_chat_apply_patches();
void multi_hud_chat_set_big(bool is_big);
void multi_hud_apply_patches();
void multi_hud_set_big(bool is_big);
void hud_scaled_bitmap(int bmh, int x, int y, float scale, rf::gr::Mode mode = rf::gr::bitmap_clamp_mode);
void hud_preload_scaled_bitmap(int bmh);
void hud_rect_border(int x, int y, int w, int h, int border, rf::gr::Mode state = rf::gr::rect_mode);
std::string hud_fit_string(std::string_view str, int max_w, int* str_w_out, int font_id);
rf::HudPoint hud_scale_coords(rf::HudPoint pt, float scale);
const char* hud_get_default_font_name(bool big);
const char* hud_get_bold_font_name(bool big);
void scoreboard_maybe_render(bool show_scoreboard);
int hud_transform_value(int val, int old_max, int new_max);
int hud_scale_value(int val, int max, float scale);
void message_log_apply_patch();
