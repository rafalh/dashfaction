#include "gr_d3d11.h"

using namespace rf;

constexpr int d3d11_batch_max_vertex = 6000;
constexpr int d3d11_batch_max_index = 10000;

D3D11BatchManager::D3D11BatchManager(ComPtr<ID3D11Device> device, ComPtr<ID3D11DeviceContext> context, D3D11RenderContext& render_context) :
    device_{std::move(device)}, context_{std::move(context)}, render_context_(render_context)
{
    create_dynamic_vb();
    create_dynamic_ib();
}

void D3D11BatchManager::create_dynamic_vb()
{
    // Fill in a buffer description.
    D3D11_BUFFER_DESC buffer_desc;
    ZeroMemory(&buffer_desc, sizeof(buffer_desc));
    buffer_desc.Usage            = D3D11_USAGE_DYNAMIC;
    buffer_desc.ByteWidth        = sizeof(GpuVertex) * d3d11_batch_max_vertex;
    buffer_desc.BindFlags        = D3D11_BIND_VERTEX_BUFFER;
    buffer_desc.CPUAccessFlags   = D3D11_CPU_ACCESS_WRITE;
    buffer_desc.MiscFlags        = 0;
    buffer_desc.StructureByteStride = 0;

    // Create the vertex buffer.
    HRESULT hr = device_->CreateBuffer(&buffer_desc, nullptr, &dynamic_vb_);
    check_hr(hr, "CreateBuffer");
}

void D3D11BatchManager::create_dynamic_ib()
{
    // Fill in a buffer description.
    D3D11_BUFFER_DESC buffer_desc;
    ZeroMemory(&buffer_desc, sizeof(buffer_desc));
    buffer_desc.Usage            = D3D11_USAGE_DYNAMIC;
    buffer_desc.ByteWidth        = sizeof(ushort) * d3d11_batch_max_index;
    buffer_desc.BindFlags        = D3D11_BIND_INDEX_BUFFER;
    buffer_desc.CPUAccessFlags   = D3D11_CPU_ACCESS_WRITE;
    buffer_desc.MiscFlags        = 0;
    buffer_desc.StructureByteStride = 0;

    // Create the vertex buffer.
    HRESULT hr = device_->CreateBuffer(&buffer_desc, nullptr, &dynamic_ib_);
    check_hr(hr, "CreateBuffer");
}

void D3D11BatchManager::map_dynamic_buffers(bool vb_full, bool ib_full)
{
    D3D11_MAPPED_SUBRESOURCE mapped_vb;
    D3D11_MAPPED_SUBRESOURCE mapped_ib;
    ZeroMemory(&mapped_vb, sizeof(D3D11_MAPPED_SUBRESOURCE));
    ZeroMemory(&mapped_ib, sizeof(D3D11_MAPPED_SUBRESOURCE));
    auto vb_map_type = vb_full ? D3D11_MAP_WRITE_DISCARD : D3D11_MAP_WRITE_NO_OVERWRITE;
    auto ib_map_type = ib_full ? D3D11_MAP_WRITE_DISCARD : D3D11_MAP_WRITE_NO_OVERWRITE;
    HRESULT hr = context_->Map(dynamic_vb_, 0, vb_map_type, 0, &mapped_vb);
    check_hr(hr, "Map VB");
    hr = context_->Map(dynamic_ib_, 0, ib_map_type, 0, &mapped_ib);
    check_hr(hr, "Map IB");
    mapped_vb_ = reinterpret_cast<GpuVertex*>(mapped_vb.pData);
    mapped_ib_ = reinterpret_cast<ushort*>(mapped_ib.pData);
    if (vb_full) {
        current_vertex_ = 0;
    }
    if (ib_full) {
        current_index_ = 0;
        start_index_ = 0;
    }
}

void D3D11BatchManager::unmap_dynamic_buffers()
{
    context_->Unmap(dynamic_vb_, 0);
    context_->Unmap(dynamic_ib_, 0);
    mapped_vb_ = nullptr;
    mapped_ib_ = nullptr;
}

