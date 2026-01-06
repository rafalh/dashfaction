#pragma once

#include <set>
#include <vector>
#include <string>
#include <memory>
#include <common/config/GameConfig.h>

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

    virtual ~VideoDeviceInfoProvider() = default;
    virtual std::vector<std::string> get_adapters() = 0;
    virtual unsigned get_format_from_bpp(unsigned bpp) = 0;
    virtual std::set<Resolution> get_resolutions(unsigned adapter, unsigned format) = 0;
    virtual std::set<uint32_t> get_multisample_types(uint32_t adapter, uint32_t format, bool windowed) = 0;
    virtual bool has_anisotropy_support(unsigned adapter) = 0;
};

std::unique_ptr<VideoDeviceInfoProvider> create_d3d8_device_info_provider();
std::unique_ptr<VideoDeviceInfoProvider> create_d3d9_device_info_provider();
std::unique_ptr<VideoDeviceInfoProvider> create_d3d11_device_info_provider();

inline std::unique_ptr<VideoDeviceInfoProvider> create_device_info_provider(GameConfig::Renderer renderer)
{
    switch (renderer) {
        case GameConfig::Renderer::d3d8:
            return create_d3d8_device_info_provider();
        case GameConfig::Renderer::d3d9:
            return create_d3d9_device_info_provider();
        case GameConfig::Renderer::d3d11:
            return create_d3d11_device_info_provider();
        default:
            return {};
    }
}
