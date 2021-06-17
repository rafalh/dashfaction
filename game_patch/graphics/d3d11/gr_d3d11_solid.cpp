#define NO_D3D8
#undef NDEBUG

#include <windows.h>
#include <cassert>
#include <vector>
#include <unordered_map>
#include <map>
#include <memory>
#include <patch_common/ComPtr.h>
#include <xlog/xlog.h>
#include "../../rf/geometry.h"
#include "../../rf/gr/gr.h"
#include "../../rf/gr/gr_direct3d.h"
#include "../../rf/level.h"
#include "../../rf/os/timer.h"
#include "../../os/console.h"
#include "gr_d3d11.h"
#include "gr_d3d11_solid.h"
#include "gr_d3d11_context.h"

using namespace rf;

namespace df::gr::d3d11
{
    class RoomRenderCache;
    class GRenderCache;

    static auto& decals_enabled = addr_as_ref<bool>(0x005A4458);
    static auto& gr_decal_self_illuminated_mode = addr_as_ref<gr::Mode>(0x01818340);
    static auto& gr_decal_tiling_u_mode = addr_as_ref<gr::Mode>(0x0180831C);
    static auto& gr_decal_tiling_v_mode = addr_as_ref<gr::Mode>(0x0181833C);
    static auto& gr_decal_mode = addr_as_ref<gr::Mode>(0x01808318);
    static auto& gr_solid_mode = addr_as_ref<gr::Mode>(0x01808328);
    static auto& gr_solid_alpha_mode = addr_as_ref<gr::Mode>(0x0180832C);
    static auto& geo_cache_rooms = addr_as_ref<GRoom*[256]>(0x01375DB4);
    static auto& geo_cache_num_rooms = addr_as_ref<int>(0x013761B8);

    static auto& set_currently_rendered_room = addr_as_ref<void (GRoom *room)>(0x004D3350);

    static gr::Mode sky_room_mode{
        gr::TEXTURE_SOURCE_WRAP,
        gr::COLOR_SOURCE_TEXTURE,
        gr::ALPHA_SOURCE_TEXTURE,
        gr::ALPHA_BLEND_ALPHA,
        gr::ZBUFFER_TYPE_FULL,
        gr::FOG_NOT_ALLOWED,
    };

    static gr::Mode alpha_detail_fullbright_mode{
        gr::TEXTURE_SOURCE_WRAP,
        gr::COLOR_SOURCE_TEXTURE,
        gr::ALPHA_SOURCE_TEXTURE,
        gr::ALPHA_BLEND_ALPHA,
        gr::ZBUFFER_TYPE_FULL_ALPHA_TEST,
        gr::FOG_ALLOWED,
    };

    inline bool should_render_face(GFace* face)
    {
        return face->attributes.portal_id == 0 && !(face->attributes.flags & (FACE_INVISIBLE|FACE_SHOW_SKY));
    }

    static gr::Mode determine_decal_mode(GDecal* decal)
    {
        if (decal->flags & DF_SELF_ILLUMINATED) {
            return gr_decal_self_illuminated_mode;
        }
        else if (decal->flags & DF_TILING_U) {
            return gr_decal_tiling_u_mode;
        }
        else if (decal->flags & DF_TILING_V) {
            return gr_decal_tiling_v_mode;
        }
        else {
            return gr_decal_mode;
        }
    }

    void link_faces_to_texture_movers(GSolid* solid)
    {
        // Note: this is normally done by geo_cache_prepare
        for (GFace& face : solid->face_list) {
            face.attributes.texture_mover = nullptr;
        }
        for (GTextureMover* texture_mover : solid->texture_movers) {
            for (GFace* face : texture_mover->faces) {
                face->attributes.texture_mover = texture_mover;
            }
        }
    }

    class GRenderCacheBuilder
    {
    private:
        using FaceBatchKey = std::tuple<FaceRenderType, int, int, float, float>;
        using DecalPolyBatchKey = std::tuple<int, int, gr::Mode>;

        int num_verts_ = 0;
        int num_inds_ = 0;
        std::map<FaceBatchKey, std::vector<GFace*>> batched_faces_;
        std::map<DecalPolyBatchKey, std::vector<DecalPoly*>> batched_decal_polys_;

    public:
        void add_solid(GSolid* solid);
        void add_room(GRoom* room, GSolid* solid);
        void add_face(GFace* face, GSolid* solid);
        GRenderCache build(gr::Mode mode, ID3D11Device* device);

        int get_num_verts() const
        {
            return num_verts_;
        }

        int get_num_inds() const
        {
            return num_inds_;
        }

