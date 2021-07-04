#pragma once

#include <optional>
#include <d3d11.h>
#include <patch_common/ComPtr.h>
#include "gr_d3d11_transform.h"
#include "gr_d3d11_vertex.h"
#include "gr_d3d11_shader.h"

namespace df::gr::d3d11
{

    class StateManager;
    class ShaderManager;
    class TextureManager;

    class LightsBuffer
    {
    public:
        LightsBuffer(ID3D11Device* device);
        void update(ID3D11DeviceContext* device_context);
        ID3D11Buffer* get_buffer() const
        {
            return buffer_;
        }

    private:
        ComPtr<ID3D11Buffer> buffer_;
    };

    class RenderContext
    {
    public:
        RenderContext(
            ComPtr<ID3D11Device> device,
            ComPtr<ID3D11DeviceContext> context,
            StateManager& state_manager,
            ShaderManager& shader_manager,
            TextureManager& texture_manager
        );

        void set_mode_and_textures(rf::gr::Mode mode, int tex_handle0, int tex_handle1, rf::Color color = {255, 255, 255})
        {
            std::array<int, 2> tex_handles = normalize_texture_handles_for_mode(mode, {tex_handle0, tex_handle1});
            bool has_tex1 = tex_handles[1] != -1;
            set_mode(mode, has_tex1, color);
            set_texture(0, tex_handle0);
            set_texture(1, tex_handle1);
        }

        void set_render_target(ID3D11RenderTargetView* render_target_view, ID3D11DepthStencilView* depth_stencil_view);
        void bind_vs_cbuffer(int index, ID3D11Buffer* cbuffer);
        void clear();
        void zbuffer_clear();
        void set_clip();
        void update_view_proj_transform();

        void fog_set()
        {
            if (current_mode_ && current_mode_.value().get_fog_type() != rf::gr::FOG_NOT_ALLOWED) {
                current_mode_.reset();
            }
        }

        ID3D11DeviceContext* device_context()
        {
            return context_;
        }

        void set_vertex_buffer(ID3D11Buffer* vertex_buffer, UINT stride, UINT slot = 0)
        {
            assert(slot < vertex_buffer_slots);
            if (current_vertex_buffers_[slot] != vertex_buffer) {
                current_vertex_buffers_[slot] = vertex_buffer;
                UINT offsets[] = { 0 };
                ID3D11Buffer* vertex_buffers[] = { vertex_buffer };
                context_->IASetVertexBuffers(slot, std::size(vertex_buffers), vertex_buffers, &stride, offsets);
            }
        }

        void set_index_buffer(ID3D11Buffer* index_buffer)
        {
            if (index_buffer != current_index_buffer_) {
                current_index_buffer_ = index_buffer;
                context_->IASetIndexBuffer(index_buffer, DXGI_FORMAT_R16_UINT, 0);
            }
        }

        void set_primitive_topology(D3D11_PRIMITIVE_TOPOLOGY primitive_topology)
        {
            if (current_primitive_topology_ != primitive_topology) {
                current_primitive_topology_ = primitive_topology;
                context_->IASetPrimitiveTopology(primitive_topology);
            }
        }

        void set_input_layout(ID3D11InputLayout* input_layout)
        {
            if (current_input_layout_ != input_layout) {
                current_input_layout_ = input_layout;
                context_->IASetInputLayout(input_layout);
            }
        }

        void set_vertex_shader(ID3D11VertexShader* vertex_shader)
        {
            if (current_vertex_shader_ != vertex_shader) {
                current_vertex_shader_ = vertex_shader;
                context_->VSSetShader(vertex_shader, nullptr, 0);
            }
        }

        void set_vertex_shader(const VertexShaderAndLayout& vertex_shader_and_layout)
        {
            set_input_layout(vertex_shader_and_layout.input_layout);
            set_vertex_shader(vertex_shader_and_layout.vertex_shader);
        }

