#pragma once

#include "../rf/common.h"
#include "../rf/bmpman.h"
#include "../rf/file.h"

void gr_delete_all_default_pool_textures();
void init_supported_texture_formats();
size_t get_surface_length_in_bytes(int w, int h, rf::BmFormat format);

rf::BmType read_dds_header(rf::File& file, int *width_out, int *height_out, rf::BmFormat *format_out,
    int *num_levels_out);
int lock_dds_bitmap(rf::BmBitmapEntry& bm_entry);
void bm_apply_patches();
int get_surface_pitch(int w, rf::BmFormat format);
int get_surface_num_rows(int h, rf::BmFormat format);
bool bm_is_compressed_format(rf::BmFormat format);
bool gr_is_format_supported(rf::BmFormat format);
bool gr_d3d_is_d3d8to9();
rf::Color bm_get_pixel(uint8_t* data, rf::BmFormat format, int stride_in_bytes, int x, int y);
void reset_gamma_ramp();
