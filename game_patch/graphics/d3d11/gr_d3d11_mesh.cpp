#include <memory>
#include <windows.h>
#include <patch_common/FunHook.h>
#include <patch_common/ComPtr.h>
#include <xlog/xlog.h>
#include "../rf/gr/gr.h"
#include "../rf/math/quaternion.h"
#include "../rf/v3d.h"
#include "../rf/character.h"
#include "gr_d3d11.h"

using namespace rf;

class BaseMeshRenderCache
{
public:
    virtual ~BaseMeshRenderCache() {}
    BaseMeshRenderCache(VifMesh* mesh, size_t vertex_stride) :
        mesh_(mesh), vertex_stride_(vertex_stride)
    {}

    void render(const MeshRenderParams& params);

    VifMesh* get_mesh() const
    {
        return mesh_;
    }

protected:
    struct Batch
    {
        int min_index;
        int start_index;
        int num_verts;
        int num_tris;
        int texture_index;
        gr::Mode mode;
    };

    VifMesh* mesh_;
    size_t vertex_stride_;
    std::vector<Batch> batches_;
    ComPtr<ID3D11Buffer> vb_;
    ComPtr<ID3D11Buffer> ib_;

    void create_buffers(DWORD fvf);
};

class MeshRenderCache : public BaseMeshRenderCache
{
public:
    MeshRenderCache(VifMesh* mesh);

private:
    void update_buffers();
};

class CharacterMeshRenderCache : public BaseMeshRenderCache
{
public:
    CharacterMeshRenderCache(VifMesh* mesh);
    void render(const Vector3& pos, const Matrix3& orient, const CharacterInstance* ci, const MeshRenderParams& params);

private:
    struct GpuVertex
    {
        float x;
        float y;
        float z;
        float weight[4];
        DWORD matrix_indices;
        int diffuse;
        float u0;
        float v0;
    };
    // FIXME: static_assert(sizeof(GpuVertex) == 32);

    void update_buffers();
};

void BaseMeshRenderCache::create_buffers(DWORD fvf)
{
    // int num_verts = 0;
    // int num_inds = 0;
    // for (int i = 0; i < mesh_->num_chunks; ++i) {
    //     auto& chunk = mesh_->chunks[i];

    //     int num_double_sided_faces = 0;
    //     for (int face_index = 0; face_index < chunk.num_faces; ++face_index) {
    //         auto& face = chunk.faces[face_index];
    //         if (face.flags & VIF_FACE_DOUBLE_SIDED) {
    //             ++num_double_sided_faces;
    //         }
    //     }

    //     if (num_double_sided_faces > 0) {
    //         xlog::warn("Double sided faces: %d", num_double_sided_faces);
    //     }

    //     Batch b;
    //     b.num_verts = chunk.num_vecs;
    //     b.num_tris = chunk.num_faces + num_double_sided_faces;
    //     b.min_index = num_verts;
    //     b.start_index = num_inds;
    //     b.texture_index = chunk.texture_idx;
    //     b.mode = chunk.mode;
    //     batches_.push_back(b);

    //     num_verts += chunk.num_vecs;
    //     num_inds += (chunk.num_faces + num_double_sided_faces) * 3;
    // }
    // xlog::warn("Creating mesh render buffer - verts %d inds %d", num_verts, num_inds);

    // HRESULT hr;
    // DWORD usage = D3DUSAGE_WRITEONLY;
    // hr = gr::d3d::device->CreateVertexBuffer(num_verts * vertex_stride_, usage, fvf, D3DPOOL_MANAGED, &vb_);
    // check_hr(hr, "CreateVertexBuffer");
    // hr = gr::d3d::device->CreateIndexBuffer(num_inds * sizeof(ushort), usage, D3DFMT_INDEX16, D3DPOOL_MANAGED, &ib_);
    // check_hr(hr, "CreateIndexBuffer");
}

void BaseMeshRenderCache::render(const MeshRenderParams& params)
{
    // int* tex_handles = mesh_->tex_handles;
    // if (params.alt_tex) {
    //     tex_handles = params.alt_tex;
    // }
    // //xlog::warn("render mesh flags %x", params.flags);
    // HRESULT hr;
    // hr = gr::d3d::device->SetStreamSource(0, vb_, vertex_stride_);
    // check_hr(hr, "SetStreamSource");
    // hr = gr::d3d::device->SetIndices(ib_, 0);
    // check_hr(hr, "SetIndices");
    // for (auto& b : batches_) {
    //     int texture = tex_handles[b.texture_index];
    //     gr::d3d::set_state_and_texture(b.mode, texture, -1);
    //     gr::d3d::device->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, b.min_index, b.num_verts, b.start_index, b.num_tris);
    // }
    // TODO: params.alpha
}

