#pragma once

#include <patch_common/MemUtils.h>
#include "../bmpman.h"
#include "../os/vtypes.h"
#include "../math/vector.h"
#include "../math/matrix.h"

namespace rf::gr
{
    struct Color
    {
        ubyte red;
        ubyte green;
        ubyte blue;
        ubyte alpha;

        constexpr Color(ubyte r, ubyte g, ubyte b, ubyte a = 255) :
            red(r), green(g), blue(b), alpha(a) {}

        void set(ubyte r, ubyte g, ubyte b, ubyte a)
        {
            this->red = r;
            this->green = g;
            this->blue = b;
            this->alpha = a;
        }

        bool operator==(const Color& other) const = default;
    };

    struct BaseVertex
    {
        Vector3 world_pos; // world or clip space
        float sx; // screen space
        float sy;
        float sw;
        ubyte codes;
        ubyte flags;
    };

    struct Vertex : BaseVertex
    {
        float u1;
        float v1;
        float u2;
        float v2;
        ubyte r;
        ubyte g;
        ubyte b;
        ubyte a;
    };
    static_assert(sizeof(Vertex) == 0x30);

    enum TextureSource
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

    enum ColorSource
    {
        COLOR_SOURCE_VERTEX = 0x0,
        COLOR_SOURCE_TEXTURE = 0x1,
        COLOR_SOURCE_VERTEX_TIMES_TEXTURE = 0x2,
        COLOR_SOURCE_VERTEX_PLUS_TEXTURE = 0x3,
        COLOR_SOURCE_VERTEX_TIMES_TEXTURE_2X = 0x4,
    };

    enum AlphaSource
    {
        ALPHA_SOURCE_VERTEX = 0x0,
        ALPHA_SOURCE_VERTEX_NONDARKENING = 0x1,
        ALPHA_SOURCE_TEXTURE = 0x2,
        ALPHA_SOURCE_VERTEX_TIMES_TEXTURE = 0x3,
    };

    enum AlphaBlend
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

    enum ZbufferType
    {
        ZBUFFER_TYPE_NONE = 0x0,
        ZBUFFER_TYPE_READ = 0x1,
        ZBUFFER_TYPE_READ_EQUAL = 0x2,
        ZBUFFER_TYPE_WRITE = 0x3,
        ZBUFFER_TYPE_FULL = 0x4,
        ZBUFFER_TYPE_FULL_ALPHA_TEST = 0x5,
    };

    enum FogType
    {
        FOG_ALLOWED = 0x0,
        FOG_ALLOWED_ADD2 = 0x1,
        FOG_ALLOWED_MULT2 = 0x2,
        FOG_NOT_ALLOWED = 0x3,
    };

    class Mode
    {
        static constexpr int mask = 0x1F;
        static constexpr int ts_shift = 0;
        static constexpr int cs_shift = 5;
        static constexpr int as_shift = 10;
        static constexpr int ab_shift = 15;
        static constexpr int zbt_shift = 20;
        static constexpr int ft_shift = 25;

        int value = 0;

    public:
        constexpr Mode() = default;
        constexpr Mode(TextureSource ts, ColorSource cs, AlphaSource as, AlphaBlend ab, ZbufferType zbt, FogType ft) :
            value((ts << ts_shift) | (cs << cs_shift) | (as << as_shift) | (ab << ab_shift) | (zbt << zbt_shift) | (ft << ft_shift))
        {}

        [[nodiscard]] bool operator==(const Mode& other) const = default;

        operator int() const
        {
            return value;
        }

        [[nodiscard]] inline TextureSource get_texture_source() const
        {
            return static_cast<TextureSource>((value >> ts_shift) & mask);
        }

        [[nodiscard]] inline ColorSource get_color_source() const
        {
            return static_cast<ColorSource>((value >> cs_shift) & mask);
        }

        [[nodiscard]] inline AlphaSource get_alpha_source() const
        {
            return static_cast<AlphaSource>((value >> as_shift) & mask);
        }

        [[nodiscard]] inline AlphaBlend get_alpha_blend() const
        {
            return static_cast<AlphaBlend>((value >> ab_shift) & mask);
        }

        [[nodiscard]] inline ZbufferType get_zbuffer_type() const
        {
            return static_cast<ZbufferType>((value >> zbt_shift) & mask);
        }

        [[nodiscard]] inline FogType get_fog_type() const
        {
            return static_cast<FogType>((value >> ft_shift) & mask);
        }

        inline void set_texture_source(TextureSource ts)
        {
            value = (value & ~(mask << ts_shift)) | (static_cast<int>(ts) << ts_shift);
        }