        int get_num_batches() const
        {
            return batched_faces_.size() + batched_decal_polys_.size();
        }

        friend class GRenderCache;
    };

    GRenderCache::GRenderCache(const GRenderCacheBuilder& builder, gr::Mode mode, ID3D11Device* device)
    {
        if (builder.num_verts_ == 0 || builder.num_inds_ == 0) {
            return;
        }

        std::vector<GpuVertex> vb_data;
        std::vector<ushort> ib_data;
        vb_data.reserve(builder.num_verts_);
        ib_data.reserve(builder.num_inds_);
        opaque_batches_.reserve(builder.batched_faces_.size());

        for (auto& e : builder.batched_faces_) {
            auto& key = e.first;
            auto& faces = e.second;
            FaceRenderType render_type = std::get<0>(key);
            Batch& batch = get_batches(render_type).emplace_back();
            batch.start_index = ib_data.size();
            batch.texture_1 = std::get<1>(key);
            batch.texture_2 = std::get<2>(key);
            batch.u_pan_speed = std::get<3>(key);
            batch.v_pan_speed = std::get<4>(key);
            if (render_type == FaceRenderType::opaque) {
                batch.mode = mode;
            }
            else if (batch.texture_2 != -1) {
                batch.mode = gr_solid_alpha_mode;
            }
            else {
                batch.mode = alpha_detail_fullbright_mode;
            }
            for (GFace* face : faces) {
                auto fvert = face->edge_loop;
                auto face_start_index = vb_data.size();
                int fvert_index = 0;
                while (fvert) {
                    auto& gpu_vert = vb_data.emplace_back();
                    gpu_vert.x = fvert->vertex->pos.x;
                    gpu_vert.y = fvert->vertex->pos.y;
                    gpu_vert.z = fvert->vertex->pos.z;
                    gpu_vert.u0 = fvert->texture_u;
                    gpu_vert.v0 = fvert->texture_v;
                    gpu_vert.u1 = fvert->lightmap_u;
                    gpu_vert.v1 = fvert->lightmap_v;
                    gpu_vert.diffuse = 0xFFFFFFFF;

                    if (fvert_index >= 2) {
                        ib_data.emplace_back(face_start_index);
                        ib_data.emplace_back(face_start_index + fvert_index - 1);
                        ib_data.emplace_back(face_start_index + fvert_index);
                    }
                    ++fvert_index;

                    fvert = fvert->next;
                    if (fvert == face->edge_loop) {
                        break;
                    }
                }
            }
            batch.num_indices = ib_data.size() - batch.start_index;
        }
        for (auto& e : builder.batched_decal_polys_) {
            auto& key = e.first;
            auto& dps = e.second;
            Batch& batch = opaque_batches_.emplace_back();
            batch.start_index = ib_data.size();
            batch.texture_1 = std::get<0>(key);
            batch.texture_2 = std::get<1>(key);
            batch.mode = std::get<2>(key);
            for (DecalPoly* dp : dps) {
                auto face = dp->face;
                auto fvert = face->edge_loop;
                auto face_start_index = vb_data.size();
                int fvert_index = 0;
                while (fvert) {
                    auto& gpu_vert = vb_data.emplace_back();
                    gpu_vert.x = fvert->vertex->pos.x;
                    gpu_vert.y = fvert->vertex->pos.y;
                    gpu_vert.z = fvert->vertex->pos.z;
                    gpu_vert.u0 = dp->uvs[fvert_index].x;
                    gpu_vert.v0 = dp->uvs[fvert_index].y;
                    gpu_vert.u1 = fvert->lightmap_u;
                    gpu_vert.v1 = fvert->lightmap_v;
                    gpu_vert.diffuse = 0xFFFFFFFF;

                    if (fvert_index >= 2) {
                        ib_data.emplace_back(face_start_index);
                        ib_data.emplace_back(face_start_index + fvert_index - 1);
                        ib_data.emplace_back(face_start_index + fvert_index);
                    }
                    ++fvert_index;

                    fvert = fvert->next;
                    if (fvert == face->edge_loop) {
                        break;
                    }
                }
            }
            batch.num_indices = ib_data.size() - batch.start_index;
        }

        CD3D11_BUFFER_DESC vb_desc{
            sizeof(GpuVertex) * builder.num_verts_,
            D3D11_BIND_VERTEX_BUFFER,
            D3D11_USAGE_IMMUTABLE,
        };
        D3D11_SUBRESOURCE_DATA vb_subres_data{vb_data.data(), 0, 0};
        HRESULT hr = device->CreateBuffer(&vb_desc, &vb_subres_data, &vb_);
        check_hr(hr, "CreateBuffer");

        CD3D11_BUFFER_DESC ib_desc{
            sizeof(rf::ushort) * builder.num_inds_,
            D3D11_BIND_INDEX_BUFFER,
            D3D11_USAGE_IMMUTABLE,
        };
        D3D11_SUBRESOURCE_DATA ib_subres_data{ib_data.data(), 0, 0};
        hr = device->CreateBuffer(&ib_desc, &ib_subres_data, &ib_);
        check_hr(hr, "CreateBuffer");

        xlog::warn("created buffers");
    }

