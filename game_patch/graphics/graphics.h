#pragma once

#include "../rf/bmpman.h"

namespace rf
{
    struct Object;
}

void graphics_init();
void graphics_draw_fps_counter();
int get_default_font_id();
void set_default_font_id(int font_id);
void obj_mesh_lighting_alloc_one(rf::Object *objp);
void obj_mesh_lighting_update_one(rf::Object *objp);
void change_user_bitmap_format(int bmh, rf::BmFormat format, bool dynamic = false);
bool gr_begin_render_to_texture(int bmh);
void gr_end_render_to_texture();

template<typename F>
void run_with_default_font(int font_id, F fun)
{
    int old_font = get_default_font_id();
    set_default_font_id(font_id);
    fun();
    set_default_font_id(old_font);
}
