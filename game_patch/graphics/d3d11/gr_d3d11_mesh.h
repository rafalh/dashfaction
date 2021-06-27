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
        BaseMeshRenderCache(rf::VifMesh* mesh) :
            mesh_(mesh)
        {}

        rf::VifMesh* get_mesh() const
        {
            return mesh_;
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

        void draw(const rf::MeshRenderParams& params, RenderContext& render_context);
        const int* get_tex_handles(const rf::MeshRenderParams& params);

        rf::VifMesh* mesh_;
        std::vector<Batch> batches_;
        ComPtr<ID3D11Buffer> vb_;
        ComPtr<ID3D11Buffer> ib_;
    };

    class MeshRenderer
    {
    public:
        MeshRenderer(ComPtr<ID3D11Device> device, RenderContext& render_context);
        ~MeshRenderer();
        void render_v3d_vif(rf::VifMesh *mesh, const rf::Vector3& pos, const rf::Matrix3& orient, const rf::MeshRenderParams& params);
        void render_character_vif(rf::VifMesh *mesh, const rf::Vector3& pos, const rf::Matrix3& orient, const rf::CharacterInstance *ci, const rf::MeshRenderParams& params);
        void clear_vif_cache(rf::VifMesh *mesh);

    private:
        ComPtr<ID3D11Device> device_;
        RenderContext& render_context_;
        std::unordered_map<rf::VifMesh*, std::unique_ptr<BaseMeshRenderCache>> render_caches_;
    };
}