    static GrMatrix3x3 build_uv_pan_matrix(float u_pan_speed, float v_pan_speed)
    {
        float delta_time = timer_get(1000) * 0.001f; // FIXME: paused game..
        return build_texture_matrix(u_pan_speed * delta_time, v_pan_speed * delta_time);
    }

    void GRenderCache::render(FaceRenderType what, RenderContext& context)
    {
        auto& batches = what == FaceRenderType::alpha
            ? alpha_batches_
            : (what == FaceRenderType::liquid ? liquid_batches_ : opaque_batches_);

        if (batches.empty()) {
            return;
        }

        context.bind_default_shaders();
        context.set_vertex_buffer(vb_, sizeof(GpuVertex));
        context.set_index_buffer(ib_);
        context.set_primitive_topology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        for (Batch& b : batches) {
            context.set_mode_and_textures(b.mode, b.texture_1, b.texture_2);
            GrMatrix3x3 tex_transform = build_uv_pan_matrix(b.u_pan_speed, b.v_pan_speed);
            context.set_texture_transform(tex_transform);
            //xlog::warn("DrawIndexed %d %d", b.num_indices, b.start_index);
            context.device_context()->DrawIndexed(b.num_indices, b.start_index, 0);
        }
    }

    void GRenderCacheBuilder::add_solid(GSolid* solid)
    {
        for (GFace& face : solid->face_list) {
            add_face(&face, solid);
        }
    }

    void GRenderCacheBuilder::add_room(GRoom* room, GSolid* solid)
    {
        link_faces_to_texture_movers(solid);
        for (GFace& face : room->face_list) {
            add_face(&face, solid);
        }
    }

    FaceRenderType determine_face_render_type(GFace* face)
    {
        if (face->attributes.flags & FACE_LIQUID) {
            return FaceRenderType::liquid;
        }
        if (face->attributes.flags & FACE_SEE_THRU) {
            return FaceRenderType::alpha;
        }
        return FaceRenderType::opaque;
    }

    void GRenderCacheBuilder::add_face(GFace* face, GSolid* solid)
    {
        if (!should_render_face(face)) {
            return;
        }
        FaceRenderType render_type = determine_face_render_type(face);
        int face_tex = face->attributes.bitmap_id;
        int lightmap_tex = -1;
        if (face->attributes.surface_index >= 0) {
            GSurface* surface = solid->surfaces[face->attributes.surface_index];
            lightmap_tex = surface->lightmap->bm_handle;
        }
        auto texture_mover = face->attributes.texture_mover;
        float u_pan_speed = texture_mover ? texture_mover->u_pan_speed : 0.0f;
        float v_pan_speed = texture_mover ? texture_mover->v_pan_speed : 0.0f;
        auto key = std::make_tuple(render_type, face_tex, lightmap_tex, u_pan_speed, v_pan_speed);
        batched_faces_[key].push_back(face);
        auto fvert = face->edge_loop;
        int num_fverts = 0;
        while (fvert) {
            ++num_fverts;
            fvert = fvert->next;
            if (fvert == face->edge_loop) {
                break;
            }
        }
        DecalPoly* dp = face->decal_list;
        int num_dp = 0;
        while (dp) {
            if (dp->my_decal->flags & DF_LEVEL_DECAL) {
                auto mode = determine_decal_mode(dp->my_decal);
                auto dp_key = std::make_tuple(dp->my_decal->bitmap_id, lightmap_tex, mode);
                batched_decal_polys_[dp_key].push_back(dp);
                ++num_dp;
            }
            dp = dp->next_for_face;
        }
        num_verts_ += (1 + num_dp) * num_fverts;
        num_inds_ += (1 + num_dp) * (num_fverts - 2) * 3;
    }

    GRenderCache GRenderCacheBuilder::build(gr::Mode mode, ID3D11Device* device)
    {
        return GRenderCache(*this, mode, device);
    }

    inline GRoom* RoomRenderCache::room() const
    {
        return room_;
    }

