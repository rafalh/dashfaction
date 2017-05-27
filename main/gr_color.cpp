#include "stdafx.h"
#include "rf.h"
#include "AsmWritter.h"
#include "main.h"
#include "gr_color.h"

using namespace rf;

auto GrD3DSetTextureData_Hook = makeFunHook(GrD3DSetTextureData);
auto BinkInitDeviceInfo_Hook = makeFunHook(BinkInitDeviceInfo);

inline void ConvertPixel_RGB8_To_RGBA8(BYTE *&pDstPtr, const BYTE *&pSrcPtr)
{
    *(pDstPtr++) = *(pSrcPtr++);
    *(pDstPtr++) = *(pSrcPtr++);
    *(pDstPtr++) = *(pSrcPtr++);
    *(pDstPtr++) = 255;
}

inline void ConvertPixel_BGR8_To_RGBA8(BYTE *&pDstPtr, const BYTE *&pSrcPtr)
{
    BYTE r = *(pSrcPtr++), g = *(pSrcPtr++), b = *(pSrcPtr++);
    *(pDstPtr++) = b;
    *(pDstPtr++) = g;
    *(pDstPtr++) = r;
    *(pDstPtr++) = 255;
}

inline void ConvertPixel_RGBA4_To_RGBA8(BYTE *&pDstPtr, const BYTE *&pSrcPtr)
{
    *(pDstPtr++) = (*(pSrcPtr) & 0x0F) * 17;
    *(pDstPtr++) = ((*(pSrcPtr++) & 0xF0) >> 4) * 17;
    *(pDstPtr++) = (*(pSrcPtr) & 0x0F) * 17;
    *(pDstPtr++) = ((*(pSrcPtr++) & 0xF0) >> 4) * 17;
}

inline void ConvertPixel_ARGB1555_To_RGBA8(BYTE *&pDstPtr, const BYTE *&pSrcPtr)
{
    WORD SrcWord = *(WORD*)pSrcPtr;
    pSrcPtr += 2;
    *(pDstPtr++) = ((SrcWord & (0x1F << 0)) >> 0) * 255 / 31;
    *(pDstPtr++) = ((SrcWord & (0x1F << 5)) >> 5) * 255 / 31;
    *(pDstPtr++) = ((SrcWord & (0x1F << 10)) >> 10) * 255 / 31;
    *(pDstPtr++) = (SrcWord & 0x8000) ? 255 : 0;
}

inline void ConvertPixel_RGB565_To_RGBA8(BYTE *&pDstPtr, const BYTE *&pSrcPtr)
{
    WORD SrcWord = *(WORD*)pSrcPtr;
    pSrcPtr += 2;
    *(pDstPtr++) = ((SrcWord & (0x1F << 0)) >> 0) * 255 / 31;
    *(pDstPtr++) = ((SrcWord & (0x3F << 5)) >> 5) * 255 / 63;
    *(pDstPtr++) = ((SrcWord & (0x1F << 11)) >> 11) * 255 / 31;
    *(pDstPtr++) = 255;
}

bool ConvertPixelFormat(BYTE *&pDstPtr, BmPixelFormat DstFmt, const BYTE *&pSrcPtr, BmPixelFormat SrcFmt)
{
    if (DstFmt == SrcFmt)
    {
        int PixelSize = GetPixelFormatSize(SrcFmt);
        memcpy(pDstPtr, pSrcPtr, PixelSize);
        pDstPtr += PixelSize;
        pSrcPtr += PixelSize;
        return true;
    }
    if (DstFmt != BMPF_8888)
    {
        ERR("unsupported dest pixel format %d (ConvertPixelFormat)", DstFmt);
        return false;
    }
    switch (SrcFmt)
    {
    case BMPF_888:
        ConvertPixel_RGB8_To_RGBA8(pDstPtr, pSrcPtr);
        return true;
    case BMPF_4444:
        ConvertPixel_RGBA4_To_RGBA8(pDstPtr, pSrcPtr);
        return true;
    case BMPF_565:
        ConvertPixel_RGB565_To_RGBA8(pDstPtr, pSrcPtr);
        return true;
    case BMPF_1555:
        ConvertPixel_ARGB1555_To_RGBA8(pDstPtr, pSrcPtr);
        return true;
    default:
        ERR("unsupported src pixel format %d", SrcFmt);
        return false;
    }
}

