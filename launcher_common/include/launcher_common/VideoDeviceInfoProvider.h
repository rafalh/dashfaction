#pragma once

#include <set>
#include <vector>

// Only include D3D header if one has not been included before (fix for afx.h including d3d9 and DF using d3d8)
#ifndef DIRECT3D_VERSION
#include <windows.h>
#include <d3d8.h>
#endif

struct IDirect3D8;

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
    std::vector<std::string> get_adapters();
    std::set<Resolution> get_resolutions(unsigned adapter, D3DFORMAT format);
    std::set<D3DMULTISAMPLE_TYPE> get_multisample_types(unsigned adapter, D3DFORMAT format, BOOL windowed);
    bool has_anisotropy_support(unsigned adapter);

private:
    IDirect3D8* m_d3d;
    HMODULE m_lib;
};
