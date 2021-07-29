#pragma once

#include <unordered_map>
#include <d3d11.h>
#include <patch_common/ComPtr.h>
#include "gr_d3d11_shader.h"

namespace rf
{
    struct VifLodMesh;
    struct VifMesh;
    struct MeshRenderParams;
    struct CharacterInstance;
}

namespace df::gr::d3d11
{
    class StateManager;
    class RenderContext;

    class BaseMeshRenderCache
    {
    public:
        struct Batch
        {
            Batch(int start_index, int num_indices, int base_vertex, int texture_index, rf::gr::Mode mode, bool double_sided) :
                start_index{start_index}, num_indices{num_indices}, base_vertex{base_vertex},
                texture_index{texture_index}, mode{mode}, double_sided{double_sided}
            {}

            int start_index;
            int num_indices;
            int base_vertex = 0;
            int texture_index;
            rf::gr::Mode mode;
            bool double_sided = false;
        };

        virtual ~BaseMeshRenderCache() {}
        BaseMeshRenderCache(rf::VifLodMesh* lod_mesh) :
            lod_mesh_(lod_mesh)
        {}

        const std::vector<Batch>& get_batches(int lod_level) const
        {
            return meshes_[lod_level].batches;
        }

    protected:
        struct Mesh
        {
            std::vector<Batch> batches;
        };

        rf::VifLodMesh* lod_mesh_;
        std::vector<Mesh> meshes_;
    };

    class MeshRenderer
    {
    public:
        MeshRenderer(ComPtr<ID3D11Device> device, ShaderManager& shader_manager, StateManager& state_manager, RenderContext& render_context);
        ~MeshRenderer();
        void render_v3d_vif(rf::VifLodMesh *lod_mesh, rf::VifMesh *mesh, const rf::Vector3& pos, const rf::Matrix3& orient, const rf::MeshRenderParams& params);
        void render_character_vif(rf::VifLodMesh *lod_mesh, rf::VifMesh *mesh, const rf::Vector3& pos, const rf::Matrix3& orient, const rf::CharacterInstance *ci, const rf::MeshRenderParams& params);
        void clear_vif_cache(rf::VifLodMesh *lod_mesh);
        void page_in_v3d_mesh(rf::VifLodMesh* lod_mesh);
        void page_in_character_mesh(rf::VifLodMesh* lod_mesh);

    private:
        void draw_cached_mesh(rf::VifLodMesh *lod_mesh, const BaseMeshRenderCache& render_cache, const rf::MeshRenderParams& params, int lod_index);

        ComPtr<ID3D11Device> device_;
        RenderContext& render_context_;
        std::unordered_map<rf::VifLodMesh*, std::unique_ptr<BaseMeshRenderCache>> render_caches_;
        VertexShaderAndLayout standard_vertex_shader_;
        VertexShaderAndLayout character_vertex_shader_;
        ComPtr<ID3D11PixelShader> pixel_shader_;
    };
}