MeshRenderCache::MeshRenderCache(VifMesh* mesh) :
    BaseMeshRenderCache(mesh, sizeof(GpuVertex))
{
    create_buffers(fvf);
    update_buffers();
}

void MeshRenderCache::update_buffers()
{
    // batches_.reserve(mesh_->num_chunks);

    // HRESULT hr;
    // GpuVertex* vb_data;
    // ushort* ib_data;
    // hr = vb_->Lock(0, 0, reinterpret_cast<BYTE**>(&vb_data), 0);
    // check_hr(hr, "Lock");
    // hr = ib_->Lock(0, 0, reinterpret_cast<BYTE**>(&ib_data), 0);
    // check_hr(hr, "Lock");

    // int vb_pos = 0;
    // int ib_pos = 0;

    // for (int chunk_index = 0; chunk_index < mesh_->num_chunks; ++chunk_index) {
    //     auto& chunk = mesh_->chunks[chunk_index];
    //     int chunk_start_index = vb_pos;
    //     for (int vert_index = 0; vert_index < chunk.num_vecs; ++vert_index) {
    //         auto& d3d_vert = vb_data[vb_pos++];
    //         d3d_vert.x = chunk.vecs[vert_index].x;
    //         d3d_vert.y = chunk.vecs[vert_index].y;
    //         d3d_vert.z = chunk.vecs[vert_index].z;
    //         d3d_vert.u0 = chunk.uvs[vert_index].x;
    //         d3d_vert.v0 = chunk.uvs[vert_index].y;
    //         d3d_vert.diffuse = 0xFFFFFFFF;
    //     }
    //     for (int face_index = 0; face_index < chunk.num_faces; ++face_index) {
    //         auto& face = chunk.faces[face_index];
    //         ib_data[ib_pos++] = chunk_start_index + face.vindex1;
    //         ib_data[ib_pos++] = chunk_start_index + face.vindex2;
    //         ib_data[ib_pos++] = chunk_start_index + face.vindex3;
    //         if (face.flags & VIF_FACE_DOUBLE_SIDED) {
    //             ib_data[ib_pos++] = chunk_start_index + face.vindex1;
    //             ib_data[ib_pos++] = chunk_start_index + face.vindex3;
    //             ib_data[ib_pos++] = chunk_start_index + face.vindex2;
    //         }
    //     }
    // }

    // hr = vb_->Unlock();
    // check_hr(hr, "Unlock");
    // hr = ib_->Unlock();
    // check_hr(hr, "Unlock");
}

CharacterMeshRenderCache::CharacterMeshRenderCache(VifMesh* mesh) :
    BaseMeshRenderCache(mesh, sizeof(GpuVertex))
{
    create_buffers(fvf);
    update_buffers();
}

void CharacterMeshRenderCache::update_buffers()
{
    // batches_.reserve(mesh_->num_chunks);

    // HRESULT hr;
    // GpuVertex* vb_data;
    // ushort* ib_data;
    // hr = vb_->Lock(0, 0, reinterpret_cast<BYTE**>(&vb_data), 0);
    // check_hr(hr, "Lock");
    // hr = ib_->Lock(0, 0, reinterpret_cast<BYTE**>(&ib_data), 0);
    // check_hr(hr, "Lock");

    // int vb_pos = 0;
    // int ib_pos = 0;

    // for (int chunk_index = 0; chunk_index < mesh_->num_chunks; ++chunk_index) {
    //     auto& chunk = mesh_->chunks[chunk_index];
    //     int chunk_start_index = vb_pos;
    //     for (int vert_index = 0; vert_index < chunk.num_vecs; ++vert_index) {
    //         auto& d3d_vert = vb_data[vb_pos++];
    //         d3d_vert.x = chunk.vecs[vert_index].x;
    //         d3d_vert.y = chunk.vecs[vert_index].y;
    //         d3d_vert.z = chunk.vecs[vert_index].z;
    //         if (chunk.wi) {
    //             d3d_vert.matrix_indices = 0;
    //             for (int i = 0; i < 4; ++i) {
    //                 d3d_vert.weight[i] = chunk.wi[vert_index].weights[i] / 255.0f;
    //                 d3d_vert.matrix_indices |= chunk.wi[vert_index].indices[i] << (i * 8);
    //             }
    //         }
    //         d3d_vert.matrix_indices = 0;
    //         d3d_vert.u0 = chunk.uvs[vert_index].x;
    //         d3d_vert.v0 = chunk.uvs[vert_index].y;
    //         d3d_vert.diffuse = 0xFFFFFFFF;
    //     }
    //     for (int face_index = 0; face_index < chunk.num_faces; ++face_index) {
    //         auto& face = chunk.faces[face_index];
    //         ib_data[ib_pos++] = chunk_start_index + face.vindex1;
    //         ib_data[ib_pos++] = chunk_start_index + face.vindex2;
    //         ib_data[ib_pos++] = chunk_start_index + face.vindex3;
    //         if (face.flags & VIF_FACE_DOUBLE_SIDED) {
    //             ib_data[ib_pos++] = chunk_start_index + face.vindex1;
    //             ib_data[ib_pos++] = chunk_start_index + face.vindex3;
    //             ib_data[ib_pos++] = chunk_start_index + face.vindex2;
    //         }
    //     }
    // }

    // hr = vb_->Unlock();
    // check_hr(hr, "Unlock");
    // hr = ib_->Unlock();
    // check_hr(hr, "Unlock");
}