        inline void set_color_source(ColorSource cs)
        {
            value = (value & ~(mask << cs_shift)) | (static_cast<int>(cs) << cs_shift);
        }

        inline void set_alpha_source(AlphaSource as)
        {
            value = (value & ~(mask << as_shift)) | (static_cast<int>(as) << as_shift);
        }

        inline void set_alpha_blend(AlphaBlend ab)
        {
            value = (value & ~(mask << ab_shift)) | (static_cast<int>(ab) << ab_shift);
        }

        inline void set_zbuffer_type(ZbufferType zbt)
        {
            value = (value & ~(mask << zbt_shift)) | (static_cast<int>(zbt) << zbt_shift);
        }

        inline void set_fog_type(FogType ft)
        {
            value = (value & ~(mask << ft_shift)) | (static_cast<int>(ft) << ft_shift);
        }
    };
    static_assert(sizeof(Mode) == 4);

    enum Depthbuffer
    {
        DEPTHBUFFER_Z = 0x0,
        DEPTHBUFFER_W = 0x1,
        DEPTHBUFFER_NONE = 0x2,
    };

    struct Screen
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
        int zbias;
        Depthbuffer depthbuffer_type;
    };
    static_assert(sizeof(Screen) == 0x90);

    enum ScreenMode
    {
        NONE = 0,
        DIRECT3D = 0x66,
    };

    enum WindowMode
    {
        FULLSCREEN = 0xC9,
        WINDOWED = 0xC8,
    };

    enum LockMode
    {
        LOCK_READ_ONLY = 0,
        LOCK_READ_ONLY_WRITE = 1,
        LOCK_WRITE_ONLY = 2,
    };

    struct LockInfo
    {
        int bm_handle;
        int section;
        bm::Format format;
        uint8_t *data;
        int w;
        int h;
        int stride_in_bytes;
        LockMode mode;
    };
    static_assert(sizeof(LockInfo) == 0x20);

    enum TMapperFlags
    {
        TMAP_FLAG_TEXTURED = 1,
        TMAP_FLAG_TEXTURED_2 = 2,
        TMAP_FLAG_RGB = 4,
        TMAP_FLAG_ALPHA = 8,
    };

    enum VertexFlags
    {
        VF_PROJECTED = 1,
    };

    static auto& screen = addr_as_ref<Screen>(0x017C7BC0);
    static auto& gamma_ramp = addr_as_ref<uint32_t[256]>(0x017C7C68);
    static auto& default_wfar = addr_as_ref<float>(0x00596140);
    static auto& gamma = addr_as_ref<float>(0x005A445C);

    static auto& bitmap_clamp_mode = addr_as_ref<Mode>(0x017756BC);
    static auto& rect_mode = addr_as_ref<Mode>(0x017756C0);
    static auto& bitmap_wrap_mode = addr_as_ref<Mode>(0x017756DC);
    static auto& line_mode = addr_as_ref<Mode>(0x01775B00);

    static auto& view_matrix = addr_as_ref<Matrix3>(0x018186C8);
    static auto& view_pos = addr_as_ref<Vector3>(0x01818690);
    static auto& eye_pos = addr_as_ref<Vector3>(0x01818680);
    static auto& eye_matrix = addr_as_ref<Matrix3>(0x01818A00);
    static auto& light_matrix = addr_as_ref<Matrix3>(0x01818A38);
    static auto& light_base = addr_as_ref<Vector3>(0x01818A28);
    static auto& matrix_scale = addr_as_ref<Vector3>(0x01818B48);
    static auto& one_over_matrix_scale_z = addr_as_ref<float>(0x01818A60);
    static auto& projection_xadd = addr_as_ref<float>(0x01818B54);
    static auto& projection_xmul = addr_as_ref<float>(0x01818B58);
    static auto& projection_yadd = addr_as_ref<float>(0x01818B5C);
    static auto& projection_ymul = addr_as_ref<float>(0x01818B60);

    static auto& screen_width = addr_as_ref<int()>(0x0050C640);
    static auto& screen_height = addr_as_ref<int()>(0x0050C650);
    static auto& clip_width = addr_as_ref<int()>(0x0050CDB0);
    static auto& clip_height = addr_as_ref<int()>(0x0050CDC0);
    static auto& set_alpha = addr_as_ref<void(ubyte a)>(0x0050D030);
    static auto& read_backbuffer = addr_as_ref<int(int x, int y, int w, int h, void* buffer)>(0x0050DFF0);
    static auto& clear = addr_as_ref<void()>(0x0050CDF0);
    static auto& cull_sphere = addr_as_ref<bool(const Vector3& pos, float radius)>(0x005186A0);
    static auto& set_texture_mip_filter = addr_as_ref<void(bool linear)>(0x0050E830);
    static auto& lock = addr_as_ref<bool(int bm_handle, int section, LockInfo* lock, LockMode mode)>(0x0050E2E0);
    static auto& unlock = addr_as_ref<void(LockInfo* lock)>(0x0050E310);
    static auto& set_clip = addr_as_ref<void(int x, int y, int w, int h)>(0x0050CC60);
    static auto& get_clip = addr_as_ref<void(int* x, int* y, int* w, int* h)>(0x0050CD80);
    static auto& reset_clip = addr_as_ref<void()>(0x0050CDD0);
    static auto& page_in = addr_as_ref<int(int bm_handle)>(0x0050CE00);
    static auto& mark_texture_dirty = addr_as_ref<int(int bm_handle)>(0x0050D080);
    static auto& tcache_add_ref = addr_as_ref<void(int bm_handle)>(0x0050E850);
    static auto& tcache_remove_ref = addr_as_ref<void(int bm_handle)>(0x0050E870);
    static auto& close = addr_as_ref<void()>(0x0050CBE0);
    static auto& set_gamma = addr_as_ref<void(float)>(0x0050CE70);
    static auto& set_texture = addr_as_ref<void (int bitmap_handle, int bitmap_handle2)>(0x0050D060);
    static auto& tmapper = addr_as_ref<void (int nv, Vertex **verts, TMapperFlags vertex_attributes, Mode mode)>(0x0050DF80);
    static auto& lighting_enabled = addr_as_ref<bool()>(0x004DB8B0);
    static auto& cull_bounding_box = addr_as_ref<bool (const Vector3& mn, const Vector3& mx)>(0x00518750);
    static auto& poly = addr_as_ref<bool (int num, Vertex **vertices, TMapperFlags vertex_attributes, Mode mode, bool constant_sw, float sw)>(0x005159A0);
    static auto& rotate_vertex = addr_as_ref<ubyte (Vertex *vertex_out, const Vector3& vec_in)>(0x00518360);
    static auto& world_poly = addr_as_ref<bool (int bm_handle, int n_verts, const Vector3* verts, const Vector2* uvs, Mode mode, const Color& color)>(0x00517110);
    static auto& start_instance = addr_as_ref<void(const Vector3& pos, const Matrix3& orient)>(0x00517F00);
    static auto& stop_instance = addr_as_ref<void()>(0x00517F20);
    static auto& project_vertex = addr_as_ref<ubyte (Vertex *p)>(0x00518440);
    static auto& show_lightmaps = *reinterpret_cast<bool*>(0x009BB5A4);
    static auto& light_set_ambient = addr_as_ref<void(float r, float g, float b)>(0x004D8CE0);

    inline void set_color(ubyte r, ubyte g, ubyte b, ubyte a = screen.current_color.alpha)
    {
        AddrCaller{0x0050CF80}.c_call(r, g, b, a);
    }

    inline void set_color(const Color& clr)
    {
        AddrCaller{0x0050D000}.c_call(&clr);
    }

    inline void line(float x0, float y0, float x1, float y1, Mode mode = line_mode)
    {
        AddrCaller{0x0050D770}.c_call(x0, y0, x1, y1, mode);
    }

    inline void rect(int x, int y, int w, int h, Mode mode = rect_mode)
    {
        AddrCaller{0x0050DBE0}.c_call(x, y, w, h, mode);
    }

    inline void rect_border(int x, int y, int w, int h)
    {
        AddrCaller{0x0050DD80}.c_call(x, y, w, h);
    }

    inline void bitmap(int bm_handle, int x, int y, Mode mode = bitmap_wrap_mode)
    {
        AddrCaller{0x0050D2A0}.c_call(bm_handle, x, y, mode);
    }

    inline void bitmap_ex(int bm_handle, int x, int y, int w, int h, int sx, int sy, Mode mode = bitmap_wrap_mode)
    {
        AddrCaller{0x0050D0A0}.c_call(bm_handle, x, y, w, h, sx, sy, mode);
    }

    inline void bitmap_scaled(int bm_handle, int x, int y, int w, int h, int sx, int sy, int sw, int sh, bool flip_x = false, bool flip_y = false, Mode mode = bitmap_wrap_mode)
    {
        AddrCaller{0x0050D250}.c_call(bm_handle, x, y, w, h, sx, sy, sw, sh, flip_x, flip_y, mode);
    }
}

namespace rf
{
    using Color = gr::Color;
}