bool ConvertBitmapFormat(BYTE *pDstBits, BmPixelFormat DstFmt, const BYTE *pSrcBits, BmPixelFormat SrcFmt, int Width, int Height, int DstPitch, int SrcPitch, bool bByteSwap = false)
{
    if (DstFmt == SrcFmt)
    {
        for (int y = 0; y < Height; ++y)
        {
            memcpy(pDstBits, pSrcBits, std::min(SrcPitch, DstPitch));
            pDstBits += DstPitch;
            pSrcBits += SrcPitch;
        }
        return true;
    }
    if (DstFmt != BMPF_8888)
        return false;
    switch (SrcFmt)
    {
    case BMPF_888:
        for (int y = 0; y < Height; ++y)
        {
            BYTE *pDstPtr = pDstBits;
            const BYTE *pSrcPtr = pSrcBits;
            for (int x = 0; x < Width; ++x)
            {
                if (bByteSwap)
                    ConvertPixel_BGR8_To_RGBA8(pDstPtr, pSrcPtr);
                else
                    ConvertPixel_RGB8_To_RGBA8(pDstPtr, pSrcPtr);
            }
            pDstBits += DstPitch;
            pSrcBits += SrcPitch;
        }
        return true;
    case BMPF_4444:
        for (int y = 0; y < Height; ++y)
        {
            BYTE *pDstPtr = pDstBits;
            const BYTE *pSrcPtr = pSrcBits;
            for (int x = 0; x < Width; ++x)
                ConvertPixel_RGBA4_To_RGBA8(pDstPtr, pSrcPtr);
            pDstBits += DstPitch;
            pSrcBits += SrcPitch;
        }
        return true;
    case BMPF_1555:
        for (int y = 0; y < Height; ++y)
        {
            BYTE *pDstPtr = pDstBits;
            const BYTE *pSrcPtr = pSrcBits;
            for (int x = 0; x < Width; ++x)
                ConvertPixel_ARGB1555_To_RGBA8(pDstPtr, pSrcPtr);
            pDstBits += DstPitch;
            pSrcBits += SrcPitch;
        }
        return true;
    case BMPF_565:
        for (int y = 0; y < Height; ++y)
        {
            BYTE *pDstPtr = pDstBits;
            const BYTE *pSrcPtr = pSrcBits;
            for (int x = 0; x < Width; ++x)
                ConvertPixel_RGB565_To_RGBA8(pDstPtr, pSrcPtr);
            pDstBits += DstPitch;
            pSrcBits += SrcPitch;
        }
        return true;
    default:
        ERR("unsupported src format %d", SrcFmt);
        return false;
    }
}

int GrD3DSetTextureData_New(int Level, const BYTE *pSrcBits, const BYTE *pPallete, int cxBm, int cyBm, int PixelFmt, void *a7, int cxTex, int cyTex, IDirect3DTexture8 *pTexture)
{
    D3DLOCKED_RECT LockedRect;
    HRESULT hr = pTexture->LockRect(Level, &LockedRect, 0, 0);
    if (FAILED(hr))
    {
        ERR("LockRect failed");
        return -1;
    }
    D3DSURFACE_DESC Desc;
    pTexture->GetLevelDesc(Level, &Desc);
    bool bSuccess = ConvertBitmapFormat((BYTE*)LockedRect.pBits, GetPixelFormatFromD3DFormat(Desc.Format), pSrcBits, (BmPixelFormat)PixelFmt,
        cxBm, cyBm, LockedRect.Pitch, GetPixelFormatSize((BmPixelFormat)PixelFmt)*cxBm);
    pTexture->UnlockRect(Level);

    if (bSuccess)
        return 0;

    WARN("Color conversion failed (format %d -> %d)", PixelFmt, GetPixelFormatFromD3DFormat(Desc.Format));
    return GrD3DSetTextureData_Hook.callTrampoline(Level, pSrcBits, pPallete, cxBm, cyBm, PixelFmt, a7, cxTex, cyTex, pTexture);
}

void RflLoadLightmaps_004ED3F6(RflLightmap *pLightmap)
{
    SGrLockData LockData;
    int ret = GrLock(pLightmap->BmHandle, 0, &LockData, 2);
    if (!ret) return;

#if 1 // cap minimal color channel value as RF does
    for (int i = 0; i < pLightmap->w * pLightmap->h * 3; ++i)
        pLightmap->pBuf[i] = std::max(pLightmap->pBuf[i], (BYTE)(4 << 3)); // 32
#endif

    bool success = ConvertBitmapFormat(LockData.pBits, (BmPixelFormat)LockData.PixelFormat,
        pLightmap->pBuf, BMPF_888, pLightmap->w, pLightmap->h, LockData.Pitch, 3 * pLightmap->w, true);
    if (!success)
        ERR("ConvertBitmapFormat failed for lightmap");

    GrUnlock(&LockData);
}

