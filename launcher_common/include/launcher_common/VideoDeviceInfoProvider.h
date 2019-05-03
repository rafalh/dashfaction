#pragma once

#include <set>

// Only include D3D header if one has not been included before (fix for afx.h including d3d9 and DF using d3d8)
#ifndef DIRECT3D_VERSION
#include <d3d9.h>
#endif

struct IDirect3D9;

class VideoDeviceInfoProvider
{
public:
    struct Resolution
    {
        unsigned width, height;
        bool operator<(const Resolution& other) const
        {
            return width * height < other.width * other.height;
        }
    };

    VideoDeviceInfoProvider();
    ~VideoDeviceInfoProvider();
    std::set<Resolution> getResolutions(D3DFORMAT format);
    std::set<D3DMULTISAMPLE_TYPE> getMultiSampleTypes(D3DFORMAT format, BOOL windowed);
    bool hasAnisotropySupport();

private:
    IDirect3D9* m_d3d;
    HMODULE m_lib;

    void populateResolutionsForFormat(std::set<Resolution>& resolutions, D3DFORMAT format);
};
