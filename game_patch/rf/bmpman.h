#pragma once

#include <patch_common/MemUtils.h>

namespace rf
{
    enum BmFormat
    {
        // All formats are listed from left to right, most-significant bit to least-significant bit as in D3DFORMAT
        BM_FORMAT_INVALID = 0x0,
        BM_FORMAT_BGR_888_INDEXED = 0x1,
        BM_FORMAT_A_8 = 0x2,
        BM_FORMAT_RGB_565 = 0x3,
        BM_FORMAT_ARGB_4444 = 0x4,
        BM_FORMAT_ARGB_1555 = 0x5,
        BM_FORMAT_RGB_888 = 0x6,
        BM_FORMAT_ARGB_8888 = 0x7,
        BM_FORMAT_UNK_8_16BIT = 0x8, // not supported by D3D routines
#ifdef DASH_FACTION
        // custom Dash Faction formats
        BM_FORMAT_BGR_888 = 0x9,        // used by lightmaps
        BM_FORMAT_RENDER_TARGET = 0x10, // texture is used as render target
#endif
    };

    enum BmType
    {
        BM_TYPE_INVALID = 0x0,
        BM_TYPE_PCX = 0x1,
        BM_TYPE_TGA = 0x2,
        BM_TYPE_USERBMAP = 0x3,
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
        BmType bitmap_type;
        int animated_entry_type;
        BmFormat pixel_format;
        char num_levels;
        char orig_num_levels;
        char num_levels_in_ext_files;
        char num_frames;
        char vbm_version;
        void* locked_data;
        float frames_per_ms;
        void* locked_palette;
        struct BmBitmapEntry* next;
        struct BmBitmapEntry* prev;
        char field_5C;
        char cached_material_idx;
        int16_t field_5E;
        int total_bytes_for_all_levels;
        int file_open_unk_arg;
        int resolution_level;
    };
    static_assert(sizeof(BmBitmapEntry) == 0x6C);

    static auto& BmLoad = AddrAsRef<int(const char *filename, int a2, bool generate_mipmaps)>(0x0050F6A0);
    static auto& BmCreate = AddrAsRef<int(BmFormat format, int w, int h)>(0x005119C0);
    static auto& BmConvertFormat = AddrAsRef<void(void *dst_bits, BmFormat dst_fmt, const void *src_bits, BmFormat src_fmt, int num_pixels)>(0x0055DD20);
    static auto& BmGetDimension = AddrAsRef<void(int bm_handle, int *w, int *h)>(0x00510630);
    static auto& BmGetFilename = AddrAsRef<const char*(int bm_handle)>(0x00511710);
    static auto& BmGetFormat = AddrAsRef<BmFormat(int bm_handle)>(0x005106F0);
    static auto& BmHandleToIdxAnimAware = AddrAsRef<int(int bm_handle)>(0x0050F440);

    static auto& bm_bitmaps = AddrAsRef<BmBitmapEntry*>(0x017C80C4);
}
