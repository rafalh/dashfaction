//#define USE_DIRECTX9

#include "VideoDeviceInfoProvider.h"
#include <common/Exception.h>
#include <common/Win32Error.h>

#if USE_D3D9

#include <d3d9.h>

typedef IDirect3D9*(__stdcall *PDIRECT3DCREATE9)(UINT SDKVersion);

#else // USE_D3D9

#include <d3d8.h>

typedef IDirect3D8*(__stdcall *PDIRECT3DCREATE8)(UINT SDKVersion);

#endif // USE_D3D9

VideoDeviceInfoProvider::VideoDeviceInfoProvider()
{
#ifndef USE_D3D9
    m_lib = LoadLibraryA("d3d8.dll");
    if (!m_lib)
        THROW_WIN32_ERROR("Failed to load d3d8.dll");

    PDIRECT3DCREATE8 Direct3DCreate8 = reinterpret_cast<PDIRECT3DCREATE8>(GetProcAddress(m_lib, "Direct3DCreate8"));
    if (!Direct3DCreate8)
        THROW_WIN32_ERROR("Failed to load get Direct3DCreate8 function address");

    m_d3d = Direct3DCreate8(D3D_SDK_VERSION);
    if (!m_d3d)
        THROW_EXCEPTION("Direct3DCreate8 failed");
#else // USE_D3D9
    m_lib = LoadLibraryA("d3d9.dll");
    if (!m_lib)
        THROW_WIN32_ERROR("Failed to load d3d9.dll");

    PDIRECT3DCREATE9 Direct3DCreate9 = reinterpret_cast<PDIRECT3DCREATE9>(GetProcAddress(m_lib, "Direct3DCreate9"));
    if (!Direct3DCreate9)
        THROW_WIN32_ERROR("Failed to load get Direct3DCreate9 function address");

    m_d3d = Direct3DCreate9(D3D_SDK_VERSION);
    if (!m_d3d)
        THROW_EXCEPTION("Direct3DCreate9 failed");
#endif // USE_D3D9
}

VideoDeviceInfoProvider::~VideoDeviceInfoProvider()
{
    m_d3d->Release();
    FreeLibrary(m_lib);
}

std::set<VideoDeviceInfoProvider::Resolution> VideoDeviceInfoProvider::get_resolutions([[maybe_unused]] D3DFORMAT format)
{
    std::set<VideoDeviceInfoProvider::Resolution> result;
    unsigned mode_idx = 0;
    D3DDISPLAYMODE d3d_display_mode;
    while (true)
    {
#ifdef USE_D3D9
        HRESULT hr = m_d3d->EnumAdapterModes(D3DADAPTER_DEFAULT, format, mode_idx++, &d3d_display_mode);
#else
        HRESULT hr = m_d3d->EnumAdapterModes(D3DADAPTER_DEFAULT, mode_idx++, &d3d_display_mode);
#endif
        if (hr == D3DERR_INVALIDCALL)
            break;
        else if (FAILED(hr))
            THROW_EXCEPTION("EnumAdapterModes failed: %lx", hr);

        Resolution res;
        res.width = d3d_display_mode.Width;
        res.height = d3d_display_mode.Height;

        result.insert(res);
    }

    return result;
}

std::set<D3DMULTISAMPLE_TYPE> VideoDeviceInfoProvider::get_multisample_types(D3DFORMAT format, BOOL windowed)
{
    std::set<D3DMULTISAMPLE_TYPE> result;
    for (unsigned i = 2; i < 16; ++i)
    {
#ifdef USE_D3D9
        HRESULT hr = m_d3d->CheckDeviceMultiSampleType(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, format, windowed,
            static_cast<D3DMULTISAMPLE_TYPE>(i), nullptr);
#else
        HRESULT hr = m_d3d->CheckDeviceMultiSampleType(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, format, windowed,
            static_cast<D3DMULTISAMPLE_TYPE>(i));
#endif
        if (SUCCEEDED(hr))
            result.insert(static_cast<D3DMULTISAMPLE_TYPE>(i));
    }
    return result;
}

bool VideoDeviceInfoProvider::has_anisotropy_support()
{
#ifdef USE_D3D9
    D3DCAPS9 caps;
#else
    D3DCAPS8 caps;
#endif
    HRESULT hr = m_d3d->GetDeviceCaps(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, &caps);
    if (FAILED(hr))
        THROW_EXCEPTION("GetDeviceCaps failed, hresult %lx", hr);

    bool anisotropy_cap = (caps.RasterCaps & D3DPRASTERCAPS_ANISOTROPY) != 0;
    return anisotropy_cap && caps.MaxAnisotropy > 0;
}
