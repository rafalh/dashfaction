#include <cassert>
#include <cstring>
#include "../../rf/gr/gr_light.h"
#include "../../rf/os/frametime.h"
#include "gr_d3d11.h"
#include "gr_d3d11_context.h"
#include "gr_d3d11_texture.h"
#include "gr_d3d11_state.h"
#include "gr_d3d11_shader.h"

using namespace rf;

namespace df::gr::d3d11
{
    RenderContext::RenderContext(
        ComPtr<ID3D11Device> device, ComPtr<ID3D11DeviceContext> device_context,
        StateManager& state_manager, ShaderManager& shader_manager,
        TextureManager& texture_manager
    ) :
        device_{std::move(device)}, device_context_{std::move(device_context)},
        state_manager_{state_manager}, shader_manager_{shader_manager}, texture_manager_{texture_manager},
        model_transform_cbuffer_{device_},
        view_proj_transform_cbuffer_{device_},
        lights_buffer_{device_},
        render_mode_cbuffer_{device_},
        per_frame_buffer_{device_}
    {
        bind_cbuffers();
    }

    void RenderContext::bind_cbuffers()
    {
        ID3D11Buffer* vs_cbuffers[] = {
            model_transform_cbuffer_,
            view_proj_transform_cbuffer_,
            per_frame_buffer_,
            nullptr,
        };
        device_context_->VSSetConstantBuffers(0, std::size(vs_cbuffers), vs_cbuffers);

        ID3D11Buffer* ps_cbuffers[] = {
            render_mode_cbuffer_,
            lights_buffer_,
        };
        device_context_->PSSetConstantBuffers(0, std::size(ps_cbuffers), ps_cbuffers);
    }

    void RenderContext::clear()
    {
        // Note: original code clears clip rect only but it is not trivial in D3D11
        if (render_target_view_) {
            float clear_color[4] = {
                gr::screen.current_color.red / 255.0f,
                gr::screen.current_color.green / 255.0f,
                gr::screen.current_color.blue / 255.0f,
                1.0f,
            };
            device_context_->ClearRenderTargetView(render_target_view_, clear_color);
        }
    }

    void RenderContext::zbuffer_clear()
    {
        // Note: original code clears clip rect only but it is not trivial in D3D11
        if (gr::screen.depthbuffer_type != gr::DEPTHBUFFER_NONE && depth_stencil_view_) {
            float depth = gr::screen.depthbuffer_type == gr::DEPTHBUFFER_Z ? 0.0f : 1.0f;
            device_context_->ClearDepthStencilView(depth_stencil_view_, D3D11_CLEAR_DEPTH, depth, 0);
        }
    }

    void RenderContext::set_clip()
    {
        D3D11_VIEWPORT vp;
        vp.TopLeftX = static_cast<float>(gr::screen.clip_left + gr::screen.offset_x);
        vp.TopLeftY = static_cast<float>(gr::screen.clip_top + gr::screen.offset_y);
        vp.Width = static_cast<float>(gr::screen.clip_width);
        vp.Height = static_cast<float>(gr::screen.clip_height);
        vp.MinDepth = 0.0f;
        vp.MaxDepth = 1.0f;
        device_context_->RSSetViewports(1, &vp);
    }

    struct alignas(16) ModelTransformBufferData
    {
        // model to world
        GpuMatrix4x3 world_mat;
    };
    static_assert(sizeof(ModelTransformBufferData) % 16 == 0);

    ModelTransformBuffer::ModelTransformBuffer(ID3D11Device* device) :
        current_model_pos_{NAN, NAN, NAN},
        current_model_orient_{{NAN, NAN, NAN}, {NAN, NAN, NAN}, {NAN, NAN, NAN}}
    {
        CD3D11_BUFFER_DESC desc{
            sizeof(ModelTransformBufferData),
            D3D11_BIND_CONSTANT_BUFFER,
            D3D11_USAGE_DYNAMIC,
            D3D11_CPU_ACCESS_WRITE,
        };
        DF_GR_D3D11_CHECK_HR(device->CreateBuffer(&desc, nullptr, &buffer_));
    }

    void ModelTransformBuffer::update_buffer(ID3D11DeviceContext* device_context)
    {
        ModelTransformBufferData data;
        data.world_mat = build_world_matrix(current_model_pos_, current_model_orient_);

        D3D11_MAPPED_SUBRESOURCE mapped_subres;
        DF_GR_D3D11_CHECK_HR(
            device_context->Map(buffer_, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped_subres)
        );
        std::memcpy(mapped_subres.pData, &data, sizeof(data));
        device_context->Unmap(buffer_, 0);
    }

    struct alignas(16) ViewProjTransformBufferData
    {
        GpuMatrix4x3 view_mat;
        GpuMatrix4x4 proj_mat;
    };
    static_assert(sizeof(ViewProjTransformBufferData) % 16 == 0);

    ViewProjTransformBuffer::ViewProjTransformBuffer(ID3D11Device* device)
    {
        CD3D11_BUFFER_DESC desc{
            sizeof(ViewProjTransformBufferData),
            D3D11_BIND_CONSTANT_BUFFER,
            D3D11_USAGE_DYNAMIC,
            D3D11_CPU_ACCESS_WRITE,
        };
        DF_GR_D3D11_CHECK_HR(device->CreateBuffer(&desc, nullptr, &buffer_));
    }

    void ViewProjTransformBuffer::update(ID3D11DeviceContext* device_context)
    {
        ViewProjTransformBufferData data;
        data.view_mat = build_view_matrix(rf::gr::eye_pos, rf::gr::eye_matrix);
        data.proj_mat = build_proj_matrix();

        D3D11_MAPPED_SUBRESOURCE mapped_subres;
        DF_GR_D3D11_CHECK_HR(
            device_context->Map(buffer_, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped_subres)
        );
        std::memcpy(mapped_subres.pData, &data, sizeof(data));
        device_context->Unmap(buffer_, 0);
    }

