#pragma once

void GraphicsInit();
void GraphicsDrawFpsCounter();
int GetDefaultFontId();
void SetDefaultFontId(int font_id);

template<typename F>
void RunWithDefaultFont(int font_id, F fun)
{
    int old_font = GetDefaultFontId();
    SetDefaultFontId(font_id);
    fun();
    SetDefaultFontId(old_font);
}