// static D3DMATRIX build_d3d_matrix(const Matrix43& mat)
// {
//     D3DMATRIX d3d_mat;
//     d3d_mat._11 = mat.orient.rvec.x;
//     d3d_mat._12 = mat.orient.uvec.x;
//     d3d_mat._13 = mat.orient.fvec.x;
//     d3d_mat._14 = 0.0f;
//     d3d_mat._21 = mat.orient.rvec.y;
//     d3d_mat._22 = mat.orient.uvec.y;
//     d3d_mat._23 = mat.orient.fvec.y;
//     d3d_mat._24 = 0.0f;
//     d3d_mat._31 = mat.orient.rvec.z;
//     d3d_mat._32 = mat.orient.uvec.z;
//     d3d_mat._33 = mat.orient.fvec.z;
//     d3d_mat._34 = 0.0f;
//     d3d_mat._41 = mat.origin.x;
//     d3d_mat._42 = mat.origin.y;
//     d3d_mat._43 = mat.origin.z;
//     d3d_mat._44 = 1.0f;
//     return d3d_mat;
// }

void CharacterMeshRenderCache::render(const Vector3& pos, const Matrix3& orient, const CharacterInstance* ci, const MeshRenderParams& params)
{
    // Matrix43 world_mat = {orient, pos};
    // gr::d3d::device->SetRenderState(D3DRS_VERTEXBLEND, D3DVBF_3WEIGHTS);
    // gr::d3d::device->SetRenderState(D3DRS_INDEXEDVERTEXBLENDENABLE, TRUE);
    // for (int i = 0; i < ci->base_character->num_bones; ++i) {
    //     auto bone_mat = ci->bone_transforms_final[i];
    //     D3DMATRIX d3d_bone_world_mat = build_d3d_matrix(bone_mat * world_mat);
    //     gr::d3d::device->SetTransform(D3DTS_WORLDMATRIX(i), &d3d_bone_world_mat);
    // }
    // BaseMeshRenderCache::render(params);
    // gr::d3d::device->SetRenderState(D3DRS_VERTEXBLEND, D3DVBF_DISABLE);
    // gr::d3d::device->SetRenderState(D3DRS_INDEXEDVERTEXBLENDENABLE, FALSE);
}

static std::vector<std::unique_ptr<BaseMeshRenderCache>> all_mesh_render_caches;

void gr_d3d_render_v3d_vif([[maybe_unused]] VifLodMesh *lod_mesh, VifMesh *mesh, const Vector3& pos, const Matrix3& orient, [[maybe_unused]] int lod_index, [[maybe_unused]] const MeshRenderParams& params)
{
    // gr::d3d::flush_buffers();
    // gr::d3d::in_optimized_drawing_proc = true;

    // HRESULT hr = gr::d3d::device->SetVertexShader(MeshRenderCache::fvf);
    // check_hr(hr, "SetVertexShader");
    // gr::d3d::device->SetRenderState(D3DRS_CLIPPING, TRUE);
    // gr::d3d::device->SetRenderState(D3DRS_CULLMODE, D3DCULL_CCW);
    // gr::d3d::device->SetRenderState(D3DRS_LIGHTING, FALSE);

    // gr_d3d_setup_proj_transform();
    // gr_d3d_setup_view_transform();
    // gr_d3d_set_world_transform(pos, orient.copy_transpose());

    // if ((mesh->flags & VIF_MESH_RENDER_CACHED) == 0) {
    //     all_mesh_render_caches.push_back(std::make_unique<MeshRenderCache>(mesh));
    //     mesh->render_cache = all_mesh_render_caches.back().get();;
    //     mesh->flags |= VIF_MESH_RENDER_CACHED;
    // }
    // mesh->render_cache->render(params);

    // // Restore state
    // hr = gr::d3d::device->SetStreamSource(0, gr::d3d::vertex_buffer, 0x28);
    // check_hr(hr, "SetStreamSource");
    // hr = gr::d3d::device->SetIndices(gr::d3d::index_buffer, 0);
    // check_hr(hr, "SetIndices");
    // hr = gr::d3d::device->SetVertexShader(D3DFVF_XYZRHW|D3DFVF_DIFFUSE|D3DFVF_TEX2|D3DFVF_SPECULAR);
    // check_hr(hr, "SetVertexShader");
    // gr::d3d::device->SetRenderState(D3DRS_CLIPPING, FALSE);
    // gr::d3d::device->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
    // gr::d3d::in_optimized_drawing_proc = false;
}

