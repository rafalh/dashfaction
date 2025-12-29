#include <windows.h>
#include <d3d11.h>
#include "VideoDeviceInfoProvider.h"
#include <common/error/Exception.h>
#include <common/error/Win32Error.h>
#include <common/DynamicLinkLibrary.h>
#include <common/ComPtr.h>
#include <xlog/xlog.h>
#include <dxgi.h>

class VideoDeviceInfoProviderD3D11 : public VideoDeviceInfoProvider
{
public:
    VideoDeviceInfoProviderD3D11();
    ~VideoDeviceInfoProviderD3D11() override = default;
    std::vector<std::string> get_adapters() override;
    std::set<Resolution> get_resolutions(unsigned adapter, unsigned format) override;
    std::set<uint32_t> get_multisample_types(uint32_t adapter, uint32_t format, bool windowed) override;
    bool has_anisotropy_support(unsigned adapter) override;

    unsigned get_format_from_bpp(unsigned bpp) override
    {
        switch (bpp) {
            case 16: return static_cast<unsigned>(DXGI_FORMAT_B5G6R5_UNORM);
            case 32: return static_cast<unsigned>(DXGI_FORMAT_B8G8R8A8_UNORM);
            default: return static_cast<unsigned>(DXGI_FORMAT_UNKNOWN);
        }
    }

private:
    DynamicLinkLibrary m_dxgi_lib;
    DynamicLinkLibrary m_d3d11_lib;
    ComPtr<IDXGIFactory> m_factory;
    decltype(D3D11CreateDevice)* m_D3D11CreateDevice;
};

VideoDeviceInfoProviderD3D11::VideoDeviceInfoProviderD3D11() :
    m_dxgi_lib{L"dxgi.dll"}, m_d3d11_lib{L"d3d11.dll"}
{
    if (!m_dxgi_lib)
        THROW_WIN32_ERROR("Failed to load dxgi.dll");

    auto pCreateDXGIFactory = m_dxgi_lib.get_proc_address<decltype(CreateDXGIFactory)*>("CreateDXGIFactory");
    if (!pCreateDXGIFactory)
        THROW_WIN32_ERROR("Failed to load get CreateDXGIFactory function address");

    HRESULT hr = pCreateDXGIFactory(IID_IDXGIFactory, reinterpret_cast<void**>(&m_factory));
    if (FAILED(hr))
        THROW_EXCEPTION("CreateDXGIFactory failed");

    if (!m_d3d11_lib)
        THROW_WIN32_ERROR("Failed to load d3d11.dll");

    m_D3D11CreateDevice = m_d3d11_lib.get_proc_address<decltype(D3D11CreateDevice)*>("D3D11CreateDevice");
    if (!m_D3D11CreateDevice)
        THROW_WIN32_ERROR("Failed to load get D3D11CreateDevice function address");
}

std::vector<std::string> VideoDeviceInfoProviderD3D11::get_adapters()
{
    std::vector<std::string> adapters;
    unsigned adapter_idx = 0;
    while (true) {
        ComPtr<IDXGIAdapter> adapter;
        HRESULT hr = m_factory->EnumAdapters(adapter_idx++, &adapter);
        if (FAILED(hr)) {
            if (hr != DXGI_ERROR_NOT_FOUND) {
                xlog::error("EnumAdapters failed: {:x}", hr);
            }
            break;
        }
        DXGI_ADAPTER_DESC desc;
        hr = adapter->GetDesc(&desc);
        if (FAILED(hr)) {
            xlog::error("GetDesc failed: {:x}", hr);
            break;
        }

        char name[std::size(desc.Description)];
        WideCharToMultiByte(CP_ACP, 0, desc.Description, -1, name, sizeof(name), nullptr, nullptr);
        adapters.emplace_back(name);
    }
    return adapters;
}

std::set<VideoDeviceInfoProvider::Resolution> VideoDeviceInfoProviderD3D11::get_resolutions(
    unsigned adapter,
    unsigned format
) {
    ComPtr<IDXGIAdapter> dxgi_adapter = nullptr;
    HRESULT hr = m_factory->EnumAdapters(adapter, &dxgi_adapter);
    if (FAILED(hr)) {
        xlog::error("EnumAdapters failed: {:x}", hr);
        return {};
    }

    std::set<Resolution> result;

    ComPtr<IDXGIOutput> dxgi_output;
    hr = dxgi_adapter->EnumOutputs(0, &dxgi_output); // FIXME: what about other outputs?
    if (FAILED(hr)) {
        xlog::error("EnumOutputs failed: {:x}", hr);
        return {};
    }

    unsigned mode_count = 0;
    hr = dxgi_output->GetDisplayModeList(static_cast<DXGI_FORMAT>(format), 0, &mode_count, nullptr);
    if (FAILED(hr)) {
        xlog::error("GetDisplayModeList failed: {:x}", hr);
    }

    std::vector<DXGI_MODE_DESC> desc_vec;
    desc_vec.resize(mode_count);
    hr = dxgi_output->GetDisplayModeList(static_cast<DXGI_FORMAT>(format), 0, &mode_count, desc_vec.data());
    if (FAILED(hr)) {
        xlog::error("GetDisplayModeList failed: {:x}", hr);
    }

    for (auto& desc : desc_vec) {
        result.insert({desc.Width, desc.Height});
    }

    return result;
}

std::set<uint32_t> VideoDeviceInfoProviderD3D11::get_multisample_types(
    const uint32_t adapter,
    const uint32_t format,
    [[maybe_unused]] const bool windowed
) {
    ComPtr<IDXGIAdapter> dxgi_adapter{};
    HRESULT hr = m_factory->EnumAdapters(adapter, &dxgi_adapter);
    if (FAILED(hr)) {
        xlog::error("EnumAdapters failed: {:x}", hr);
        return {};
    }

    ComPtr<ID3D11Device> d3d11_device{};
    hr = m_D3D11CreateDevice(
        dxgi_adapter,
        D3D_DRIVER_TYPE_UNKNOWN,
        nullptr,
        0,
        nullptr,
        0,
        D3D11_SDK_VERSION,
        &d3d11_device,
        nullptr,
        nullptr
    );
    if (FAILED(hr)) {
        xlog::error("D3D11CreateDevice failed: {:x}", hr);
        return {};
    }

    std::set<uint32_t> result{};
    for (uint32_t i = 2; i <= D3D11_MAX_MULTISAMPLE_SAMPLE_COUNT; i <<= 1) {
        UINT quality_levels = 0;
        const HRESULT hr = d3d11_device->CheckMultisampleQualityLevels(
            static_cast<DXGI_FORMAT>(format),
            i,
            &quality_levels
        );
        if (SUCCEEDED(hr) && quality_levels > 0) {
            result.insert(i);
        }
    }
    return result;
}

bool VideoDeviceInfoProviderD3D11::has_anisotropy_support(
    [[maybe_unused]] unsigned adapter
) {
    // 9_1 feature level already supports anistoropy level 2
    return true;
}

std::unique_ptr<VideoDeviceInfoProvider> create_d3d11_device_info_provider()
{
    return std::make_unique<VideoDeviceInfoProviderD3D11>();
}
