#pragma once

#include "../rf/common.h"
#include "../rf/bmpman.h"
#include "../rf/fs.h"

void ReleaseAllDefaultPoolTextures();
void DestroyTexture(int bmh);
void ChangeUserBitmapPixelFormat(int bmh, rf::BmPixelFormat pixel_fmt, bool dynamic = false);
void InitSupportedTextureFormats();
#ifdef DIRECT3D_VERSION
size_t GetSurfaceLengthInBytes(int w, int h, D3DFORMAT d3d_fmt);
#endif

rf::BmBitmapType ReadDdsHeader(rf::File& file, int *width_out, int *height_out, rf::BmPixelFormat *pixel_fmt_out,
    int *num_levels_out);
int LockDdsBitmap(rf::BmBitmapEntry& bm_entry);
