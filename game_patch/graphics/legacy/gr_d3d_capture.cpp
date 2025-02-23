#include <windows.h>
#include <d3d8.h>
#include <cstddef>
#include <cstring>
#include <common/error/d3d-error.h>
#include <common/config/BuildConfig.h>
#include <patch_common/CallHook.h>
#include <patch_common/FunHook.h>
#include <patch_common/AsmWriter.h>
#include <common/ComPtr.h>
#include "../../rf/gr/gr.h"
#include "../../rf/gr/gr_direct3d.h"
#include "../../main/main.h"
#include "../../bmpman/bmpman.h"
#include "../gr_internal.h"
#include "gr_d3d_internal.h"

static ComPtr<IDirect3DSurface8> g_capture_tmp_surface;
static ComPtr<IDirect3DSurface8> g_depth_stencil_surface;

bool g_render_to_texture_active = false;
ComPtr<IDirect3DSurface8> g_orig_render_target;
ComPtr<IDirect3DSurface8> g_orig_depth_stencil_surface;

IDirect3DSurface8* get_cached_depth_stencil_surface()
{
    if (!g_depth_stencil_surface) {
        auto hr = rf::gr::d3d::device->CreateDepthStencilSurface(
            rf::gr::d3d::pp.BackBufferWidth, rf::gr::d3d::pp.BackBufferHeight, rf::gr::d3d::pp.AutoDepthStencilFormat,
            D3DMULTISAMPLE_NONE, &g_depth_stencil_surface);
        if (FAILED(hr)) {
            ERR_ONCE("IDirect3DDevice8::CreateDepthStencilSurface failed: {}", get_d3d_error_str(hr));
            return nullptr;
        }
    }
    return g_depth_stencil_surface;
}

bool gr_d3d_set_render_target(int bmh)
{
    if (rf::gr::d3d::buffers_locked) {
        rf::gr::d3d::flush_buffers();
    }

    if (bmh == -1) {
        if (!g_render_to_texture_active) {
            return true;
        }
        auto hr = rf::gr::d3d::device->SetRenderTarget(g_orig_render_target, g_orig_depth_stencil_surface);
        if (FAILED(hr)) {
            ERR_ONCE("IDirect3DDevice8::SetRenderTarget failed: {}", get_d3d_error_str(hr));
        }
        g_orig_render_target.release();
        g_orig_depth_stencil_surface.release();
        g_render_to_texture_active = false;
        return true;
    }

    // Note: texture reference counter is not increased here so ComPtr is not used
    IDirect3DTexture8* d3d_tex = rf::gr::d3d::get_texture(bmh);
    if (!d3d_tex) {
        WARN_ONCE("Bitmap without D3D texture provided in gr_render_to_texture");
        return false;
    }

    ComPtr<IDirect3DSurface8> orig_render_target;
    ComPtr<IDirect3DSurface8> orig_depth_stencil_surface;

    if (!g_render_to_texture_active) {
        auto hr = rf::gr::d3d::device->GetRenderTarget(&orig_render_target);
        if (FAILED(hr)) {
            ERR_ONCE("IDirect3DDevice8::GetRenderTarget failed: {}", get_d3d_error_str(hr));
            return false;
        }

        hr = rf::gr::d3d::device->GetDepthStencilSurface(&orig_depth_stencil_surface);
        if (FAILED(hr)) {
            ERR_ONCE("IDirect3DDevice8::GetDepthStencilSurface failed: {}", get_d3d_error_str(hr));
            return false;
        }
    }

    ComPtr<IDirect3DSurface8> tex_surface;
    auto hr = d3d_tex->GetSurfaceLevel(0, &tex_surface);
    if (FAILED(hr)) {
        ERR_ONCE("IDirect3DTexture8::GetSurfaceLevel failed: {}", get_d3d_error_str(hr));
        return false;
    }

    IDirect3DSurface8* depth_stencil = get_cached_depth_stencil_surface();
    hr = rf::gr::d3d::device->SetRenderTarget(tex_surface, depth_stencil);
    if (FAILED(hr)) {
        ERR_ONCE("IDirect3DDevice8::SetRenderTarget failed: {}", get_d3d_error_str(hr));
        return false;
    }

    if (!g_render_to_texture_active) {
        g_orig_render_target = orig_render_target;
        g_orig_depth_stencil_surface = orig_depth_stencil_surface;
        g_render_to_texture_active = true;
    }
    return true;
}