void GeoModGenerateTexture_004F2F23(uintptr_t v3)
{
    uintptr_t v75 = *(uintptr_t*)(v3 + 12);
    unsigned hbm = *(unsigned *)(v75 + 16);
    SGrLockData LockData;
    if (GrLock(hbm, 0, &LockData, 1))
    {
        int OffsetY = *(int *)(v3 + 20);
        int OffsetX = *(int *)(v3 + 16);
        int cxSrcWidth = *(int *)(v75 + 4);
        int DstPixelSize = GetPixelFormatSize((BmPixelFormat)LockData.PixelFormat);
        BYTE *pSrcData = *(BYTE**)(v75 + 12) + 3 * (OffsetX + OffsetY * cxSrcWidth);
        BYTE *pDstData = &LockData.pBits[DstPixelSize * OffsetX + OffsetY * LockData.Pitch];
        int cyHeight = *(int*)(v3 + 28);
        bool bSuccess = ConvertBitmapFormat(pDstData, (BmPixelFormat)LockData.PixelFormat, pSrcData, BMPF_888,
            cxSrcWidth, cyHeight, LockData.Pitch, 3 * cxSrcWidth);
        if (!bSuccess)
            ERR("ConvertBitmapFormat failed for geomod (fmt %d)", LockData.PixelFormat);
        GrUnlock(&LockData);
    }
}

int GeoModGenerateLightmap_004E487B(uintptr_t v6)
{
    uintptr_t v48 = *(uintptr_t*)(v6 + 12);
    SGrLockData LockData;
    unsigned hbm = *(unsigned *)(v48 + 16);
    int ret = GrLock(hbm, 0, &LockData, 1);
    if (ret)
    {
        int OffsetY = *(int *)(v6 + 20);
        int cxSrcWidth = *(int *)(v48 + 4);
        int OffsetX = *(int *)(v6 + 16);
        BYTE *pSrcDataBegin = *(BYTE **)(v48 + 12);
        int SrcOffset = 3 * (OffsetX + cxSrcWidth * *(int *)(v6 + 20)); // src offset
        BYTE *pSrcData = SrcOffset + pSrcDataBegin;
        int cyHeight = *(int *)(v6 + 28);
        int DstPixelSize = GetPixelFormatSize((BmPixelFormat)LockData.PixelFormat);
        BYTE *pDstRow = &LockData.pBits[DstPixelSize * OffsetX + OffsetY * LockData.Pitch];
        bool bSuccess = ConvertBitmapFormat(pDstRow, (BmPixelFormat)LockData.PixelFormat, pSrcData, BMPF_888,
            cxSrcWidth, cyHeight, LockData.Pitch, 3 * cxSrcWidth);
        if (!bSuccess)
            ERR("ConvertBitmapFormat failed for geomod2 (fmt %d)", LockData.PixelFormat);
        GrUnlock(&LockData);
    }
    return ret;
}

void WaterGenerateTexture_004E68D1(uintptr_t v1)
{
    unsigned v8 = *(unsigned *)(v1 + 36);
    SGrLockData SrcLockData, DstLockData;
    GrLock(v8, 0, &SrcLockData, 0);
    GrLock(*(unsigned *)(v1 + 24), 0, &DstLockData, 2);
    int v9 = *(unsigned *)(v1 + 16);
    int8_t v10 = 0;
    int8_t v41 = 0;
    if (v9 > 0)
    {
        int v11 = 1;
        if (!(v9 & 1))
        {
            do
            {
                v11 *= 2;
                ++v10;
            } while (!(v9 & v11));
            v41 = v10;
        }
    }

    constexpr auto byte_1370F90 = (uint8_t*)0x1370F90;
    constexpr auto byte_1371B14 = (uint8_t*)0x1371B14;
    constexpr auto byte_1371090 = (uint8_t*)0x1371090;

    BYTE *pDstRow = DstLockData.pBits;
    int SrcPixelSize = GetPixelFormatSize((BmPixelFormat)SrcLockData.PixelFormat);

    for (int y = 0; y < DstLockData.Height; ++y)
    {
        int v30 = byte_1370F90[y];
        int v38 = byte_1371B14[y];
        uint8_t *v32 = &byte_1371090[-v30];
        BYTE *pDstPtr = pDstRow;
        for (int x = 0; x < DstLockData.Width; ++x)
        {
            int SrcOffset = (v30 & (DstLockData.Width - 1)) + (((DstLockData.Height - 1) & (v38 + v32[v30])) << v41);
            const BYTE *pSrcPtr = SrcLockData.pBits + SrcOffset*SrcPixelSize;
            ConvertPixelFormat(pDstPtr, (BmPixelFormat)DstLockData.PixelFormat, pSrcPtr, (BmPixelFormat)SrcLockData.PixelFormat);
            ++v30;
        }
        pDstRow += DstLockData.Pitch;
    }

    GrUnlock(&SrcLockData);
    GrUnlock(&DstLockData);
}

