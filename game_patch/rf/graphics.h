#pragma once

#include <patch_common/MemUtils.h>
#include "bmpman.h"
#include "common.h"

namespace rf
{
    /* Graphics */

    enum GrTextureSource
    {
        TEXTURE_SOURCE_NONE = 0x0,
        TEXTURE_SOURCE_WRAP = 0x1,
        TEXTURE_SOURCE_CLAMP = 0x2,
        TEXTURE_SOURCE_CLAMP_NO_FILTERING = 0x3,
        TEXTURE_SOURCE_MT_WRAP = 0x4,
        TEXTURE_SOURCE_MT_WRAP_M2X = 0x5,
        TEXTURE_SOURCE_MT_CLAMP = 0x6,
        TEXTURE_SOURCE_7 = 0x7,
        TEXTURE_SOURCE_MT_U_WRAP_V_CLAMP = 0x8,
        TEXTURE_SOURCE_MT_U_CLAMP_V_WRAP = 0x9,
        TEXTURE_SOURCE_MT_WRAP_TRILIN = 0xA,
        TEXTURE_SOURCE_MT_CLAMP_TRILIN = 0xB,
    };

    enum GrColorSource
    {
        COLOR_SOURCE_VERTEX = 0x0,
        COLOR_SOURCE_TEXTURE = 0x1,
        COLOR_SOURCE_VERTEX_TIMES_TEXTURE = 0x2,
        COLOR_SOURCE_VERTEX_PLUS_TEXTURE = 0x3,
        COLOR_SOURCE_VERTEX_TIMES_TEXTURE_2X = 0x4,
    };

    enum GrAlphaSource
    {
        ALPHA_SOURCE_VERTEX = 0x0,
        ALPHA_SOURCE_VERTEX_NONDARKENING = 0x1,
        ALPHA_SOURCE_TEXTURE = 0x2,
        ALPHA_SOURCE_VERTEX_TIMES_TEXTURE = 0x3,
    };

    enum GrAlphaBlend
    {
        ALPHA_BLEND_NONE = 0x0,
        ALPHA_BLEND_ADDITIVE = 0x1,
        ALPHA_BLEND_ALPHA_ADDITIVE = 0x2,
        ALPHA_BLEND_ALPHA = 0x3,
        ALPHA_BLEND_SRC_COLOR = 0x4,
        ALPHA_BLEND_LIGHTMAP = 0x5,
        ALPHA_BLEND_SUBTRACTIVE = 0x6,
        ALPHA_BLEND_SWAPPED_SRC_DEST_COLOR = 0x7,
    };

    enum GrZbufferType
    {
        ZBUFFER_TYPE_NONE = 0x0,
        ZBUFFER_TYPE_READ = 0x1,
        ZBUFFER_TYPE_READ_EQUAL = 0x2,
        ZBUFFER_TYPE_WRITE = 0x3,
        ZBUFFER_TYPE_FULL = 0x4,
        ZBUFFER_TYPE_FULL_ALPHA_TEST = 0x5,
    };

    enum GrFogType
    {
        FOG_ALLOWED = 0x0,
        FOG_ALLOWED_ADD2 = 0x1,
        FOG_ALLOWED_MULT2 = 0x2,
        FOG_NOT_ALLOWED = 0x3,
    };

    struct GrMode
    {
        int value;

        constexpr GrMode(GrTextureSource ts, GrColorSource cs, GrAlphaSource as, GrAlphaBlend ab, GrZbufferType zbt, GrFogType ft) :
            value(ts | 32 * (cs | 32 * (as | 32 * (ab | 32 * (zbt | 32 * ft)))))
        {}

        bool operator==(const GrMode& other) const
        {
            return value == other.value;
        }
    };

