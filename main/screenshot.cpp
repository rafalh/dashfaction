#include "stdafx.h"
#include "screenshot.h"
#include "utils.h"
#include "rf.h"
#include "BuildConfig.h"
#include "main.h"
#include "gr_color.h"
#include <CallHook2.h>

const char g_screenshot_dir_name[] = "screenshots";

static std::unique_ptr<byte* []> g_screenshot_scanlines_buf;
static int g_screenshot_dir_id;

CallHook2<rf::BmPixelFormat(int, int, int, int, byte*)> GrD3DReadBackBuffer_Hook{
    0x0050E015,
    [](int x, int y, int width, int height, byte* buffer) {
        HRESULT hr;
        rf::BmPixelFormat pixel_fmt = rf::BMPF_INVALID;
        IDirect3DSurface8* back_buffer = nullptr;
        IDirect3DSurface8* tmp_surface = nullptr;

        // Note: function is sometimes called with all parameters set to 0 to get backbuffer format

        rf::GrFlushBuffers();

        hr = rf::g_pGrDevice->GetBackBuffer(0, D3DBACKBUFFER_TYPE_MONO, &back_buffer);
        if (FAILED(hr)) {
            ERR("IDirect3DDevice8::GetBackBuffer failed 0x%x", hr);
            return rf::BMPF_INVALID;
        }

        D3DSURFACE_DESC desc;
        hr = back_buffer->GetDesc(&desc);
        if (FAILED(hr)) {
            ERR("IDirect3DSurface8::GetDesc failed 0x%x", hr);
            back_buffer->Release();
            return rf::BMPF_INVALID;
        }

#if 1
        if (SUCCEEDED(hr)) {
            hr = rf::g_pGrDevice->CreateRenderTarget(desc.Width, desc.Height, desc.Format, D3DMULTISAMPLE_NONE, TRUE, &tmp_surface);
            //hr = rf::g_pGrDevice->CreateImageSurface(desc.Width, desc.Height, desc.Format, &tmp_surface);
            //hr = rf::g_pGrDevice->CreateTexture(desc.Width, desc.Height, 1, 0, desc.Format, D3DPOOL_MANAGED, &pTmpTexture);
            if (FAILED(hr))
                ERR("IDirect3DDevice8::CreateRenderTarget failed 0x%x", hr);
        }

        if (SUCCEEDED(hr)) {
            RECT src_rect;
            POINT dst_pt{ x, y };
            SetRect(&src_rect, x, y, x + width - 1, y + height - 1);

            if (width > 0 && height > 0) {
                hr = rf::g_pGrDevice->CopyRects(back_buffer, &src_rect, 1, tmp_surface, &dst_pt);
                if (FAILED(hr))
                    ERR("IDirect3DDevice8::CopyRects failed 0x%x", hr);
            }
        }
#else
        tmp_surface = back_buffer;
        back_buffer->AddRef();
#endif

        if (SUCCEEDED(hr)) {
            // Note: locking fragment of Render Target fails
            D3DLOCKED_RECT locked_rect;
            hr = tmp_surface->LockRect(&locked_rect, NULL, D3DLOCK_READONLY | D3DLOCK_NO_DIRTY_UPDATE);
            if (FAILED(hr))
                ERR("IDirect3DSurface8::LockRect failed 0x%x (%s)", hr, getDxErrorStr(hr));
            else {
                int i, bytes_per_pixel;
                uint8_t *src_ptr, *dst_ptr = buffer;

                pixel_fmt = GetPixelFormatFromD3DFormat(desc.Format);
                bytes_per_pixel = GetPixelFormatSize(pixel_fmt);
                src_ptr = ((uint8_t*)locked_rect.pBits) + y * locked_rect.Pitch + x * bytes_per_pixel;

                for (i = 0; i < height; ++i) {
                    memcpy(dst_ptr, src_ptr, width * bytes_per_pixel);
                    src_ptr += locked_rect.Pitch;
                    dst_ptr += width * bytes_per_pixel;
                }
                tmp_surface->UnlockRect();

                TRACE("GrReadBackBufferHook (%d %d %d %d) returns %d", x, y, width, height, pixel_fmt);
            }
        }

        if (tmp_surface)
            tmp_surface->Release();
        if (back_buffer)
            back_buffer->Release();

        return pixel_fmt;
    }
};

void InitScreenshot()
{
#if D3D_SWAP_DISCARD
    /* Override default because IDirect3DSurface8::LockRect fails on multisampled back-buffer */
    GrD3DReadBackBuffer_Hook.Install();
#endif

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
        AsmWritter(0x0055A066, 0x0055A06D).mov(AsmRegs::ECX, (int32_t)&g_screenshot_scanlines_buf[0]);
        AsmWritter(0x0055A0DF, 0x0055A0E6).mov(AsmRegs::EAX, (int32_t)g_screenshot_scanlines_buf.get());
    }

    char full_path[MAX_PATH];
    sprintf(full_path, "%s\\%s", rf::g_pszRootPath, g_screenshot_dir_name);
    if (CreateDirectoryA(full_path, NULL))
        INFO("Created screenshots directory");
    else if (GetLastError() != ERROR_ALREADY_EXISTS)
        ERR("Failed to create screenshots directory %lu", GetLastError());
    g_screenshot_dir_id = rf::FsAddDirectoryEx(g_screenshot_dir_name, "", 1);
}
