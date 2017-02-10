#include "stdafx.h"
#include "screenshot.h"
#include "utils.h"
#include "rf.h"
#include "BuildConfig.h"
#include "main.h"

const char SCREENSHOT_DIR_NAME[] = "screenshots";

static BYTE **g_ScreenshotScanlinesBuf = NULL;
static int g_ScreenshotDirId;

int GetPixelFormatID(int PixelFormat, int *BytesPerPixel)
{
    switch (PixelFormat)
    {
    case D3DFMT_R5G6B5:
        *BytesPerPixel = 2;
        return 3;
    case D3DFMT_A4R4G4B4:
    case D3DFMT_X4R4G4B4:
        *BytesPerPixel = 2;
        return 4;
    case D3DFMT_X1R5G5B5:
    case D3DFMT_A1R5G5B5:
        *BytesPerPixel = 2;
        return 5;
    case D3DFMT_R8G8B8:
        *BytesPerPixel = 3;
        return 6;
    case D3DFMT_A8R8G8B8:
    case D3DFMT_X8R8G8B8:
        *BytesPerPixel = 4;
        return 7;
    default:
        return 0;
    }
}

int GrReadBackBufferHook(LONG x, LONG y, int Width, int Height, BYTE *pBuffer)
{
    HRESULT hr;
    int Result = 0;
    IDirect3DSurface8 *pBackBuffer = NULL;
    IDirect3DSurface8 *pTmpSurface = NULL;

    if (*((DWORD*)0x17C7BCC) != 102)
        return 0;

    // Note: function is sometimes called with all parameters set to 0 to get backbuffer format

    rf::GrFlushBuffers();

    hr = IDirect3DDevice8_GetBackBuffer(*rf::g_ppGrDevice, 0, D3DBACKBUFFER_TYPE_MONO, &pBackBuffer);
    if (FAILED(hr))
    {
        ERR("IDirect3DDevice8_GetBackBuffer failed 0x%x", hr);
        return 0;
    }

    D3DSURFACE_DESC Desc;
    hr = IDirect3DSurface8_GetDesc(pBackBuffer, &Desc);
    if (FAILED(hr))
    {
        ERR("IDirect3DSurface8_GetDesc failed 0x%x", hr);
        IDirect3DSurface8_Release(pBackBuffer);
        return 0;
    }

#if 1
    if (SUCCEEDED(hr))
    {
        hr = IDirect3DDevice8_CreateRenderTarget(*rf::g_ppGrDevice, Desc.Width, Desc.Height, Desc.Format, D3DMULTISAMPLE_NONE, TRUE, &pTmpSurface);
        //hr = IDirect3DDevice8_CreateImageSurface(*rf::g_ppGrDevice, Desc.Width, Desc.Height, Desc.Format, &pTmpSurface);
        //hr = IDirect3DDevice8_CreateTexture(*rf::g_ppGrDevice, Desc.Width, Desc.Height, 1, 0, Desc.Format, D3DPOOL_MANAGED, &pTmpTexture);
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
            hr = IDirect3DDevice8_CopyRects(*rf::g_ppGrDevice, pBackBuffer, &SrcRect, 1, pTmpSurface, &DstPoint);
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
        hr = IDirect3DSurface8_LockRect(pTmpSurface, &LockedRect, NULL, D3DLOCK_READONLY | D3DLOCK_NO_DIRTY_UPDATE);
        if (FAILED(hr))
            ERR("IDirect3DSurface8_LockRect failed 0x%x (%s)", hr, getDxErrorStr(hr));
        else
        {
            int i, BytesPerPixel;
            BYTE *SrcPtr, *DstPtr = pBuffer;

            Result = GetPixelFormatID(Desc.Format, &BytesPerPixel);
            SrcPtr = ((BYTE*)LockedRect.pBits) + y * LockedRect.Pitch + x * BytesPerPixel;

            for (i = 0; i < Height; ++i)
            {
                memcpy(DstPtr, SrcPtr, Width * BytesPerPixel);
                SrcPtr += LockedRect.Pitch;
                DstPtr += Width * BytesPerPixel;
            }
            IDirect3DSurface8_UnlockRect(pTmpSurface);

            TRACE("GrReadBackBufferHook (%d %d %d %d) returns %d", x, y, Width, Height, Result);
        }
    }

    if (pTmpSurface)
        IDirect3DSurface8_Release(pTmpSurface);
    if (pBackBuffer)
        IDirect3DSurface8_Release(pBackBuffer);

    return Result;
}

void InitScreenshot(void)
{
    /* Override default because IDirect3DSurface8::LockRect fails on multisampled back-buffer */
#if MULTISAMPLING_SUPPORT
    if (g_gameConfig.msaa)
    {
        WriteMemUInt8(0x0050DFF0, ASM_LONG_JMP_REL);
        WriteMemInt32(0x0050DFF0 + 1, (uintptr_t)GrReadBackBufferHook - (0x0050DFF0 + 0x5));
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
