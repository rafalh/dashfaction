#include <xlog/xlog.h>
#include "gr_d3d11_shader.h"
#include "gr_d3d11.h"
#include "../../rf/file/file.h"
#include "gr_d3d11_context.h"

namespace df::gr::d3d11
{
    ShaderManager::ShaderManager(ComPtr<ID3D11Device> device) : device_{device}
    {
    }

    VertexShaderAndLayout
    ShaderManager::load_vertex_shader(const char* filename, VertexLayout vertex_layout)
    {
        std::vector<D3D11_INPUT_ELEMENT_DESC> desc = get_vertex_layout_desc(vertex_layout);
        return load_vertex_shader(filename, desc.data(), desc.size());
    }

    VertexShaderAndLayout
    ShaderManager::load_vertex_shader(const char* filename, const D3D11_INPUT_ELEMENT_DESC input_elements[], std::size_t num_input_elements)
    {
        rf::File file;
        if (file.open(filename) != 0) {
            xlog::error("Cannot open vertex shader file %s", filename);
            return {{}, {}};
        }
        int size = file.size();
        auto shader_data = std::make_unique<std::byte[]>(size);
        [[maybe_unused]] int bytes_read = file.read(shader_data.get(), size);
        assert(bytes_read == size);

        xlog::debug("Loading vertex shader %s size %d", filename, size);
        ComPtr<ID3D11VertexShader> vertex_shader;
        DF_GR_D3D11_CHECK_HR(
            device_->CreateVertexShader(shader_data.get(), size, nullptr, &vertex_shader)
        );

        ComPtr<ID3D11InputLayout> input_layout;
        DF_GR_D3D11_CHECK_HR(
            device_->CreateInputLayout(
                input_elements,
                num_input_elements,
                shader_data.get(),
                size,
                &input_layout
            )
        );

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
        [[maybe_unused]] int bytes_read = file.read(shader_data.get(), size);
        assert(bytes_read == size);

        xlog::debug("Loading pixel shader %s size %d", filename, size);
        ComPtr<ID3D11PixelShader> pixel_shader;
        DF_GR_D3D11_CHECK_HR(
            device_->CreatePixelShader(shader_data.get(), size, nullptr, &pixel_shader)
        );
        return pixel_shader;
    }
}
