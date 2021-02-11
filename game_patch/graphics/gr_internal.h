#pragma once

void gr_d3d_texture_device_lost();
void gr_d3d_texture_init();
bool gr_d3d_is_d3d8to9();
void gr_d3d_gamma_reset();
void gr_d3d_delete_texture(int bm_handle);
void gr_font_apply_patch();
void gr_light_apply_patch();
void gr_d3d_apply_patch();
void gr_d3d_texture_apply_patch();
void gr_d3d_capture_apply_patch();
void gr_d3d_gamma_apply_patch();
void bink_apply_patch();
