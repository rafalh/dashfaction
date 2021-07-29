#pragma once

#include <cassert>
#include <array>
#include <d3d11.h>
#include <patch_common/ComPtr.h>
#include "../../rf/gr/gr.h"
#include "gr_d3d11_shader.h"
#include "gr_d3d11_buffer.h"

namespace df::gr::d3d11
{
    struct GpuTransformedVertex;
    class RenderContext;

    class DynamicGeometryRenderer
    {
    public:
        DynamicGeometryRenderer(ComPtr<ID3D11Device> device, ShaderManager& shader_manager, RenderContext& render_context);
        void add_poly(int nv, const rf::gr::Vertex **vertices, int vertex_attributes, const std::array<int, 2>& tex_handles, rf::gr::Mode mode);
        void line(const gr::Vertex **vertices, rf::gr::Mode mode, bool is_3d);
        void line_3d(const rf::gr::Vertex& v0, const rf::gr::Vertex& v1, rf::gr::Mode mode);
        void line_2d(float x1, float y1, float x2, float y2, rf::gr::Mode mode);
        void bitmap(int bm_handle, float x, float y, float w, float h, float sx, float sy, float sw, float sh, bool flip_x, bool flip_y, gr::Mode mode);
        void flush();

    private:
        struct State
        {
            D3D11_PRIMITIVE_TOPOLOGY primitive_topology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
            std::array<int, 2> textures = { -1, -1 };
            rf::gr::Mode mode{
                rf::gr::TEXTURE_SOURCE_NONE,
                rf::gr::COLOR_SOURCE_VERTEX,
                rf::gr::ALPHA_SOURCE_VERTEX,
                rf::gr::ALPHA_BLEND_NONE,
                rf::gr::ZBUFFER_TYPE_NONE,
                rf::gr::FOG_ALLOWED,
            };
            ID3D11PixelShader* pixel_shader = nullptr;

            bool operator==(const State& other) const
            {
                return primitive_topology == other.primitive_topology &&
                    textures == other.textures &&
                    mode == other.mode &&
                    pixel_shader == other.pixel_shader;
            }

            bool operator!=(const State& other) const
            {
                return !(*this == other);
            }
        };

        std::tuple<GpuTransformedVertex*, rf::ushort*, rf::ushort> setup(int num_vert, int num_ind, const State& state)
        {
            if (state_ != state || vertex_ring_buffer_.is_full(num_vert) || index_ring_buffer_.is_full(num_ind)) {
                flush();
                state_ = state;
            }
            auto base_vertex = static_cast<rf::ushort>(vertex_ring_buffer_.get_pos());
            auto gpu_verts = vertex_ring_buffer_.alloc(num_vert);
            auto gpu_inds = index_ring_buffer_.alloc(num_ind);
            return {gpu_verts, gpu_inds, base_vertex};
        }

        ComPtr<ID3D11Device> device_;
        RenderContext& render_context_;
        RingBuffer<GpuTransformedVertex> vertex_ring_buffer_;
        RingBuffer<rf::ushort> index_ring_buffer_;
        VertexShaderAndLayout vertex_shader_;
        ComPtr<ID3D11PixelShader> std_pixel_shader_;
        ComPtr<ID3D11PixelShader> ui_pixel_shader_;
        State state_;
    };
}
