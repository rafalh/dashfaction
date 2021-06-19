#pragma once

#include <patch_common/MemUtils.h>
#include "os/vtypes.h"

namespace rf::bm
{
    enum Format
    {
        // All formats are listed from left to right, most-significant bit to least-significant bit as in D3DFORMAT
        FORMAT_NONE = 0x0,
        FORMAT_8_PALETTED = 0x1,
        FORMAT_8_ALPHA = 0x2,
        FORMAT_565_RGB = 0x3,
        FORMAT_4444_ARGB = 0x4,
        FORMAT_1555_ARGB = 0x5,
        FORMAT_888_RGB = 0x6,
        FORMAT_8888_ARGB = 0x7,
        FORMAT_88_BUMPDUDV = 0x8, // not supported by D3D routines
#ifdef DASH_FACTION
        // custom Dash Faction formats
        FORMAT_888_BGR = 0x9,        // used by lightmaps
        FORMAT_RENDER_TARGET = 0x10, // texture is used as render target
        FORMAT_DXT1 = 0x11,
        FORMAT_DXT2 = 0x12,
        FORMAT_DXT3 = 0x13,
        FORMAT_DXT4 = 0x14,
        FORMAT_DXT5 = 0x15,
#endif
    };

    enum Type
    {
        TYPE_NONE = 0x0,
        TYPE_PCX = 0x1,
        TYPE_TGA = 0x2,
        TYPE_USER = 0x3,
        TYPE_VAF = 0x4,
        TYPE_VBM = 0x5,
        TYPE_M2V = 0x6,
#ifdef DASH_FACTION
        // Custom Dash Faction bitmap types
        TYPE_DDS = 0x10,
#endif
    };

    struct BitmapEntry
    {
        char name[32];
        int name_checksum;
        int handle;
        ushort orig_width;
        ushort orig_height;
        ushort width;
        ushort height;
        int num_pixels_in_all_levels;
        Type bm_type;
        int animated_entry_type;
        Format format;
        ubyte num_levels;
        ubyte orig_num_levels;
        ubyte num_levels_in_ext_files;
        ubyte num_frames;
        ubyte vbm_version;
        void* locked_data;
        float frames_per_ms;
        void* locked_palette;
        BitmapEntry* next;
        BitmapEntry* prev;
        char field_5C;
        ubyte cached_material_idx;
#ifdef DASH_FACTION
        bool dynamic;
#endif
        int total_bytes_for_all_levels;
        int file_open_unk_arg;
        int resolution_level;
    };
    static_assert(sizeof(BitmapEntry) == 0x6C);

    static auto& load = addr_as_ref<int(const char *filename, int a2, bool generate_mipmaps)>(0x0050F6A0);
    static auto& create = addr_as_ref<int(Format format, int w, int h)>(0x005119C0);
    static auto& convert_format = addr_as_ref<void(void *dst_bits, Format dst_fmt, const void *src_bits, Format src_fmt, int num_pixels)>(0x0055DD20);
    static auto& get_dimensions = addr_as_ref<void(int bm_handle, int *w, int *h)>(0x00510630);
    static auto& get_filename = addr_as_ref<const char*(int bm_handle)>(0x00511710);
    static auto& get_format = addr_as_ref<Format(int bm_handle)>(0x005106F0);
    static auto& get_type = addr_as_ref<Type(int bm_handle)>(0x0050F350);
    static auto& get_cache_slot = addr_as_ref<int(int bm_handle)>(0x0050F440);
    static auto& lock = addr_as_ref<Format(int handle, ubyte **data, ubyte **pal)>(0x00510780);
    static auto& unlock = addr_as_ref<void(int)>(0x00511700);

    static auto& bitmaps = addr_as_ref<BitmapEntry*>(0x017C80C4);
}
