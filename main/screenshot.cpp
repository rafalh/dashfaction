#include "stdafx.h"
#include "screenshot.h"
#include "utils.h"
#include "rf.h"
#include "BuildConfig.h"
#include "main.h"
#include "gr_color.h"

const char SCREENSHOT_DIR_NAME[] = "screenshots";

static BYTE **g_ScreenshotScanlinesBuf = NULL;
static int g_ScreenshotDirId;

rf::BmPixelFormat GrD3DReadBackBufferHook(int x, int y, int Width, int Height, BYTE *pBuffer)
{
    HRESULT hr;
    rf::BmPixelFormat PixelFmt = rf::BMPF_INVALID;
    IDirect3DSurface8 *pBackBuffer = NULL;
    IDirect3DSurface8 *pTmpSurface = NULL;

    // Note: function is sometimes called with all parameters set to 0 to get backbuffer format

    rf::GrFlushBuffers();

    hr = (*rf::g_ppGrDevice)->GetBackBuffer(0, D3DBACKBUFFER_TYPE_MONO, &pBackBuffer);
    if (FAILED(hr))
    {
        ERR("IDirect3DDevice8_GetBackBuffer failed 0x%x", hr);
        return rf::BMPF_INVALID;
    }

    D3DSURFACE_DESC Desc;
    hr = pBackBuffer->GetDesc(&Desc);
    if (FAILED(hr))
    {
        ERR("IDirect3DSurface8_GetDesc failed 0x%x", hr);
        pBackBuffer->Release();
        return rf::BMPF_INVALID;
    }

#if 1
    if (SUCCEEDED(hr))
    {
        hr = (*rf::g_ppGrDevice)->CreateRenderTarget(Desc.Width, Desc.Height, Desc.Format, D3DMULTISAMPLE_NONE, TRUE, &pTmpSurface);
        //hr = (*rf::g_ppGrDevice)->CreateImageSurface(Desc.Width, Desc.Height, Desc.Format, &pTmpSurface);
        //hr = (*rf::g_ppGrDevice)->CreateTexture(Desc.Width, Desc.Height, 1, 0, Desc.Format, D3DPOOL_MANAGED, &pTmpTexture);
        if (FAILED(hr))
            ERR("IDirect3DDevice8_CreateRenderTarget failed 0x%x", hr);
    }
    
    if (SUCCEEDED(hr))
    {
        RECT SrcRect;
        POINT DstPoint = { x, y };
        SetRect(&SrcRect, x, y, x + Width - 1, y + Height - 1);

        if (Width > 0 && Height > 0)
        {
            hr = (*rf::g_ppGrDevice) ->CopyRects(pBackBuffer, &SrcRect, 1, pTmpSurface, &DstPoint);
            if (FAILED(hr))
                ERR("IDirect3DDevice8_CopyRects failed 0x%x", hr);
        }
    }
#else
    pTmpSurface = pBackBuffser;
    IDirect3DSurface8_AddRef(pBackBuffer);
#endif

    if (SUCCEEDED(hr))
    {
        // Note: locking fragment of Render Target fails
        D3DLOCKED_RECT LockedRect;
        hr = pTmpSurface->LockRect(&LockedRect, NULL, D3DLOCK_READONLY | D3DLOCK_NO_DIRTY_UPDATE);
        if (FAILED(hr))
            ERR("IDirect3DSurface8_LockRect failed 0x%x (%s)", hr, getDxErrorStr(hr));
        else
        {
            int i, BytesPerPixel;
            BYTE *SrcPtr, *DstPtr = pBuffer;

            PixelFmt = GetPixelFormatFromD3DFormat(Desc.Format);
            BytesPerPixel = GetPixelFormatSize(PixelFmt);
            SrcPtr = ((BYTE*)LockedRect.pBits) + y * LockedRect.Pitch + x * BytesPerPixel;

            for (i = 0; i < Height; ++i)
            {
                memcpy(DstPtr, SrcPtr, Width * BytesPerPixel);
                SrcPtr += LockedRect.Pitch;
                DstPtr += Width * BytesPerPixel;
            }
            pTmpSurface->UnlockRect();

            TRACE("GrReadBackBufferHook (%d %d %d %d) returns %d", x, y, Width, Height, PixelFmt);
        }
    }

    if (pTmpSurface)
        pTmpSurface->Release();
    if (pBackBuffer)
        pBackBuffer->Release();

    return PixelFmt;
}

void InitScreenshot(void)
{
    /* Override default because IDirect3DSurface8::LockRect fails on multisampled back-buffer */
#if MULTISAMPLING_SUPPORT
    if (g_gameConfig.msaa)
    {
        WriteMemInt32(0x0050E015 + 1, (uintptr_t)GrD3DReadBackBufferHook - (0x0050E015 + 0x5));
    }
#endif

    WriteMemPtr(0x004367CA + 2, &g_ScreenshotDirId);
}

void CleanupScreenshot(void)
{
    free(g_ScreenshotScanlinesBuf);
}

void ScreenshotAfterGameInit()
{
    /* Fix for screenshots creation when height > 1024 */
    if (rf::g_pGrScreen->MaxHeight > 1024)
    {
        g_ScreenshotScanlinesBuf = (BYTE**)malloc(rf::g_pGrScreen->MaxHeight * sizeof(BYTE*));
        WriteMemUInt8(0x0055A066, ASM_MOV_ECX);
        WriteMemPtr(0x0055A066 + 1, &g_ScreenshotScanlinesBuf[0]);
        WriteMemUInt8(0x0055A06B, ASM_NOP, 2);
        WriteMemUInt8(0x0055A0DF, ASM_MOV_EAX);
        WriteMemPtr(0x0055A0DF + 1, g_ScreenshotScanlinesBuf);
        WriteMemUInt8(0x0055A0E4, ASM_NOP, 2);
    }

    char FullPath[MAX_PATH];
    sprintf(FullPath, "%s\\%s", rf::g_pszRootPath, SCREENSHOT_DIR_NAME);
    if (CreateDirectoryA(FullPath, NULL))
        INFO("Created screenshots directory");
    else if (GetLastError() != ERROR_ALREADY_EXISTS)
        ERR("Failed to create screenshots directory %lu", GetLastError());
    g_ScreenshotDirId = rf::FsAddDirectoryEx(SCREENSHOT_DIR_NAME, "", 1);
}
