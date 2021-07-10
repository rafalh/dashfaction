#pragma once

#include <vector>
#include <optional>
#include <memory>
#include <d3d11.h>
#include <patch_common/ComPtr.h>
#include "gr_d3d11_shader.h"

namespace rf
{
    struct GRoom;
    struct GSolid;
    struct GDecal;
}

namespace df::gr::d3d11
{
    class DynamicGeometryRenderer;
    class RenderContext;
    class GRenderCacheBuilder;

    enum class FaceRenderType { opaque, alpha, liquid, sky };

    class GRenderCache
    {
    public:
        GRenderCache(const GRenderCacheBuilder& builder, ID3D11Device* device);
        void render(FaceRenderType what, RenderContext& context);

    private:
        struct Batch
        {
            int start_index;
            int num_indices;
            int texture_1;
            int texture_2;
            float u_pan_speed;
            float v_pan_speed;
            rf::gr::Mode mode;
        };

        std::vector<Batch> opaque_batches_;
        std::vector<Batch> alpha_batches_;
        std::vector<Batch> liquid_batches_;
        std::vector<Batch> sky_batches_;
        ComPtr<ID3D11Buffer> vb_;
        ComPtr<ID3D11Buffer> ib_;

        std::vector<Batch>& get_batches(FaceRenderType render_type)
        {
            if (render_type == FaceRenderType::alpha) {
                return alpha_batches_;
            }
            else if (render_type == FaceRenderType::liquid) {
                return liquid_batches_;
            }
            else if (render_type == FaceRenderType::sky) {
                return sky_batches_;
            }
            else {
                return opaque_batches_;
            }
        }
    };

    class RoomRenderCache
    {
    public:
        RoomRenderCache(rf::GSolid* solid, rf::GRoom* room, ID3D11Device* device);
        void render(FaceRenderType render_type, ID3D11Device* device, RenderContext& context);
        rf::GRoom* room() const;

    private:
        char padding_[0x20];
        int state_ = 0; // modified by the game engine during geomod operation
        rf::GRoom* room_;
        rf::GSolid* solid_;
        std::optional<GRenderCache> cache_;

        void update(ID3D11Device* device);
        bool invalid() const;
    };

    class SolidRenderer
    {
    public:
        SolidRenderer(ComPtr<ID3D11Device> device, ShaderManager& shader_manager, DynamicGeometryRenderer& dyn_geo_renderer, RenderContext& render_context);
        void render_solid(rf::GSolid* solid, rf::GRoom** rooms, int num_rooms);
        void render_movable_solid(rf::GSolid* solid, const rf::Vector3& pos, const rf::Matrix3& orient);
        void render_sky_room(rf::GRoom *room);
        void render_alpha_detail(rf::GRoom *room, rf::GSolid *solid);
        void render_room_liquid_surface(rf::GSolid* solid, rf::GRoom* room);
        void clear_cache();

    private:
        void before_render(const rf::Vector3& pos, const rf::Matrix3& orient);
        void after_render();
        void render_room_faces(rf::GSolid* solid, rf::GRoom* room, FaceRenderType render_type);
        void render_detail(rf::GSolid* solid, rf::GRoom* room, bool alpha);
        void render_dynamic_decals(rf::GRoom** rooms, int num_rooms);
        void render_movable_solid_dynamic_decals(rf::GSolid* solid, const rf::Vector3& pos, const rf::Matrix3& orient);
        void render_dynamic_decal(rf::GDecal* decal, rf::GRoom* room);

        ComPtr<ID3D11Device> device_;
        ComPtr<ID3D11DeviceContext> context_;
        VertexShaderAndLayout vertex_shader_;
        ComPtr<ID3D11PixelShader> pixel_shader_;
        DynamicGeometryRenderer& dyn_geo_renderer_;
        RenderContext& render_context_;
        std::vector<std::unique_ptr<RoomRenderCache>> room_cache_;
        std::vector<std::unique_ptr<GRenderCache>> detail_render_cache_;
        std::unordered_map<rf::GSolid*, std::unique_ptr<GRenderCache>> mover_render_cache_;
    };
}
