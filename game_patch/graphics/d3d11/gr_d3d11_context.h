#pragma once

#include <d3d11.h>
#include <patch_common/ComPtr.h>
#include "gr_d3d11_transform.h"

class D3D11StateManager;
class D3D11TextureManager;

struct GpuVertex
{
    float x;
    float y;
    float z;
    float u0;
    float v0;
    float u1;
    float v1;
    int diffuse;
};

struct CameraUniforms
{
    std::array<std::array<float, 4>, 4> model_mat;
    std::array<std::array<float, 4>, 4> view_mat;
    std::array<std::array<float, 4>, 4> proj_mat;
};

struct TexCoordTransformUniform
{
    std::array<std::array<float, 4>, 3> mat;
};

struct PixelShaderUniforms
{
    std::array<float, 2> vcolor_mul;
    std::array<float, 2> vcolor_mul_inv;
    std::array<float, 2> tex0_mul;
    std::array<float, 2> tex0_mul_inv;
    float tex0_add_rgb;
    float output_mul_rgb;
    float alpha_test;
    float pad[1];
};

class D3D11RenderContext
{
public:
    D3D11RenderContext(
        ComPtr<ID3D11Device> device,
        ComPtr<ID3D11DeviceContext> context,
        D3D11StateManager& state_manager,
        D3D11TextureManager& texture_manager
    );
    void set_mode_and_textures(rf::gr::Mode mode, int bm_handle1, int bm_handle2);
    void set_mode(rf::gr::Mode mode);
    void set_texture(int slot, int bm_handle);
    void set_render_target(
        const ComPtr<ID3D11RenderTargetView>& back_buffer_view,
        const ComPtr<ID3D11DepthStencilView>& depth_stencil_buffer_view
    );
    void update_camera_uniforms(const CameraUniforms& uniforms);
    void set_vertex_buffer(ID3D11Buffer* vb);
    void set_index_buffer(ID3D11Buffer* ib);
    void set_texture_transform(const GrMatrix3x3& transform);
    void bind_vs_cbuffer(int index, ID3D11Buffer* cbuffer);

    ID3D11DeviceContext* device_context()
    {
        return context_;
    }

    const CameraUniforms& camera_uniforms()
    {
        return camera_uniforms_;
    }

private:
    void init_ps_cbuffer();
    void init_vs_cbuffer();

    ComPtr<ID3D11Device> device_;
    ComPtr<ID3D11DeviceContext> context_;
    D3D11StateManager& state_manager_;
    D3D11TextureManager& texture_manager_;
    ComPtr<ID3D11Buffer> vs_cbuffer_;
    ComPtr<ID3D11Buffer> ps_cbuffer_;
    ComPtr<ID3D11Buffer> texture_transform_cbuffer_;
    int white_bm_;
    GrMatrix3x3 current_texture_transform_;
    CameraUniforms camera_uniforms_;
};
