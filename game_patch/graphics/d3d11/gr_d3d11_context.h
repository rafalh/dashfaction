#pragma once

#include <d3d11.h>
#include <patch_common/ComPtr.h>
#include "gr_d3d11_transform.h"

class D3D11StateManager;
class D3D11TextureManager;
class D3D11ShaderManager;

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
        D3D11ShaderManager& shader_manager,
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
    void set_texture_transform(const GrMatrix3x3& transform);
    void bind_vs_cbuffer(int index, ID3D11Buffer* cbuffer);
    void bind_default_shaders();
    void bind_character_shaders();

    ID3D11DeviceContext* device_context()
    {
        return context_;
    }

    const CameraUniforms& camera_uniforms()
    {
        return camera_uniforms_;
    }

    void set_vertex_buffer(ID3D11Buffer* vertex_buffer, UINT stride)
    {
        if (vertex_buffer != current_vertex_buffer_) {
            UINT offset = 0;
            ID3D11Buffer* vertex_buffers[] = { vertex_buffer };
            context_->IASetVertexBuffers(0, std::size(vertex_buffers), vertex_buffers, &stride, &offset);
            current_vertex_buffer_ = vertex_buffer;
        }
    }

    void set_index_buffer(ID3D11Buffer* index_buffer)
    {
        if (index_buffer != current_index_buffer_) {
            context_->IASetIndexBuffer(index_buffer, DXGI_FORMAT_R16_UINT, 0);
            current_index_buffer_ = index_buffer;
        }
    }

    void set_primitive_topology(D3D11_PRIMITIVE_TOPOLOGY primitive_topology)
    {
        if (primitive_topology != current_primitive_topology_) {
            current_primitive_topology_ = primitive_topology;
            context_->IASetPrimitiveTopology(primitive_topology);
        }
    }

    void set_input_layout(ID3D11InputLayout* input_layout)
    {
        if (input_layout != current_input_layout_) {
            current_input_layout_ = input_layout;
            context_->IASetInputLayout(input_layout);
        }
    }

    void set_vertex_shader(ID3D11VertexShader* vertex_shader)
    {
        if (vertex_shader != current_vertex_shader_) {
            current_vertex_shader_ = vertex_shader;
            context_->VSSetShader(vertex_shader, nullptr, 0);
        }
    }

    void set_pixel_shader(ID3D11PixelShader* pixel_shader)
    {
        if (pixel_shader != current_pixel_shader_) {
            current_pixel_shader_ = pixel_shader;
            context_->PSSetShader(pixel_shader, nullptr, 0);
        }
    }

private:
    void init_ps_cbuffer();
    void init_vs_cbuffer();

    ComPtr<ID3D11Device> device_;
    ComPtr<ID3D11DeviceContext> context_;
    D3D11StateManager& state_manager_;
    D3D11ShaderManager& shader_manager_;
    D3D11TextureManager& texture_manager_;
    ComPtr<ID3D11Buffer> vs_cbuffer_;
    ComPtr<ID3D11Buffer> ps_cbuffer_;
    ComPtr<ID3D11Buffer> texture_transform_cbuffer_;
    int white_bm_ = -1;
    GrMatrix3x3 current_texture_transform_;
    CameraUniforms camera_uniforms_;

    ID3D11Buffer* current_vertex_buffer_ = nullptr;
    ID3D11Buffer* current_index_buffer_ = nullptr;
    ID3D11InputLayout* current_input_layout_ = nullptr;
    ID3D11VertexShader* current_vertex_shader_ = nullptr;
    ID3D11PixelShader* current_pixel_shader_ = nullptr;
    D3D11_PRIMITIVE_TOPOLOGY current_primitive_topology_ = D3D11_PRIMITIVE_TOPOLOGY_UNDEFINED;
};
