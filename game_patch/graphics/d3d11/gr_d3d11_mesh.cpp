#include <memory>
#include <cstring>
#include <cassert>
#include <windows.h>
#include <patch_common/FunHook.h>
#include <patch_common/ComPtr.h>
#include <xlog/xlog.h>
#include "../../rf/gr/gr.h"
#include "../../rf/math/quaternion.h"
#include "../../rf/v3d.h"
#include "../../rf/character.h"
#include "gr_d3d11.h"
#include "gr_d3d11_mesh.h"
#include "gr_d3d11_context.h"
#include "gr_d3d11_shader.h"

using namespace rf;

namespace df::gr::d3d11
{
    class MeshRenderCache : public BaseMeshRenderCache
    {
    public:
        MeshRenderCache(VifLodMesh* lod_mesh, ID3D11Device* device);
        void render(const Vector3& pos, const Matrix3& orient, const rf::MeshRenderParams& params, int lod_index, RenderContext& render_context);
    };

    class CharacterMeshRenderCache : public BaseMeshRenderCache
    {
    public:
        CharacterMeshRenderCache(VifLodMesh* lod_mesh, ID3D11Device* device);
        void render(const Vector3& pos, const Matrix3& orient, const CharacterInstance* ci, const MeshRenderParams& params, int lod_index, RenderContext& render_context);

    private:
        struct alignas(16) BoneTransformsBufferData
        {
            GpuMatrix4x3 matrices[50];
        };

        ComPtr<ID3D11Buffer> matrices_cbuffer_;

        void update_matrices_buffer(const CharacterInstance* ci, RenderContext& render_context);
    };

    const int* BaseMeshRenderCache::get_tex_handles(const MeshRenderParams& params, int lod_index)
    {
        if (params.alt_tex) {
            return params.alt_tex;
        }
        return lod_mesh_->meshes[lod_index]->tex_handles;
    }

    void BaseMeshRenderCache::draw(const MeshRenderParams& params, int lod_index, RenderContext& render_context)
    {
        const int* tex_handles = get_tex_handles(params, lod_index);
        render_context.set_uv_pan(rf::vec2_zero_vector);
        render_context.set_primitive_topology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

        std::optional<gr::Mode> forced_mode;
        if (params.flags & 1) {
            // used by rail gun scanner for heat overlays
            forced_mode.emplace(
                TEXTURE_SOURCE_NONE,
                COLOR_SOURCE_VERTEX,
                ALPHA_SOURCE_VERTEX,
                ALPHA_BLEND_ALPHA,
                ZBUFFER_TYPE_FULL,
                FOG_ALLOWED
            );
        }
        else if (params.flags & 8) {
            // used by rocket launcher scanner together with flag 1 so this code block seems unused
            forced_mode.emplace(
                TEXTURE_SOURCE_NONE,
                COLOR_SOURCE_VERTEX,
                ALPHA_SOURCE_VERTEX,
                ALPHA_BLEND_ALPHA_ADDITIVE,
                ZBUFFER_TYPE_NONE,
                FOG_ALLOWED
            );
        }
        rf::ubyte alpha = static_cast<ubyte>(params.alpha);
        rf::Color color{255, 255, 255, 255};
        if ((params.flags & 2) && (params.flags & 9)) {
            color.set(params.self_illum.red, params.self_illum.green, params.self_illum.blue, alpha);
        }

        for (auto& b : meshes_[lod_index].batches) {
            // ccrunch tool chunkifies mesh and inits render mode flags
            // 0x110C21 is used for materials with additive blending (except admin_poshlight01.v3d):
            // - TEXTURE_SOURCE_WRAP
            // - COLOR_SOURCE_TEXTURE
            // - ALPHA_SOURCE_VERTEX_TIMES_TEXTURE
            // - ALPHA_BLEND_ALPHA_ADDITIVE
            // - ZBUFFER_TYPE_READ
            // - FOG_ALLOWED
            // 0x518C41 is used for other materials:
            // - TEXTURE_SOURCE_WRAP
            // - COLOR_SOURCE_VERTEX_TIMES_TEXTURE
            // - ALPHA_SOURCE_VERTEX_TIMES_TEXTURE
            // - ALPHA_BLEND_ALPHA
            // - ZBUFFER_TYPE_FULL_ALPHA_TEST
            // - FOG_ALLOWED
            // This information may be useful for simplifying shaders
            render_context.set_cull_mode(b.double_sided ? D3D11_CULL_NONE : D3D11_CULL_BACK);
            int texture = tex_handles[b.texture_index];
            render_context.set_mode_and_textures(forced_mode.value_or(b.mode), texture, -1, color);
            render_context.device_context()->DrawIndexed(b.num_indices, b.start_index, 0);
            if (params.powerup_bitmaps[0] != -1) {
                gr::Mode powerup_mode{
                    gr::TEXTURE_SOURCE_CLAMP,
                    gr::COLOR_SOURCE_TEXTURE,
                    gr::ALPHA_SOURCE_TEXTURE,
                    gr::ALPHA_BLEND_ALPHA_ADDITIVE,
                    gr::ZBUFFER_TYPE_READ,
                    gr::FOG_NOT_ALLOWED,
                };
                render_context.set_mode_and_textures(powerup_mode, params.powerup_bitmaps[0], -1, color);
                render_context.device_context()->DrawIndexed(b.num_indices, b.start_index, 0);
                if (params.powerup_bitmaps[1] != -1) {
                    render_context.set_mode_and_textures(powerup_mode, params.powerup_bitmaps[1], -1, color);
                    render_context.device_context()->DrawIndexed(b.num_indices, b.start_index, 0);
                }
            }
        }
    }

