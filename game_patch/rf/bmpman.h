#pragma once

#include <patch_common/MemUtils.h>

namespace rf
{
    enum BmFormat
    {
        // All formats are listed from left to right, most-significant bit to least-significant bit as in D3DFORMAT
        BM_FORMAT_NONE = 0x0,
        BM_FORMAT_8_PALETTED = 0x1,
        BM_FORMAT_8_ALPHA = 0x2,
        BM_FORMAT_565_RGB = 0x3,
        BM_FORMAT_4444_ARGB = 0x4,
        BM_FORMAT_1555_ARGB = 0x5,
        BM_FORMAT_888_RGB = 0x6,
        BM_FORMAT_8888_ARGB = 0x7,
        BM_FORMAT_88_BUMPDUDV = 0x8, // not supported by D3D routines
#ifdef DASH_FACTION
        // custom Dash Faction formats
        BM_FORMAT_888_BGR = 0x9,        // used by lightmaps
        BM_FORMAT_RENDER_TARGET = 0x10, // texture is used as render target
        BM_FORMAT_DXT1 = 0x11,
        BM_FORMAT_DXT2 = 0x12,
        BM_FORMAT_DXT3 = 0x13,
        BM_FORMAT_DXT4 = 0x14,
        BM_FORMAT_DXT5 = 0x15,
#endif
    };

    enum BmType
    {
        BM_TYPE_NONE = 0x0,
        BM_TYPE_PCX = 0x1,
        BM_TYPE_TGA = 0x2,
        BM_TYPE_USER = 0x3,
        BM_TYPE_VAF = 0x4,
        BM_TYPE_VBM = 0x5,
        BM_TYPE_M2V = 0x6,
#ifdef DASH_FACTION
        // Custom Dash Faction bitmap types
        BM_TYPE_DDS = 0x10,
#endif
    };

    struct BmBitmapEntry
    {
        char name[32];
        int name_checksum;
        int handle;
        int16_t orig_width;
        int16_t orig_height;
        int16_t width;
        int16_t height;
        int num_pixels_in_all_levels;
        BmType bm_type;
        int animated_entry_type;
        BmFormat format;
        char num_levels;
        char orig_num_levels;
        char num_levels_in_ext_files;
        char num_frames;
        char vbm_version;
        void* locked_data;
        float frames_per_ms;
        void* locked_palette;
        BmBitmapEntry* next;
        BmBitmapEntry* prev;
        char field_5C;
        char cached_material_idx;
        int total_bytes_for_all_levels;
        int file_open_unk_arg;
        int resolution_level;
    };
    static_assert(sizeof(BmBitmapEntry) == 0x6C);

    static auto& bm_load = addr_as_ref<int(const char *filename, int a2, bool generate_mipmaps)>(0x0050F6A0);
    static auto& bm_create = addr_as_ref<int(BmFormat format, int w, int h)>(0x005119C0);
    static auto& bm_convert_format = addr_as_ref<void(void *dst_bits, BmFormat dst_fmt, const void *src_bits, BmFormat src_fmt, int num_pixels)>(0x0055DD20);
    static auto& bm_get_dimensions = addr_as_ref<void(int bm_handle, int *w, int *h)>(0x00510630);
    static auto& bm_get_filename = addr_as_ref<const char*(int bm_handle)>(0x00511710);
    static auto& bm_get_format = addr_as_ref<BmFormat(int bm_handle)>(0x005106F0);
    static auto& bm_handle_to_index_anim_aware = addr_as_ref<int(int bm_handle)>(0x0050F440);

    static auto& bm_bitmaps = addr_as_ref<BmBitmapEntry*>(0x017C80C4);
}