    inline bool RoomRenderCache::invalid() const
    {
        static_assert(offsetof(RoomRenderCache, state_) == 0x20, "Bad state_ offset");
        // if (state_ != 0) {
        //     xlog::warn("room %d state %d", room_->room_index, state_);
        // }
        return state_ == 2;
    }

    RoomRenderCache::RoomRenderCache(GSolid* solid, GRoom* room, ID3D11Device* device) :
        room_(room), solid_(solid)
    {
        update(device);
    }

    void RoomRenderCache::update(ID3D11Device* device)
    {
        GRenderCacheBuilder builder;
        builder.add_room(room_, solid_);

        if (builder.get_num_batches() == 0) {
            state_ = 0;
            xlog::warn("Skipping empty room %d", room_->room_index);
            return;
        }

        xlog::warn("Creating render cache for room %d - verts %d inds %d batches %d", room_->room_index,
            builder.get_num_verts(), builder.get_num_inds(), builder.get_num_batches());

        gr::Mode mode = room_->is_sky ? sky_room_mode : gr_solid_mode;
        cache_ = std::optional{builder.build(mode, device)};

        state_ = 0;
    }

    void RoomRenderCache::render(FaceRenderType render_type, ID3D11Device* device, RenderContext& context)
    {
        if (invalid()) {
            xlog::warn("Room render cache invalidated!");
            cache_.reset();
            update(device);
        }

        if (cache_) {
            cache_.value().render(render_type, context);
        }
    }

    static gr::Mode dynamic_decal_mode{
        gr::TEXTURE_SOURCE_CLAMP,
        gr::COLOR_SOURCE_VERTEX,
        gr::ALPHA_SOURCE_VERTEX_TIMES_TEXTURE,
        gr::ALPHA_BLEND_ALPHA,
        gr::ZBUFFER_TYPE_READ,
        gr::FOG_ALLOWED,
    };

    void SolidRenderer::render_dynamic_decal(GDecal* decal, GRoom* room)
    {
        constexpr int MAX_DECAL_POLY_VERTICES = 25;
        Vector3 verts[MAX_DECAL_POLY_VERTICES];
        Vector2 uvs[MAX_DECAL_POLY_VERTICES];
        Color color{255, 255, 255, decal->alpha};

        auto dp = decal->poly_list;
        while (dp) {
            bool dp_room_matches = !room || dp->face->which_room == room || dp->face->which_room->room_to_render_with == room;
            if (dp_room_matches && should_render_face(dp->face) && dp->face->plane.distance_to_point(gr::eye_pos) > 0.0) {
                for (int i = 0; i < dp->nv; ++i) {
                    verts[i] = dp->verts[i].pos;
                    uvs[i] = dp->verts[i].uv;
                }
                gr::world_poly(decal->bitmap_id, dp->nv, verts, uvs, dynamic_decal_mode, color);
            }

            dp = dp->next;
            if (dp == decal->poly_list) {
                break;
            }
        }
    }

    void SolidRenderer::render_dynamic_decals(rf::GRoom** rooms, int num_rooms)
    {
        int num_rendered_decals = 0;
        // TODO: try using D3D11_RASTERIZER_DESC::DepthBias
        // gr::d3d::zbias = gr::matrix_scale.z * 0.025f;
        for (int i = 0; i < num_rooms; ++i) {
            auto room = rooms[i];
            if (gr::cull_bounding_box(room->bbox_min, room->bbox_max)) {
                continue;
            }
            for (auto decal : room->decals) {
                if (!(decal->flags & DF_LEVEL_DECAL) && !gr::cull_bounding_box(decal->bb_min, decal->bb_max)) {
                    render_dynamic_decal(decal, room);
                    ++num_rendered_decals;
                }
            }
        }
        // gr::d3d::zbias = 0.0f;
    }

    void SolidRenderer::render_room_faces(rf::GSolid* solid, rf::GRoom* room, FaceRenderType render_type)
    {
        auto cache = reinterpret_cast<RoomRenderCache*>(room->geo_cache);
        if (!cache) {
            xlog::warn("Creating render cache for room %d", room->room_index);
            room_cache_.push_back(std::make_unique<RoomRenderCache>(solid, room, device_));
            cache = room_cache_.back().get();
            room->geo_cache = reinterpret_cast<GCache*>(cache);
            geo_cache_rooms[geo_cache_num_rooms++] = room;
        }

        cache->render(render_type, device_, render_context_);
    }

