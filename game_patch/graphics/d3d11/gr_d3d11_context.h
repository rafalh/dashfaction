#pragma once

#include <optional>
#include <d3d11.h>
#include <patch_common/ComPtr.h>
#include "gr_d3d11_transform.h"
#include "gr_d3d11_vertex.h"
#include "gr_d3d11_shader.h"
#include "gr_d3d11_texture.h"
#include "gr_d3d11_state.h"

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

        operator ID3D11Buffer*() const
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

        operator ID3D11Buffer*() const
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

        operator ID3D11Buffer*() const
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

        operator ID3D11Buffer*() const
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

        void update(gr::Mode mode, rf::Color color, ID3D11DeviceContext* device_context)
        {
            bool alpha_test = mode.get_zbuffer_type() == gr::ZBUFFER_TYPE_FULL_ALPHA_TEST;
            bool fog_allowed = mode.get_fog_type() != gr::FOG_NOT_ALLOWED;
            if (force_update_ || current_alpha_test_ != alpha_test || current_fog_allowed_ != fog_allowed || current_color_ != color) {
                current_alpha_test_ = alpha_test;
                current_fog_allowed_ = fog_allowed;
                current_color_ = color;
                force_update_ = false;
                update_buffer(device_context);
            }
        }

        operator ID3D11Buffer*() const
        {
            return buffer_;
        }

        void handle_fog_change()
        {
            if (current_fog_allowed_) {
                force_update_ = true;
            }
        }

    private:
        void update_buffer(ID3D11DeviceContext* device_context);

        ComPtr<ID3D11Buffer> buffer_;
        bool current_alpha_test_ = false;
        bool current_fog_allowed_ = false;
        bool force_update_ = true;
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

        ID3D11DeviceContext* device_context() const
        {
            return device_context_;
        }

        void set_textures(int tex_handle0, int tex_handle1 = -1)
        {
            if (current_tex_handles_[0] != tex_handle0 || current_tex_handles_[1] != tex_handle1) {
                current_tex_handles_[0] = tex_handle0;
                current_tex_handles_[1] = tex_handle1;
                ID3D11ShaderResourceView* shader_resources[] = {
                    get_diffuse_texture_view(tex_handle0),
                    get_lightmap_texture_view(tex_handle1),
                };
                device_context_->PSSetShaderResources(0, std::size(shader_resources), shader_resources);
            }
        }

        void set_mode(gr::Mode mode, rf::Color color = {255, 255, 255, 255})
        {
            render_mode_cbuffer_.update(mode, color, device_context_);
            if (!current_mode_ || current_mode_.value() != mode) {
                if (!current_mode_ || current_mode_.value().get_texture_source() != mode.get_texture_source()) {
                    std::array<ID3D11SamplerState*, 2> sampler_states = {
                        state_manager_.lookup_sampler_state(mode.get_texture_source(), 0),
                        state_manager_.lookup_sampler_state(mode.get_texture_source(), 1),
                    };
                    set_sampler_states(sampler_states);
                }
                if (!current_mode_ || current_mode_.value().get_alpha_blend() != mode.get_alpha_blend()) {
                    ID3D11BlendState* blend_state =
                        state_manager_.lookup_blend_state(mode.get_alpha_blend());
                    set_blend_state(blend_state);
                }
                if (!current_mode_ || current_mode_.value().get_zbuffer_type() != mode.get_zbuffer_type()) {
                    ID3D11DepthStencilState* depth_stencil_state =
                        state_manager_.lookup_depth_stencil_state(mode.get_zbuffer_type());
                    set_depth_stencil_state(depth_stencil_state);
                }
                current_mode_.emplace(mode);
            }
        }

        void set_sampler_states(std::array<ID3D11SamplerState*, 2> sampler_states)
        {
            if (current_sampler_states_ != sampler_states) {
                current_sampler_states_ = sampler_states;
                device_context_->PSSetSamplers(0, sampler_states.size(), sampler_states.data());
            }
        }

        void set_blend_state(ID3D11BlendState* blend_state)
        {
            if (current_blend_state_ != blend_state) {
                current_blend_state_ = blend_state;
                device_context_->OMSetBlendState(blend_state, nullptr, 0xffffffff);
            }
        }

        void set_depth_stencil_state(ID3D11DepthStencilState* depth_stencil_state)
        {
            if (current_depth_stencil_state_ != depth_stencil_state) {
                current_depth_stencil_state_ = depth_stencil_state;
                device_context_->OMSetDepthStencilState(depth_stencil_state, 0);
            }
        }

        void set_rasterizer_state(ID3D11RasterizerState* rasterizer_state)
        {
            if (current_rasterizer_state_ != rasterizer_state) {
                current_rasterizer_state_ = rasterizer_state;
                device_context_->RSSetState(rasterizer_state);
            }
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
                set_rasterizer_state(state_manager_.lookup_rasterizer_state(current_cull_mode_, zbias_));
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
            if (zbias_ != zbias) {
                zbias_ = zbias;
                zbias_changed_ = true;
            }
        }

        void update_lights()
        {
            lights_buffer_.update(device_context_);
        }

    private:
        void bind_cbuffers();

        ID3D11ShaderResourceView* get_diffuse_texture_view(int tex_handle)
        {
            if (tex_handle != -1) {
                return texture_manager_.lookup_texture(tex_handle);
            }
            return texture_manager_.get_white_texture();
        }

        ID3D11ShaderResourceView* get_lightmap_texture_view(int tex_handle)
        {
            if (tex_handle != -1) {
                return texture_manager_.lookup_texture(tex_handle);
            }
            return texture_manager_.get_gray_texture();
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

        ID3D11RenderTargetView* render_target_view_ = nullptr;
        ID3D11DepthStencilView* depth_stencil_view_ = nullptr;
        ID3D11Buffer* current_vertex_buffers_[vertex_buffer_slots] = {};
        ID3D11Buffer* current_index_buffer_ = nullptr;
        ID3D11InputLayout* current_input_layout_ = nullptr;
        ID3D11VertexShader* current_vertex_shader_ = nullptr;
        ID3D11PixelShader* current_pixel_shader_ = nullptr;
        D3D11_PRIMITIVE_TOPOLOGY current_primitive_topology_ = D3D11_PRIMITIVE_TOPOLOGY_UNDEFINED;
        std::array<int, 2> current_tex_handles_ = {-2, -2};
        D3D11_CULL_MODE current_cull_mode_ = D3D11_CULL_NONE;
        std::optional<gr::Mode> current_mode_;
        std::array<ID3D11SamplerState*, 2> current_sampler_states_ = {nullptr, nullptr};
        ID3D11BlendState* current_blend_state_ = nullptr;
        ID3D11DepthStencilState* current_depth_stencil_state_ = nullptr;
        ID3D11RasterizerState* current_rasterizer_state_ = nullptr;
        int zbias_ = 0;
        bool zbias_changed_ = true;
    };
}
