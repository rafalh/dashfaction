#include <memory>
#include <cstring>
#include <cassert>
#include <windows.h>
#include <common/ComPtr.h>
#include <xlog/xlog.h>
#include "../../rf/gr/gr.h"
#include "../../rf/gr/gr_light.h"
#include "../../rf/math/quaternion.h"
#include "../../rf/v3d.h"
#include "../../rf/character.h"
#include "../../main/main.h"
#include "../../multi/multi.h"
#include "gr_d3d11.h"
#include "gr_d3d11_mesh.h"
#include "gr_d3d11_context.h"
#include "gr_d3d11_shader.h"

using namespace rf;

namespace df::gr::d3d11
{
    constexpr unsigned initial_vb_size = 6000;
    constexpr unsigned initial_ib_size = 10000;

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

    class MeshRenderCache : public BaseMeshRenderCache
    {
    public:
        MeshRenderCache(VifLodMesh* lod_mesh, BufferWrapper& vertex_buffer, BufferWrapper& index_buffer, RenderContext& render_context);
    };

    MeshRenderCache::MeshRenderCache(VifLodMesh* lod_mesh, BufferWrapper& vertex_buffer, BufferWrapper& index_buffer, RenderContext& render_context) :
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
                std::size_t start_index = gpu_inds.size();
                std::size_t base_vertex = vertex_buffer.size() + gpu_verts.size();
                bool double_sided = is_vif_chunk_double_sided(chunk);

                for (int vert_index = 0; vert_index < chunk.num_vecs; ++vert_index) {
                    GpuVertex& gpu_vert = gpu_verts.emplace_back();
                    gpu_vert.x = chunk.vecs[vert_index].x;
                    gpu_vert.y = chunk.vecs[vert_index].y;
                    gpu_vert.z = chunk.vecs[vert_index].z;
                    gpu_vert.norm = {
                        chunk.norms[vert_index].x,
                        chunk.norms[vert_index].y,
                        chunk.norms[vert_index].z,
                    };
                    gpu_vert.diffuse = 0xFFFFFFFF;
                    gpu_vert.u0 = chunk.uvs[vert_index].x;
                    gpu_vert.v0 = chunk.uvs[vert_index].y;
                    gpu_vert.u0_pan_speed = 0.0f;
                    gpu_vert.v0_pan_speed = 0.0f;
                    gpu_vert.u1 = 0.0f;
                    gpu_vert.v1 = 0.0f;
                }
                for (int face_index = 0; face_index < chunk.num_faces; ++face_index) {
                    auto& face = chunk.faces[face_index];
                    gpu_inds.emplace_back(face.vindex1);
                    gpu_inds.emplace_back(face.vindex2);
                    gpu_inds.emplace_back(face.vindex3);
                    if ((face.flags & VIF_FACE_DOUBLE_SIDED) && !double_sided) {
                        gpu_inds.emplace_back(face.vindex1);
                        gpu_inds.emplace_back(face.vindex3);
                        gpu_inds.emplace_back(face.vindex2);
                    }
                }