    static bool is_vif_chunk_double_sided(const VifChunk& chunk)
    {
        for (int face_index = 0; face_index < chunk.num_faces; ++face_index) {
            auto& face = chunk.faces[face_index];
            // If any face belonging to a chunk is not double sided mark the chunk as not double sided and
            // duplicate individual faces that have double sided flag
            if (!(face.flags & VIF_FACE_DOUBLE_SIDED)) {
                return false;
            }
        }
        return true;
    }

    MeshRenderCache::MeshRenderCache(VifLodMesh* lod_mesh, ID3D11Device* device) :
        BaseMeshRenderCache(lod_mesh)
    {
        std::size_t num_verts = 0;
        std::size_t num_inds = 0;
        for (int lod_index = 0; lod_index < lod_mesh->num_levels; ++lod_index) {
            rf::VifMesh* mesh = lod_mesh->meshes[lod_index];
            for (int chunk_index = 0; chunk_index < mesh->num_chunks; ++chunk_index) {
                rf::VifChunk& chunk = mesh->chunks[chunk_index];
                num_verts += chunk.num_vecs;
                // Note: we may reserve too little if mesh uses double sided faces but it is fine
                num_inds += chunk.num_faces * 3;
            }
        }

        std::vector<GpuVertex> gpu_verts;
        std::vector<rf::ushort> gpu_inds;
        gpu_verts.reserve(num_verts);
        gpu_inds.reserve(num_inds);
        meshes_.resize(lod_mesh->num_levels);

        for (int lod_index = 0; lod_index < lod_mesh->num_levels; ++lod_index) {
            rf::VifMesh* mesh = lod_mesh->meshes[lod_index];
            meshes_[lod_index].batches.reserve(mesh->num_chunks);
            for (int chunk_index = 0; chunk_index < mesh->num_chunks; ++chunk_index) {
                rf::VifChunk& chunk = mesh->chunks[chunk_index];

                Batch& b = meshes_[lod_index].batches.emplace_back();
                b.start_index = gpu_inds.size();
                b.texture_index = chunk.texture_idx;
                b.mode = chunk.mode;
                b.double_sided = is_vif_chunk_double_sided(chunk);

                int chunk_start_index = gpu_verts.size();
                for (int vert_index = 0; vert_index < chunk.num_vecs; ++vert_index) {
                    GpuVertex& gpu_vert = gpu_verts.emplace_back();
                    gpu_vert.x = chunk.vecs[vert_index].x;
                    gpu_vert.y = chunk.vecs[vert_index].y;
                    gpu_vert.z = chunk.vecs[vert_index].z;
                    gpu_vert.u0 = chunk.uvs[vert_index].x;
                    gpu_vert.v0 = chunk.uvs[vert_index].y;
                    gpu_vert.diffuse = 0xFFFFFFFF;
                }
                for (int face_index = 0; face_index < chunk.num_faces; ++face_index) {
                    auto& face = chunk.faces[face_index];
                    gpu_inds.emplace_back(chunk_start_index + face.vindex1);
                    gpu_inds.emplace_back(chunk_start_index + face.vindex2);
                    gpu_inds.emplace_back(chunk_start_index + face.vindex3);
                    if ((face.flags & VIF_FACE_DOUBLE_SIDED) && !b.double_sided) {
                        gpu_inds.emplace_back(chunk_start_index + face.vindex1);
                        gpu_inds.emplace_back(chunk_start_index + face.vindex3);
                        gpu_inds.emplace_back(chunk_start_index + face.vindex2);
                    }
                }

                b.num_indices = gpu_inds.size() - b.start_index;
            }
        }
        xlog::info("Creating mesh geometry buffers: verts %d inds %d", gpu_verts.size(), gpu_inds.size());

        CD3D11_BUFFER_DESC vb_desc{
            sizeof(gpu_verts[0]) * gpu_verts.size(),
            D3D11_BIND_VERTEX_BUFFER,
            D3D11_USAGE_IMMUTABLE,
        };
        D3D11_SUBRESOURCE_DATA vb_subres_data{gpu_verts.data(), 0, 0};
        HRESULT hr = device->CreateBuffer(&vb_desc, &vb_subres_data, &vb_);
        check_hr(hr, "CreateBuffer (static mesh vertex buffer)");

        CD3D11_BUFFER_DESC ib_desc{
            sizeof(gpu_inds[0]) * gpu_inds.size(),
            D3D11_BIND_INDEX_BUFFER,
            D3D11_USAGE_IMMUTABLE,
        };
        D3D11_SUBRESOURCE_DATA ib_subres_data{gpu_inds.data(), 0, 0};
        hr = device->CreateBuffer(&ib_desc, &ib_subres_data, &ib_);
        check_hr(hr, "CreateBuffer (static mesh index buffer)");
    }

