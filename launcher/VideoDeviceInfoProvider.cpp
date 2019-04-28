//#define WINBOOL BOOL

#include "VideoDeviceInfoProvider.h"
#include "exception.h"
//#include <d3d8.h>
#include <d3d9.h>

//typedef IDirect3D8*(*PDIRECT3DCREATE8)(UINT SDKVersion);
typedef IDirect3D9*(__stdcall *PDIRECT3DCREATE9)(UINT SDKVersion);

VideoDeviceInfoProvider::VideoDeviceInfoProvider()
{
#if 0
    m_lib = LoadLibraryA("d3d8.dll");
    if (!m_lib)
        THROW_EXCEPTION("Failed to load d3d8.dll: win32 error %lu", GetLastError());

    PDIRECT3DCREATE pDirect3DCreate8 = (PDIRECT3DCREATE)GetProcAddress(m_lib, "Direct3DCreate8");
    if (!pDirect3DCreate8)
        THROW_EXCEPTION("Failed to load get Direct3DCreate8 function address: win32 error %lu", GetLastError());

    m_d3d = pDirect3DCreate8(D3D_SDK_VERSION);
    if (!m_d3d)
        THROW_EXCEPTION("Direct3DCreate8 failed");
#elif 0
    m_d3d = Direct3DCreate9(D3D_SDK_VERSION);
    if (!m_d3d)
        THROW_EXCEPTION("Direct3DCreate9 failed");
#else
    m_lib = LoadLibraryA("d3d9.dll");
    if (!m_lib)
        THROW_EXCEPTION("Failed to load d3d9.dll: win32 error %lu", GetLastError());

    PDIRECT3DCREATE9 pDirect3DCreate9 = (PDIRECT3DCREATE9)GetProcAddress(m_lib, "Direct3DCreate9");
    if (!pDirect3DCreate9)
        THROW_EXCEPTION("Failed to load get Direct3DCreate9 function address: win32 error %lu", GetLastError());

    m_d3d = pDirect3DCreate9(D3D_SDK_VERSION);
    if (!m_d3d)
        THROW_EXCEPTION("Direct3DCreate9 failed");
#endif
}

VideoDeviceInfoProvider::~VideoDeviceInfoProvider()
{
    m_d3d->Release();
    FreeLibrary(m_lib);
}

std::set<VideoDeviceInfoProvider::Resolution> VideoDeviceInfoProvider::getResolutions(D3DFORMAT format)
{
    std::set<VideoDeviceInfoProvider::Resolution> result;
    unsigned modeIdx = 0;
    D3DDISPLAYMODE d3dDisplayMode;
    while (true)
    {
        HRESULT hr = m_d3d->EnumAdapterModes(D3DADAPTER_DEFAULT, format, modeIdx++, &d3dDisplayMode);
        if (hr == D3DERR_INVALIDCALL)
            break;
        else if (FAILED(hr))
            THROW_EXCEPTION("EnumAdapterModes failed: %lx", hr);

        Resolution res;
        res.width = d3dDisplayMode.Width;
        res.height = d3dDisplayMode.Height;

        result.insert(res);
    }
    
    return result;
}

std::set<D3DMULTISAMPLE_TYPE> VideoDeviceInfoProvider::getMultiSampleTypes(D3DFORMAT format, BOOL windowed)
{
    std::set<D3DMULTISAMPLE_TYPE> result;
    for (unsigned i = 2; i < 16; ++i)
    {
        HRESULT hr = m_d3d->CheckDeviceMultiSampleType(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, format, windowed, 
            (D3DMULTISAMPLE_TYPE) i, NULL);
        if (SUCCEEDED(hr))
            result.insert((D3DMULTISAMPLE_TYPE)i);
    }
    return result;
}

bool VideoDeviceInfoProvider::hasAnisotropySupport()
{
    D3DCAPS9 caps;
    HRESULT hr = m_d3d->GetDeviceCaps(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, &caps);
    if (FAILED(hr))
        THROW_EXCEPTION("GetDeviceCaps failed, hresult %lx", hr);

    return (caps.RasterCaps & D3DPRASTERCAPS_ANISOTROPY) != 0 && caps.MaxAnisotropy > 0;
}
