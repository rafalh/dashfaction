#include "screenshot.h"
#include "BuildConfig.h"
#include "gr_color.h"
#include "main.h"
#include "rf.h"
#include "stdafx.h"
#include "utils.h"
#include <cstddef>
#include <CallHook.h>
#include <FunHook.h>
#include <ComPtr.h>

const char g_screenshot_dir_name[] = "screenshots";

static std::unique_ptr<byte* []> g_screenshot_scanlines_buf;
static int g_screenshot_dir_id;

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
  char field_B;
  GrTextureSlot *sections;
};
static_assert(sizeof(GrTexture) == 0x10);

static auto& GrGetBitmapTexture = AddrAsRef<IDirect3DTexture8*(int bm_handle)>(0x0055D1E0);

} // namespace rf

#if !D3D_LOCKABLE_BACKBUFFER
CallHook<rf::BmPixelFormat(int, int, int, int, std::byte*)> GrD3DReadBackBuffer_Hook{
    0x0050E015,
    [](int x, int y, int width, int height, std::byte* buffer) {
        // Note: function is sometimes called with all parameters set to 0 to get backbuffer format

        rf::GrFlushBuffers();

        ComPtr<IDirect3DSurface8> back_buffer;
        HRESULT hr = rf::g_GrDevice->GetBackBuffer(0, D3DBACKBUFFER_TYPE_MONO, &back_buffer);
        if (FAILED(hr)) {
            ERR("IDirect3DDevice8::GetBackBuffer failed 0x%x", hr);
            return rf::BMPF_INVALID;
        }

        D3DSURFACE_DESC desc;
        hr = back_buffer->GetDesc(&desc);
        if (FAILED(hr)) {
            ERR("IDirect3DSurface8::GetDesc failed 0x%x", hr);
            return rf::BMPF_INVALID;
        }

        ComPtr<IDirect3DSurface8> tmp_surface;
#if 1
        hr = rf::g_GrDevice->CreateRenderTarget(desc.Width, desc.Height, desc.Format, D3DMULTISAMPLE_NONE, TRUE,
                                                &tmp_surface);
        // hr = rf::g_GrDevice->CreateImageSurface(desc.Width, desc.Height, desc.Format, &tmp_surface);
        // hr = rf::g_GrDevice->CreateTexture(desc.Width, desc.Height, 1, 0, desc.Format, D3DPOOL_MANAGED,
        // &TmpTexture);
        if (FAILED(hr)) {
            ERR("IDirect3DDevice8::CreateRenderTarget failed 0x%x", hr);
            return rf::BMPF_INVALID;
        }

        RECT src_rect;
        POINT dst_pt{x, y};
        SetRect(&src_rect, x, y, x + width - 1, y + height - 1);

        if (width > 0 && height > 0) {
            hr = rf::g_GrDevice->CopyRects(back_buffer, &src_rect, 1, tmp_surface, &dst_pt);
            if (FAILED(hr)) {
                ERR("IDirect3DDevice8::CopyRects failed 0x%x", hr);
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
            ERR("IDirect3DSurface8::LockRect failed 0x%x (%s)", hr, getDxErrorStr(hr));
            return rf::BMPF_INVALID;
        }

        rf::BmPixelFormat pixel_fmt = GetPixelFormatFromD3DFormat(desc.Format);
        // Remove alpha
        if (pixel_fmt == rf::BMPF_8888)
            pixel_fmt = rf::BMPF_888;
        if (pixel_fmt == rf::BMPF_1555)
            pixel_fmt = rf::BMPF_565;

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
    auto d3d_tex = rf::GrGetBitmapTexture(bm_handle);
    if (!d3d_tex) {
        WARN("Bitmap without D3D texture provided in GrCaptureBackBuffer");
        return false;
    }

    ComPtr<IDirect3DSurface8> back_buffer;
    HRESULT hr = rf::g_GrDevice->GetBackBuffer(0, D3DBACKBUFFER_TYPE_MONO, &back_buffer);
    if (FAILED(hr)) {
        ERR("IDirect3DDevice8::GetBackBuffer failed 0x%x", hr);
        return false;
    }

    ComPtr<IDirect3DSurface8> tex_surface;
    hr = d3d_tex->GetSurfaceLevel(0, &tex_surface);
    if (FAILED(hr)) {
        ERR("IDirect3DTexture8::GetSurfaceLevel failed 0x%X", hr);
        return false;
    }

    D3DSURFACE_DESC back_buffer_desc;
    hr = back_buffer->GetDesc(&back_buffer_desc);
    if (FAILED(hr)) {
        ERR("IDirect3DSurface8::GetDesc failed 0x%X", hr);
        return false;
    }

    D3DSURFACE_DESC tex_desc;
    hr = tex_surface->GetDesc(&tex_desc);
    if (FAILED(hr)) {
        ERR("IDirect3DSurface8::GetDesc failed 0x%X", hr);
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
        hr = rf::g_GrDevice->CopyRects(back_buffer, &src_rect, 1, tex_surface, &dst_pt);
        if (FAILED(hr)) {
            ERR("IDirect3DDevice8::CopyRects failed 0x%x", hr);
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

void InitScreenshot()
{
#if !D3D_LOCKABLE_BACKBUFFER
    /* Override default because IDirect3DSurface8::LockRect fails on multisampled back-buffer */
    GrD3DReadBackBuffer_Hook.Install();
#endif

    // Use fast GrCaptureBackBuffer implementation which copies backbuffer to texture without copying from VRAM to RAM
    GrCaptureBackBuffer_hook.Install();
    // Use format 888 instead of 8888 for scanner bitmap (needed by railgun and rocket launcher)
    WriteMem<uint8_t>(0x004A34BF + 1, rf::BMPF_888);

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
    if (rf::g_GrScreen.MaxHeight > 1024) {
        g_screenshot_scanlines_buf = std::make_unique<byte* []>(rf::g_GrScreen.MaxHeight);
        AsmWritter(0x0055A066, 0x0055A06D)
            .mov(asm_regs::ecx, reinterpret_cast<int32_t>(&g_screenshot_scanlines_buf[0]));
        AsmWritter(0x0055A0DF, 0x0055A0E6)
            .mov(asm_regs::eax, reinterpret_cast<int32_t>(g_screenshot_scanlines_buf.get()));
    }

    auto full_path = StringFormat("%s\\%s", rf::g_RootPath, g_screenshot_dir_name);
    if (CreateDirectoryA(full_path.c_str(), nullptr))
        INFO("Created screenshots directory");
    else if (GetLastError() != ERROR_ALREADY_EXISTS)
        ERR("Failed to create screenshots directory %lu", GetLastError());
    g_screenshot_dir_id = rf::FsAddDirectoryEx(g_screenshot_dir_name, "", 1);
}
