#include "gr_d3d11.h"

using namespace rf;

D3D11StateManager::D3D11StateManager(ComPtr<ID3D11Device> device) :
    device_{std::move(device)}
{
    init_rasterizer_state();
}

ID3D11RasterizerState* D3D11StateManager::get_rasterizer_state()
{
    return rasterizer_state_;
}

void D3D11StateManager::init_rasterizer_state()
{
    CD3D11_RASTERIZER_DESC desc{CD3D11_DEFAULT{}};
    desc.CullMode = D3D11_CULL_BACK;
    HRESULT hr = device_->CreateRasterizerState(&desc, &rasterizer_state_);
    check_hr(hr, "CreateRasterizerState");
}

ID3D11SamplerState* D3D11StateManager::lookup_sampler_state(gr::Mode mode, int slot)
{
    auto ts = mode.get_texture_source();
    if (ts == gr::TEXTURE_SOURCE_NONE) {
        // we are binding a dummy white textures
        ts = gr::TEXTURE_SOURCE_CLAMP;
    }

    if (slot == 1) {
        ts = gr::TEXTURE_SOURCE_CLAMP;
    }

    int cache_key = static_cast<int>(ts);
    const auto& it = sampler_state_cache_.find(cache_key);
    if (it != sampler_state_cache_.end()) {
        return it->second;
    }

    CD3D11_SAMPLER_DESC desc{CD3D11_DEFAULT()};
    switch (ts) {
        case gr::TEXTURE_SOURCE_WRAP:
        case gr::TEXTURE_SOURCE_CLAMP_1_WRAP_0:
        case gr::TEXTURE_SOURCE_CLAMP_1_WRAP_0_MOD2X:
        case gr::TEXTURE_SOURCE_MT_WRAP_TRILIN: // WRAP_1_WRAP_0
            desc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
            desc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
            break;
        case gr::TEXTURE_SOURCE_CLAMP:
        case gr::TEXTURE_SOURCE_CLAMP_1_CLAMP_0:
        case gr::TEXTURE_SOURCE_MT_CLAMP_TRILIN: // WRAP_1_CLAMP_0
            desc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
            desc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
            break;
        case gr::TEXTURE_SOURCE_CLAMP_NO_FILTERING:
            desc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
            desc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
            desc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
            break;
        case gr::TEXTURE_SOURCE_MT_U_WRAP_V_CLAMP:
            desc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
            desc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
            break;
        case gr::TEXTURE_SOURCE_MT_U_CLAMP_V_WRAP:
            desc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
            desc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
            break;
        default:
            break;
    }

    ComPtr<ID3D11SamplerState> sampler_state;
    HRESULT hr = device_->CreateSamplerState(&desc, &sampler_state);
    check_hr(hr, "CreateSamplerState");

    sampler_state_cache_.insert({cache_key, sampler_state});
    return sampler_state;
}

