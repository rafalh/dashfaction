#pragma once

#include "../rf/bmpman.h"

rf::bm::Type
read_dds_header(rf::File& file, int* width_out, int* height_out, rf::bm::Format* format_out, int* num_levels_out);
int lock_dds_bitmap(rf::bm::BitmapEntry& bm_entry);
