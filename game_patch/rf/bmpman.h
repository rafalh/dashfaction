#pragma once

#include <patch_common/MemUtils.h>

namespace rf
{
    enum BmPixelFormat
    {
        // All formats are listed from left to right, most-significant bit to least-significant bit as in D3DFORMAT
        BMPF_INVALID = 0x0,
        BMPF_BGR_888_INDEXED = 0x1,
        BMPF_A_8 = 0x2,
        BMPF_RGB_565 = 0x3,
        BMPF_RGBA_4444 = 0x4,
        BMPF_RGBA_1555 = 0x5,
        BMPF_RGB_888 = 0x6,
        BMPF_RGBA_8888 = 0x7,
        BMPF_UNK_8_16BIT = 0x8, // not supported by D3D routines
    };

    enum BmBitmapType
    {
        BM_INVALID = 0x0,
        BM_PCX = 0x1,
        BM_TGA = 0x2,
        BM_USERBMAP = 0x3,
        BM_VAF = 0x4,
        BM_VBM = 0x5,
        BM_M2V = 0x6,
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
        BmBitmapType bitmap_type;
        int animated_entry_type;
        BmPixelFormat pixel_format;
        char num_levels;
        char orig_num_levels;
        char num_levels_in_ext_files;
        char num_frames;
        char vbm_version;
        void* locked_data;
        float frames_per_ms;
        void* locked_palette;
        struct BmBitmapEntry* prev_bitmap;
        struct BmBitmapEntry* next_bitmap;
        char field_5C;
        char cached_material_idx;
        int16_t field_5E;
        int total_bytes_for_all_levels;
        int file_open_unk_arg;
        int resolution_level;
    };
    static_assert(sizeof(BmBitmapEntry) == 0x6C);

    static auto& BmLoad = AddrAsRef<int(const char *filename, int a2, bool generate_mipmaps)>(0x0050F6A0);
    static auto& BmCreateUserBmap = AddrAsRef<int(BmPixelFormat pixel_format, int width, int height)>(0x005119C0);
    static auto& BmConvertFormat = AddrAsRef<void(void *dst_bits, BmPixelFormat dst_pixel_fmt, const void *src_bits, BmPixelFormat src_pixel_fmt, int num_pixels)>(0x0055DD20);
    static auto& BmGetBitmapSize = AddrAsRef<void(int bm_handle, int *width, int *height)>(0x00510630);
    static auto& BmGetFilename = AddrAsRef<const char*(int bm_handle)>(0x00511710);
    static auto& BmGetPixelFormat = AddrAsRef<BmPixelFormat(int bm_handle)>(0x005106F0);
    static auto& BmHandleToIdxAnimAware = AddrAsRef<int(int bm_handle)>(0x0050F440);

    static auto& bm_bitmaps = AddrAsRef<BmBitmapEntry*>(0x017C80C4);
}
