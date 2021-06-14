#include <cassert>
#include <cstring>
#include "gr_d3d11.h"

using namespace rf;

std::array<std::array<float, 4>, 4> gr_d3d11_build_identity_matrix();

D3D11RenderContext::D3D11RenderContext(ComPtr<ID3D11Device> device, ComPtr<ID3D11DeviceContext> context, D3D11StateManager& state_manager, D3D11TextureManager& texture_manager) :
    device_{std::move(device)}, context_{std::move(context)},
    state_manager_{state_manager}, texture_manager_{texture_manager}
{
    context_->RSSetState(state_manager_.get_rasterizer_state());
    init_ps_cbuffer();
    init_vs_cbuffer();

    white_bm_ = rf::bm::create(rf::bm::FORMAT_888_RGB, 1, 1);
    assert(white_bm_ != -1);
    rf::gr::LockInfo lock;
    if (rf::gr::lock(white_bm_, 0, &lock, rf::gr::LOCK_WRITE_ONLY)) {
        std::memset(lock.data, 0xFF, lock.stride_in_bytes * lock.h);
        rf::gr::unlock(&lock);
    }
}

void D3D11RenderContext::init_ps_cbuffer()
{
    D3D11_BUFFER_DESC buffer_desc;
    ZeroMemory(&buffer_desc, sizeof(buffer_desc));
    buffer_desc.Usage            = D3D11_USAGE_DYNAMIC;
    buffer_desc.ByteWidth        = sizeof(PixelShaderUniforms);
    buffer_desc.BindFlags        = D3D11_BIND_CONSTANT_BUFFER;
    buffer_desc.CPUAccessFlags   = D3D11_CPU_ACCESS_WRITE;
    buffer_desc.MiscFlags        = 0;
    buffer_desc.StructureByteStride = 0;

    PixelShaderUniforms data;
    std::memset(&data, 0, sizeof(data));
    D3D11_SUBRESOURCE_DATA subres_data;
    subres_data.pSysMem = &data;
    subres_data.SysMemPitch = 0;
    subres_data.SysMemSlicePitch = 0;

    HRESULT hr = device_->CreateBuffer(&buffer_desc, &subres_data, &ps_cbuffer_);
    check_hr(hr, "CreateBuffer");

    ID3D11Buffer* ps_cbuffers[] = { ps_cbuffer_ };
    context_->PSSetConstantBuffers(0, std::size(ps_cbuffers), ps_cbuffers);
}

void D3D11RenderContext::init_vs_cbuffer()
{
    D3D11_BUFFER_DESC buffer_desc;
    ZeroMemory(&buffer_desc, sizeof(buffer_desc));
    buffer_desc.Usage            = D3D11_USAGE_DYNAMIC;
    buffer_desc.ByteWidth        = sizeof(VertexShaderUniforms);
    buffer_desc.BindFlags        = D3D11_BIND_CONSTANT_BUFFER;
    buffer_desc.CPUAccessFlags   = D3D11_CPU_ACCESS_WRITE;
    buffer_desc.MiscFlags        = 0;
    buffer_desc.StructureByteStride = 0;

    VertexShaderUniforms data;
    std::memset(&data, 0, sizeof(data));
    data.model_mat = gr_d3d11_build_identity_matrix();
    data.view_mat = gr_d3d11_build_identity_matrix();
    data.proj_mat = gr_d3d11_build_identity_matrix();

    D3D11_SUBRESOURCE_DATA subres_data;
    subres_data.pSysMem = &data;
    subres_data.SysMemPitch = 0;
    subres_data.SysMemSlicePitch = 0;

    HRESULT hr = device_->CreateBuffer(&buffer_desc, &subres_data, &vs_cbuffer_);
    check_hr(hr, "CreateBuffer");

    ZeroMemory(&buffer_desc, sizeof(buffer_desc));
    buffer_desc.Usage            = D3D11_USAGE_DYNAMIC;
    buffer_desc.ByteWidth        = sizeof(TexCoordTransformUniform);
    buffer_desc.BindFlags        = D3D11_BIND_CONSTANT_BUFFER;
    buffer_desc.CPUAccessFlags   = D3D11_CPU_ACCESS_WRITE;
    buffer_desc.MiscFlags        = 0;
    buffer_desc.StructureByteStride = 0;

    TexCoordTransformUniform texture_transform_uniform;
    current_texture_transform_ = gr_d3d11_build_identity_matrix3();
    texture_transform_uniform.mat = gr_d3d11_convert_to_4x3_matrix(current_texture_transform_);
    subres_data.pSysMem = &texture_transform_uniform;
    hr = device_->CreateBuffer(&buffer_desc, &subres_data, &texture_transform_cbuffer_);
    check_hr(hr, "CreateBuffer");

    ID3D11Buffer* vs_cbuffers[] = { vs_cbuffer_, texture_transform_cbuffer_ };
    context_->VSSetConstantBuffers(0, std::size(vs_cbuffers), vs_cbuffers);
}

