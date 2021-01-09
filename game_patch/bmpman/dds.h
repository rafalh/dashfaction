#pragma once

#include "../rf/bmpman.h"

rf::BmType read_dds_header(rf::File& file, int *width_out, int *height_out, rf::BmFormat *format_out,
    int *num_levels_out);
int lock_dds_bitmap(rf::BmBitmapEntry& bm_entry);
