#pragma once

#include <patch_common/MemUtils.h>
#include "../bmpman.h"
#include "../common.h"

namespace rf
{
    enum GrTextureSource
    {
        TEXTURE_SOURCE_NONE = 0x0,
        TEXTURE_SOURCE_WRAP = 0x1,
        TEXTURE_SOURCE_CLAMP = 0x2,
        TEXTURE_SOURCE_CLAMP_NO_FILTERING = 0x3,
        TEXTURE_SOURCE_CLAMP_1_WRAP_0 = 0x4,
        TEXTURE_SOURCE_CLAMP_1_WRAP_0_MOD2X = 0x5,
        TEXTURE_SOURCE_CLAMP_1_CLAMP_0 = 0x6,
        TEXTURE_SOURCE_BUMPENV_MAP = 0x7,
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

    class GrMode
    {
        static constexpr int mask = 0x1F;
        static constexpr int ts_shift = 0;
        static constexpr int cs_shift = 5;
        static constexpr int as_shift = 10;
        static constexpr int ab_shift = 15;
        static constexpr int zbt_shift = 20;
        static constexpr int ft_shift = 25;

        int value;

    public:
        constexpr GrMode() : value(0) {}
        constexpr GrMode(GrTextureSource ts, GrColorSource cs, GrAlphaSource as, GrAlphaBlend ab, GrZbufferType zbt, GrFogType ft) :
            value((ts << ts_shift) | (cs << cs_shift) | (as << as_shift) | (ab << ab_shift) | (zbt << zbt_shift) | (ft << ft_shift))
        {}

        bool operator==(const GrMode& other) const
        {
            return value == other.value;
        }

        bool operator!=(const GrMode& other) const
        {
            return value != other.value;
        }

        operator int() const
        {
            return value;
        }

        inline GrTextureSource get_texture_source()
        {
            return static_cast<GrTextureSource>((value >> ts_shift) & mask);
        }

        inline GrColorSource get_color_source() const
        {
            return static_cast<GrColorSource>((value >> cs_shift) & mask);
        }

        inline GrAlphaSource get_alpha_source() const
        {
            return static_cast<GrAlphaSource>((value >> as_shift) & mask);
        }

        inline GrAlphaBlend get_alpha_blend() const
        {
            return static_cast<GrAlphaBlend>((value >> ab_shift) & mask);
        }

        inline GrZbufferType get_zbuffer_type() const
        {
            return static_cast<GrZbufferType>((value >> zbt_shift) & mask);
        }

        inline GrFogType get_fog_type() const
        {
            return static_cast<GrFogType>((value >> ft_shift) & mask);
        }

        inline void set_texture_source(GrTextureSource ts)
        {
            value = (value & ~(mask << ts_shift)) | (static_cast<int>(ts) << ts_shift);
        }

        inline void set_color_source(GrColorSource cs)
        {
            value = (value & ~(mask << cs_shift)) | (static_cast<int>(cs) << cs_shift);
        }

        inline void set_alpha_source(GrAlphaSource as)
        {
            value = (value & ~(mask << as_shift)) | (static_cast<int>(as) << as_shift);
        }

        inline void set_alpha_blend(GrAlphaBlend ab)
        {
            value = (value & ~(mask << ab_shift)) | (static_cast<int>(ab) << ab_shift);
        }

        inline void set_zbuffer_type(GrZbufferType zbt)
        {
            value = (value & ~(mask << zbt_shift)) | (static_cast<int>(zbt) << zbt_shift);
        }

        inline void set_fog_type(GrFogType ft)
        {
            value = (value & ~(mask << ft_shift)) | (static_cast<int>(ft) << ft_shift);
        }
    };
    static_assert(sizeof(GrMode) == 4);

    enum GrDepthbuffer
    {
        GR_DEPTHBUFFER_Z = 0x0,
        GR_DEPTHBUFFER_W = 0x1,
        GR_DEPTHBUFFER_NONE = 0x2,
    };

    struct GrScreen
    {
        int signature;
        int max_w;
        int max_h;
        int mode;
        int window_mode;
        bool dual_monitors;
        float aspect;
        int rowsize;
        int bits_per_pixel;
        int bytes_per_pixel;
        float far_zvalue;
        int offset_x;
        int offset_y;
        int clip_width;
        int clip_height;
        int max_bitmap_w;
        int max_bitmap_h;
        int clip_left;
        int clip_right;
        int clip_top;
        int clip_bottom;
        Color current_color;
        int current_texture_1;
        int current_texture_2;
        bool fog_mode;
        Color fog_color;
        float fog_near;
        float fog_far;
        float fog_far_scaled;
        bool force_tint;
        float tint_r;
        float tint_g;
        float tint_b;
        bool sync_to_retrace;
        float zbias;
        GrDepthbuffer depthbuffer_type;
    };
    static_assert(sizeof(GrScreen) == 0x90);

