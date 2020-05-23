#pragma once

#include <patch_common/MemUtils.h>
#include "bmpman.h"
#include "common.h"

namespace rf
{
    /* Graphics */

    //using GrRenderState = int;

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

    enum GrColorOp
    {
        COLOR_OP_SEL_TEXTURE_UNK = 0x0,
        COLOR_OP_SEL_TEXTURE = 0x1,
        COLOR_OP_MUL = 0x2,
        COLOR_OP_ADD = 0x3,
        COLOR_OP_MUL2X = 0x4,
    };

    enum GrAlphaOp
    {
        ALPHA_OP_SEL_DIFFUSE = 0x0,
        ALPHA_OP_UNK = 0x1,
        ALPHA_OP_SEL_TEXTURE = 0x2,
        ALPHA_OP_MUL = 0x3,
    };

    enum GrAlphaBlend
    {
        ALPHA_BLEND_NONE = 0x0,
        ALPHA_BLEND_ADDITIVE = 0x1,
        ALPHA_BLEND_ADDITIVE_SRC_ALPHA_2 = 0x2,
        ALPHA_BLEND_DEFAULT = 0x3,
        ALPHA_BLEND_ADDITIVE_SRC_ALPHA_4 = 0x4,
        ALPHA_BLEND_DEST_COLOR = 0x5,
        ALPHA_BLEND_INV_DEST_COLOR = 0x6,
        ALPHA_BLEND_SWAPPED_SRC_DEST_COLOR = 0x7,
    };

    enum GrZbufferType
    {
        ZBUFFER_TYPE_NONE = 0x0,
        ZBUFFER_TYPE_READ = 0x1,
        ZBUFFER_TYPE_READ_EQ_FUNC = 0x2,
        ZBUFFER_TYPE_WRITE = 0x3,
        ZBUFFER_TYPE_FULL = 0x4,
        ZBUFFER_TYPE_FULL_ALPHA_TEST = 0x5,
    };

    enum GrFogType
    {
        FOG_TYPE_0 = 0x0,
        FOG_TYPE_1 = 0x1,
        FOG_TYPE_2 = 0x2,
        FOG_TYPE_FORCE_OFF = 0x3,
    };

    struct GrRenderState
    {
        int value;

        constexpr GrRenderState(GrTextureSource tex_src, GrColorOp clr_op, GrAlphaOp alpha_op, GrAlphaBlend alpha_blend, GrZbufferType zbuf_type, GrFogType fog) :
            value(tex_src | 32 * (clr_op | 32 * (alpha_op | 32 * (alpha_blend | 32 * (zbuf_type | 32 * fog)))))
        {}

        bool operator==(const GrRenderState& other) const
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
        float fog_far_scalled;
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

    struct GrLockData
    {
        int bm_handle;
        int section_idx;
        BmPixelFormat pixel_format;
        uint8_t *bits;
        int width;
        int height;
        int pitch;
        int field_1c;
    };
    static_assert(sizeof(GrLockData) == 0x20);

    enum GrTextAlignment
    {
        GR_ALIGN_LEFT = 0,
        GR_ALIGN_CENTER = 1,
        GR_ALIGN_RIGHT = 2,
    };

    struct GrD3DTextureSection
    {
#ifdef DIRECT3D_VERSION
        IDirect3DTexture8 *d3d_texture;
#else
        void *d3d_texture;
#endif
        int num_vram_bytes;
        float u_scale;
        float v_scale;
        int x;
        int y;
        int width;
        int height;
    };

    struct GrD3DTexture
    {
        int bm_handle;
        short num_sections;
        short preserve_counter;
        short lock_count;
        uint8_t ref_count;
        bool reset;
        GrD3DTextureSection *sections;
    };

#if defined(DIRECT3D_VERSION)
    static auto& gr_d3d = AddrAsRef<IDirect3D8*>(0x01CFCBE0);
    static auto& gr_d3d_device = AddrAsRef<IDirect3DDevice8*>(0x01CFCBE4);
    static auto& gr_d3d_pp = AddrAsRef<D3DPRESENT_PARAMETERS>(0x01CFCA18);
#elif defined(HRESULT)
    static auto& gr_d3d = AddrAsRef<IUnknown*>(0x01CFCBE0);
    static auto& gr_d3d_device = AddrAsRef<IUnknown*>(0x01CFCBE4);
#endif
    static auto& gr_adapter_idx = AddrAsRef<uint32_t>(0x01CFCC34);
    static auto& gr_screen = AddrAsRef<GrScreen>(0x017C7BC0);
    static auto& gr_gamma_ramp = AddrAsRef<uint32_t[256]>(0x017C7C68);

    static auto& gr_bitmap_clamp_state = AddrAsRef<GrRenderState>(0x017756BC);
    static auto& gr_rect_state = AddrAsRef<GrRenderState>(0x017756C0);
    static auto& gr_bitmap_wrap_state = AddrAsRef<GrRenderState>(0x017756DC);
    static auto& gr_line_state = AddrAsRef<GrRenderState>(0x01775B00);
    static auto& gr_string_state = AddrAsRef<GrRenderState>(0x017C7C5C);

    static auto& rfpc_large_font_id = AddrAsRef<int>(0x0063C05C);
    static auto& rfpc_medium_font_id = AddrAsRef<int>(0x0063C060);
    static auto& rfpc_small_font_id = AddrAsRef<int>(0x0063C068);
    static auto& big_font_id = AddrAsRef<int>(0x006C74C0);

    static auto& current_fps = AddrAsRef<float>(0x005A4018);
    static auto& frametime = AddrAsRef<float>(0x005A4014);
    static auto& min_framerate = AddrAsRef<float>(0x005A4024);

