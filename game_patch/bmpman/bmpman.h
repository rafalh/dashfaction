#pragma once

#include <xlog/xlog.h>
#include "../rf/bmpman.h"
#include "../rf/gr/gr.h"

void bm_set_dynamic(int bm_handle, bool dynamic);
bool bm_is_dynamic(int bm_handle);
void bm_change_format(int bm_handle, rf::bm::Format format);
void bm_apply_patch();
bool bm_is_compressed_format(rf::bm::Format format);
bool bm_convert_format(void* dst_bits_ptr, rf::bm::Format dst_fmt, const void* src_bits_ptr,
                       rf::bm::Format src_fmt, int width, int height, int dst_pitch, int src_pitch,
                       const uint8_t* palette = nullptr);
rf::Color bm_get_pixel(uint8_t* data, rf::bm::Format format, int stride_in_bytes, int x, int y);
size_t bm_calculate_total_bytes(int w, int h, rf::bm::Format format);
int bm_calculate_pitch(int w, rf::bm::Format format);
int bm_calculate_rows(int h, rf::bm::Format format);

inline int bm_bytes_per_pixel(rf::bm::Format format)
{
    switch (format) {
    case rf::bm::FORMAT_8_PALETTED:
    case rf::bm::FORMAT_8_ALPHA:
        return 1;
    case rf::bm::FORMAT_565_RGB:
    case rf::bm::FORMAT_4444_ARGB:
    case rf::bm::FORMAT_1555_ARGB:
        return 2;
    case rf::bm::FORMAT_888_RGB:
        return 3;
    case rf::bm::FORMAT_8888_ARGB:
        return 4;
    default:
        xlog::warn("Unknown format in bm_bytes_per_pixel: {}", static_cast<int>(format));
        return 2;
    }
}

inline int bm_get_bytes_per_compressed_block(rf::bm::Format format)
{
    switch (format) {
        case rf::bm::FORMAT_DXT1:
            return 8;
        case rf::bm::FORMAT_DXT2:
        case rf::bm::FORMAT_DXT3:
        case rf::bm::FORMAT_DXT4:
        case rf::bm::FORMAT_DXT5:
            return 16;
        default:
            xlog::error("Unsupported format in bm_get_bytes_per_compressed_block: {}", static_cast<int>(format));
            return 0;
    }
}
