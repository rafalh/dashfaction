#pragma once

#include <vector>
#include <optional>
#include <memory>
#include <d3d11.h>
#include <patch_common/ComPtr.h>

namespace rf
{
    struct GRoom;
    struct GSolid;
    struct GDecal;
}

namespace df::gr::d3d11
{
    class RenderContext;
    class GRenderCacheBuilder;

    enum class FaceRenderType { opaque, alpha, liquid };

    class GRenderCache
    {
    public:
        GRenderCache(const GRenderCacheBuilder& builder, rf::gr::Mode mode, ID3D11Device* device);
        void render(FaceRenderType what, RenderContext& context);

    private:
        // struct GpuVertex
        // {
        //     float x;
        //     float y;
        //     float z;
        //     int diffuse;
        //     float u0;
        //     float v0;
        //     float u1;
        //     float v1;
        // };

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
        SolidRenderer(ComPtr<ID3D11Device> device, ComPtr<ID3D11DeviceContext> context, RenderContext& render_context);
        void render_solid(rf::GSolid* solid, rf::GRoom** rooms, int num_rooms);
        void render_movable_solid(rf::GSolid* solid, const rf::Vector3& pos, const rf::Matrix3& orient);
        void render_sky_room(rf::GRoom *room);
        void render_alpha_detail(rf::GRoom *room, rf::GSolid *solid);
        void render_room_liquid_surface(rf::GSolid* solid, rf::GRoom* room);
        void clear_cache();

    private:
        void before_render(const rf::Vector3& pos, const rf::Matrix3& orient, bool is_skyroom);
        void after_render();
        void render_room_faces(rf::GSolid* solid, rf::GRoom* room, FaceRenderType render_type);
        void render_detail(rf::GSolid* solid, rf::GRoom* room, bool alpha);
        void render_dynamic_decals(rf::GRoom** rooms, int num_rooms);
        void render_dynamic_decal(rf::GDecal* decal, rf::GRoom* room);

        ComPtr<ID3D11Device> device_;
        ComPtr<ID3D11DeviceContext> context_;
        RenderContext& render_context_;
        std::vector<std::unique_ptr<RoomRenderCache>> room_cache_;
        std::vector<std::unique_ptr<GRenderCache>> mover_render_cache_;
        std::vector<std::unique_ptr<GRenderCache>> detail_render_cache_;
    };
}
