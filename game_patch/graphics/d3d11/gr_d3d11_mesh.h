#pragma once

#include <unordered_map>
#include <d3d11.h>
#include <patch_common/ComPtr.h>

namespace rf
{
    struct VifLodMesh;
    struct VifMesh;
    struct MeshRenderParams;
    struct CharacterInstance;
}

namespace df::gr::d3d11
{
    class RenderContext;

    class BaseMeshRenderCache
    {
    public:
        virtual ~BaseMeshRenderCache() {}
        BaseMeshRenderCache(rf::VifLodMesh* lod_mesh) :
            lod_mesh_(lod_mesh)
        {}

        rf::VifLodMesh* get_lod_mesh() const
        {
            return lod_mesh_;
        }

    protected:
        struct Batch
        {
            int start_index;
            int num_indices;
            int texture_index;
            rf::gr::Mode mode;
            bool double_sided = false;
        };

        struct Mesh
        {
            std::vector<Batch> batches;
        };

        void draw(const rf::MeshRenderParams& params, int lod_index, RenderContext& render_context);
        const int* get_tex_handles(const rf::MeshRenderParams& params, int lod_index);

        rf::VifLodMesh* lod_mesh_;

        std::vector<Mesh> meshes_;
        ComPtr<ID3D11Buffer> vb_;
        ComPtr<ID3D11Buffer> ib_;
    };

    class MeshRenderer
    {
    public:
        MeshRenderer(ComPtr<ID3D11Device> device, RenderContext& render_context);
        ~MeshRenderer();
        void render_v3d_vif(rf::VifLodMesh *lod_mesh, rf::VifMesh *mesh, const rf::Vector3& pos, const rf::Matrix3& orient, const rf::MeshRenderParams& params);
        void render_character_vif(rf::VifLodMesh *lod_mesh, rf::VifMesh *mesh, const rf::Vector3& pos, const rf::Matrix3& orient, const rf::CharacterInstance *ci, const rf::MeshRenderParams& params);
        void clear_vif_cache(rf::VifLodMesh *lod_mesh);

    private:
        ComPtr<ID3D11Device> device_;
        RenderContext& render_context_;
        std::unordered_map<rf::VifLodMesh*, std::unique_ptr<BaseMeshRenderCache>> render_caches_;
    };
}
