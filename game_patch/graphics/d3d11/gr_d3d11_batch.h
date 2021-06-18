#pragma once

#include <array>
#include <d3d11.h>
#include <patch_common/ComPtr.h>
#include "../../rf/gr/gr.h"

namespace df::gr::d3d11
{

    struct GpuVertex;
    struct RenderContext;

    class BatchManager
    {
    public:
        BatchManager(ComPtr<ID3D11Device> device, ComPtr<ID3D11DeviceContext> context, RenderContext& render_context);
        void tmapper(int nv, rf::gr::Vertex **vertices, int tmap_flags, rf::gr::Mode mode);
        void flush();

    private:
        void create_dynamic_vb();
        void create_dynamic_ib();
        void map_dynamic_buffers(bool vb_full, bool ib_full);
        void unmap_dynamic_buffers();
        void bind_resources();

        ComPtr<ID3D11Device> device_;
        ComPtr<ID3D11DeviceContext> context_;
        RenderContext& render_context_;
        ComPtr<ID3D11Buffer> dynamic_vb_;
        ComPtr<ID3D11Buffer> dynamic_ib_;
        GpuVertex* mapped_vb_ = nullptr;
        rf::ushort* mapped_ib_ = nullptr;
        D3D11_PRIMITIVE_TOPOLOGY primitive_topology_ = D3D11_PRIMITIVE_TOPOLOGY_UNDEFINED;
        int current_vertex_ = 0;
        int start_index_ = 0;
        int current_index_ = 0;
        std::array<int, 2> textures_ = { -1, -1 };
        rf::gr::Mode mode_{
            rf::gr::TEXTURE_SOURCE_NONE,
            rf::gr::COLOR_SOURCE_VERTEX,
            rf::gr::ALPHA_SOURCE_VERTEX,
            rf::gr::ALPHA_BLEND_NONE,
            rf::gr::ZBUFFER_TYPE_NONE,
            rf::gr::FOG_ALLOWED,
        };
    };
}