    static auto& gr_d3d_buffers_locked = AddrAsRef<bool>(0x01E652ED);
    static auto& gr_d3d_in_optimized_drawing_proc = AddrAsRef<bool>(0x01E652EE);
    static auto& gr_d3d_num_vertices = AddrAsRef<int>(0x01E652F0);
    static auto& gr_d3d_num_indices = AddrAsRef<int>(0x01E652F4);
    static auto& gr_d3d_max_hw_vertex = AddrAsRef<int>(0x01818348);
    static auto& gr_d3d_max_hw_index = AddrAsRef<int>(0x0181834C);
    static auto& gr_d3d_min_hw_vertex = AddrAsRef<int>(0x01E652F8);
    static auto& gr_d3d_min_hw_index = AddrAsRef<int>(0x01E652FC);
#ifdef DIRECT3D_VERSION
    static auto& gr_d3d_primitive_type = AddrAsRef<D3DPRIMITIVETYPE>(0x01D862A8);
#endif

    static auto& gr_scale_vec = AddrAsRef<Vector3>(0x01818B48);
    static auto& gr_view_matrix = AddrAsRef<Matrix3>(0x018186C8);
    static auto& gr_default_wfar = AddrAsRef<float>(0x00596140);
#ifdef DIRECT3D_VERSION
    static auto& gr_d3d_device_caps = AddrAsRef<D3DCAPS8>(0x01CFCAC8);
#endif

    static auto& GrGetMaxWidth = AddrAsRef<int()>(0x0050C640);
    static auto& GrGetMaxHeight = AddrAsRef<int()>(0x0050C650);
    static auto& GrGetClipWidth = AddrAsRef<int()>(0x0050CDB0);
    static auto& GrGetClipHeight = AddrAsRef<int()>(0x0050CDC0);
    static auto& GrSetColor = AddrAsRef<void(unsigned r, unsigned g, unsigned b, unsigned a)>(0x0050CF80);
    static auto& GrSetColorPtr = AddrAsRef<void(Color *color)>(0x0050D000);
    static auto& GrSetColorAlpha = AddrAsRef<void(int a)>(0x0050D030);
    static auto& GrReadBackBuffer = AddrAsRef<int(int x, int y, int width, int height, void *buffer)>(0x0050DFF0);
    static auto& GrD3DFlushBuffers = AddrAsRef<void()>(0x00559D90);
    static auto& GrClear = AddrAsRef<void()>(0x0050CDF0);
    static auto& GrIsSphereOutsideView = AddrAsRef<bool(rf::Vector3& pos, float radius)>(0x005186A0);
    static auto& GrSetTextureMipFilter = AddrAsRef<void(bool linear)>(0x0050E830);
    static auto& GrLock = AddrAsRef<char(int bmh, int section_idx, GrLockData *data, int a4)>(0x0050E2E0);
    static auto& GrUnlock = AddrAsRef<void(GrLockData *data)>(0x0050E310);
    static auto& GrSetClip = AddrAsRef<void(int x, int y, int w, int h)>(0x0050CC60);
    static auto& GrGetClip = AddrAsRef<void(int* x, int* y, int* w, int* h)>(0x0050CD80);
    static auto& GrTcacheAddRef = AddrAsRef<void(int bm_handle)>(0x0050E850);


    inline void GrLine(float x0, float y0, float x1, float y1, GrRenderState render_state = gr_line_state)
    {
        AddrCaller{0x0050D770}.c_call(x0, y0, x1, y1, render_state);
    }

    inline void GrRect(int x, int y, int cx, int cy, GrRenderState state = gr_rect_state)
    {
        AddrCaller{0x0050DBE0}.c_call(x, y, cx, cy, state);
    }

    inline void GrBitmap(int bmh, int x, int y, GrRenderState state = gr_bitmap_wrap_state)
    {
        AddrCaller{0x0050D2A0}.c_call(bmh, x, y, state);
    }

    inline void GrBitmapStretched(int bmh, int dst_x, int dst_y, int dst_w, int dst_h, int src_x, int src_y, int src_w, int src_h, bool a10 = false, bool a11 = false, GrRenderState state = gr_bitmap_wrap_state)
    {
        AddrCaller{0x0050D250}.c_call(bmh, dst_x, dst_y, dst_w, dst_h, src_x, src_y, src_w, src_h, a10, a11, state);
    }

    inline void GrString(int x, int y, const char *text, int font_num = -1, GrRenderState state = gr_string_state)
    {
        AddrCaller{0x0051FEB0}.c_call(x, y, text, font_num, state);
    }

    inline void GrStringAligned(GrTextAlignment align, int x, int y, const char *text, int font_num = -1, GrRenderState state = gr_string_state)
    {
        AddrCaller{0x0051FE50}.c_call(align, x, y, text, font_num, state);
    }

    inline int GrLoadFont(const char *file_name, int a2 = -1)
    {
        return AddrCaller{0x0051F6E0}.c_call<int>(file_name, a2);
    }

    inline int GrGetFontHeight(int font_num = -1)
    {
        return AddrCaller{0x0051F4D0}.c_call<int>(font_num);
    }

    static auto& GrFitMultilineText = AddrAsRef<int(int *len_array, int *offset_array, char *text, int max_width, int max_lines, char unk_char, int font_num)>(0x00520810);
    static auto& GrGetTextWidth = AddrAsRef<void(int *out_width, int *out_height, const char *text, int text_len, int font_num)>(0x0051F530);
    static auto& GrSetDefaultFont = AddrAsRef<bool(const char *file_name)>(0x0051FE20);
}
