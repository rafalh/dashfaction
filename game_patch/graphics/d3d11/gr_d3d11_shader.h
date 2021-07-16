#pragma once

#include <utility>
#include <unordered_map>
#include <d3d11.h>
#include <patch_common/ComPtr.h>
#include "gr_d3d11_vertex.h"

namespace df::gr::d3d11
{
    class RenderContext;

    struct VertexShaderAndLayout
    {
        ComPtr<ID3D11VertexShader> vertex_shader;
        ComPtr<ID3D11InputLayout> input_layout;
    };

    enum class VertexShaderId
    {
        standard,
        character,
        transformed,
    };

    enum class PixelShaderId
    {
        standard,
        ui,
    };

    inline const char* get_vertex_shader_filename(VertexShaderId vertex_shader_id)
    {
        switch (vertex_shader_id) {
            case VertexShaderId::standard:
                return "standard_vs.bin";
            case VertexShaderId::character:
                return "character_vs.bin";
            case VertexShaderId::transformed:
                return "transformed_vs.bin";
            default:
                return nullptr;
        }
    }

    inline VertexLayout get_vertex_shader_layout(VertexShaderId vertex_shader_id)
    {
        switch (vertex_shader_id) {
            case VertexShaderId::standard:
                return VertexLayout::standard;
            case VertexShaderId::character:
                return VertexLayout::character;
            case VertexShaderId::transformed:
                return VertexLayout::transformed;
            default:
                return VertexLayout::standard;
        }
    }

    inline const char* get_pixel_shader_filename(PixelShaderId pixel_shader_id)
    {
        switch (pixel_shader_id) {
            case PixelShaderId::standard:
                return "standard_ps.bin";
            case PixelShaderId::ui:
                return "ui_ps.bin";
            default:
                return nullptr;
        }
    }

    class ShaderManager
    {
    public:
        ShaderManager(ComPtr<ID3D11Device> device);

        const VertexShaderAndLayout& get_vertex_shader(const char* filename, VertexLayout vertex_layout)
        {
            auto it = vertex_shaders_.find(filename);
            if (it == vertex_shaders_.end()) {
                it = vertex_shaders_.insert({filename, load_vertex_shader(filename, vertex_layout)}).first;
            }
            return it->second;
        }

        const ComPtr<ID3D11PixelShader>& get_pixel_shader(const char* filename)
        {
            auto it = pixel_shaders_.find(filename);
            if (it == pixel_shaders_.end()) {
                it = pixel_shaders_.insert({filename, load_pixel_shader(filename)}).first;
            }
            return it->second;
        }

        const VertexShaderAndLayout& get_vertex_shader(VertexShaderId vertex_shader_id)
        {
            return get_vertex_shader(
                get_vertex_shader_filename(vertex_shader_id),
                get_vertex_shader_layout(vertex_shader_id)
            );
        }

        const ComPtr<ID3D11PixelShader>& get_pixel_shader(PixelShaderId pixel_shader_id)
        {
            return get_pixel_shader(get_pixel_shader_filename(pixel_shader_id));
        }

    private:
        VertexShaderAndLayout load_vertex_shader(const char* filename, VertexLayout vertex_layout);
        VertexShaderAndLayout load_vertex_shader(const char* filename, const D3D11_INPUT_ELEMENT_DESC input_elements[], std::size_t num_input_elements);
        ComPtr<ID3D11PixelShader> load_pixel_shader(const char* filename);

        ComPtr<ID3D11Device> device_;
        ComPtr<ID3D11VertexShader> transformed_vertex_shader_;
        ComPtr<ID3D11VertexShader> default_vertex_shader_;
        ComPtr<ID3D11VertexShader> character_vertex_shader_;
        ComPtr<ID3D11PixelShader> default_pixel_shader_;
        ComPtr<ID3D11InputLayout> transformed_input_layout_;
        ComPtr<ID3D11InputLayout> default_input_layout_;
        ComPtr<ID3D11InputLayout> character_input_layout_;

        std::unordered_map<std::string, VertexShaderAndLayout> vertex_shaders_;
        std::unordered_map<std::string, ComPtr<ID3D11PixelShader>> pixel_shaders_;
    };
}
