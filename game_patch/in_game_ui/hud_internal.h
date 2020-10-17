#pragma once

#include "../rf/graphics.h"

namespace rf
{
    struct HudPoint;
}

void InstallHealthArmorHudPatches();
void SetBigHealthArmorHud(bool is_big);
void InstallChatboxPatches();
void SetBigChatbox(bool is_big);
void InstallWeaponCycleHudPatches();
void SetBigWeaponCycleHud(bool is_big);
void InstallPersonaMsgHudPatches();
void SetBigPersonaMsgHud(bool is_big);
void ApplyTeamScoresHudPatches();
void SetBigTeamScoresHud(bool is_big);
void HudScaledBitmap(int bmh, int x, int y, float scale, rf::GrMode mode = rf::gr_bitmap_clamp_mode);
void HudPreloadScaledBitmap(int bmh);
void HudRectBorder(int x, int y, int w, int h, int border, rf::GrMode state = rf::gr_rect_mode);
std::string HudFitString(std::string_view str, int max_w, int* str_w_out, int font_id);
rf::HudPoint HudScaleCoords(rf::HudPoint pt, float scale);
const char* HudGetDefaultFontName(bool big);
const char* HudGetBoldFontName(bool big);