    void MeshRenderCache::render(const Vector3& pos, const Matrix3& orient, const MeshRenderParams& params, int lod_index, RenderContext& render_context)
    {
        //xlog::warn("render mesh flags %x", params.flags);
        render_context.set_model_transform(pos, orient);
        render_context.set_vertex_buffer(vb_, sizeof(GpuVertex));
        render_context.set_index_buffer(ib_);
        draw(params, lod_index, render_context);

        // TODO: params.alpha
    }

    CharacterMeshRenderCache::CharacterMeshRenderCache(VifLodMesh* lod_mesh, ID3D11Device* device) :
        BaseMeshRenderCache(lod_mesh)
    {
        std::size_t num_verts = 0;
        std::size_t num_inds = 0;
        for (int lod_index = 0; lod_index < lod_mesh->num_levels; ++lod_index) {
            rf::VifMesh* mesh = lod_mesh->meshes[lod_index];
            for (int chunk_index = 0; chunk_index < mesh->num_chunks; ++chunk_index) {
                rf::VifChunk& chunk = mesh->chunks[chunk_index];
                num_verts += chunk.num_vecs;
                // Note: we may reserve too little if mesh uses double sided faces but it is fine
                num_inds += chunk.num_faces * 3;
            }
        }

        std::vector<GpuCharacterVertex> gpu_verts;
        std::vector<rf::ushort> gpu_inds;
        gpu_verts.reserve(num_verts);
        gpu_inds.reserve(num_inds);
        meshes_.resize(lod_mesh->num_levels);

        for (int lod_index = 0; lod_index < lod_mesh->num_levels; ++lod_index) {
            rf::VifMesh* mesh = lod_mesh->meshes[lod_index];
            meshes_[lod_index].batches.reserve(mesh->num_chunks);

            for (int chunk_index = 0; chunk_index < mesh->num_chunks; ++chunk_index) {
                rf::VifChunk& chunk = mesh->chunks[chunk_index];

                Batch& b = meshes_[lod_index].batches.emplace_back();
                b.start_index = gpu_inds.size();
                b.texture_index = chunk.texture_idx;
                b.mode = chunk.mode;
                b.double_sided = is_vif_chunk_double_sided(chunk);

                int chunk_start_index = gpu_verts.size();
                for (int vert_index = 0; vert_index < chunk.num_vecs; ++vert_index) {
                    GpuCharacterVertex& gpu_vert = gpu_verts.emplace_back();
                    gpu_vert.x = chunk.vecs[vert_index].x;
                    gpu_vert.y = chunk.vecs[vert_index].y;
                    gpu_vert.z = chunk.vecs[vert_index].z;
                    gpu_vert.u0 = chunk.uvs[vert_index].x;
                    gpu_vert.v0 = chunk.uvs[vert_index].y;
                    gpu_vert.diffuse = 0xFFFFFFFF;
                    if (chunk.wi) {
                        for (int i = 0; i < 4; ++i) {
                            gpu_vert.weights[i] = chunk.wi[vert_index].weights[i];
                            gpu_vert.indices[i] = chunk.wi[vert_index].indices[i];
                            if (gpu_vert.indices[i] >= 50) {
                                gpu_vert.indices[i] = 49;
                            }
                        }
                    }
                }
                for (int face_index = 0; face_index < chunk.num_faces; ++face_index) {
                    auto& face = chunk.faces[face_index];
                    gpu_inds.emplace_back(chunk_start_index + face.vindex1);
                    gpu_inds.emplace_back(chunk_start_index + face.vindex2);
                    gpu_inds.emplace_back(chunk_start_index + face.vindex3);
                    if ((face.flags & VIF_FACE_DOUBLE_SIDED) && !b.double_sided) {
                        gpu_inds.emplace_back(chunk_start_index + face.vindex1);
                        gpu_inds.emplace_back(chunk_start_index + face.vindex3);
                        gpu_inds.emplace_back(chunk_start_index + face.vindex2);
                    }
                }

                b.num_indices = gpu_inds.size() - b.start_index;
            }
        }
        xlog::warn("Creating mesh render buffer - verts %d inds %d", gpu_verts.size(), gpu_inds.size());

        CD3D11_BUFFER_DESC vb_desc{
            sizeof(gpu_verts[0]) * gpu_verts.size(),
            D3D11_BIND_VERTEX_BUFFER,
            D3D11_USAGE_IMMUTABLE,
        };
        D3D11_SUBRESOURCE_DATA vb_subres_data{gpu_verts.data(), 0, 0};
        HRESULT hr = device->CreateBuffer(&vb_desc, &vb_subres_data, &vb_);
        check_hr(hr, "CreateBuffer (character vertex buffer)");

        CD3D11_BUFFER_DESC ib_desc{
            sizeof(gpu_inds[0]) * gpu_inds.size(),
            D3D11_BIND_INDEX_BUFFER,
            D3D11_USAGE_IMMUTABLE,
        };
        D3D11_SUBRESOURCE_DATA ib_subres_data{gpu_inds.data(), 0, 0};
        hr = device->CreateBuffer(&ib_desc, &ib_subres_data, &ib_);
        check_hr(hr, "CreateBuffer (character index buffer)");

        CD3D11_BUFFER_DESC matrices_buffer_desc{
            sizeof(BoneTransformsBufferData),
            D3D11_BIND_CONSTANT_BUFFER,
            D3D11_USAGE_DYNAMIC,
            D3D11_CPU_ACCESS_WRITE,
        };
        hr = device->CreateBuffer(&matrices_buffer_desc, nullptr, &matrices_cbuffer_);
        check_hr(hr, "CreateBuffer (character constant buffer)");
    }

