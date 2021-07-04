#include <cassert>
#include "gr_d3d11.h"
#include "gr_d3d11_batch.h"
#include "gr_d3d11_shader.h"
#include "gr_d3d11_context.h"
#include "../../rf/os/frametime.h"
#define NO_D3D8
#include "../../rf/gr/gr_direct3d.h"

using namespace rf;

namespace df::gr::d3d11
{
    constexpr int batch_max_vertex = 6000;
    constexpr int batch_max_index = 10000;

    BatchManager::BatchManager(ComPtr<ID3D11Device> device, ShaderManager& shader_manager, RenderContext& render_context) :
        device_{device}, render_context_(render_context),
        vertex_ring_buffer_{batch_max_vertex, D3D11_BIND_VERTEX_BUFFER, device_, render_context.device_context()},
        index_ring_buffer_{batch_max_index, D3D11_BIND_INDEX_BUFFER, device_, render_context.device_context()}
    {
        vertex_shader_ = shader_manager.get_vertex_shader(VertexShaderId::transformed);
        pixel_shader_ = shader_manager.get_pixel_shader(PixelShaderId::standard);
    }

    void BatchManager::flush()
    {
        auto [start_vertex, num_vertex] = vertex_ring_buffer_.submit();
        if (num_vertex == 0) {
            return;
        }
        auto [start_index, num_index] = index_ring_buffer_.submit();

        render_context_.set_vertex_buffer(vertex_ring_buffer_.get_buffer(), sizeof(GpuTransformedVertex));
        render_context_.set_index_buffer(index_ring_buffer_.get_buffer());
        render_context_.set_vertex_shader(vertex_shader_);
        render_context_.set_pixel_shader(pixel_shader_);
        render_context_.set_primitive_topology(primitive_topology_);
        render_context_.set_mode_and_textures(mode_, textures_[0], textures_[1]);
        render_context_.set_cull_mode(D3D11_CULL_NONE);
        render_context_.set_uv_pan(rf::vec2_zero_vector);

        render_context_.device_context()->DrawIndexed(num_index, start_index, 0);
    }

    void BatchManager::add_poly(int nv, const gr::Vertex **vertices, int vertex_attributes, const std::array<int, 2>& tex_handles, gr::Mode mode)
    {
        int num_index = (nv - 2) * 3;
        if (nv > batch_max_vertex || num_index > batch_max_index) {
            xlog::error("too many vertices/indices in gr_d3d11_tmapper");
            return;
        }

        std::array<int, 2> normalized_tex_handles = normalize_texture_handles_for_mode(mode, tex_handles);

        set_primitive_topology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        set_mode(mode);
        set_textures(normalized_tex_handles);

        if (vertex_ring_buffer_.is_full(nv) || index_ring_buffer_.is_full(num_index)) {
            flush();
        }

        auto [gpu_verts, base_vertex] = vertex_ring_buffer_.alloc(nv);
        auto [gpu_ind_ptr, base_index] = index_ring_buffer_.alloc(num_index);

        rf::Color color{255, 255, 255, 255};
        if (!(vertex_attributes & gr::TMAP_FLAG_RGB)) {
            color.red = gr::screen.current_color.red;
            color.green = gr::screen.current_color.green;
            color.blue = gr::screen.current_color.blue;
        }
        if (!(vertex_attributes & gr::TMAP_FLAG_ALPHA)) {
            color.alpha = gr::screen.current_color.alpha;
        }
        // Note: gr_matrix_scale is zero before first gr_setup_3d call
        float matrix_scale_z = gr::matrix_scale.z ? gr::matrix_scale.z : 1.0f;
        for (int i = 0; i < nv; ++i) {
            const gr::Vertex& in_vert = *vertices[i];
            GpuTransformedVertex& out_vert = gpu_verts[i];
            out_vert.x = (in_vert.sx - gr::screen.offset_x) / gr::screen.clip_width * 2.0f - 1.0f;
            out_vert.y = (in_vert.sy - gr::screen.offset_y) / gr::screen.clip_height * -2.0f + 1.0f;
            out_vert.z = in_vert.sw * gr::d3d::zm;
            // Set w to depth in camera space (needed for 3D rendering)
            out_vert.w = 1.0f / in_vert.sw / matrix_scale_z;
            if (vertex_attributes & gr::TMAP_FLAG_RGB) {
                color.red = in_vert.r;
                color.green = in_vert.g;
                color.blue = in_vert.b;
            }
            if (vertex_attributes & gr::TMAP_FLAG_ALPHA) {
                color.alpha = in_vert.a;
            }
            out_vert.diffuse = pack_color(color);
            out_vert.u0 = in_vert.u1;
            out_vert.v0 = in_vert.v1;
            // out_vert.u1 = in_vert.u2;
            // out_vert.v1 = in_vert.v2;
            //xlog::info("vert[%d]: pos <%.2f %.2f %.2f> uv <%.2f %.2f>", i, out_vert.x, out_vert.y, in_vert.sw, out_vert.u0, out_vert.v0);

            if (i >= 2) {
                *(gpu_ind_ptr++) = base_vertex;
                *(gpu_ind_ptr++) = base_vertex + i - 1;
                *(gpu_ind_ptr++) = base_vertex + i;
            }
        }
    }

    void BatchManager::add_line(const gr::Vertex **vertices, rf::gr::Mode mode)
    {
        set_primitive_topology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);
        set_mode(mode);
        if (vertex_ring_buffer_.is_full(2) || index_ring_buffer_.is_full(2)) {
            flush();
        }

        auto [gpu_verts, base_vertex] = vertex_ring_buffer_.alloc(2);
        auto [gpu_ind_ptr, base_index] = index_ring_buffer_.alloc(2);

        // Note: gr_matrix_scale is zero before first gr_setup_3d call
        float matrix_scale_z = gr::matrix_scale.z ? gr::matrix_scale.z : 1.0f;
        for (int i = 0; i < 2; ++i) {
            const gr::Vertex& in_vert = *vertices[i];
            GpuTransformedVertex& out_vert = gpu_verts[i];
            out_vert.x = (in_vert.sx - gr::screen.offset_x) / gr::screen.clip_width * 2.0f - 1.0f;
            out_vert.y = (in_vert.sy - gr::screen.offset_y) / gr::screen.clip_height * -2.0f + 1.0f;
            out_vert.z = in_vert.sw * gr::zm;
            // Set w to depth in camera space (needed for 3D rendering)
            out_vert.w = 1.0f / in_vert.sw / matrix_scale_z;
            out_vert.diffuse = pack_color(gr::screen.current_color);
            *(gpu_ind_ptr++) = base_vertex;
            *(gpu_ind_ptr++) = base_vertex + 1;
        }
    }
}