                int num_indices = gpu_inds.size() - start_index;
                meshes_[lod_index].batches.emplace_back(
                    index_buffer.size() + start_index, num_indices, base_vertex,
                    chunk.texture_idx, chunk.mode, double_sided);
            }
        }
        xlog::debug("Creating mesh geometry buffers: verts {} inds {}", gpu_verts.size(), gpu_inds.size());

        vertex_buffer.write(gpu_verts.data(), gpu_verts.size(), render_context);
        index_buffer.write(gpu_inds.data(), gpu_inds.size(), render_context);
    }

    struct alignas(16) BoneTransformsBufferData
    {
        GpuMatrix4x3 matrices[50];
    };

    class BoneTransformsBuffer
    {
    public:
        BoneTransformsBuffer(ID3D11Device* device);
        void update(const CharacterInstance* ci, ID3D11DeviceContext* device_context);

        operator ID3D11Buffer*() const
        {
            return buffer_;
        }

    private:
        ComPtr<ID3D11Buffer> buffer_;
    };

    BoneTransformsBuffer::BoneTransformsBuffer(ID3D11Device* device)
    {
        CD3D11_BUFFER_DESC buffer_desc{
            sizeof(BoneTransformsBufferData),
            D3D11_BIND_CONSTANT_BUFFER,
            D3D11_USAGE_DYNAMIC,
            D3D11_CPU_ACCESS_WRITE,
        };
        DF_GR_D3D11_CHECK_HR(
            device->CreateBuffer(&buffer_desc, nullptr, &buffer_)
        );
    }

    static inline GpuMatrix4x3 convert_bone_matrix(const Matrix43& mat)
    {
        return {{
            {mat.orient.rvec.x, mat.orient.uvec.x, mat.orient.fvec.x, mat.origin.x},
            {mat.orient.rvec.y, mat.orient.uvec.y, mat.orient.fvec.y, mat.origin.y},
            {mat.orient.rvec.z, mat.orient.uvec.z, mat.orient.fvec.z, mat.origin.z},
        }};
    }

    void BoneTransformsBuffer::update(const CharacterInstance* ci, ID3D11DeviceContext* device_context)
    {
        // Note: if some matrices that are unused by skeleton are referenced by vertices and not get initialized
        //       bad things can happen even if weight is zero (e.g. in case of NaNs)
        BoneTransformsBufferData data{};

        for (int i = 0; i < ci->base_character->num_bones; ++i) {
            const Matrix43& bone_mat = ci->bone_transforms_final[i];
            data.matrices[i] = convert_bone_matrix(bone_mat);
        }

        D3D11_MAPPED_SUBRESOURCE mapped_subres;
        DF_GR_D3D11_CHECK_HR(
            device_context->Map(buffer_, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped_subres)
        );
        std::memcpy(mapped_subres.pData, &data, sizeof(data));
        device_context->Unmap(buffer_, 0);
    }


    class CharacterMeshRenderCache : public BaseMeshRenderCache
    {
    public:
        CharacterMeshRenderCache(VifLodMesh* lod_mesh, ID3D11Device* device);

        void bind_buffers(RenderContext& render_context, bool morphed)
        {
            render_context.bind_vs_cbuffer(3, bone_transforms_buffer_);

            ID3D11Buffer* vertex_buffer_0 = morphed ? morphed_vertex_buffer_0_ : vertex_buffer_0_;
            render_context.set_vertex_buffer(vertex_buffer_0, sizeof(GpuCharacterVertex0), 0);
            render_context.set_vertex_buffer(vertex_buffer_1_, sizeof(GpuCharacterVertex1), 1);
            render_context.set_index_buffer(index_buffer_);
        }

        void update_bone_transforms_buffer(const CharacterInstance* ci, RenderContext& render_context)
        {
            bone_transforms_buffer_.update(ci, render_context.device_context());
        }

        void update_morphed_vertices_buffer(rf::Skeleton* skeleton, int time, RenderContext& render_context);

    private:
        ComPtr<ID3D11Buffer> vertex_buffer_0_;
        ComPtr<ID3D11Buffer> vertex_buffer_1_;
        ComPtr<ID3D11Buffer> morphed_vertex_buffer_0_;
        ComPtr<ID3D11Buffer> index_buffer_;
        BoneTransformsBuffer bone_transforms_buffer_;
    };

    CharacterMeshRenderCache::CharacterMeshRenderCache(VifLodMesh* lod_mesh, ID3D11Device* device) :
        BaseMeshRenderCache(lod_mesh), bone_transforms_buffer_{device}
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

        std::vector<GpuCharacterVertex0> gpu_verts_0;
        std::vector<GpuCharacterVertex1> gpu_verts_1;
        std::vector<rf::ushort> gpu_inds;
        gpu_verts_0.reserve(num_verts);
        gpu_verts_1.reserve(num_verts);
        gpu_inds.reserve(num_inds);
        meshes_.resize(lod_mesh->num_levels);

        for (int lod_index = 0; lod_index < lod_mesh->num_levels; ++lod_index) {
            rf::VifMesh* mesh = lod_mesh->meshes[lod_index];
            meshes_[lod_index].batches.reserve(mesh->num_chunks);

            for (int chunk_index = 0; chunk_index < mesh->num_chunks; ++chunk_index) {
                rf::VifChunk& chunk = mesh->chunks[chunk_index];

                std::size_t start_index = gpu_inds.size();
                std::size_t base_vertex = gpu_verts_0.size();
                bool double_sided = is_vif_chunk_double_sided(chunk);

                for (int vert_index = 0; vert_index < chunk.num_vecs; ++vert_index) {
                    int pos_vert_index = vert_index;
                    int pos_vert_offset = chunk.same_vertex_offsets[vert_index];
                    if (pos_vert_offset > 0) {
                        pos_vert_index -= pos_vert_offset;
                    }
                    GpuCharacterVertex0& gpu_vert_0 = gpu_verts_0.emplace_back();
                    GpuCharacterVertex1& gpu_vert_1 = gpu_verts_1.emplace_back();
                    gpu_vert_0.x = chunk.vecs[pos_vert_index].x;
                    gpu_vert_0.y = chunk.vecs[pos_vert_index].y;
                    gpu_vert_0.z = chunk.vecs[pos_vert_index].z;
                    gpu_vert_1.norm = {
                        chunk.norms[pos_vert_index].x,
                        chunk.norms[pos_vert_index].y,
                        chunk.norms[pos_vert_index].z,
                    };
                    gpu_vert_1.diffuse = 0xFFFFFFFF;
                    gpu_vert_1.u0 = chunk.uvs[vert_index].x;
                    gpu_vert_1.v0 = chunk.uvs[vert_index].y;
                    if (chunk.wi) {
                        for (int i = 0; i < 4; ++i) {
                            gpu_vert_1.weights[i] = chunk.wi[pos_vert_index].weights[i];
                            gpu_vert_1.indices[i] = chunk.wi[pos_vert_index].indices[i];
                            if (gpu_vert_1.indices[i] >= 50) {
                                gpu_vert_1.indices[i] = 49;
                            }
                        }
                    }
                }
                for (int face_index = 0; face_index < chunk.num_faces; ++face_index) {
                    auto& face = chunk.faces[face_index];
                    gpu_inds.emplace_back(face.vindex1);
                    gpu_inds.emplace_back(face.vindex2);
                    gpu_inds.emplace_back(face.vindex3);
                    if ((face.flags & VIF_FACE_DOUBLE_SIDED) && !double_sided) {
                        gpu_inds.emplace_back(face.vindex1);
                        gpu_inds.emplace_back(face.vindex3);
                        gpu_inds.emplace_back(face.vindex2);
                    }
                }

                int num_indices = gpu_inds.size() - start_index;
                meshes_[lod_index].batches.emplace_back(
                    start_index, num_indices, base_vertex,
                    chunk.texture_idx, chunk.mode, double_sided);
            }
        }
        xlog::debug("Creating mesh render buffer - verts {} inds {}", gpu_verts_0.size(), gpu_inds.size());

        CD3D11_BUFFER_DESC vb_0_desc{
            sizeof(gpu_verts_0[0]) * gpu_verts_0.size(),
            D3D11_BIND_VERTEX_BUFFER,
            D3D11_USAGE_IMMUTABLE,
        };
        D3D11_SUBRESOURCE_DATA vb_0_subres_data{gpu_verts_0.data(), 0, 0};
        DF_GR_D3D11_CHECK_HR(
            device->CreateBuffer(&vb_0_desc, &vb_0_subres_data, &vertex_buffer_0_)
        );

        CD3D11_BUFFER_DESC vb_1_desc{
            sizeof(gpu_verts_1[1]) * gpu_verts_1.size(),
            D3D11_BIND_VERTEX_BUFFER,
            D3D11_USAGE_IMMUTABLE,
        };
        D3D11_SUBRESOURCE_DATA vb_1_subres_data{gpu_verts_1.data(), 0, 0};
        DF_GR_D3D11_CHECK_HR(
            device->CreateBuffer(&vb_1_desc, &vb_1_subres_data, &vertex_buffer_1_)
        );

        CD3D11_BUFFER_DESC ib_desc{
            sizeof(gpu_inds[0]) * gpu_inds.size(),
            D3D11_BIND_INDEX_BUFFER,
            D3D11_USAGE_IMMUTABLE,
        };
        D3D11_SUBRESOURCE_DATA ib_subres_data{gpu_inds.data(), 0, 0};
        DF_GR_D3D11_CHECK_HR(
            device->CreateBuffer(&ib_desc, &ib_subres_data, &index_buffer_)
        );
    }

    void CharacterMeshRenderCache::update_morphed_vertices_buffer(rf::Skeleton* skeleton, int time, RenderContext& render_context)
    {
        rf::VifMesh* mesh = lod_mesh_->meshes[0];
        if (!morphed_vertex_buffer_0_) {
            int num_verts = 0;
            for (int chunk_index = 0; chunk_index < mesh->num_chunks; ++chunk_index) {
                rf::VifChunk& chunk = mesh->chunks[chunk_index];
                num_verts += chunk.num_vecs;
            }
            CD3D11_BUFFER_DESC morphed_vb_0_desc{
                sizeof(GpuCharacterVertex0) * num_verts,
                D3D11_BIND_VERTEX_BUFFER,
                D3D11_USAGE_DYNAMIC,
                D3D11_CPU_ACCESS_WRITE,
            };
            DF_GR_D3D11_CHECK_HR(
                render_context.device()->CreateBuffer(&morphed_vb_0_desc, nullptr, &morphed_vertex_buffer_0_)
            );
        }

        D3D11_MAPPED_SUBRESOURCE mapped_vb;
        DF_GR_D3D11_CHECK_HR(
            render_context.device_context()->Map(morphed_vertex_buffer_0_, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped_vb)
        );

        for (int chunk_index = 0; chunk_index < mesh->num_chunks; ++chunk_index) {
            int base_vertex = meshes_[0].batches[chunk_index].base_vertex;
            auto* gpu_vecs = reinterpret_cast<rf::Vector3*>(mapped_vb.pData) + base_vertex;
            rf::VifChunk& chunk = mesh->chunks[chunk_index];
            std::vector<Vector3> morphed_vecs(chunk.vecs, chunk.vecs + chunk.num_vecs);
            skeleton->morph(morphed_vecs.data(), chunk.num_vecs, time, chunk.orig_map, mesh->num_original_vecs);
            for (int vert_index = 0; vert_index < chunk.num_vecs; ++vert_index) {
                int pos_vert_offset = chunk.same_vertex_offsets[vert_index];
                if (pos_vert_offset > 0) {
                    morphed_vecs[vert_index] = morphed_vecs[vert_index - pos_vert_offset];
                }
            }
            std::memcpy(gpu_vecs, morphed_vecs.data(), chunk.num_vecs * sizeof(rf::Vector3));
            gpu_vecs += chunk.num_vecs;
        }
        render_context.device_context()->Unmap(morphed_vertex_buffer_0_, 0);
    }

    MeshRenderer::MeshRenderer(ComPtr<ID3D11Device> device, ShaderManager& shader_manager,
        [[maybe_unused]] StateManager& state_manager, RenderContext& render_context) :
        device_{std::move(device)}, render_context_{render_context},
        v3d_vb_{initial_vb_size, sizeof(GpuVertex), D3D11_BIND_VERTEX_BUFFER, device_},
        v3d_ib_{initial_ib_size, sizeof(rf::ushort), D3D11_BIND_INDEX_BUFFER, device_}
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

    void MeshRenderer::render_v3d_vif(rf::VifLodMesh *lod_mesh, int lod_index, const rf::Vector3& pos, const rf::Matrix3& orient, rf::MeshRenderParams params)
    {
        page_in_v3d_mesh(lod_mesh);

        render_context_.set_vertex_shader(standard_vertex_shader_);
        render_context_.set_pixel_shader(pixel_shader_);
        render_context_.set_model_transform(pos, orient);
        render_context_.set_vertex_buffer(v3d_vb_.buffer(), sizeof(GpuVertex));
        render_context_.set_index_buffer(v3d_ib_.buffer());

        auto render_cache = reinterpret_cast<MeshRenderCache*>(lod_mesh->render_cache);

        const bool ir_scanner = (params.flags & MRF_SCANNER_1) != 0;
        if (!ir_scanner) {
            rf::Vector3 ambient_light{0.f, 0.f, 0.f};
            light_get_ambient(&ambient_light.x, &ambient_light.y, &ambient_light.z);
            ambient_light *= 255.f;
            // RF uses some hard-coded lights here but for now let's keep it simple
            ambient_light += 40.f;
            // Ignore ambient_color from params, it changes sharply and RF uses it only indirectly for
            // its hard-coded lights
            params.ambient_color.set(
                static_cast<rf::ubyte>(std::min(ambient_light.x, 255.f)),
                static_cast<rf::ubyte>(std::min(ambient_light.y, 255.f)),
                static_cast<rf::ubyte>(std::min(ambient_light.z, 255.f)),
                255
            );
        }

        draw_cached_mesh(lod_mesh, *render_cache, v3d_params, lod_index);
    }

    void MeshRenderer::render_character_vif(rf::VifLodMesh *lod_mesh, int lod_index, const rf::Vector3& pos, const rf::Matrix3& orient, const rf::CharacterInstance *ci, f::MeshRenderParams params)
    {
        page_in_character_mesh(lod_mesh);
        auto render_cache = reinterpret_cast<CharacterMeshRenderCache*>(lod_mesh->render_cache);

        render_context_.set_vertex_shader(character_vertex_shader_);
        render_context_.set_pixel_shader(pixel_shader_);
        render_context_.set_model_transform(pos, orient);

        bool morphed = false;
        // Note: morphing data exists only for the most detailed LOD
        if (lod_index == 0) {
            for (int i = 0; i < ci->num_active_anims; ++i) {
                const rf::CiAnimInfo& anim_info = ci->active_anims[i];
                rf::Skeleton* skeleton = ci->base_character->animations[anim_info.anim_index];
                if (skeleton->has_morph_vertices()) {
                    morphed = true;
                    render_cache->update_morphed_vertices_buffer(skeleton, anim_info.cur_time, render_context_);
                    break;
                }
            }
        }
        render_cache->update_bone_transforms_buffer(ci, render_context_);
        render_cache->bind_buffers(render_context_, morphed);

        const bool ir_scanner = (params.flags & MRF_SCANNER_1) != 0;
        if (!ir_scanner) {
            const bool allow_full_bright = !(get_remote_server_info()
                && !get_remote_server_info().value().allow_full_bright_entities);
            if (g_game_config.full_bright_entities
                && allow_full_bright
                && !(params.flags & MRF_FIRST_PERSON)) {
                params.ambient_color.set(255, 255, 255, 255);
            }
        }

        draw_cached_mesh(lod_mesh, *render_cache, params, lod_index);
    }

    void MeshRenderer::clear_vif_cache(rf::VifLodMesh *lod_mesh)
    {
        render_caches_.erase(lod_mesh);
    }

    static inline const int* get_tex_handles(rf::VifLodMesh* lod_mesh, const MeshRenderParams& params, int lod_index)
    {
        if (params.alt_tex) {
            return params.alt_tex;
        }
        return lod_mesh->meshes[lod_index]->tex_handles;
    }

    void MeshRenderer::draw_cached_mesh(rf::VifLodMesh *lod_mesh, const BaseMeshRenderCache& cache, const MeshRenderParams& params, int lod_index)
    {
        const int* tex_handles = get_tex_handles(lod_mesh, params, lod_index);
        render_context_.set_primitive_topology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

        std::optional<gr::Mode> forced_mode;
        bool ir_scanner = (params.flags & MRF_SCANNER_1) != 0;
        if (ir_scanner) {
            // used by rail gun scanner for heat overlays
            forced_mode.emplace(
                TEXTURE_SOURCE_NONE,
                COLOR_SOURCE_VERTEX,
                ALPHA_SOURCE_VERTEX,
                ALPHA_BLEND_ALPHA,
                ZBUFFER_TYPE_FULL,
                FOG_ALLOWED
            );
            static int null_tex_handles[7] = {-1, -1, -1, -1, -1, -1, -1};
            tex_handles = null_tex_handles;
        }
        else if (params.flags & MRF_SCANNER_2) {
            // used by rocket launcher scanner together with flag 1 so this code block seems unused
            assert(false);
        }
        rf::Color color{255, 255, 255};
        if (!ir_scanner) {
            color = params.ambient_color;
        } else {
            color = params.self_illum;
        }
        color.alpha = static_cast<ubyte>(params.alpha);

        auto& batches = cache.get_batches(lod_index);

        for (auto& b : batches) {
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
            render_context_.set_cull_mode(b.double_sided ? D3D11_CULL_NONE : D3D11_CULL_BACK);
            int texture = tex_handles[b.texture_index];
            render_context_.set_mode(forced_mode.value_or(b.mode), color);
            render_context_.set_textures(texture, -1);
            render_context_.draw_indexed(b.num_indices, b.start_index, b.base_vertex);
        }
        if (params.powerup_bitmaps[0] != -1 && !ir_scanner) {
            gr::Mode powerup_mode{
                gr::TEXTURE_SOURCE_CLAMP,
                gr::COLOR_SOURCE_TEXTURE,
                gr::ALPHA_SOURCE_TEXTURE,
                gr::ALPHA_BLEND_ALPHA_ADDITIVE,
                gr::ZBUFFER_TYPE_READ,
                gr::FOG_NOT_ALLOWED,
            };
            render_context_.set_mode(powerup_mode);
            render_context_.set_textures(params.powerup_bitmaps[0], -1);
            for (auto& b : batches) {
                render_context_.set_cull_mode(b.double_sided ? D3D11_CULL_NONE : D3D11_CULL_BACK);
                render_context_.draw_indexed(b.num_indices, b.start_index, b.base_vertex);
            }
            if (params.powerup_bitmaps[1] != -1) {
                render_context_.set_textures(params.powerup_bitmaps[1], -1);
                for (auto& b : batches) {
                    render_context_.set_cull_mode(b.double_sided ? D3D11_CULL_NONE : D3D11_CULL_BACK);
                    render_context_.draw_indexed(b.num_indices, b.start_index, b.base_vertex);
                }
            }
        }
    }

    void MeshRenderer::page_in_v3d_mesh(rf::VifLodMesh* lod_mesh)
    {
        if (!lod_mesh->render_cache) {
            auto p = render_caches_.insert_or_assign(lod_mesh, std::make_unique<MeshRenderCache>(lod_mesh, v3d_vb_, v3d_ib_, render_context_));
            lod_mesh->render_cache = p.first->second.get();
        }
    }

    void MeshRenderer::page_in_character_mesh(rf::VifLodMesh* lod_mesh)
    {
        if (!lod_mesh->render_cache) {
            auto p = render_caches_.insert_or_assign(lod_mesh, std::make_unique<CharacterMeshRenderCache>(lod_mesh, device_));
            lod_mesh->render_cache = p.first->second.get();
        }
    }

    void MeshRenderer::flush_caches()
    {
        for (auto& it : render_caches_) {
            it.first->render_cache = nullptr;
        }
        render_caches_.clear();

        v3d_vb_.clear();
        v3d_ib_.clear();
    }

    BufferWrapper::BufferWrapper(unsigned initial_capacity, unsigned el_size, UINT bind_flag, ID3D11Device* device) :
        bind_flag_(bind_flag), capacity_(initial_capacity), el_size_(el_size)
    {
        create_buffer(device);
    }

    void BufferWrapper::create_buffer(ID3D11Device* device)
    {
        CD3D11_BUFFER_DESC desc{
            el_size_ * capacity_,
            bind_flag_,
            D3D11_USAGE_DEFAULT,
        };
        DF_GR_D3D11_CHECK_HR(
            device->CreateBuffer(&desc, nullptr, &buffer_)
        );
    }

    void BufferWrapper::write(void* data, unsigned n, RenderContext& render_context)
    {
        if (size_ + n > capacity_) {
            unsigned new_cap = std::max(capacity_ * 2, size_ + n);
            xlog::info("Reallocating mesh buffer: {}", new_cap);
            reserve(new_cap, render_context);
        }

        unsigned offset = size_ * el_size_;
        unsigned bytes_to_write = n * el_size_;
        D3D11_BOX box{offset, 0, 0, offset + bytes_to_write, 1, 1};
        render_context.device_context()->UpdateSubresource(buffer_, 0, &box, data, bytes_to_write, bytes_to_write);
        size_ += n;
    }

    void BufferWrapper::reserve(unsigned n, RenderContext& render_context)
    {
        if (n <= capacity_) {
            return;
        }
        capacity_ = n;

        ComPtr<ID3D11Buffer> old_buffer = std::move(buffer_);

        create_buffer(render_context.device());

        D3D11_BOX box{0, 0, 0, size_ * el_size_, 1, 1};
        render_context.device_context()->CopySubresourceRegion(buffer_, 0, 0, 0, 0, old_buffer, 0, &box);
    }
}