ID3D11BlendState* D3D11StateManager::lookup_blend_state(gr::Mode mode)
{
    auto ab = mode.get_alpha_blend();
    int cache_key = static_cast<int>(ab);

    const auto& it = blend_state_cache_.find(cache_key);
    if (it != blend_state_cache_.end()) {
        return it->second;
    }
    CD3D11_BLEND_DESC desc{CD3D11_DEFAULT()};

    switch (ab) {
    case gr::ALPHA_BLEND_NONE:
        desc.RenderTarget[0].BlendEnable = FALSE;
        break;
    case gr::ALPHA_BLEND_ADDITIVE:
        desc.RenderTarget[0].BlendEnable = TRUE;
        desc.RenderTarget[0].SrcBlend = D3D11_BLEND_ONE;
        desc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
        desc.RenderTarget[0].DestBlend = D3D11_BLEND_ONE;
        desc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ONE;
        break;
    case gr::ALPHA_BLEND_ALPHA_ADDITIVE:
    case gr::ALPHA_BLEND_SRC_COLOR:
        desc.RenderTarget[0].BlendEnable = TRUE;
        desc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
        desc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_SRC_ALPHA;
        desc.RenderTarget[0].DestBlend = D3D11_BLEND_ONE;
        desc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ONE;
        break;
    case gr::ALPHA_BLEND_ALPHA:
        desc.RenderTarget[0].BlendEnable = TRUE;
        desc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
        desc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_SRC_ALPHA;
        desc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
        desc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
        break;
    case gr::ALPHA_BLEND_LIGHTMAP:
        desc.RenderTarget[0].BlendEnable = TRUE;
        desc.RenderTarget[0].SrcBlend = D3D11_BLEND_DEST_COLOR;
        desc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_DEST_COLOR;
        desc.RenderTarget[0].DestBlend = D3D11_BLEND_ZERO;
        desc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
        break;
    case gr::ALPHA_BLEND_SUBTRACTIVE:
        desc.RenderTarget[0].BlendEnable = TRUE;
        desc.RenderTarget[0].SrcBlend = D3D11_BLEND_INV_DEST_COLOR;
        desc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_INV_DEST_COLOR;
        desc.RenderTarget[0].DestBlend = D3D11_BLEND_ZERO;
        desc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
        break;
    case gr::ALPHA_BLEND_SWAPPED_SRC_DEST_COLOR:
        desc.RenderTarget[0].BlendEnable = TRUE;
        desc.RenderTarget[0].SrcBlend = D3D11_BLEND_DEST_COLOR;
        desc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_DEST_COLOR;
        desc.RenderTarget[0].DestBlend = D3D11_BLEND_SRC_COLOR;
        desc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_SRC_COLOR;
        break;
    }

    ComPtr<ID3D11BlendState> blend_state;
    HRESULT hr = device_->CreateBlendState(&desc, &blend_state);
    check_hr(hr, "CreateBlendState");

    blend_state_cache_.insert({cache_key, blend_state});
    return blend_state;
}

ID3D11DepthStencilState* D3D11StateManager::lookup_depth_stencil_state(gr::Mode mode)
{
    auto zbt = mode.get_zbuffer_type();
    int cache_key = static_cast<int>(zbt) | (static_cast<int>(gr::screen.depthbuffer_type) << 8);

    const auto& it = depth_stencil_state_cache_.find(cache_key);
    if (it != depth_stencil_state_cache_.end()) {
        return it->second;
    }
    CD3D11_DEPTH_STENCIL_DESC desc{CD3D11_DEFAULT()};

    if (gr::screen.depthbuffer_type == gr::DEPTHBUFFER_Z) {
        desc.DepthFunc = D3D11_COMPARISON_GREATER_EQUAL;
    }
    else {
        desc.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;
    }
    switch (zbt) {
    case gr::ZBUFFER_TYPE_NONE:
        desc.DepthEnable = FALSE;
        desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
        break;
    case gr::ZBUFFER_TYPE_READ:
        desc.DepthEnable = TRUE;
        desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
        break;
    case gr::ZBUFFER_TYPE_READ_EQUAL:
        desc.DepthEnable = TRUE;
        desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
        desc.DepthFunc = D3D11_COMPARISON_EQUAL;
        break;
    case gr::ZBUFFER_TYPE_WRITE:
        desc.DepthEnable = TRUE;
        desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
        desc.DepthFunc = D3D11_COMPARISON_ALWAYS;
        break;
    case gr::ZBUFFER_TYPE_FULL:
    case gr::ZBUFFER_TYPE_FULL_ALPHA_TEST:
        desc.DepthEnable = TRUE;
        desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
        break;
    }

    ComPtr<ID3D11DepthStencilState> depth_stencil_state;
    HRESULT hr = device_->CreateDepthStencilState(&desc, &depth_stencil_state);
    check_hr(hr, "CreateDepthStencilState");

    depth_stencil_state_cache_.insert({cache_key, depth_stencil_state});
    return depth_stencil_state;
}
