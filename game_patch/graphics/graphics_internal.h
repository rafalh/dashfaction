#pragma once

#include "../rf/bmpman.h"

// Custom pixel formats
constexpr auto render_target_pixel_format = static_cast<rf::BmPixelFormat>(0x10); // texture is used as render target
constexpr auto dynamic_texture_pixel_format = static_cast<rf::BmPixelFormat>(0x11); // texture is written frequently by CPU

void ReleaseAllDefaultPoolTextures();
void DestroyTexture(int bmh);
void ChangeUserBitmapPixelFormat(int bmh, rf::BmPixelFormat pixel_fmt);
void InitSupportedTextureFormats();
