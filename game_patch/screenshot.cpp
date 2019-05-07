#include "screenshot.h"
#include "gr_color.h"
#include "main.h"
#include "rf.h"
#include "stdafx.h"
#include "utils.h"
#include <cstddef>
#include <common/BuildConfig.h>
#include <patch_common/CallHook.h>
#include <patch_common/FunHook.h>
#include <patch_common/CodeInjection.h>
#include <patch_common/ComPtr.h>

const char g_screenshot_dir_name[] = "screenshots";

static std::unique_ptr<byte* []> g_screenshot_scanlines_buf;
static int g_screenshot_dir_id;
static bool g_force_texture_in_backbuffer_format = false;
static ComPtr<IDirect3DSurface8> g_render_target;
static ComPtr<IDirect3DSurface8> g_depth_stencil_surface;

namespace rf
{
struct GrTextureSlot
{
    IDirect3DTexture8* d3d_texture;
    int num_bytes_used_by_texture;
    float scale_unk_x;
    float scale_unk_y;
    int x;
    int y;
    int width;
    int height;
};
static_assert(sizeof(GrTextureSlot) == 0x20);

struct GrTexture
{
    int field_0;
    uint16_t num_sections;
    uint16_t field_6;
    uint16_t lock_count;
    uint8_t ref_count;
    char field_b;
    GrTextureSlot *sections;
};
static_assert(sizeof(GrTexture) == 0x10);

static auto& GrGetBitmapTexture = AddrAsRef<IDirect3DTexture8*(int bm_handle)>(0x0055D1E0);

} // namespace rf

#if !D3D_LOCKABLE_BACKBUFFER
CallHook<rf::BmPixelFormat(int, int, int, int, std::byte*)> GrD3DReadBackBuffer_hook{
    0x0050E015,
    [](int x, int y, int width, int height, std::byte* buffer) {
        // Note: function is sometimes called with all parameters set to 0 to get backbuffer format

        rf::GrFlushBuffers();

        ComPtr<IDirect3DSurface8> back_buffer;
        HRESULT hr = rf::gr_d3d_device->GetBackBuffer(0, D3DBACKBUFFER_TYPE_MONO, &back_buffer);
        if (FAILED(hr)) {
            ERR("IDirect3DDevice8::GetBackBuffer failed 0x%lX", hr);
            return rf::BMPF_INVALID;
        }

        D3DSURFACE_DESC desc;
        hr = back_buffer->GetDesc(&desc);
        if (FAILED(hr)) {
            ERR("IDirect3DSurface8::GetDesc failed 0x%lX", hr);
            return rf::BMPF_INVALID;
        }

        ComPtr<IDirect3DSurface8> tmp_surface;
#if 1
        hr = rf::gr_d3d_device->CreateRenderTarget(desc.Width, desc.Height, desc.Format, D3DMULTISAMPLE_NONE, TRUE,
                                                   &tmp_surface);
        // hr = rf::gr_d3d_device->CreateImageSurface(desc.Width, desc.Height, desc.Format, &tmp_surface);
        /*ComPtr<IDirect3DTexture8> tmp_texture;
        hr = rf::gr_d3d_device->CreateTexture(desc.Width, desc.Height, 1, 0, desc.Format, D3DPOOL_MANAGED,
                                              &tmp_texture);*/
        if (FAILED(hr)) {
            ERR("IDirect3DDevice8::CreateRenderTarget failed 0x%lX", hr);
            return rf::BMPF_INVALID;
        }

        /*hr = tmp_texture->GetSurfaceLevel(0, &tmp_surface);
        if (FAILED(hr)) {
            ERR("IDirect3DTexture8::GetSurfaceLevel failed 0x%lX", hr);
            return rf::BMPF_INVALID;
        }*/

        RECT src_rect;
        POINT dst_pt{x, y};
        SetRect(&src_rect, x, y, x + width - 1, y + height - 1);

        if (width > 0 && height > 0) {
            hr = rf::gr_d3d_device->CopyRects(back_buffer, &src_rect, 1, tmp_surface, &dst_pt);
            if (FAILED(hr)) {
                ERR("IDirect3DDevice8::CopyRects failed 0x%lX", hr);
                return rf::BMPF_INVALID;
            }
        }
#else
        tmp_surface = back_buffer;
#endif

        // Note: locking fragment of Render Target fails
        D3DLOCKED_RECT locked_rect;
        hr = tmp_surface->LockRect(&locked_rect, nullptr, D3DLOCK_READONLY | D3DLOCK_NO_DIRTY_UPDATE);
        if (FAILED(hr)) {
            ERR("IDirect3DSurface8::LockRect failed 0x%lX (%s)", hr, getDxErrorStr(hr));
            return rf::BMPF_INVALID;
        }

        rf::BmPixelFormat pixel_fmt = GetPixelFormatFromD3DFormat(desc.Format);
        int bytes_per_pixel = GetPixelFormatSize(pixel_fmt);
        std::byte* src_ptr =
            reinterpret_cast<std::byte*>(locked_rect.pBits) + y * locked_rect.Pitch + x * bytes_per_pixel;
        std::byte* dst_ptr = buffer;

        for (int i = 0; i < height; ++i) {
            std::memcpy(dst_ptr, src_ptr, width * bytes_per_pixel);
            src_ptr += locked_rect.Pitch;
            dst_ptr += width * bytes_per_pixel;
        }
        tmp_surface->UnlockRect();

        TRACE("GrReadBackBufferHook (%d %d %d %d) returns %d", x, y, width, height, pixel_fmt);

        return pixel_fmt;
    },
};
#endif // D3D_LOCKABLE_BACKBUFFER

