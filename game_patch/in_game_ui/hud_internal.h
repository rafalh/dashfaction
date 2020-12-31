#pragma once

#include "../rf/graphics.h"

namespace rf
{
    struct HudPoint;
}

void hud_status_apply_patches();
void hud_status_set_big(bool is_big);
void multi_hud_chat_apply_patches();
void multi_hud_chat_set_big(bool is_big);
void hud_weapon_cycle_apply_patches();
void hud_weapon_cycle_set_big(bool is_big);
void hud_persona_msg_apply_patches();
void hud_persona_msg_set_big(bool is_big);
void hud_team_scores_apply_patches();
void hud_team_scores_set_big(bool is_big);
void hud_scaled_bitmap(int bmh, int x, int y, float scale, rf::GrMode mode = rf::gr_bitmap_clamp_mode);
void hud_preload_scaled_bitmap(int bmh);
void hud_rect_border(int x, int y, int w, int h, int border, rf::GrMode state = rf::gr_rect_mode);
std::string hud_fit_string(std::string_view str, int max_w, int* str_w_out, int font_id);
rf::HudPoint hud_scale_coords(rf::HudPoint pt, float scale);
const char* hud_get_default_font_name(bool big);
const char* hud_get_bold_font_name(bool big);
void scoreboard_maybe_render(bool show_scoreboard);
