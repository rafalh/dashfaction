#include "gr_d3d11.h"

using namespace rf;

D3D11RenderContext::D3D11RenderContext(ComPtr<ID3D11Device> device, ComPtr<ID3D11DeviceContext> context, D3D11StateManager& state_manager, D3D11TextureManager& texture_manager) :
    device_{std::move(device)}, context_{std::move(context)},
    state_manager_{state_manager}, texture_manager_{texture_manager}
{
    context_->RSSetState(state_manager_.get_rasterizer_state());
    init_constant_buffer();
}

void D3D11RenderContext::init_constant_buffer()
{
    D3D11_BUFFER_DESC buffer_desc;
    ZeroMemory(&buffer_desc, sizeof(buffer_desc));
    buffer_desc.Usage            = D3D11_USAGE_DYNAMIC;
    buffer_desc.ByteWidth        = sizeof(PixelShaderConstantBuffer);
    buffer_desc.BindFlags        = D3D11_BIND_CONSTANT_BUFFER;
    buffer_desc.CPUAccessFlags   = D3D11_CPU_ACCESS_WRITE;
    buffer_desc.MiscFlags        = 0;
    buffer_desc.StructureByteStride = 0;

    PixelShaderConstantBuffer data;
    data.ts = 0.0f;
    D3D11_SUBRESOURCE_DATA subres_data;
    subres_data.pSysMem = &data;
    subres_data.SysMemPitch = 0;
    subres_data.SysMemSlicePitch = 0;

    HRESULT hr = device_->CreateBuffer(&buffer_desc, &subres_data, &ps_cbuffer_);
    check_hr(hr, "CreateBuffer");

    ID3D11Buffer* ps_cbuffers[] = { ps_cbuffer_ };
    context_->PSSetConstantBuffers(0, std::size(ps_cbuffers), ps_cbuffers);
}

void D3D11RenderContext::set_mode(gr::Mode mode)
{
    D3D11_MAPPED_SUBRESOURCE mapped_cbuffer;
    HRESULT hr = context_->Map(ps_cbuffer_, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped_cbuffer);
    check_hr(hr, "Map");

    PixelShaderConstantBuffer& ps_data = *reinterpret_cast<PixelShaderConstantBuffer*>(mapped_cbuffer.pData);
    ps_data.ts = mode.get_texture_source();
    ps_data.cs = mode.get_color_source();
    ps_data.as = mode.get_alpha_source();
    bool alpha_test = mode.get_zbuffer_type() == gr::ZBUFFER_TYPE_FULL_ALPHA_TEST;
    ps_data.alpha_test = alpha_test ? 1.0f : 0.0f;
    context_->Unmap(ps_cbuffer_, 0);

    ID3D11SamplerState* sampler_states[] = {
        state_manager_.lookup_sampler_state(mode, 0),
        state_manager_.lookup_sampler_state(mode, 1),
    };
    context_->PSSetSamplers(0, std::size(sampler_states), sampler_states);

    ID3D11BlendState* blend_state = state_manager_.lookup_blend_state(mode);
    context_->OMSetBlendState(blend_state, nullptr, 0xffffffff);

    ID3D11DepthStencilState* depth_stencil_state = state_manager_.lookup_depth_stencil_state(mode);
    context_->OMSetDepthStencilState(depth_stencil_state, 0);

    // if (get_texture_source(mode) == TEXTURE_SOURCE_NONE) {
    //     gr::screen.current_bitmap = -1;
    // }
}

void D3D11RenderContext::set_texture(int slot, int bm_handle)
{
    ID3D11ShaderResourceView* shader_resources[] = { texture_manager_.lookup_texture(bm_handle) };
    context_->PSSetShaderResources(slot, std::size(shader_resources), shader_resources);
}

void D3D11RenderContext::set_render_target(
    const ComPtr<ID3D11RenderTargetView>& back_buffer_view,
    const ComPtr<ID3D11DepthStencilView>& depth_stencil_buffer_view
)
{
    ID3D11RenderTargetView* render_targets[] = { back_buffer_view };
    context_->OMSetRenderTargets(std::size(render_targets), render_targets, depth_stencil_buffer_view);
}
