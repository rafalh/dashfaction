#pragma once

#include <patch_common/MemUtils.h>
#ifndef NO_D3D8
#include <windef.h>
#include <d3d8.h>
#endif
#include "bmpman.h"
#include "common.h"

namespace rf
{
    /* Graphics */

    using GrRenderState = int;

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
        int viewport_width;
        int viewport_height;
        int max_tex_width_unk3_c;
        int max_tex_height_unk40;
        int clip_left;
        int clip_right;
        int clip_top;
        int clip_bottom;
        int current_color;
        int current_bitmap;
        int field_5c;
        int fog_mode;
        int fog_color;
        float fog_near;
        float fog_far;
        float field_70;
        int field_74;
        float field_78;
        float field_7c;
        float field_80;
        int field_84;
        int field_88;
        int field_8c;
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

#ifndef NO_D3D8
    static auto& gr_d3d = AddrAsRef<IDirect3D8*>(0x01CFCBE0);
    static auto& gr_d3d_device = AddrAsRef<IDirect3DDevice8*>(0x01CFCBE4);
    static auto& gr_d3d_pp = AddrAsRef<D3DPRESENT_PARAMETERS>(0x01CFCA18);
#else
    static auto& gr_d3d = AddrAsRef<IUnknown*>(0x01CFCBE0);
    static auto& gr_d3d_device = AddrAsRef<IUnknown*>(0x01CFCBE4);
#endif
    static auto& gr_adapter_idx = AddrAsRef<uint32_t>(0x01CFCC34);
    static auto& gr_screen = AddrAsRef<GrScreen>(0x017C7BC0);

    static auto& gr_bitmap_material = AddrAsRef<GrRenderState>(0x017756BC);
    static auto& gr_rect_material = AddrAsRef<GrRenderState>(0x017756C0);
    static auto& gr_image_material = AddrAsRef<GrRenderState>(0x017756DC);
    static auto& gr_line_material = AddrAsRef<GrRenderState>(0x01775B00);
    static auto& gr_text_material = AddrAsRef<GrRenderState>(0x017C7C5C);

    static auto& large_font_id = AddrAsRef<int>(0x0063C05C);
    static auto& medium_font_id = AddrAsRef<int>(0x0063C060);
    static auto& small_font_id = AddrAsRef<int>(0x0063C068);
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
    static auto& gr_d3d_primitive_type = AddrAsRef<D3DPRIMITIVETYPE>(0x01D862A8);

    static auto& GrGetMaxWidth = AddrAsRef<int()>(0x0050C640);
    static auto& GrGetMaxHeight = AddrAsRef<int()>(0x0050C650);
    static auto& GrGetViewportWidth = AddrAsRef<unsigned()>(0x0050CDB0);
    static auto& GrGetViewportHeight = AddrAsRef<unsigned()>(0x0050CDC0);
    static auto& GrSetColor = AddrAsRef<void(unsigned r, unsigned g, unsigned b, unsigned a)>(0x0050CF80);
    static auto& GrReadBackBuffer = AddrAsRef<int(int x, int y, int width, int height, void *buffer)>(0x0050DFF0);
    static auto& GrD3DFlushBuffers = AddrAsRef<void()>(0x00559D90);
    static auto& GrClear = AddrAsRef<void()>(0x0050CDF0);

    static auto& GrDrawRect = AddrAsRef<void(unsigned x, unsigned y, unsigned cx, unsigned cy, GrRenderState render_state)>(0x0050DBE0);
    static auto& GrDrawImage = AddrAsRef<void(int bm_handle, int x, int y, GrRenderState render_state)>(0x0050D2A0);
    static auto& GrDrawBitmapStretched = AddrAsRef<void(int bm_handle, int dst_x, int dst_y, int dst_w, int dst_h, int src_x, int src_y, int src_w, int src_h, float a10, float a11, GrRenderState render_state)>(0x0050D250);
    static auto& GrDrawText = AddrAsRef<void(unsigned x, unsigned y, const char *text, int font, GrRenderState render_state)>(0x0051FEB0);
    static auto& GrDrawAlignedText = AddrAsRef<void(GrTextAlignment align, unsigned x, unsigned y, const char *text, int font, GrRenderState render_state)>(0x0051FE50);

    static auto& GrFitText = AddrAsRef<String* (String* result, String::Pod str, int cx_max)>(0x00471EC0);
    static auto& GrLoadFont = AddrAsRef<int(const char *file_name, int a2)>(0x0051F6E0);
    static auto& GrGetFontHeight = AddrAsRef<unsigned(int font_id)>(0x0051F4D0);
    static auto& GrGetTextWidth = AddrAsRef<void(int *out_width, int *out_height, const char *text, int text_len, int font_id)>(0x0051F530);

    static auto& GrLock = AddrAsRef<char(int bm_handle, int section_idx, GrLockData *data, int a4)>(0x0050E2E0);
    static auto& GrUnlock = AddrAsRef<void(GrLockData *data)>(0x0050E310);
}