void gr_d3d_render_character_vif([[maybe_unused]] VifLodMesh *lod_mesh, VifMesh *mesh, const Vector3& pos, const Matrix3& orient, const CharacterInstance *ci, [[maybe_unused]] int lod_index, [[maybe_unused]] const MeshRenderParams& params)
{
    // gr::d3d::flush_buffers();
    // gr::d3d::in_optimized_drawing_proc = true;

    // HRESULT hr = gr::d3d::device->SetVertexShader(CharacterMeshRenderCache::fvf);
    // check_hr(hr, "SetVertexShader");
    // gr::d3d::device->SetRenderState(D3DRS_CLIPPING, TRUE);
    // gr::d3d::device->SetRenderState(D3DRS_CULLMODE, D3DCULL_CCW);
    // gr::d3d::device->SetRenderState(D3DRS_LIGHTING, FALSE);

    // gr_d3d_setup_proj_transform();
    // gr_d3d_setup_view_transform();
    // gr_d3d_set_world_transform(pos, orient.copy_transpose());

    // if ((mesh->flags & VIF_MESH_RENDER_CACHED) == 0) {
    //     all_mesh_render_caches.emplace_back(static_cast<BaseMeshRenderCache*>(new CharacterMeshRenderCache(mesh)));
    //     mesh->render_cache = all_mesh_render_caches.back().get();
    //     mesh->flags |= VIF_MESH_RENDER_CACHED;
    // }
    // static_cast<CharacterMeshRenderCache*>(mesh->render_cache)->render(pos, orient, ci, params);

    // // Restore state
    // hr = gr::d3d::device->SetStreamSource(0, gr::d3d::vertex_buffer, 0x28);
    // check_hr(hr, "SetStreamSource");
    // hr = gr::d3d::device->SetIndices(gr::d3d::index_buffer, 0);
    // check_hr(hr, "SetIndices");
    // hr = gr::d3d::device->SetVertexShader(D3DFVF_XYZRHW|D3DFVF_DIFFUSE|D3DFVF_TEX2|D3DFVF_SPECULAR);
    // check_hr(hr, "SetVertexShader");
    // gr::d3d::device->SetRenderState(D3DRS_CLIPPING, FALSE);
    // gr::d3d::device->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
    // gr::d3d::in_optimized_drawing_proc = false;
}

static FunHook gr_d3d_render_v3d_vif_hook{0x0052DE10, gr_d3d_render_v3d_vif};
static FunHook gr_d3d_render_character_vif_hook{0x0052E9E0, gr_d3d_render_character_vif};

void gr_d3d_mesh_apply_patch()
{
    gr_d3d_render_v3d_vif_hook.install();
    // Wine doesnt support indexed blend matrices... See https://bugs.winehq.org/show_bug.cgi?id=39057
    //gr_d3d_render_character_vif_hook.install();
    write_mem<int8_t>(0x00569884 + 1, sizeof(VifMesh));
    write_mem<int8_t>(0x00569732 + 1, sizeof(VifLodMesh));
}

void gr_d3d_mesh_init()
{
    // xlog::warn("D3D MaxVertexBlendMatrices %lu", gr::d3d::device_caps.MaxVertexBlendMatrices);
    // xlog::warn("D3D MaxVertexBlendMatrixIndex %lu", gr::d3d::device_caps.MaxVertexBlendMatrixIndex);
}

void gr_d3d_mesh_close()
{
    for (auto& cache : all_mesh_render_caches) {
        cache->get_mesh()->flags &= ~VIF_MESH_RENDER_CACHED;
    }
    all_mesh_render_caches.clear();
}
