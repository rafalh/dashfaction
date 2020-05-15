#pragma once

#include "../rf/graphics.h"

void InstallHealthArmorHudPatches();
void SetBigHealthArmorHud(bool is_big);
void InstallChatboxPatches();
void SetBigChatbox(bool is_big);
void InstallWeaponCycleHudPatches();
void SetBigWeaponCycleHud(bool is_big);
void InstallPersonaMsgHudPatches();
void SetBigPersonaMsgHud(bool is_big);
void GrScaledBitmap(int bmh, int x, int y, float scale, rf::GrRenderState render_state = rf::gr_bitmap_clamp_state);
void GrRectBorder(int x, int y, int w, int h, int border, rf::GrRenderState state = rf::gr_rect_state);