        void set_pixel_shader(ID3D11PixelShader* pixel_shader)
        {
            if (current_pixel_shader_ != pixel_shader) {
                current_pixel_shader_ = pixel_shader;
                context_->PSSetShader(pixel_shader, nullptr, 0);
            }
        }

        void set_cull_mode(D3D11_CULL_MODE cull_mode)
        {
            if (current_cull_mode_ != cull_mode || zbias_changed_) {
                current_cull_mode_ = cull_mode;
                zbias_changed_ = false;
                bind_rasterizer_state();
            }
        }

        void set_model_transform(const rf::Vector3& pos, const rf::Matrix3& orient)
        {
            if (current_model_pos_ != pos || current_model_orient_ != orient) {
                current_model_pos_ = pos;
                current_model_orient_ = orient;
                update_model_transform();
            }
        }

        void set_uv_pan(const rf::Vector2& uv_pan)
        {
            if (current_uv_pan_ != uv_pan) {
                current_uv_pan_ = uv_pan;
                update_texture_transform();
            }
        }

        void set_zbias(int zbias)
        {
            zbias_ = zbias;
            zbias_changed_ = true;
        }

        void update_lights()
        {
            lights_buffer_.update(context_);
        }

    private:
        void init_cbuffers();
        void bind_cbuffers();
        void bind_shader_program();
        void bind_rasterizer_state();
        void bind_texture(int slot);
        void change_mode(gr::Mode mode, bool has_tex1);
        void update_vertex_transform_type();
        void update_model_transform();
        void update_texture_transform();

        void set_mode(gr::Mode mode, bool has_tex1, rf::Color color)
        {
            if (!current_mode_ || current_mode_.value() != mode || current_color_ != color) {
                current_mode_.emplace(mode);
                current_color_ = color;
                change_mode(mode, has_tex1);
            }
        }

        void set_texture(int slot, int tex_handle)
        {
            if (current_tex_handles_[slot] != tex_handle) {
                current_tex_handles_[slot] = tex_handle;
                bind_texture(slot);
            }
        }

        static constexpr int vertex_buffer_slots = 2;

        ComPtr<ID3D11Device> device_;
        ComPtr<ID3D11DeviceContext> context_;
        StateManager& state_manager_;
        ShaderManager& shader_manager_;
        TextureManager& texture_manager_;
        ComPtr<ID3D11Buffer> model_transform_cbuffer_;
        ComPtr<ID3D11Buffer> view_proj_transform_cbuffer_;
        ComPtr<ID3D11Buffer> uv_offset_cbuffer_;
        ComPtr<ID3D11Buffer> render_mode_cbuffer_;
        int white_bm_ = -1;

        ID3D11RenderTargetView* render_target_view_ = nullptr;
        ID3D11DepthStencilView* depth_stencil_view_ = nullptr;
        ID3D11Buffer* current_vertex_buffers_[vertex_buffer_slots] = {};
        ID3D11Buffer* current_index_buffer_ = nullptr;
        ID3D11InputLayout* current_input_layout_ = nullptr;
        ID3D11VertexShader* current_vertex_shader_ = nullptr;
        ID3D11PixelShader* current_pixel_shader_ = nullptr;
        D3D11_PRIMITIVE_TOPOLOGY current_primitive_topology_ = D3D11_PRIMITIVE_TOPOLOGY_UNDEFINED;
        std::optional<gr::Mode> current_mode_;
        std::array<int, 2> current_tex_handles_ = {-1, -1};
        rf::Color current_color_{255, 255, 255};
        D3D11_CULL_MODE current_cull_mode_ = D3D11_CULL_NONE;
        rf::Vector2 current_uv_pan_;
        rf::Vector3 current_model_pos_;
        rf::Matrix3 current_model_orient_;
        int zbias_ = 0;
        bool zbias_changed_ = true;
        LightsBuffer lights_buffer_;
    };
}
