#include <windows.h>
#include <d3d8.h>
#include "gr_color.h"
#include "graphics_internal.h"
#include "../main.h"
#include "../rf/graphics.h"
#include "../rf/gr_direct3d.h"
#include <common/utils/com-utils.h>
#include <cstddef>
#include <cstring>
#include <common/BuildConfig.h>
#include <patch_common/CallHook.h>
#include <patch_common/FunHook.h>
#include <patch_common/CodeInjection.h>
#include <patch_common/AsmWriter.h>
#include <patch_common/ComPtr.h>

const char g_screenshot_dir_name[] = "screenshots";

static ComPtr<IDirect3DSurface8> g_capture_tmp_surface;
static ComPtr<IDirect3DSurface8> g_depth_stencil_surface;
static std::unique_ptr<byte* []> g_screenshot_scanlines_buf;
static int g_screenshot_dir_id;

bool g_render_to_texture_active = false;
ComPtr<IDirect3DSurface8> g_orig_render_target;
ComPtr<IDirect3DSurface8> g_orig_depth_stencil_surface;

IDirect3DSurface8* get_cached_depth_stencil_surface()
{
    if (!g_depth_stencil_surface) {
        auto hr = rf::gr_d3d_device->CreateDepthStencilSurface(
            rf::gr_d3d_pp.BackBufferWidth, rf::gr_d3d_pp.BackBufferHeight, rf::gr_d3d_pp.AutoDepthStencilFormat,
            D3DMULTISAMPLE_NONE, &g_depth_stencil_surface);
        if (FAILED(hr)) {
            ERR_ONCE("IDirect3DDevice8::CreateDepthStencilSurface failed 0x%lX", hr);
            return nullptr;
        }
    }
    return g_depth_stencil_surface;
}

bool gr_begin_render_to_texture(int bmh)
{
    // Note: texture reference counter is not increased here so ComPtr is not used
    IDirect3DTexture8* d3d_tex = rf::gr_d3d_get_texture(bmh);
    if (!d3d_tex) {
        WARN_ONCE("Bitmap without D3D texture provided in gr_begin_render_to_texture");
        return false;
    }

    ComPtr<IDirect3DSurface8> orig_render_target;
    ComPtr<IDirect3DSurface8> orig_depth_stencil_surface;

    if (!g_render_to_texture_active) {
        auto hr = rf::gr_d3d_device->GetRenderTarget(&orig_render_target);
        if (FAILED(hr)) {
            ERR_ONCE("IDirect3DDevice8::GetRenderTarget failed 0x%lX", hr);
            return false;
        }

        hr = rf::gr_d3d_device->GetDepthStencilSurface(&orig_depth_stencil_surface);
        if (FAILED(hr)) {
            ERR_ONCE("IDirect3DDevice8::GetDepthStencilSurface failed 0x%lX", hr);
            return false;
        }
    }

    ComPtr<IDirect3DSurface8> tex_surface;
    auto hr = d3d_tex->GetSurfaceLevel(0, &tex_surface);
    if (FAILED(hr)) {
        ERR_ONCE("IDirect3DTexture8::GetSurfaceLevel failed 0x%lX", hr);
        return false;
    }

    auto depth_stencil = get_cached_depth_stencil_surface();
    hr = rf::gr_d3d_device->SetRenderTarget(tex_surface, depth_stencil);
    if (FAILED(hr)) {
        ERR_ONCE("IDirect3DDevice8::SetRenderTarget failed 0x%lX", hr);
        return false;
    }

    if (!g_render_to_texture_active) {
        g_orig_render_target = orig_render_target;
        g_orig_depth_stencil_surface = orig_depth_stencil_surface;
        g_render_to_texture_active = true;
    }
    return true;
}

void gr_end_render_to_texture()
{
    if (!g_render_to_texture_active) {
        return;
    }
    auto hr = rf::gr_d3d_device->SetRenderTarget(g_orig_render_target, g_orig_depth_stencil_surface);
    if (FAILED(hr)) {
        ERR_ONCE("IDirect3DDevice8::SetRenderTarget failed 0x%lX", hr);
    }
    g_orig_render_target.release();
    g_orig_depth_stencil_surface.release();
    g_render_to_texture_active = false;
}

