#pragma once

#include <d3d9.h>

class VideoDeviceInfoProvider
{
public:
    struct Resolution
    {
        unsigned width, height;
        bool operator<(const Resolution &other) const
        {
            return width*height < other.width*other.height;
        }
    };

    VideoDeviceInfoProvider();
    ~VideoDeviceInfoProvider();
    std::set<Resolution> getResolutions(D3DFORMAT format);
    std::set<D3DMULTISAMPLE_TYPE> getMultiSampleTypes(D3DFORMAT format, BOOL windowed);
    bool hasAnisotropySupport();

private:
    IDirect3D9 *m_d3d;
    HMODULE m_lib;

    void populateResolutionsForFormat(std::set<Resolution> &resolutions, D3DFORMAT format);
};