void D3D11BatchManager::flush()
{
    int num_index = current_index_ - start_index_;
    if (num_index == 0) {
        return;
    }

    if (mapped_vb_) {
        unmap_dynamic_buffers();
    }

    context_->IASetPrimitiveTopology(primitive_topology_);
    render_context_.set_mode(mode_);
    render_context_.set_texture(0, textures_[0]);
    render_context_.set_texture(1, textures_[1]);

    context_->DrawIndexed(num_index, start_index_, 0);
    start_index_ = current_index_;
}

void D3D11BatchManager::tmapper(int nv, gr::Vertex **vertices, int tmap_flags, gr::Mode mode)
{
    if (mode.get_zbuffer_type() == gr::ZBUFFER_TYPE_FULL) {
        //xlog::info("gr_d3d11_tmapper nv %d sx %f sy %f sw %f", nv, vertices[0]->sx, vertices[0]->sy, vertices[0]->sw);
        //mode.set_texture_source(TEXTURE_SOURCE_NONE);
        //gr::screen.current_color.alpha = 255;
    }
    //
    //gr_d3d11_set_mode(mode);

    int num_index = (nv - 2) * 3;
    if (nv > d3d11_batch_max_vertex || num_index > d3d11_batch_max_index) {
        xlog::error("too many vertices/indices in gr_d3d11_tmapper");
        return;
    }
    bool vb_full = current_vertex_ + nv >= d3d11_batch_max_vertex;
    bool ib_full = current_index_ + num_index > d3d11_batch_max_index;
    bool mode_changed = mode_ != mode;
    bool texture_changed = textures_[0] != gr::screen.current_texture_1 || textures_[1] != gr::screen.current_texture_2;
    bool topology_changed = primitive_topology_ != D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
    if (vb_full || ib_full || topology_changed || mode_changed || texture_changed) {
        flush();
        mode_ = mode;
        textures_[0] = gr::screen.current_texture_1;
        textures_[1] = gr::screen.current_texture_2;
        primitive_topology_ = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
    }
    if (!mapped_vb_) {
        map_dynamic_buffers(vb_full, ib_full);
    }

    int r = 255, g = 255, b = 255, a = 255;
    if (!(tmap_flags & gr::TMAP_FLAG_RGB)) {
        r = gr::screen.current_color.red;
        g = gr::screen.current_color.green;
        b = gr::screen.current_color.blue;
    }
    if (!(tmap_flags & gr::TMAP_FLAG_ALPHA)) {
        a = gr::screen.current_color.alpha;
    }
    int vert0 = current_vertex_;
    for (int i = 0; i < nv; ++i) {
        auto& in_vert = *vertices[i];
        auto& out_vert = mapped_vb_[current_vertex_];
        out_vert.x = (in_vert.sx - gr::screen.offset_x) / gr::screen.clip_width * 2.0f - 1.0f;
        out_vert.y = (in_vert.sy - gr::screen.offset_y) / gr::screen.clip_height * -2.0f + 1.0f;
        out_vert.z = in_vert.sw * d3d11_zm;
        if (tmap_flags & gr::TMAP_FLAG_RGB) {
            r = in_vert.r;
            g = in_vert.g;
            b = in_vert.b;
        }
        if (tmap_flags & gr::TMAP_FLAG_ALPHA) {
            a = in_vert.a;
        }
        out_vert.diffuse = pack_color(r, g, b, a);
        out_vert.u0 = in_vert.u1;
        out_vert.v0 = in_vert.v1;
        // out_vert.u1 = in_vert.u2;
        // out_vert.v1 = in_vert.v2;
        //xlog::info("vert[%d]: pos <%.2f %.2f %.2f> uv <%.2f %.2f>", i, out_vert.x, out_vert.y, in_vert.sw, out_vert.u0, out_vert.v0);

        if (i >= 2) {
            mapped_ib_[current_index_++] = vert0;
            mapped_ib_[current_index_++] = current_vertex_ - 1;
            mapped_ib_[current_index_++] = current_vertex_;
        }
        ++current_vertex_;
    }
}

void D3D11BatchManager::bind_buffers()
{
    UINT stride = sizeof(GpuVertex);
    UINT offset = 0;
    ID3D11Buffer* vertex_buffers[] = { dynamic_vb_ };
    context_->IASetVertexBuffers(0, std::size(vertex_buffers), vertex_buffers, &stride, &offset);
    context_->IASetIndexBuffer(dynamic_ib_, DXGI_FORMAT_R16_UINT, 0);
}
