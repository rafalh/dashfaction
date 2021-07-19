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
    class StateManager;
    class DynamicGeometryRenderer;
    class RenderContext;
    class GRenderCacheBuilder;
    class RoomRenderCache;
    class GRenderCache;

    enum class FaceRenderType { opaque, alpha, liquid };

    class SolidRenderer
    {
    public:
        SolidRenderer(ComPtr<ID3D11Device> device, ShaderManager& shader_manager, StateManager& state_manager, DynamicGeometryRenderer& dyn_geo_renderer, RenderContext& render_context);
        ~SolidRenderer();
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
        void render_alpha_detail_dynamic_decals(rf::GRoom* detail_room);
        void render_movable_solid_dynamic_decals(rf::GSolid* solid, const rf::Vector3& pos, const rf::Matrix3& orient);
        void before_render_decals();
        void after_render_decals();

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