    struct GrScreen
    {
        int signature;
        int max_width;
        int max_height;
        int mode;
        int window_mode;
        int field_14;
        float aspect;
        int field_1c;
        int bits_per_pixel;
        int bytes_ber_pixel;
        int field_28;
        int offset_x;
        int offset_y;
        int clip_width;
        int clip_height;
        int max_tex_width;
        int max_tex_height;
        int clip_left;
        int clip_right;
        int clip_top;
        int clip_bottom;
        int current_color;
        int current_bitmap;
        int current_bitmap2;
        int fog_mode;
        int fog_color;
        float fog_near;
        float fog_far;
        float fog_far_scaled;
        bool recolor_enabled;
        float recolor_red;
        float recolor_green;
        float recolor_blue;
        int field_84;
        int field_88;
        int zbuffer_mode;
    };
    static_assert(sizeof(GrScreen) == 0x90);

    enum GrScreenMode
    {
        GR_NONE = 0,
        GR_DIRECT3D = 102,
    };

    struct GrVertex
    {
        Vector3 pos; // world or clip space
        Vector3 spos; // screen space
        uint8_t codes;
        uint8_t flags;
        uint8_t unk0;
        uint8_t unk1;
        float u0;
        float v0;
        float u1;
        float v1;
        int color;
    };
    static_assert(sizeof(GrVertex) == 0x30);

    enum GrLockMode
    {
        GR_LOCK_READ_ONLY = 0,
        GR_LOCK_READ_ONLY_WRITE = 1,
        GR_LOCK_WRITE_ONLY = 2,
    };

    struct GrLockInfo
    {
        int bm_handle;
        int section;
        BmFormat format;
        uint8_t *data;
        int w;
        int h;
        int stride_in_bytes;
        GrLockMode mode;
    };
    static_assert(sizeof(GrLockInfo) == 0x20);

    enum GrTextAlignment
    {
        GR_ALIGN_LEFT = 0,
        GR_ALIGN_CENTER = 1,
        GR_ALIGN_RIGHT = 2,
    };

    struct GrFontKernEntry
    {
        char ch0;
        char ch1;
        char offset;
    };

    struct GrFontCharInfo
    {
        int spacing;
        int width;
        int pixel_data_offset;
        short kerning_entry;
        short user_data;
    };

    struct GrFont
    {
        char filename[32];
        int signature;
        int format;
        int num_chars;
        int first_ascii;
        int default_spacing;
        int height;
        int height2;
        int num_kern_pairs;
        int pixel_data_size;
        GrFontKernEntry *kern_data;
        GrFontCharInfo *chars;
        unsigned char *pixel_data;
        int *palette;
        int bm_handle;
        int bm_width;
        int bm_height;
        int *char_x_coords;
        int *char_y_coords;
        unsigned char *bm_bits;
    };

    static auto& gr_screen = addr_as_ref<GrScreen>(0x017C7BC0);
    static auto& gr_gamma_ramp = addr_as_ref<uint32_t[256]>(0x017C7C68);

    static auto& gr_bitmap_clamp_mode = addr_as_ref<GrMode>(0x017756BC);
    static auto& gr_rect_mode = addr_as_ref<GrMode>(0x017756C0);
    static auto& gr_bitmap_wrap_mode = addr_as_ref<GrMode>(0x017756DC);
    static auto& gr_line_mode = addr_as_ref<GrMode>(0x01775B00);
    static auto& gr_string_mode = addr_as_ref<GrMode>(0x017C7C5C);

    static auto& current_fps = addr_as_ref<float>(0x005A4018);
    static auto& frametime = addr_as_ref<float>(0x005A4014);
    static auto& framerate_min = addr_as_ref<float>(0x005A4024);

    static auto& gr_default_wfar = addr_as_ref<float>(0x00596140);

    static constexpr int max_fonts = 12;
    static auto& gr_fonts = addr_as_ref<GrFont[max_fonts]>(0x01886C90);
    static auto& gr_num_fonts = addr_as_ref<int>(0x018871A0);

