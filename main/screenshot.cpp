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
    IDirect3DSurface8 *pBackBuffer = NULL, *pTmpSurface = NULL;

    if (*((DWORD*)0x17C7BCC) != 102)
        return 0;

    GrFlushBuffers();

    hr = IDirect3DDevice8_GetBackBuffer(*g_ppGrDevice, 0, D3DBACKBUFFER_TYPE_MONO, &pBackBuffer);
    if (FAILED(hr))
    {
        ERR("IDirect3DDevice8_GetBackBuffer failed %x", hr);
        return 0;
    }

    D3DSURFACE_DESC Desc;
    hr = IDirect3DSurface8_GetDesc(pBackBuffer, &Desc);
    if (FAILED(hr))
    {
        ERR("IDirect3DSurface8_GetDesc failed %x", hr);
        return 0;
    }

#if 1
    if (SUCCEEDED(hr))
    {
        hr = IDirect3DDevice8_CreateRenderTarget(*g_ppGrDevice, Desc.Width, Desc.Height, Desc.Format, D3DMULTISAMPLE_NONE, TRUE, &pTmpSurface);
        //hr = IDirect3DDevice8_CreateImageSurface(*g_ppGrDevice, Desc.Width, Desc.Height, Desc.Format, &pTmpSurface);
        if (FAILED(hr))
            ERR("IDirect3DDevice8_CreateImageSurface failed %x", hr);
    }
    
    if (SUCCEEDED(hr))
    {
        RECT SrcRect;
        POINT DstPoint = { 0, 0 };
        SetRect(&SrcRect, 0, 0, Desc.Width, Desc.Height);
        hr = IDirect3DDevice8_CopyRects(*g_ppGrDevice, pBackBuffer, &SrcRect, 1, pTmpSurface, &DstPoint);
        if (FAILED(hr))
            ERR("IDirect3DDevice8_CopyRects failed %x", hr);
    }
#else
    pTmpSurface = pBackBuffer;
    IDirect3DSurface8_AddRef(pBackBuffer);
#endif

    if (SUCCEEDED(hr))
    {
        RECT rc;
        D3DLOCKED_RECT LockedRect;
        SetRect(&rc, x, y, x + Width - 1, y + Height - 1);
        hr = IDirect3DSurface8_LockRect(pTmpSurface, &LockedRect, &rc, D3DLOCK_READONLY);
        if (FAILED(hr))
            ERR("IDirect3DSurface8_LockRect failed %x", hr);
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
    /* Fix for screenshots creation when height > 1024 */
    int ScreenHeight = GetSystemMetrics(SM_CYSCREEN);
    if(ScreenHeight > 1024)
    {
        g_ScreenshotScanlinesBuf = (BYTE**)malloc(ScreenHeight * sizeof(BYTE*));
        WriteMemUInt8((PVOID)0x0055A066, ASM_MOV_ECX);
        WriteMemUInt32((PVOID)0x0055A067, (uint32_t)&g_ScreenshotScanlinesBuf[0]);
        WriteMemUInt8Repeat((PVOID)0x0055A06B, ASM_NOP, 2);
        WriteMemUInt8((PVOID)0x0055A0DF, ASM_MOV_EAX);
        WriteMemUInt32((PVOID)0x0055A0E0, (uint32_t)g_ScreenshotScanlinesBuf);
        WriteMemUInt8Repeat((PVOID)0x0055A0E4, ASM_NOP, 2);
    }

    /* Override default because IDirect3DSurface8::LockRect fails on multisampled back-buffer */
#if MULTISAMPLING_SUPPORT
    if (g_gameConfig.msaa)
    {
        WriteMemUInt8((PVOID)0x0050DFF0, ASM_LONG_JMP_REL);
        WriteMemPtr((PVOID)0x0050DFF1, (PVOID)((ULONG_PTR)GrReadBackBufferHook - (0x0050DFF0 + 0x5)));
    }
#endif

    WriteMemPtr((PVOID)(0x004367CA + 2), &g_ScreenshotDirId);
}

void CleanupScreenshot(void)
{
    free(g_ScreenshotScanlinesBuf);
}

void ScreenshotAfterGameInit()
{
    char FullPath[MAX_PATH];
    sprintf(FullPath, "%s\\%s", g_pszRootPath, SCREENSHOT_DIR_NAME);
    if (CreateDirectoryA(FullPath, NULL))
        INFO("Created screenshots directory");
    else if (GetLastError() != ERROR_ALREADY_EXISTS)
        ERR("Failed to create screenshots directory %lu", GetLastError());
    g_ScreenshotDirId = FsAddDirectoryEx(SCREENSHOT_DIR_NAME, "", 1);
}