#if !D3D_LOCKABLE_BACKBUFFER
CallHook<rf::BmFormat(int, int, int, int, std::byte*)> gr_d3d_read_back_buffer_hook{
    0x0050E015,
    [](int x, int y, int width, int height, std::byte* buffer) {
        rf::gr_d3d_flush_buffers();

        ComPtr<IDirect3DSurface8> back_buffer;
        HRESULT hr = rf::gr_d3d_device->GetRenderTarget(&back_buffer);
        if (FAILED(hr)) {
            ERR_ONCE("IDirect3DDevice8::GetRenderTarget failed 0x%lX", hr);
            return rf::BM_FORMAT_NONE;
        }

        D3DSURFACE_DESC desc;
        hr = back_buffer->GetDesc(&desc);
        if (FAILED(hr)) {
            ERR_ONCE("IDirect3DSurface8::GetDesc failed 0x%lX", hr);
            return rf::BM_FORMAT_NONE;
        }

        // function is sometimes called with all parameters set to 0 to get backbuffer format
        xlog::info("gr_d3d_read_back_buffer_hook");
        rf::BmFormat pixel_fmt = get_bm_format_from_d3d_format(desc.Format);
        if (width == 0 || height == 0) {
            return pixel_fmt;
        }

        // According to Windows performance tests surface created with CreateImageSurface is faster than one from
        // CreateRenderTarget or CreateTexture.
        // Note: it can be slower during resource allocation so create it once
        if (!g_capture_tmp_surface) {
            hr = rf::gr_d3d_device->CreateImageSurface(desc.Width, desc.Height, desc.Format, &g_capture_tmp_surface);
            if (FAILED(hr)) {
                ERR_ONCE("IDirect3DDevice8::CreateImageSurface failed 0x%lX", hr);
                return rf::BM_FORMAT_NONE;
            }
        }

        RECT src_rect;
        POINT dst_pt{x, y};
        SetRect(&src_rect, x, y, x + width - 1, y + height - 1);

        hr = rf::gr_d3d_device->CopyRects(back_buffer, &src_rect, 1, g_capture_tmp_surface, &dst_pt);
        if (FAILED(hr)) {
            ERR_ONCE("IDirect3DDevice8::CopyRects failed 0x%lX", hr);
            return rf::BM_FORMAT_NONE;
        }

        // Note: locking fragment of Render Target fails
        D3DLOCKED_RECT locked_rect;
        hr = g_capture_tmp_surface->LockRect(&locked_rect, nullptr, D3DLOCK_READONLY | D3DLOCK_NO_DIRTY_UPDATE);
        if (FAILED(hr)) {
            ERR_ONCE("IDirect3DSurface8::LockRect failed 0x%lX (%s)", hr, get_d3d_error_str(hr));
            return rf::BM_FORMAT_NONE;
        }

        int bytes_per_pixel = bm_bytes_per_pixel(pixel_fmt);
        std::byte* src_ptr =
            reinterpret_cast<std::byte*>(locked_rect.pBits) + y * locked_rect.Pitch + x * bytes_per_pixel;
        std::byte* dst_ptr = buffer;

        for (int i = 0; i < height; ++i) {
            std::memcpy(dst_ptr, src_ptr, width * bytes_per_pixel);
            src_ptr += locked_rect.Pitch;
            dst_ptr += width * bytes_per_pixel;
        }
        g_capture_tmp_surface->UnlockRect();

        xlog::trace("GrReadBackBufferHook (%d %d %d %d) returns %d", x, y, width, height, pixel_fmt);
        return pixel_fmt;
    },
};
#endif // D3D_LOCKABLE_BACKBUFFER

FunHook<void(int, int, int, int, int)> gr_capture_back_buffer_hook{
    0x0050E4F0,
    [](int x, int y, int width, int height, int bm_handle) {
        if (g_render_to_texture_active) {
            // Nothing to do because we render directly to texture
            if (rf::gr_screen.mode == rf::GR_DIRECT3D) {
                rf::gr_d3d_flush_buffers();
            }
        } else {
            gr_capture_back_buffer_hook.call_target(x, y, width, height, bm_handle);
        }
    },
};

CodeInjection d3d_device_lost_patch{
    0x00545042,
    []() {
        void monitor_refresh_all();

        xlog::trace("D3D device lost");
        g_depth_stencil_surface.release();
        // Note: g_capture_tmp_surface is in D3DPOOL_SYSTEMMEM so no need to release here
        gr_delete_all_default_pool_textures();
        monitor_refresh_all();
    },
};

CodeInjection d3d_cleanup_patch{
    0x0054527A,
    []() {
        g_depth_stencil_surface.release();
        g_capture_tmp_surface.release();
    },
};

CodeInjection screenshot_scanlines_array_overflow_fix1{
    0x0055A066,
    [](auto& regs) {
        g_screenshot_scanlines_buf = std::make_unique<byte* []>(rf::gr_screen.max_height);
        regs.ecx = reinterpret_cast<int32_t>(g_screenshot_scanlines_buf.get());
        regs.eip = 0x0055A06D;
    },
};

CodeInjection screenshot_scanlines_array_overflow_fix2{
    0x0055A0DF,
    [](auto& regs) {
        regs.eax = reinterpret_cast<int32_t>(g_screenshot_scanlines_buf.get());
        regs.eip = 0x0055A0E6;
    },
};

void graphics_capture_init()
{
#if !D3D_LOCKABLE_BACKBUFFER
    /* Override default because IDirect3DSurface8::LockRect fails on multisampled back-buffer */
    gr_d3d_read_back_buffer_hook.install();
#endif

    // Use fast GrCaptureBackBuffer implementation which copies backbuffer to texture without copying from VRAM to RAM
    gr_capture_back_buffer_hook.install();
    d3d_device_lost_patch.install();
    d3d_cleanup_patch.install();

    // Override screenshot directory
    write_mem_ptr(0x004367CA + 2, &g_screenshot_dir_id);

    // Fix buffer overflow in screenshot to JPG conversion code
    screenshot_scanlines_array_overflow_fix1.install();
    screenshot_scanlines_array_overflow_fix2.install();

    // Make sure scanner bitmap is a render target
    write_mem<u8>(0x004A34BF + 1, rf::BM_FORMAT_RENDER_TARGET);
}

void graphics_capture_after_game_init()
{
    auto full_path = string_format("%s\\%s", rf::root_path, g_screenshot_dir_name);
    if (CreateDirectoryA(full_path.c_str(), nullptr))
        xlog::info("Created screenshots directory");
    else if (GetLastError() != ERROR_ALREADY_EXISTS)
        xlog::error("Failed to create screenshots directory %lu", GetLastError());
    g_screenshot_dir_id = rf::file_add_path(g_screenshot_dir_name, "", true);
}
