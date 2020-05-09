#pragma once

#include "../rf/common.h"
#include "../rf/bmpman.h"

// Custom pixel formats
constexpr auto render_target_pixel_format = static_cast<rf::BmPixelFormat>(0x10); // texture is used as render target
constexpr auto dynamic_texture_pixel_format = static_cast<rf::BmPixelFormat>(0x11); // texture is written frequently by CPU

void ReleaseAllDefaultPoolTextures();
void DestroyTexture(int bmh);
void ChangeUserBitmapPixelFormat(int bmh, rf::BmPixelFormat pixel_fmt);
void InitSupportedTextureFormats();
size_t GetSurfaceLengthInBytes(int w, int h, D3DFORMAT d3d_fmt);

rf::BmBitmapType ReadDdsHeader(rf::File& file, int *width_out, int *height_out, rf::BmPixelFormat *pixel_fmt_out,
    int *num_levels_out);
int LockDdsBitmap(rf::BmBitmapEntry& bm_entry);

constexpr rf::BmBitmapType dds_bm_type = static_cast<rf::BmBitmapType>(0x10);
