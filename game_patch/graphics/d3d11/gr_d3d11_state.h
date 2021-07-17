#pragma once

#include <unordered_map>
#include <map>
#include <d3d11.h>
#include <patch_common/ComPtr.h>

namespace df::gr::d3d11
{
    class StateManager
    {
    public:
        StateManager(ComPtr<ID3D11Device> device);

        ID3D11RasterizerState* lookup_rasterizer_state(D3D11_CULL_MODE cull_mode, int depth_bias = 0)
        {
            auto key = std::make_tuple(cull_mode, depth_bias);
            auto it = rasterizer_state_cache_.find(key);
            if (it != rasterizer_state_cache_.end()) {
                return it->second;
            }
            auto rasterizer_state = create_rasterizer_state(cull_mode, depth_bias);
            auto p = rasterizer_state_cache_.emplace(key, create_rasterizer_state(cull_mode, depth_bias));
            return p.first->second;
        }

        ID3D11SamplerState* lookup_sampler_state(rf::gr::TextureSource ts, int slot)
        {
            if (ts == gr::TEXTURE_SOURCE_NONE) {
                // we are binding a dummy white textures
                ts = gr::TEXTURE_SOURCE_CLAMP;
            }

            if (slot == 1) {
                ts = gr::TEXTURE_SOURCE_CLAMP;
            }

            int key = static_cast<int>(ts);
            auto it = sampler_state_cache_.find(key);
            if (it != sampler_state_cache_.end()) {
                return it->second;
            }

            auto p = sampler_state_cache_.emplace(key, create_sampler_state(ts));
            return p.first->second;
        }

        ID3D11BlendState* lookup_blend_state(rf::gr::AlphaBlend ab)
        {
            int key = static_cast<int>(ab);
            auto it = blend_state_cache_.find(key);
            if (it != blend_state_cache_.end()) {
                return it->second;
            }

            auto p = blend_state_cache_.emplace(key, create_blend_state(ab));
            return p.first->second;
        }

        ID3D11DepthStencilState* lookup_depth_stencil_state(gr::ZbufferType zbt)
        {
            int key = static_cast<int>(zbt) | (static_cast<int>(gr::screen.depthbuffer_type) << 8);
            auto it = depth_stencil_state_cache_.find(key);
            if (it != depth_stencil_state_cache_.end()) {
                return it->second;
            }

            auto p = depth_stencil_state_cache_.emplace(key, create_depth_stencil_state(zbt));
            return p.first->second;
        }

    private:
        ComPtr<ID3D11RasterizerState> create_rasterizer_state(D3D11_CULL_MODE cull_mode, int depth_bias);
        ComPtr<ID3D11SamplerState> create_sampler_state(rf::gr::TextureSource ts);
        ComPtr<ID3D11BlendState> create_blend_state(rf::gr::AlphaBlend ab);
        ComPtr<ID3D11DepthStencilState> create_depth_stencil_state(gr::ZbufferType zbt);

        ComPtr<ID3D11Device> device_;
        std::unordered_map<int, ComPtr<ID3D11SamplerState>> sampler_state_cache_;
        std::unordered_map<int, ComPtr<ID3D11BlendState>> blend_state_cache_;
        std::unordered_map<int, ComPtr<ID3D11DepthStencilState>> depth_stencil_state_cache_;
        std::map<std::tuple<D3D11_CULL_MODE, int>, ComPtr<ID3D11RasterizerState>> rasterizer_state_cache_;
    };
}