    void SolidRenderer::render_detail([[maybe_unused]] rf::GSolid* solid, [[maybe_unused]] GRoom* room, [[maybe_unused]] bool alpha)
    {
        auto cache = reinterpret_cast<GRenderCache*>(room->geo_cache);
        if (!cache) {
            //xlog::warn("Creating render cache for detail room %d", room->room_index);
            GRenderCacheBuilder builder;
            builder.add_room(room, solid);
            detail_render_cache_.push_back(std::make_unique<GRenderCache>(builder.build(gr_solid_mode, device_)));
            cache = detail_render_cache_.back().get();
            room->geo_cache = reinterpret_cast<GCache*>(cache);
        }

        cache->render(alpha ? FaceRenderType::alpha : FaceRenderType::opaque, render_context_);
    }

    void SolidRenderer::clear_cache()
    {
        xlog::warn("Room render cache clear");
        for (auto& ptr : room_cache_) {
            ptr->room()->geo_cache = nullptr;
        }
        room_cache_.clear();
        mover_render_cache_.clear();
        detail_render_cache_.clear();
        geo_cache_num_rooms = 0;
    }

    void SolidRenderer::render_sky_room(GRoom *room)
    {
        //xlog::warn("Rendering skybox...");
        before_render(rf::zero_vector, rf::identity_matrix, true);
        render_room_faces(rf::level.geometry, room, FaceRenderType::opaque);
        after_render();
    }

    void SolidRenderer::render_movable_solid(GSolid* solid, const Vector3& pos, const Matrix3& orient)
    {
        if (!solid->field_370) {
            xlog::warn("Creating render cache for a mover");
            GRenderCacheBuilder cache_builder;
            link_faces_to_texture_movers(solid);
            cache_builder.add_solid(solid);
            GRenderCache cache = cache_builder.build(gr_solid_mode, device_);
            // TODO: make sure field_370 is not used by the game...
            mover_render_cache_.emplace_back(new GRenderCache{cache});
            solid->field_370 = reinterpret_cast<int>(mover_render_cache_.back().get());
        }
        auto cache = reinterpret_cast<GRenderCache*>(solid->field_370);
        before_render(pos, orient.copy_transpose(), false);
        cache->render(FaceRenderType::opaque, render_context_);
        after_render();
    }

    void SolidRenderer::render_alpha_detail(GRoom *room, GSolid *solid)
    {
        if (gr::screen.mode != gr::DIRECT3D) {
            return;
        }
        if (room->face_list.empty()) {
            // Happens when glass is killed
            return;
        }
        before_render(rf::zero_vector, rf::identity_matrix, false);
        render_detail(solid, room, true);
        after_render();
    }

    void SolidRenderer::render_room_liquid_surface(GSolid* solid, GRoom* room)
    {
        before_render(rf::zero_vector, rf::identity_matrix, false);
        render_room_faces(solid, room, FaceRenderType::liquid);
        after_render();
    }

    SolidRenderer::SolidRenderer(ComPtr<ID3D11Device> device, ComPtr<ID3D11DeviceContext> context, RenderContext& render_context) :
        device_{std::move(device)}, context_{std::move(context)}, render_context_(render_context)
    {
    }

    void SolidRenderer::render_solid(rf::GSolid* solid, rf::GRoom** rooms, int num_rooms)
    {
        before_render(rf::zero_vector, rf::identity_matrix, false);

        for (int i = 0; i < num_rooms; ++i) {
            auto room = rooms[i];
            set_currently_rendered_room(room);
            if (gr::cull_bounding_box(room->bbox_min, room->bbox_max)) {
                continue;
            }

            render_room_faces(solid, room, FaceRenderType::opaque);

            for (GRoom* subroom : room->detail_rooms) {
                if (subroom->room_to_render_with == room && !gr::cull_bounding_box(subroom->bbox_min, subroom->bbox_max)) {
                    render_detail(solid, subroom, false);
                }
            }
        }
        set_currently_rendered_room(nullptr);

        // Restore state
        after_render();



        // if (decals_enabled) {
        //     render_dynamic_decals(rooms, num_rooms);
        //     gr::d3d::flush_buffers();
        // }
    }

    void SolidRenderer::before_render(const rf::Vector3& pos, const rf::Matrix3& orient, bool is_skyroom)
    {
        CameraUniforms uniforms;
        uniforms.model_mat = build_model_matrix(pos, orient);
        uniforms.view_mat = is_skyroom ? build_sky_room_view_matrix() : build_camera_view_matrix();
        uniforms.proj_mat = build_proj_matrix();
        render_context_.update_camera_uniforms(uniforms);
    }

    void SolidRenderer::after_render()
    {
        CameraUniforms uniforms;
        uniforms.proj_mat = uniforms.view_mat = uniforms.model_mat = build_identity_matrix();
        render_context_.update_camera_uniforms(uniforms);
    }
}
