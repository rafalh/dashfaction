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

    class ModelTransformBuffer
    {
    public:
        ModelTransformBuffer(ID3D11Device* device);

        void update(const rf::Vector3& pos, const rf::Matrix3& orient, ID3D11DeviceContext* device_context)
        {
            if (current_model_pos_ != pos || current_model_orient_ != orient) {
                current_model_pos_ = pos;
                current_model_orient_ = orient;
                update_buffer(device_context);
            }
        }

        ID3D11Buffer* get_buffer() const
        {
            return buffer_;
        }

    private:
        void update_buffer(ID3D11DeviceContext* device_context);

        ComPtr<ID3D11Buffer> buffer_;
        rf::Vector3 current_model_pos_;
        rf::Matrix3 current_model_orient_;
    };

    class ViewProjTransformBuffer
    {
    public:
        ViewProjTransformBuffer(ID3D11Device* device);

        void update(ID3D11DeviceContext* device_context);

        ID3D11Buffer* get_buffer() const
        {
            return buffer_;
        }

    private:
        ComPtr<ID3D11Buffer> buffer_;
    };

    class UvOffsetBuffer
    {
    public:
        UvOffsetBuffer(ID3D11Device* device);

        void update(const rf::Vector2& uv_offset, ID3D11DeviceContext* device_context)
        {
            if (current_uv_offset_ != uv_offset) {
                current_uv_offset_ = uv_offset;
                update_buffer(device_context);
            }
        }

        ID3D11Buffer* get_buffer() const
        {
            return buffer_;
        }

    private:
        void update_buffer(ID3D11DeviceContext* device_context);

        ComPtr<ID3D11Buffer> buffer_;
        rf::Vector2 current_uv_offset_;
    };

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

    class RenderModeBuffer
    {
    public:
        RenderModeBuffer(ID3D11Device* device);

        void update(gr::Mode mode, bool has_tex1, rf::Color color, ID3D11DeviceContext* device_context)
        {
            if (!current_mode_ || current_mode_.value() != mode || current_color_ != color) {
                current_mode_.emplace(mode);
                current_color_ = color;
                update_buffer(has_tex1, device_context);
            }
        }

        ID3D11Buffer* get_buffer() const
        {
            return buffer_;
        }

        void handle_fog_change()
        {
            if (current_mode_ && current_mode_.value().get_fog_type() != rf::gr::FOG_NOT_ALLOWED) {
                current_mode_.reset();
            }
        }

    private:
        void update_buffer(bool has_tex1, ID3D11DeviceContext* device_context);

        ComPtr<ID3D11Buffer> buffer_;
        std::optional<gr::Mode> current_mode_;
        rf::Color current_color_{255, 255, 255};
    };

    class RenderContext
    {
    public:
        RenderContext(
            ComPtr<ID3D11Device> device,
            ComPtr<ID3D11DeviceContext> device_context,
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

        void set_render_target(ID3D11RenderTargetView* render_target_view, ID3D11DepthStencilView* depth_stencil_view)
        {
            render_target_view_ = render_target_view;
            depth_stencil_view_ = depth_stencil_view;
            ID3D11RenderTargetView* render_targets[] = { render_target_view };
            device_context_->OMSetRenderTargets(std::size(render_targets), render_targets, depth_stencil_view);
        }

        void bind_vs_cbuffer(int index, ID3D11Buffer* cbuffer)
        {
            ID3D11Buffer* vs_cbuffers[] = { cbuffer };
            device_context_->VSSetConstantBuffers(index, std::size(vs_cbuffers), vs_cbuffers);
        }

        void clear();
        void zbuffer_clear();
        void set_clip();

        void update_view_proj_transform()
        {
            view_proj_transform_cbuffer_.update(device_context_);
        }

        void fog_set()
        {
            render_mode_cbuffer_.handle_fog_change();
        }

        ID3D11DeviceContext* device_context()
        {
            return device_context_;
        }

        void set_vertex_buffer(ID3D11Buffer* vertex_buffer, UINT stride, UINT slot = 0)
        {
            assert(slot < vertex_buffer_slots);
            if (current_vertex_buffers_[slot] != vertex_buffer) {
                current_vertex_buffers_[slot] = vertex_buffer;
                UINT offsets[] = { 0 };
                ID3D11Buffer* vertex_buffers[] = { vertex_buffer };
                device_context_->IASetVertexBuffers(slot, std::size(vertex_buffers), vertex_buffers, &stride, offsets);
            }
        }

        void set_index_buffer(ID3D11Buffer* index_buffer)
        {
            if (index_buffer != current_index_buffer_) {
                current_index_buffer_ = index_buffer;
                device_context_->IASetIndexBuffer(index_buffer, DXGI_FORMAT_R16_UINT, 0);
            }
        }

        void set_primitive_topology(D3D11_PRIMITIVE_TOPOLOGY primitive_topology)
        {
            if (current_primitive_topology_ != primitive_topology) {
                current_primitive_topology_ = primitive_topology;
                device_context_->IASetPrimitiveTopology(primitive_topology);
            }
        }

        void set_input_layout(ID3D11InputLayout* input_layout)
        {
            if (current_input_layout_ != input_layout) {
                current_input_layout_ = input_layout;
                device_context_->IASetInputLayout(input_layout);
            }
        }

        void set_vertex_shader(ID3D11VertexShader* vertex_shader)
        {
            if (current_vertex_shader_ != vertex_shader) {
                current_vertex_shader_ = vertex_shader;
                device_context_->VSSetShader(vertex_shader, nullptr, 0);
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
                device_context_->PSSetShader(pixel_shader, nullptr, 0);
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
            model_transform_cbuffer_.update(pos, orient, device_context_);
        }

        void set_uv_offset(const rf::Vector2& uv_offset)
        {
            uv_offset_cbuffer_.update(uv_offset, device_context_);
        }

        void set_zbias(int zbias)
        {
            zbias_ = zbias;
            zbias_changed_ = true;
        }

        void update_lights()
        {
            lights_buffer_.update(device_context_);
        }

    private:
        void bind_cbuffers();
        void bind_shader_program();
        void bind_rasterizer_state();
        void bind_texture(int slot);
        void update_texture_transform();
        void set_sampler_state(gr::TextureSource ts);
        void set_blend_state(gr::AlphaBlend ab);
        void set_depth_stencil_state(gr::ZbufferType zbt);

        void set_mode(gr::Mode mode, bool has_tex1, rf::Color color)
        {
            render_mode_cbuffer_.update(mode, has_tex1, color, device_context_);
            if (!current_mode_ || current_mode_.value() != mode) {
                if (!current_mode_ || current_mode_.value().get_texture_source() != mode.get_texture_source()) {
                    set_sampler_state(mode.get_texture_source());
                }
                if (!current_mode_ || current_mode_.value().get_alpha_blend() != mode.get_alpha_blend()) {
                    set_blend_state(mode.get_alpha_blend());
                }
                if (!current_mode_ || current_mode_.value().get_zbuffer_type() != mode.get_zbuffer_type()) {
                    set_depth_stencil_state(mode.get_zbuffer_type());
                }
                current_mode_.emplace(mode);
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
        ComPtr<ID3D11DeviceContext> device_context_;
        StateManager& state_manager_;
        ShaderManager& shader_manager_;
        TextureManager& texture_manager_;
        ModelTransformBuffer model_transform_cbuffer_;
        ViewProjTransformBuffer view_proj_transform_cbuffer_;
        UvOffsetBuffer uv_offset_cbuffer_;
        LightsBuffer lights_buffer_;
        RenderModeBuffer render_mode_cbuffer_;
        int white_bm_ = -1;

        ID3D11RenderTargetView* render_target_view_ = nullptr;
        ID3D11DepthStencilView* depth_stencil_view_ = nullptr;
        ID3D11Buffer* current_vertex_buffers_[vertex_buffer_slots] = {};
        ID3D11Buffer* current_index_buffer_ = nullptr;
        ID3D11InputLayout* current_input_layout_ = nullptr;
        ID3D11VertexShader* current_vertex_shader_ = nullptr;
        ID3D11PixelShader* current_pixel_shader_ = nullptr;
        D3D11_PRIMITIVE_TOPOLOGY current_primitive_topology_ = D3D11_PRIMITIVE_TOPOLOGY_UNDEFINED;
        std::array<int, 2> current_tex_handles_ = {-1, -1};
        D3D11_CULL_MODE current_cull_mode_ = D3D11_CULL_NONE;
        std::optional<gr::Mode> current_mode_;
        int zbias_ = 0;
        bool zbias_changed_ = true;
    };
}