bool GrCaptureBackBufferFast(int x, int y, int width, int height, int bm_handle)
{
    rf::GrFlushBuffers();

    // Note: Release call on D3D texture is not needed (no additional ref is added)
    g_force_texture_in_backbuffer_format = true;
    auto d3d_tex = rf::GrGetBitmapTexture(bm_handle);
    g_force_texture_in_backbuffer_format = false;
    if (!d3d_tex) {
        WARN("Bitmap without D3D texture provided in GrCaptureBackBuffer");
        return false;
    }

    ComPtr<IDirect3DSurface8> render_target;
    HRESULT hr = rf::gr_d3d_device->GetRenderTarget(&render_target);
    if (FAILED(hr)) {
        ERR("IDirect3DDevice8::GetBackBuffer failed 0x%lX", hr);
        return false;
    }

    ComPtr<IDirect3DSurface8> tex_surface;
    hr = d3d_tex->GetSurfaceLevel(0, &tex_surface);
    if (FAILED(hr)) {
        ERR("IDirect3DTexture8::GetSurfaceLevel failed 0x%lX", hr);
        return false;
    }

    D3DSURFACE_DESC back_buffer_desc;
    hr = render_target->GetDesc(&back_buffer_desc);
    if (FAILED(hr)) {
        ERR("IDirect3DSurface8::GetDesc failed 0x%lX", hr);
        return false;
    }

    D3DSURFACE_DESC tex_desc;
    hr = tex_surface->GetDesc(&tex_desc);
    if (FAILED(hr)) {
        ERR("IDirect3DSurface8::GetDesc failed 0x%lX", hr);
        return false;
    }

    if (tex_desc.Format != back_buffer_desc.Format) {
        static bool diff_format_warned = false;
        if (!diff_format_warned) {
            WARN("back buffer and texture has different D3D format in GrCaptureBackBuffer: %d vs %d",
                back_buffer_desc.Format, tex_desc.Format);
            diff_format_warned = true;
        }
        return false;
    }

    if (width > 0 && height > 0) {
        RECT src_rect;
        POINT dst_pt{x, y};
        SetRect(&src_rect, x, y, x + width - 1, y + height - 1);
        hr = rf::gr_d3d_device->CopyRects(render_target, &src_rect, 1, tex_surface, &dst_pt);
        if (FAILED(hr)) {
            ERR("IDirect3DDevice8::CopyRects failed 0x%lX", hr);
            return false;
        }
    }

    return true;
}

FunHook<void(int, int, int, int, int)> GrCaptureBackBuffer_hook{
    0x0050E4F0,
    [](int x, int y, int width, int height, int bm_handle) {
        if (!GrCaptureBackBufferFast(x, y, width, height, bm_handle)) {
            GrCaptureBackBuffer_hook.CallTarget(x, y, width, height, bm_handle);
        }
    },
};

CodeInjection GrD3DCreateVramTexture_patch{
    0x0055B9CA,
    [](auto& regs) {
        if (g_force_texture_in_backbuffer_format) {
            TRACE("Forcing texture in backbuffer format");
            regs.eax = rf::gr_d3d_pp.BackBufferFormat;
        }
    },
};

CodeInjection MonitorInit_bitmap_format_fix{
    0x0041254C,
    [](auto& regs) {
        g_force_texture_in_backbuffer_format = true;
        rf::GrGetBitmapTexture(regs.eax);
        g_force_texture_in_backbuffer_format = false;
    },
};

CodeInjection d3d_device_lost_release_render_target_patch{
    0x00545042,
    []([[maybe_unused]] auto& regs) {
        TRACE("Releasing render target");
        g_render_target.release();
        g_depth_stencil_surface.release();
    },
};

CodeInjection d3d_cleanup_release_render_target_patch{
    0x0054527A,
    []([[maybe_unused]] auto& regs) {
        g_render_target.release();
        g_depth_stencil_surface.release();
    },
};

