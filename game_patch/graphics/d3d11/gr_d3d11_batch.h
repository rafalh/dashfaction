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

    class BatchManager
    {
    public:
        BatchManager(ComPtr<ID3D11Device> device, ShaderManager& shader_manager, RenderContext& render_context);
        void add_vertices(int nv, const rf::gr::Vertex **vertices, int vertex_attributes, const std::array<int, 2>& tex_handles, rf::gr::Mode mode);
        void flush();

    private:
        ComPtr<ID3D11Device> device_;
        RenderContext& render_context_;
        RingBuffer<GpuTransformedVertex> vertex_ring_buffer_;
        RingBuffer<rf::ushort> index_ring_buffer_;
        D3D11_PRIMITIVE_TOPOLOGY primitive_topology_ = D3D11_PRIMITIVE_TOPOLOGY_UNDEFINED;
        std::array<int, 2> textures_ = { -1, -1 };
        rf::gr::Mode mode_{
            rf::gr::TEXTURE_SOURCE_NONE,
            rf::gr::COLOR_SOURCE_VERTEX,
            rf::gr::ALPHA_SOURCE_VERTEX,
            rf::gr::ALPHA_BLEND_NONE,
            rf::gr::ZBUFFER_TYPE_NONE,
            rf::gr::FOG_ALLOWED,
        };
        VertexShaderAndLayout vertex_shader_;
        ComPtr<ID3D11PixelShader> pixel_shader_;
    };
}
