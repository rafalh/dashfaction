#pragma once

#include <set>
#include <vector>
#include <string>
#include <windows.h>

#define USE_D3D9 1

// Only include D3D header if one has not been included before (fix for afx.h including d3d9 and DF using d3d8)
#if USE_D3D9
#include <d3d9.h>
#elif !defined(DIRECT3D_VERSION)
#include <d3d8.h>
#else
struct IDirect3D8;
#endif

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
#if USE_D3D9
    IDirect3D9* m_d3d;
#else
    IDirect3D8* m_d3d;
#endif
    HMODULE m_lib;
};