    static inline GpuMatrix4x3 convert_bone_matrix(const Matrix43& mat)
    {
        return {{
            {mat.orient.rvec.x, mat.orient.uvec.x, mat.orient.fvec.x, mat.origin.x},
            {mat.orient.rvec.y, mat.orient.uvec.y, mat.orient.fvec.y, mat.origin.y},
            {mat.orient.rvec.z, mat.orient.uvec.z, mat.orient.fvec.z, mat.origin.z},
        }};
    }

    void CharacterMeshRenderCache::render(const Vector3& pos, const Matrix3& orient, const CharacterInstance* ci, const MeshRenderParams& params, int lod_index, RenderContext& render_context)
    {
        update_matrices_buffer(ci, render_context);
        render_context.set_model_transform(pos, orient);
        render_context.bind_vs_cbuffer(3, matrices_cbuffer_);
        render_context.set_vertex_buffer(vb_, sizeof(GpuCharacterVertex));
        render_context.set_index_buffer(ib_);
        draw(params, lod_index, render_context);
        render_context.bind_vs_cbuffer(3, nullptr);
    }

    void CharacterMeshRenderCache::update_matrices_buffer(const CharacterInstance* ci, RenderContext& render_context)
    {
        BoneTransformsBufferData data;
        // Note: if some matrices that are unused by skeleton are referenced by vertices and not get initialized
        //       bad things can happen even if weight is zero (e.g. in case of NaNs)
        std::memset(&data, 0, sizeof(data));
        for (int i = 0; i < ci->base_character->num_bones; ++i) {
            const Matrix43& bone_mat = ci->bone_transforms_final[i];
            data.matrices[i] = convert_bone_matrix(bone_mat);
        }

        D3D11_MAPPED_SUBRESOURCE mapped_cb;
        HRESULT hr = render_context.device_context()->Map(matrices_cbuffer_, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped_cb);
        check_hr(hr, "Map character cbuffer");
        std::memcpy(mapped_cb.pData, &data, sizeof(data));
        render_context.device_context()->Unmap(matrices_cbuffer_, 0);
    }

