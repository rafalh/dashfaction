#pragma once

#include "../../rf/bmpman.h"

bool gr_d3d_is_texture_format_supported(rf::bm::Format format);
bool gr_d3d_set_render_target(int bm_handle);
void gr_d3d_update_window_mode();
void gr_d3d_update_texture_filtering();
void gr_d3d_apply_patch();
void gr_d3d_texture_apply_patch();
void gr_d3d_capture_apply_patch();
void gr_d3d_gamma_apply_patch();
void gr_d3d_bitmap_float(int bitmap_handle, float x, float y, float w, float h, float sx, float sy, float sw, float sh, bool flip_x, bool flip_y, rf::gr::Mode mode);