FunHook<void()> GameRenderToDynamicTextures_msaa_fix{
    0x00431820,
    []() {
        HRESULT hr;
        if (!g_render_target) {
            hr = rf::gr_d3d_device->CreateRenderTarget(
                rf::gr_d3d_pp.BackBufferWidth, rf::gr_d3d_pp.BackBufferHeight, rf::gr_d3d_pp.BackBufferFormat,
                D3DMULTISAMPLE_NONE, FALSE, &g_render_target);
            if (FAILED(hr)) {
                ERR("IDirect3DDevice8::CreateRenderTarget failed 0x%lX", hr);
                return;
            }
        }

        if (!g_depth_stencil_surface) {
            hr = rf::gr_d3d_device->CreateDepthStencilSurface(
                rf::gr_d3d_pp.BackBufferWidth, rf::gr_d3d_pp.BackBufferHeight, rf::gr_d3d_pp.AutoDepthStencilFormat,
                D3DMULTISAMPLE_NONE, &g_depth_stencil_surface);
            if (FAILED(hr)) {
                ERR("IDirect3DDevice8::CreateDepthStencilSurface failed 0x%lX", hr);
                return;
            }
        }

        ComPtr<IDirect3DSurface8> orig_render_target;
        hr = rf::gr_d3d_device->GetRenderTarget(&orig_render_target);
        if (FAILED(hr)) {
            ERR("IDirect3DDevice8::GetRenderTarget failed 0x%lX", hr);
            return;
        }

        ComPtr<IDirect3DSurface8> orig_depth_stencil_surface;
        hr = rf::gr_d3d_device->GetDepthStencilSurface(&orig_depth_stencil_surface);
        if (FAILED(hr)) {
            ERR("IDirect3DDevice8::GetDepthStencilSurface failed 0x%lX", hr);
            return;
        }
        hr = rf::gr_d3d_device->SetRenderTarget(g_render_target, g_depth_stencil_surface);
        if (FAILED(hr)) {
            ERR("IDirect3DDevice8::SetRenderTarget failed 0x%lX", hr);
            return;
        }

        GameRenderToDynamicTextures_msaa_fix.CallTarget();

        hr = rf::gr_d3d_device->SetRenderTarget(orig_render_target, orig_depth_stencil_surface);
        if (FAILED(hr)) {
            ERR("IDirect3DDevice8::SetRenderTarget failed 0x%lX", hr);
            return;
        }
    },
};

void InitScreenshot()
{
#if !D3D_LOCKABLE_BACKBUFFER
    /* Override default because IDirect3DSurface8::LockRect fails on multisampled back-buffer */
    GrD3DReadBackBuffer_hook.Install();
#endif

    // Use fast GrCaptureBackBuffer implementation which copies backbuffer to texture without copying from VRAM to RAM
    GrCaptureBackBuffer_hook.Install();
    if (g_game_config.msaa) {
        // According to tests on Windows in MSAA mode it is better to render to render target instead of copying from
        // multi-sampled back-buffer
        GameRenderToDynamicTextures_msaa_fix.Install();
        d3d_device_lost_release_render_target_patch.Install();
        d3d_cleanup_release_render_target_patch.Install();
    }

    // Make sure bitmaps used together with GrCaptureBackBuffer have the same format as backbuffer
    GrD3DCreateVramTexture_patch.Install();
    MonitorInit_bitmap_format_fix.Install();

    // Override screenshot directory
    WriteMemPtr(0x004367CA + 2, &g_screenshot_dir_id);
}

void CleanupScreenshot()
{
    g_screenshot_scanlines_buf.reset();
}

void ScreenshotAfterGameInit()
{
    /* Fix for screenshots creation when height > 1024 */
    if (rf::gr_screen.max_height > 1024) {
        g_screenshot_scanlines_buf = std::make_unique<byte* []>(rf::gr_screen.max_height);
        AsmWritter(0x0055A066, 0x0055A06D)
            .mov(asm_regs::ecx, reinterpret_cast<int32_t>(&g_screenshot_scanlines_buf[0]));
        AsmWritter(0x0055A0DF, 0x0055A0E6)
            .mov(asm_regs::eax, reinterpret_cast<int32_t>(g_screenshot_scanlines_buf.get()));
    }

    auto full_path = StringFormat("%s\\%s", rf::root_path, g_screenshot_dir_name);
    if (CreateDirectoryA(full_path.c_str(), nullptr))
        INFO("Created screenshots directory");
    else if (GetLastError() != ERROR_ALREADY_EXISTS)
        ERR("Failed to create screenshots directory %lu", GetLastError());
    g_screenshot_dir_id = rf::FsAddDirectoryEx(g_screenshot_dir_name, "", 1);
}
