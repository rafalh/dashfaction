#pragma once

#include "../rf/common.h"
#include "../rf/bmpman.h"
#include "../rf/file.h"

void ReleaseAllDefaultPoolTextures();
void DestroyTexture(int bmh);
void ChangeUserBitmapPixelFormat(int bmh, rf::BmFormat format, bool dynamic = false);
void InitSupportedTextureFormats();
size_t GetSurfaceLengthInBytes(int w, int h, rf::BmFormat format);

rf::BmType ReadDdsHeader(rf::File& file, int *width_out, int *height_out, rf::BmFormat *format_out,
    int *num_levels_out);
int LockDdsBitmap(rf::BmBitmapEntry& bm_entry);
void BmApplyPatches();
int GetSurfacePitch(int w, rf::BmFormat format);
int GetSurfaceNumRows(int h, rf::BmFormat format);
bool BmIsCompressedFormat(rf::BmFormat format);
bool GrIsFormatSupported(rf::BmFormat format);