    enum GrScreenMode
    {
        GR_NONE = 0,
        GR_DIRECT3D = 0x66,
    };

    enum GrWindowMode
    {
        GR_FULLSCREEN = 0xC9,
        GR_WINDOWED = 0xC8,
    };

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

    enum TMapperFlags
    {
        TMAP_FLAG_TEXTURED = 1,
        TMAP_FLAG_RGB = 4,
        TMAP_FLAG_ALPHA = 8,
    };

    static auto& gr_screen = addr_as_ref<GrScreen>(0x017C7BC0);
    static auto& gr_gamma_ramp = addr_as_ref<uint32_t[256]>(0x017C7C68);
    static auto& gr_default_wfar = addr_as_ref<float>(0x00596140);

    static auto& gr_bitmap_clamp_mode = addr_as_ref<GrMode>(0x017756BC);
    static auto& gr_rect_mode = addr_as_ref<GrMode>(0x017756C0);
    static auto& gr_bitmap_wrap_mode = addr_as_ref<GrMode>(0x017756DC);
    static auto& gr_line_mode = addr_as_ref<GrMode>(0x01775B00);

    static auto& gr_view_matrix = addr_as_ref<Matrix3>(0x018186C8);
    static auto& gr_view_pos = addr_as_ref<Vector3>(0x01818690);
    static auto& gr_light_matrix = addr_as_ref<Matrix3>(0x01818A38);
    static auto& gr_light_base = addr_as_ref<Vector3>(0x01818A28);

    static auto& gr_screen_width = addr_as_ref<int()>(0x0050C640);
    static auto& gr_screen_height = addr_as_ref<int()>(0x0050C650);
    static auto& gr_clip_width = addr_as_ref<int()>(0x0050CDB0);
    static auto& gr_clip_height = addr_as_ref<int()>(0x0050CDC0);
    static auto& gr_set_alpha = addr_as_ref<void(ubyte a)>(0x0050D030);
    static auto& gr_read_backbuffer = addr_as_ref<int(int x, int y, int w, int h, void* buffer)>(0x0050DFF0);
    static auto& gr_clear = addr_as_ref<void()>(0x0050CDF0);
    static auto& gr_cull_sphere = addr_as_ref<bool(const Vector3& pos, float radius)>(0x005186A0);
    static auto& gr_set_texture_mip_filter = addr_as_ref<void(bool linear)>(0x0050E830);
    static auto& gr_lock = addr_as_ref<bool(int bm_handle, int section, GrLockInfo* lock, GrLockMode mode)>(0x0050E2E0);
    static auto& gr_unlock = addr_as_ref<void(GrLockInfo* lock)>(0x0050E310);
    static auto& gr_set_clip = addr_as_ref<void(int x, int y, int w, int h)>(0x0050CC60);
    static auto& gr_get_clip = addr_as_ref<void(int* x, int* y, int* w, int* h)>(0x0050CD80);
    static auto& gr_page_in = addr_as_ref<int(int bm_handle)>(0x0050CE00);
    static auto& gr_tcache_add_ref = addr_as_ref<void(int bm_handle)>(0x0050E850);
    static auto& gr_close = addr_as_ref<void()>(0x0050CBE0);
    static auto& gr_set_texture = addr_as_ref<void (int bitmap_handle, int bitmap_handle2)>(0x0050D060);
    static auto& gr_tmapper = addr_as_ref<void (int nv, Vertex **verts, TMapperFlags vertex_attributes, GrMode mode)>(0x0050DF80);

    inline void gr_set_color(ubyte r, ubyte g, ubyte b, ubyte a = gr_screen.current_color.alpha)
    {
        AddrCaller{0x0050CF80}.c_call(r, g, b, a);
    }

    inline void gr_set_color(const Color& clr)
    {
        AddrCaller{0x0050D000}.c_call(&clr);
    }

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

    inline void gr_bitmap_ex(int bm_handle, int x, int y, int w, int h, int sx, int sy, GrMode mode = gr_bitmap_wrap_mode)
    {
        AddrCaller{0x0050D0A0}.c_call(bm_handle, x, y, w, h, sx, sy, mode);
    }

    inline void gr_bitmap_stretched(int bm_handle, int x, int y, int w, int h, int sx, int sy, int sw, int sh, bool flip_x = false, bool flip_y = false, GrMode mode = gr_bitmap_wrap_mode)
    {
        AddrCaller{0x0050D250}.c_call(bm_handle, x, y, w, h, sx, sy, sw, sh, flip_x, flip_y, mode);
    }
}
