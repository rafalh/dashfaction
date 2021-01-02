#pragma once

#include "../rf/bmpman.h"

namespace rf
{
    struct Object;
}

void graphics_init();
void graphics_after_game_init();
void graphics_draw_fps_counter();
int get_default_font_id();
void set_default_font_id(int font_id);
void obj_mesh_lighting_alloc_one(rf::Object *objp);
void obj_mesh_lighting_update_one(rf::Object *objp);
void bm_change_format(int bm_handle, rf::BmFormat format);
bool gr_render_to_texture(int bm_handle);
void gr_render_to_back_buffer();
void gr_delete_texture(int bm_handle);
void bm_set_dynamic(int bm_handle, bool dynamic);
bool bm_is_dynamic(int bm_handle);

template<typename F>
void run_with_default_font(int font_id, F fun)
{
    int old_font = get_default_font_id();
    set_default_font_id(font_id);
    fun();
    set_default_font_id(old_font);
}