    struct alignas(16) PerFrameBufferData
    {
        float time;
    };
    static_assert(sizeof(PerFrameBufferData) % 16 == 0);

    PerFrameBuffer::PerFrameBuffer(ID3D11Device* device)
    {
        CD3D11_BUFFER_DESC desc{
            sizeof(PerFrameBufferData),
            D3D11_BIND_CONSTANT_BUFFER,
            D3D11_USAGE_DYNAMIC,
            D3D11_CPU_ACCESS_WRITE,
        };
        DF_GR_D3D11_CHECK_HR(device->CreateBuffer(&desc, nullptr, &buffer_));
    }

    void PerFrameBuffer::update(ID3D11DeviceContext* device_context)
    {
        PerFrameBufferData data;
        data.time = rf::frametime_total_milliseconds / 1000.0f;

        D3D11_MAPPED_SUBRESOURCE mapped_subres;
        DF_GR_D3D11_CHECK_HR(
            device_context->Map(buffer_, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped_subres)
        );
        std::memcpy(mapped_subres.pData, &data, sizeof(data));
        device_context->Unmap(buffer_, 0);
    }

    struct LightsBufferData
    {
        static constexpr int max_point_lights = 8;

        struct PointLight
        {
            std::array<float, 3> pos;
            float radius;
            std::array<float, 4> color;
        };

        std::array<float, 3> ambient_light;
        float num_point_lights;
        PointLight point_lights[max_point_lights];
    };

    LightsBuffer::LightsBuffer(ID3D11Device* device)
    {
        CD3D11_BUFFER_DESC desc{
            sizeof(LightsBufferData),
            D3D11_BIND_CONSTANT_BUFFER,
            D3D11_USAGE_DYNAMIC,
            D3D11_CPU_ACCESS_WRITE,
        };
        DF_GR_D3D11_CHECK_HR(device->CreateBuffer(&desc, nullptr, &buffer_));
    }

    void LightsBuffer::update(ID3D11DeviceContext* device_context)
    {
        D3D11_MAPPED_SUBRESOURCE mapped_subres;
        DF_GR_D3D11_CHECK_HR(
            device_context->Map(buffer_, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped_subres)
        );

        LightsBufferData data;
        std::memset(&data, 0, sizeof(data));
        for (int i = 0; i < std::min(rf::gr::num_relevant_lights, LightsBufferData::max_point_lights); ++i) {
            LightsBufferData::PointLight& gpu_light = data.point_lights[i];
            // Make sure radius is never 0 because we divide by it
            gpu_light.radius = 0.0001f;
        }

        rf::gr::light_get_ambient(&data.ambient_light[0], &data.ambient_light[1], &data.ambient_light[2]);

        int num_point_lights = std::min(rf::gr::num_relevant_lights, LightsBufferData::max_point_lights);
        data.num_point_lights = static_cast<float>(num_point_lights);

        for (int i = 0; i < num_point_lights; ++i) {
            rf::gr::Light* light = rf::gr::relevant_lights[i];
            LightsBufferData::PointLight& gpu_light = data.point_lights[i];
            gpu_light.pos = {light->vec.x, light->vec.y, light->vec.z};
            gpu_light.color = {light->r, light->g, light->b, 1.0f};
            gpu_light.radius = light->rad_2;
        }

        std::memcpy(mapped_subres.pData, &data, sizeof(data));

        device_context->Unmap(buffer_, 0);
    }

    struct alignas(16) RenderModeBufferData
    {
        std::array<float, 4> current_color;
        float alpha_test;
        float fog_far;
        float pad0[2];
        std::array<float, 3> fog_color;
        float pad1;
    };
    static_assert(sizeof(RenderModeBufferData) % 16 == 0);

    RenderModeBuffer::RenderModeBuffer(ID3D11Device* device)
    {
        CD3D11_BUFFER_DESC desc{
            sizeof(RenderModeBufferData),
            D3D11_BIND_CONSTANT_BUFFER,
            D3D11_USAGE_DYNAMIC,
            D3D11_CPU_ACCESS_WRITE,
        };
        DF_GR_D3D11_CHECK_HR(device->CreateBuffer(&desc, nullptr, &buffer_));
    }

    void RenderModeBuffer::update_buffer(ID3D11DeviceContext* device_context)
    {
        RenderModeBufferData data;
        data.current_color = {
            current_color_.red / 255.0f,
            current_color_.green / 255.0f,
            current_color_.blue / 255.0f,
            current_color_.alpha / 255.0f,
        };
        data.alpha_test = current_alpha_test_ ? 0.1f : 0.0f;
        if (!current_fog_allowed_ || !gr::screen.fog_mode) {
            data.fog_far = std::numeric_limits<float>::infinity();
            data.fog_color = {0.0f, 0.0f, 0.0f};
        }
        else {
            data.fog_far = gr::screen.fog_far;
            data.fog_color = {
                static_cast<float>(gr::screen.fog_color.red) / 255.0f,
                static_cast<float>(gr::screen.fog_color.green) / 255.0f,
                static_cast<float>(gr::screen.fog_color.blue) / 255.0f,
            };
        }

        D3D11_MAPPED_SUBRESOURCE mapped_subres;
        DF_GR_D3D11_CHECK_HR(
            device_context->Map(buffer_, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped_subres)
        );
        std::memcpy(mapped_subres.pData, &data, sizeof(data));
        device_context->Unmap(buffer_, 0);
    }
}
