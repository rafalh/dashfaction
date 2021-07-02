#pragma once

#include <utility>
#include <d3d11.h>
#include <patch_common/ComPtr.h>

namespace df::gr::d3d11
{
    class RenderContext;

    class ShaderManager
    {
    public:
        ShaderManager(ComPtr<ID3D11Device> device);
        void bind_default_shaders(RenderContext& render_context);
        void bind_character_shaders(RenderContext& render_context);
        void bind_transformed_shaders(RenderContext& render_context);

    private:
        void init_transformed_vertex_shader();
        void init_default_shaders();
        void init_character_shaders();
        std::pair<ComPtr<ID3D11VertexShader>, ComPtr<ID3D11InputLayout>> load_vertex_shader(const char* filename, const D3D11_INPUT_ELEMENT_DESC input_elements[], std::size_t num_input_elements);
        ComPtr<ID3D11PixelShader> load_pixel_shader(const char* filename);

        ComPtr<ID3D11Device> device_;
        ComPtr<ID3D11VertexShader> transformed_vertex_shader_;
        ComPtr<ID3D11VertexShader> default_vertex_shader_;
        ComPtr<ID3D11VertexShader> character_vertex_shader_;
        ComPtr<ID3D11PixelShader> default_pixel_shader_;
        ComPtr<ID3D11InputLayout> transformed_input_layout_;
        ComPtr<ID3D11InputLayout> default_input_layout_;
        ComPtr<ID3D11InputLayout> character_input_layout_;
    };
}