void D3D11RenderContext::set_mode(gr::Mode mode)
{
    float vcolor_rgb_mul = 0.0f;
    float tex0_rgb_mul = 0.0f;
    float tex0_rgb_add = 0.0f;
    switch (mode.get_color_source()) {
        case rf::gr::COLOR_SOURCE_VERTEX:
            vcolor_rgb_mul = 1.0f;
            tex0_rgb_mul = 0.0f;
            tex0_rgb_add = 0.0f;
            break;
        case rf::gr::COLOR_SOURCE_TEXTURE:
            vcolor_rgb_mul = 0.0f;
            tex0_rgb_mul = 1.0f;
            tex0_rgb_add = 0.0f;
            break;
        case rf::gr::COLOR_SOURCE_VERTEX_TIMES_TEXTURE:
        case rf::gr::COLOR_SOURCE_VERTEX_TIMES_TEXTURE_2X:
            vcolor_rgb_mul = 1.0f;
            tex0_rgb_mul = 1.0f;
            tex0_rgb_add = 0.0f;
            break;
        case rf::gr::COLOR_SOURCE_VERTEX_PLUS_TEXTURE:
            vcolor_rgb_mul = 1.0f;
            tex0_rgb_mul = 0.0f;
            tex0_rgb_add = 1.0f;
            break;
    }

    float vcolor_a_mul = 0.0f;
    float tex0_a_mul = 0.0f;
    switch (mode.get_alpha_source()) {
        case rf::gr::ALPHA_SOURCE_VERTEX:
        case rf::gr::ALPHA_SOURCE_VERTEX_NONDARKENING:
            vcolor_a_mul = 1.0f;
            tex0_a_mul = 0.0f;
            break;
        case rf::gr::ALPHA_SOURCE_TEXTURE:
            vcolor_a_mul = 0.0f;
            tex0_a_mul = 1.0f;
            break;
        case rf::gr::ALPHA_SOURCE_VERTEX_TIMES_TEXTURE:
            vcolor_a_mul = 1.0f;
            tex0_a_mul = 1.0f;
            break;
    }

    float output_mul_rgb = 1.0f;
    if (mode.get_texture_source() == rf::gr::TEXTURE_SOURCE_CLAMP_1_WRAP_0_MOD2X ||
        mode.get_texture_source() == rf::gr::TEXTURE_SOURCE_CLAMP_1_WRAP_0) {
        output_mul_rgb = 2.0f;
    }

    float alpha_test = mode.get_zbuffer_type() == gr::ZBUFFER_TYPE_FULL_ALPHA_TEST ? 1.0f : 0.0f;

    PixelShaderUniforms ps_data;
    ps_data.vcolor_mul = {vcolor_rgb_mul, vcolor_a_mul};
    ps_data.vcolor_mul_inv = {1.0f - vcolor_rgb_mul, 1.0f - vcolor_a_mul};
    ps_data.tex0_mul = {tex0_rgb_mul, tex0_a_mul};
    ps_data.tex0_mul_inv = {1.0f - tex0_rgb_mul, 1.0f - tex0_a_mul};
    ps_data.tex0_add_rgb = tex0_rgb_add;
    ps_data.output_mul_rgb = output_mul_rgb;
    ps_data.alpha_test = alpha_test ? 1.0f : 0.0f;

    D3D11_MAPPED_SUBRESOURCE mapped_cbuffer;
    HRESULT hr = context_->Map(ps_cbuffer_, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped_cbuffer);
    check_hr(hr, "Map");
    std::memcpy(mapped_cbuffer.pData, &ps_data, sizeof(ps_data));
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
}

void D3D11RenderContext::set_mode_and_textures(rf::gr::Mode mode, int bm_handle1, int bm_handle2)
{
    set_mode(mode);
    auto textures = gr_d3d11_normalize_texture_handles_for_mode(mode, {bm_handle1, bm_handle2});
    set_texture(0, textures[0]);
    set_texture(1, textures[1]);
}

void D3D11RenderContext::update_vs_uniforms(const VertexShaderUniforms& data)
{
    D3D11_MAPPED_SUBRESOURCE mapped_cbuffer;
    HRESULT hr = context_->Map(vs_cbuffer_, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped_cbuffer);
    check_hr(hr, "Map");
    std::memcpy(mapped_cbuffer.pData, &data, sizeof(data));
    context_->Unmap(vs_cbuffer_, 0);
}

void D3D11RenderContext::set_texture(int slot, int bm_handle)
{
    if (bm_handle == -1) {
        bm_handle = white_bm_;
    }
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

void D3D11RenderContext::set_vertex_buffer(ID3D11Buffer* vb)
{
    UINT stride = sizeof(GpuVertex);
    UINT offset = 0;
    ID3D11Buffer* vertex_buffers[] = { vb };
    context_->IASetVertexBuffers(0, std::size(vertex_buffers), vertex_buffers, &stride, &offset);
}

void D3D11RenderContext::set_index_buffer(ID3D11Buffer* ib)
{
    context_->IASetIndexBuffer(ib, DXGI_FORMAT_R16_UINT, 0);
}

void D3D11RenderContext::set_texture_transform(const GrMatrix3& transform)
{
    if (current_texture_transform_ == transform) {
        return;
    }
    current_texture_transform_ = transform;
    TexCoordTransformUniform uniforms;
    uniforms.mat = gr_d3d11_convert_to_4x3_matrix(transform);
    D3D11_MAPPED_SUBRESOURCE mapped_cbuffer;
    HRESULT hr = context_->Map(texture_transform_cbuffer_, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped_cbuffer);
    check_hr(hr, "Map");
    std::memcpy(mapped_cbuffer.pData, &uniforms, sizeof(uniforms));
    context_->Unmap(texture_transform_cbuffer_, 0);
}