    MeshRenderer::MeshRenderer(ComPtr<ID3D11Device> device, ShaderManager& shader_manager, RenderContext& render_context) :
        device_{std::move(device)}, render_context_{render_context}
    {
        standard_vertex_shader_ = shader_manager.get_vertex_shader(VertexShaderId::standard);
        character_vertex_shader_ = shader_manager.get_vertex_shader(VertexShaderId::character);
        pixel_shader_ = shader_manager.get_pixel_shader(PixelShaderId::standard);
    }

    MeshRenderer::~MeshRenderer()
    {
        // Note: meshes are already destroyed here
        render_caches_.clear();
    }

    static inline int get_vif_mesh_lod_index(rf::VifLodMesh *lod_mesh, rf::VifMesh *mesh)
    {
        for (int i = 0; i < lod_mesh->num_levels; ++i) {
            if (lod_mesh->meshes[i] == mesh) {
                return i;
            }
        }
        assert(false);
        return 0;
    }

    void MeshRenderer::render_v3d_vif([[maybe_unused]] rf::VifLodMesh *lod_mesh, rf::VifMesh *mesh, const rf::Vector3& pos, const rf::Matrix3& orient, const rf::MeshRenderParams& params)
    {
        if (!lod_mesh->render_cache) {
            auto p = render_caches_.insert_or_assign(lod_mesh, std::make_unique<MeshRenderCache>(lod_mesh, device_));
            lod_mesh->render_cache = p.first->second.get();
        }

        render_context_.set_vertex_shader(standard_vertex_shader_);
        render_context_.set_pixel_shader(pixel_shader_);

        int lod_index = get_vif_mesh_lod_index(lod_mesh, mesh);
        auto render_cache = reinterpret_cast<MeshRenderCache*>(lod_mesh->render_cache);
        render_cache->render(pos, orient, params, lod_index, render_context_);
    }

    void MeshRenderer::render_character_vif([[maybe_unused]] rf::VifLodMesh *lod_mesh, rf::VifMesh *mesh, const rf::Vector3& pos, const rf::Matrix3& orient, const rf::CharacterInstance *ci, const rf::MeshRenderParams& params)
    {
        if (!lod_mesh->render_cache) {
            auto p = render_caches_.insert_or_assign(lod_mesh, std::make_unique<CharacterMeshRenderCache>(lod_mesh, device_));
            lod_mesh->render_cache = p.first->second.get();
        }

        render_context_.set_vertex_shader(character_vertex_shader_);
        render_context_.set_pixel_shader(pixel_shader_);

        int lod_index = get_vif_mesh_lod_index(lod_mesh, mesh);
        auto render_cache = reinterpret_cast<CharacterMeshRenderCache*>(lod_mesh->render_cache);
        render_cache->render(pos, orient, ci, params, lod_index, render_context_);
    }

    void MeshRenderer::clear_vif_cache(rf::VifLodMesh *lod_mesh)
    {
        render_caches_.erase(lod_mesh);
    }
}
