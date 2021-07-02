#include <xlog/xlog.h>
#include "gr_d3d11_shader.h"
#include "gr_d3d11.h"
#include "../../rf/file/file.h"
#include "gr_d3d11_context.h"

namespace df::gr::d3d11
{
    ShaderManager::ShaderManager(ComPtr<ID3D11Device> device) : device_{device}
    {
        init_default_shaders();
        init_character_shaders();
        init_transformed_vertex_shader();
    }

    void ShaderManager::bind_default_shaders(RenderContext& render_context)
    {
        render_context.set_input_layout(default_input_layout_);
        render_context.set_vertex_shader(default_vertex_shader_);
        render_context.set_pixel_shader(default_pixel_shader_);
    }

    void ShaderManager::bind_character_shaders(RenderContext& render_context)
    {
        render_context.set_input_layout(character_input_layout_);
        render_context.set_vertex_shader(character_vertex_shader_);
        render_context.set_pixel_shader(default_pixel_shader_);
    }

    void ShaderManager::bind_transformed_shaders(RenderContext& render_context)
    {
        render_context.set_input_layout(transformed_input_layout_);
        render_context.set_vertex_shader(transformed_vertex_shader_);
        render_context.set_pixel_shader(default_pixel_shader_);
    }

    std::pair<ComPtr<ID3D11VertexShader>, ComPtr<ID3D11InputLayout>> ShaderManager::load_vertex_shader(const char* filename, const D3D11_INPUT_ELEMENT_DESC input_elements[], std::size_t num_input_elements)
    {
        rf::File file;
        if (file.open(filename) != 0) {
            xlog::error("Cannot open vertex shader file %s", filename);
            return {{}, {}};
        }
        int size = file.size();
        auto shader_data = std::make_unique<std::byte[]>(size);
        int bytes_read = file.read(shader_data.get(), size);
        xlog::info("Vertex shader size %d file size %d first byte %02x", bytes_read, size,
            static_cast<rf::ubyte>(shader_data.get()[0]));
        ComPtr<ID3D11VertexShader> vertex_shader;
        HRESULT hr = device_->CreateVertexShader(shader_data.get(), size, nullptr, &vertex_shader);
        check_hr(hr, "CreateVertexShader");

        ComPtr<ID3D11InputLayout> input_layout;
        hr = device_->CreateInputLayout(
            input_elements,
            num_input_elements,
            shader_data.get(),
            size,
            &input_layout
        );
        check_hr(hr, "CreateInputLayout");

        return {std::move(vertex_shader), std::move(input_layout)};
    }

    ComPtr<ID3D11PixelShader> ShaderManager::load_pixel_shader(const char* filename)
    {
        rf::File file;
        if (file.open(filename) != 0) {
            xlog::error("Cannot open pixel shader file");
            return {};
        }
        int size = file.size();
        auto shader_data = std::make_unique<std::byte[]>(size);
        file.read(shader_data.get(), size);
        ComPtr<ID3D11PixelShader> pixel_shader;
        HRESULT hr = device_->CreatePixelShader(shader_data.get(), size, nullptr, &pixel_shader);
        check_hr(hr, "CreatePixelShader");
        return pixel_shader;
    }

    void ShaderManager::init_default_shaders()
    {
        D3D11_INPUT_ELEMENT_DESC desc[] = {
            { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            // { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "TEXCOORD", 1, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "COLOR", 0, DXGI_FORMAT_R8G8B8A8_UNORM, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        };
        auto p = load_vertex_shader("default_vs.bin", desc, std::size(desc));
        default_vertex_shader_ = std::move(p.first);
        default_input_layout_ = std::move(p.second);
        default_pixel_shader_ = load_pixel_shader("default_ps.bin");
    }

    void ShaderManager::init_transformed_vertex_shader()
    {
        D3D11_INPUT_ELEMENT_DESC desc[] = {
            { "POSITIONT", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            // { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "TEXCOORD", 1, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "COLOR", 0, DXGI_FORMAT_R8G8B8A8_UNORM, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        };
        auto p = load_vertex_shader("transformed_vs.bin", desc, std::size(desc));
        transformed_vertex_shader_ = std::move(p.first);
        transformed_input_layout_ = std::move(p.second);
    }

    void ShaderManager::init_character_shaders()
    {
        D3D11_INPUT_ELEMENT_DESC desc[] = {
            { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            // { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "COLOR", 0, DXGI_FORMAT_R8G8B8A8_UNORM, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "COLOR", 1, DXGI_FORMAT_R8G8B8A8_UNORM, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "COLOR", 2, DXGI_FORMAT_R8G8B8A8_UNORM, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        };
        auto p = load_vertex_shader("character_vs.bin", desc, std::size(desc));
        character_vertex_shader_ = std::move(p.first);
        character_input_layout_ = std::move(p.second);
    }
}