#if !D3D_LOCKABLE_BACKBUFFER
CallHook<rf::bm::Format(int, int, int, int, std::byte*)> gr_d3d_read_back_buffer_hook{
    0x0050E015,
    [](int x, int y, int width, int height, std::byte* buffer) {
        rf::gr::d3d::flush_buffers();

        ComPtr<IDirect3DSurface8> back_buffer;
        HRESULT hr = rf::gr::d3d::device->GetRenderTarget(&back_buffer);
        if (FAILED(hr)) {
            ERR_ONCE("IDirect3DDevice8::GetRenderTarget failed: {}", get_d3d_error_str(hr));
            return rf::bm::FORMAT_NONE;
        }

        D3DSURFACE_DESC desc;
        hr = back_buffer->GetDesc(&desc);
        if (FAILED(hr)) {
            ERR_ONCE("IDirect3DSurface8::GetDesc failed: {}", get_d3d_error_str(hr));
            return rf::bm::FORMAT_NONE;
        }

        // function is sometimes called with all parameters set to 0 to get backbuffer format
        rf::bm::Format pixel_fmt = get_bm_format_from_d3d_format(desc.Format);
        if (width == 0 || height == 0) {
            return pixel_fmt;
        }

        // According to Windows performance tests surface created with CreateImageSurface is faster than one from
        // CreateRenderTarget or CreateTexture.
        // Note: it can be slower during resource allocation so create it once
        if (!g_capture_tmp_surface) {
            hr = rf::gr::d3d::device->CreateImageSurface(desc.Width, desc.Height, desc.Format, &g_capture_tmp_surface);
            if (FAILED(hr)) {
                ERR_ONCE("IDirect3DDevice8::CreateImageSurface failed: {}", get_d3d_error_str(hr));
                return rf::bm::FORMAT_NONE;
            }
        }

        RECT src_rect;
        POINT dst_pt{x, y};
        SetRect(&src_rect, x, y, x + width - 1, y + height - 1);

        hr = rf::gr::d3d::device->CopyRects(back_buffer, &src_rect, 1, g_capture_tmp_surface, &dst_pt);
        if (FAILED(hr)) {
            ERR_ONCE("IDirect3DDevice8::CopyRects failed: {}", get_d3d_error_str(hr));
            return rf::bm::FORMAT_NONE;
        }

        // Note: locking fragment of Render Target fails
        D3DLOCKED_RECT locked_rect;
        hr = g_capture_tmp_surface->LockRect(&locked_rect, nullptr, D3DLOCK_READONLY | D3DLOCK_NO_DIRTY_UPDATE);
        if (FAILED(hr)) {
            ERR_ONCE("IDirect3DSurface8::LockRect failed: {}", get_d3d_error_str(hr));
            return rf::bm::FORMAT_NONE;
        }

        int bytes_per_pixel = bm_bytes_per_pixel(pixel_fmt);
        std::byte* src_ptr =
            static_cast<std::byte*>(locked_rect.pBits) + y * locked_rect.Pitch + x * bytes_per_pixel;
        std::byte* dst_ptr = buffer;

        for (int i = 0; i < height; ++i) {
            std::memcpy(dst_ptr, src_ptr, width * bytes_per_pixel);
            src_ptr += locked_rect.Pitch;
            dst_ptr += width * bytes_per_pixel;
        }
        g_capture_tmp_surface->UnlockRect();

        xlog::trace("GrReadBackBufferHook ({} {} {} {}) returns {}", x, y, width, height, pixel_fmt);
        return pixel_fmt;
    },
};
#endif // D3D_LOCKABLE_BACKBUFFER

void gr_d3d_capture_device_lost()
{
    g_depth_stencil_surface.release();
    // Note: g_capture_tmp_surface is in D3DPOOL_SYSTEMMEM so no need to release here
}

void gr_d3d_capture_close()
{
    g_depth_stencil_surface.release();
    g_capture_tmp_surface.release();
}

void gr_d3d_capture_apply_patch()
{
#if !D3D_LOCKABLE_BACKBUFFER
    // Override default because IDirect3DSurface8::LockRect fails on multisampled back-buffer
    gr_d3d_read_back_buffer_hook.install();
#endif
}