void GetAmbientColorFromLightmaps_004E5CE3(unsigned BmHandle, int x, int y, unsigned *pColor)
{
    SGrLockData LockData;
    if (GrLock(BmHandle, 0, &LockData, 0))
    {
        const BYTE *pSrcPtr = LockData.pBits + y * LockData.Pitch + x * GetPixelFormatSize((BmPixelFormat)LockData.PixelFormat);
        BYTE *pDestPtr = (BYTE*)pColor;
        ConvertPixelFormat(pDestPtr, BMPF_8888, pSrcPtr, (BmPixelFormat)LockData.PixelFormat);
        GrUnlock(&LockData);
    }
}

unsigned BinkInitDeviceInfo_New()
{
    unsigned BinkFlags = BinkInitDeviceInfo_Hook.callTrampoline();

    if (g_gameConfig.trueColorTextures && g_gameConfig.resBpp == 32)
    {
        constexpr auto pBinkBmPixelFmt = (uint32_t*)0x018871C0;
        *pBinkBmPixelFmt = BMPF_888;
        BinkFlags = 3;
    }

    return BinkFlags;
}

void GrColorInit()
{
    // True Color textures
    if (g_gameConfig.resBpp == 32 && g_gameConfig.trueColorTextures)
    {
        // Available texture formats (tested for compatibility)
        WriteMemUInt32(0x005A7DFC, D3DFMT_X8R8G8B8); // old: D3DFMT_R5G6B5
        WriteMemUInt32(0x005A7E00, D3DFMT_A8R8G8B8); // old: D3DFMT_X1R5G5B5
        WriteMemUInt32(0x005A7E04, D3DFMT_A8R8G8B8); // old: D3DFMT_A1R5G5B5, lightmaps
        WriteMemUInt32(0x005A7E08, D3DFMT_A8R8G8B8); // old: D3DFMT_A4R4G4B4
        WriteMemUInt32(0x005A7E0C, D3DFMT_A4R4G4B4); // old: D3DFMT_A8R3G3B2

        GrD3DSetTextureData_Hook.hook(GrD3DSetTextureData_New);
        BinkInitDeviceInfo_Hook.hook(BinkInitDeviceInfo_New);

        // lightmaps
        AsmWritter(0x004ED3E9).push(AsmRegs::EBX).callLong((uintptr_t)RflLoadLightmaps_004ED3F6).addEsp(4).jmpLong(0x004ED4FA);
        // geomod
        AsmWritter(0x004F2F23).push(AsmRegs::ESI).callLong((uintptr_t)GeoModGenerateTexture_004F2F23).addEsp(4).jmpLong(0x004F3023);
        AsmWritter(0x004E487B).push(AsmRegs::ESI).callLong((uintptr_t)GeoModGenerateLightmap_004E487B).addEsp(4).jmpLong(0x004E4993);
        // water
        AsmWritter(0x004E68B0, 0x004E68B6).nop();
        AsmWritter(0x004E68D1).push(AsmRegs::ESI).callLong((uintptr_t)WaterGenerateTexture_004E68D1).addEsp(4).jmpLong(0x004E6B68);
        // ambient color
        AsmWritter(0x004E5CE3).leaEdxEsp(0x34 - 0x28).push(AsmRegs::EDX).push(AsmRegs::EBX).push(AsmRegs::EDI).push(AsmRegs::EAX)
            .callLong((uintptr_t)GetAmbientColorFromLightmaps_004E5CE3).addEsp(16).jmpNear(0x004E5D57);
        // sub_412410 (what is it?)
        WriteMemUInt8(0x00412430 + 3, 0x34 - 0x20 + 0x18); // pitch instead of width
        WriteMemUInt8(0x0041243D, ASM_NOP, 2); // dont multiply by 2

#ifdef DEBUG
        // FIXME: is it used?
        WriteMemUInt8(0x00412410, 0xC3);
        WriteMemUInt8(0x00412370, 0xC3);
#endif
    }
}