    static auto& gr_screen_width = addr_as_ref<int()>(0x0050C640);
    static auto& gr_screen_height = addr_as_ref<int()>(0x0050C650);
    static auto& gr_clip_width = addr_as_ref<int()>(0x0050CDB0);
    static auto& gr_clip_height = addr_as_ref<int()>(0x0050CDC0);
    static auto& gr_set_color_rgba = addr_as_ref<void(unsigned r, unsigned g, unsigned b, unsigned a)>(0x0050CF80);
    static auto& gr_set_color = addr_as_ref<void(const Color& clr)>(0x0050D000);
    static auto& gr_set_alpha = addr_as_ref<void(int a)>(0x0050D030);
    static auto& gr_read_backbuffer = addr_as_ref<int(int x, int y, int w, int h, void* buffer)>(0x0050DFF0);
    static auto& gr_clear = addr_as_ref<void()>(0x0050CDF0);
    static auto& gr_cull_sphere = addr_as_ref<bool(Vector3& pos, float radius)>(0x005186A0);
    static auto& gr_set_texture_mip_filter = addr_as_ref<void(bool linear)>(0x0050E830);
    static auto& gr_lock = addr_as_ref<bool(int bm_handle, int section, GrLockInfo* lock, GrLockMode mode)>(0x0050E2E0);
    static auto& gr_unlock = addr_as_ref<void(GrLockInfo* lock)>(0x0050E310);
    static auto& gr_set_clip = addr_as_ref<void(int x, int y, int w, int h)>(0x0050CC60);
    static auto& gr_get_clip = addr_as_ref<void(int* x, int* y, int* w, int* h)>(0x0050CD80);
    static auto& gr_preload_bitmap = addr_as_ref<int(int bm_handle)>(0x0050CE00);
    static auto& gr_tcache_add_ref = addr_as_ref<void(int bm_handle)>(0x0050E850);

    static constexpr int center_x = 0x8000; // supported by gr_string

    inline void gr_line(float x0, float y0, float x1, float y1, GrMode mode = gr_line_mode)
    {
        AddrCaller{0x0050D770}.c_call(x0, y0, x1, y1, mode);
    }

    inline void gr_rect(int x, int y, int cx, int cy, GrMode mode = gr_rect_mode)
    {
        AddrCaller{0x0050DBE0}.c_call(x, y, cx, cy, mode);
    }

    inline void gr_bitmap(int bm_handle, int x, int y, GrMode mode = gr_bitmap_wrap_mode)
    {
        AddrCaller{0x0050D2A0}.c_call(bm_handle, x, y, mode);
    }

    inline void gr_bitmap_ex(int bm_handle, int x, int y, int w, int h, int src_x, int src_y, GrMode mode = gr_bitmap_wrap_mode)
    {
        AddrCaller{0x0050D0A0}.c_call(bm_handle, x, y, w, h, src_x, src_y, mode);
    }

    inline void gr_bitmap_stretched(int bm_handle, int x, int y, int w, int h, int sx, int sy, int sw, int sh, bool a10 = false, bool a11 = false, GrMode mode = gr_bitmap_wrap_mode)
    {
        AddrCaller{0x0050D250}.c_call(bm_handle, x, y, w, h, sx, sy, sw, sh, a10, a11, mode);
    }

    inline void gr_string(int x, int y, const char *s, int font_num = -1, GrMode mode = gr_string_mode)
    {
        AddrCaller{0x0051FEB0}.c_call(x, y, s, font_num, mode);
    }

    inline void gr_string_aligned(GrTextAlignment align, int x, int y, const char *s, int font_num = -1, GrMode state = gr_string_mode)
    {
        AddrCaller{0x0051FE50}.c_call(align, x, y, s, font_num, state);
    }

    inline int gr_load_font(const char *file_name, int a2 = -1)
    {
        return AddrCaller{0x0051F6E0}.c_call<int>(file_name, a2);
    }

    inline int gr_get_font_height(int font_num = -1)
    {
        return AddrCaller{0x0051F4D0}.c_call<int>(font_num);
    }

    static auto& gr_split_str = addr_as_ref<int(int *len_array, int *offset_array, char *s, int max_w, int max_lines, char unk_char, int font_num)>(0x00520810);
    static auto& gr_get_string_size = addr_as_ref<void(int *out_w, int *out_h, const char *s, int s_len, int font_num)>(0x0051F530);
    static auto& gr_set_default_font = addr_as_ref<bool(const char *file_name)>(0x0051FE20);
}
