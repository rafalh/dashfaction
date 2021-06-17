#pragma once

#include <vector>
#include <d3d11.h>
#include <patch_common/ComPtr.h>

class D3D11RenderContext;
namespace rf
{
    struct VifLodMesh;
    struct VifMesh;
    struct MeshRenderParams;
    struct CharacterInstance;
}

class BaseMeshRenderCache
{
public:
    virtual ~BaseMeshRenderCache() {}
    BaseMeshRenderCache(rf::VifMesh* mesh) :
        mesh_(mesh)
    {}

    void render(const rf::MeshRenderParams& params, D3D11RenderContext& render_context);

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
    };

    rf::VifMesh* mesh_;
    std::vector<Batch> batches_;
    ComPtr<ID3D11Buffer> vb_;
    ComPtr<ID3D11Buffer> ib_;
};

class D3D11MeshRenderer
{
public:
    D3D11MeshRenderer(ComPtr<ID3D11Device> device, D3D11RenderContext& render_context);
    ~D3D11MeshRenderer();
    void render_v3d_vif(rf::VifMesh *mesh, const rf::Vector3& pos, const rf::Matrix3& orient, const rf::MeshRenderParams& params);
    void render_character_vif(rf::VifMesh *mesh, const rf::Vector3& pos, const rf::Matrix3& orient, const rf::CharacterInstance *ci, const rf::MeshRenderParams& params);

private:
    ComPtr<ID3D11Device> device_;
    D3D11RenderContext& render_context_;
    std::vector<std::unique_ptr<BaseMeshRenderCache>> render_caches_;
};
