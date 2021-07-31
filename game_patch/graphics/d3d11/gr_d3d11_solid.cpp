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
#include "../../rf/gr/gr_light.h"
#include "../../rf/gr/gr_direct3d.h"
#include "../../rf/level.h"
#include "../../rf/os/timer.h"
#include "../../os/console.h"
#include "gr_d3d11.h"
#include "gr_d3d11_solid.h"
#include "gr_d3d11_shader.h"
#include "gr_d3d11_context.h"
#include "gr_d3d11_dynamic_geometry.h"

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
    static auto& sky_room_center = addr_as_ref<Vector3>(0x0088FB10);
    static auto& sky_room_offset = addr_as_ref<rf::Vector3>(0x0087BB00);

    static auto& set_currently_rendered_room = addr_as_ref<void (GRoom *room)>(0x004D3350);

    static gr::Mode sky_room_opaque_mode{
        gr::TEXTURE_SOURCE_WRAP,
        gr::COLOR_SOURCE_TEXTURE,
        gr::ALPHA_SOURCE_TEXTURE,
        gr::ALPHA_BLEND_NONE,
        gr::ZBUFFER_TYPE_FULL,
        gr::FOG_NOT_ALLOWED,
    };

    static gr::Mode sky_room_alpha_mode{
        gr::TEXTURE_SOURCE_WRAP,
        gr::COLOR_SOURCE_TEXTURE,
        gr::ALPHA_SOURCE_TEXTURE,
        gr::ALPHA_BLEND_ALPHA,
        gr::ZBUFFER_TYPE_READ,
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

    static inline bool should_render_face(GFace* face)
    {
        return !face->attributes.is_portal() &&
            !face->attributes.is_invisible() &&
            !face->attributes.is_show_sky();
    }

    static inline gr::Mode determine_decal_mode(GDecal* decal)
    {
        if (decal->flags & DF_SELF_ILLUMINATED) {
            return gr_decal_self_illuminated_mode;
        }
        if (decal->flags & DF_TILING_U) {
            return gr_decal_tiling_u_mode;
        }
        if (decal->flags & DF_TILING_V) {
            return gr_decal_tiling_v_mode;
        }
        return gr_decal_mode;
    }

    static inline gr::Mode determine_face_mode(FaceRenderType render_type, bool has_lightmap, bool is_sky)
    {
        if (is_sky) {
            if (render_type == FaceRenderType::opaque) {
                return sky_room_opaque_mode;
            }
            return sky_room_alpha_mode;
        }
        if (render_type == FaceRenderType::opaque) {
            return gr_solid_mode;
        }
        if (render_type == FaceRenderType::alpha && has_lightmap) {
            return gr_solid_alpha_mode;
        }
        return alpha_detail_fullbright_mode;
    }

    template<int N>
    inline void link_faces_to_texture_movers(VList<GFace, N>& face_list, GSolid* solid)
    {
        // Note: this is normally done by geo_cache_prepare
        for (GFace& face : face_list) {
            face.attributes.texture_mover = nullptr;
        }
        for (GTextureMover* texture_mover : solid->texture_movers) {
            for (GFace* face : texture_mover->faces) {
                face->attributes.texture_mover = texture_mover;
            }
        }
    }

    class SolidGeometryBuffers
    {
    public:
        SolidGeometryBuffers(const std::vector<GpuVertex>& vb_data, const std::vector<ushort>& ib_data, ID3D11Device* device);

        void bind_buffers(RenderContext& render_context)
        {
            render_context.set_vertex_buffer(vertex_buffer_, sizeof(GpuVertex));
            render_context.set_index_buffer(index_buffer_);
        }

    private:
        ComPtr<ID3D11Buffer> vertex_buffer_;
        ComPtr<ID3D11Buffer> index_buffer_;
    };

    SolidGeometryBuffers::SolidGeometryBuffers(const std::vector<GpuVertex>& vb_data, const std::vector<ushort>& ib_data, ID3D11Device* device)
    {
        if (vb_data.empty() || ib_data.empty()) {
            return;
        }

        CD3D11_BUFFER_DESC vb_desc{
            sizeof(vb_data[0]) * vb_data.size(),
            D3D11_BIND_VERTEX_BUFFER,
            D3D11_USAGE_IMMUTABLE,
        };
        D3D11_SUBRESOURCE_DATA vb_subres_data{vb_data.data(), 0, 0};
        DF_GR_D3D11_CHECK_HR(
            device->CreateBuffer(&vb_desc, &vb_subres_data, &vertex_buffer_)
        );

        CD3D11_BUFFER_DESC ib_desc{
            sizeof(ib_data[0]) * ib_data.size(),
            D3D11_BIND_INDEX_BUFFER,
            D3D11_USAGE_IMMUTABLE,
        };
        D3D11_SUBRESOURCE_DATA ib_subres_data{ib_data.data(), 0, 0};
        DF_GR_D3D11_CHECK_HR(
            device->CreateBuffer(&ib_desc, &ib_subres_data, &index_buffer_)
        );
    }

    struct SolidBatch
    {
        SolidBatch(int start_index, int num_indices, int base_vertex, std::array<int, 2> textures, rf::gr::Mode mode) :
            start_index{start_index}, num_indices{num_indices}, base_vertex{base_vertex}, textures{textures}, mode{mode}
        {}

        int start_index;
        int num_indices;
        int base_vertex = 0;
        std::array<int, 2> textures;
        rf::gr::Mode mode;
    };

    class SolidBatches
    {
    public:
        std::vector<SolidBatch>& get_batches(FaceRenderType render_type)
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

    private:
        std::vector<SolidBatch> opaque_batches_;
        std::vector<SolidBatch> alpha_batches_;
        std::vector<SolidBatch> liquid_batches_;
    };

    class GRenderCache
    {
    public:
        GRenderCache(SolidBatches batches, SolidGeometryBuffers geometry_buffers) :
            batches_{batches}, geometry_buffers_{geometry_buffers}
        {}

        void render(FaceRenderType what, RenderContext& context);

    private:
        SolidBatches batches_;
        SolidGeometryBuffers geometry_buffers_;
    };

    void GRenderCache::render(FaceRenderType what, RenderContext& render_context)
    {
        auto& batches = batches_.get_batches(what);

        if (batches.empty()) {
            return;
        }

        geometry_buffers_.bind_buffers(render_context);
        for (SolidBatch& b : batches) {
            render_context.set_mode(b.mode);
            render_context.set_textures(b.textures[0], b.textures[1]);
            //xlog::warn("DrawIndexed %d %d", b.num_indices, b.start_index);
            render_context.draw_indexed(b.num_indices, b.start_index, b.base_vertex);
        }
    }

    class GRenderCacheBuilder
    {
    private:
        // render_type, texture_1, texture_2
        using FaceBatchKey = std::tuple<FaceRenderType, int, int>;
        // render_type, texture_1, texture_2, mode
        using DecalPolyBatchKey = std::tuple<FaceRenderType, int, int, gr::Mode>;

        int num_verts_ = 0;
        int num_inds_ = 0;
        std::map<FaceBatchKey, std::vector<GFace*>> batched_faces_;
        std::map<DecalPolyBatchKey, std::vector<DecalPoly*>> batched_decal_polys_;
        bool is_sky_ = false;

    public:
        void add_solid(GSolid* solid);
        void add_room(GRoom* room, GSolid* solid);
        void add_face(GFace* face, GSolid* solid);
        GRenderCache build(ID3D11Device* device);

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

    static Vector3 calculate_face_vertex_normal(GFaceVertex* fvert, GFace* face)
    {
        Vector3 normal = face->plane.normal;
        bool normalize = false;
        for (GFace* adj_face : fvert->vertex->adjacent_faces) {
            if (adj_face != face && (adj_face->attributes.group_id & face->attributes.group_id) != 0) {
                normal += adj_face->plane.normal;
                normalize = true;
            }
        }
        if (normalize) {
            normal.normalize();
        }
        return normal;
    }

    void GRenderCacheBuilder::add_solid(GSolid* solid)
    {
        link_faces_to_texture_movers(solid->face_list, solid);
        for (GFace& face : solid->face_list) {
            add_face(&face, solid);
        }
    }

    void GRenderCacheBuilder::add_room(GRoom* room, GSolid* solid)
    {
        link_faces_to_texture_movers(room->face_list, solid);
        for (GFace& face : room->face_list) {
            add_face(&face, solid);
        }
        if (room->is_sky) {
            is_sky_ = true;
            for (GRoom* detail_room : room->detail_rooms) {
                add_room(detail_room, solid);
            }
            // TODO: sort
        }
    }

    static inline FaceRenderType determine_face_render_type(GFace* face)
    {
        if (face->attributes.is_liquid()) {
            return FaceRenderType::liquid;
        }
        if (face->attributes.is_see_thru()) {
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
        FaceBatchKey key = std::make_tuple(render_type, face_tex, lightmap_tex);
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
                rf::gr::Mode mode = determine_decal_mode(dp->my_decal);
                std::array<int, 2> textures = normalize_texture_handles_for_mode(mode, {dp->my_decal->bitmap_id, lightmap_tex});
                DecalPolyBatchKey dp_key = std::make_tuple(render_type, textures[0], textures[1], mode);
                batched_decal_polys_[dp_key].push_back(dp);
                ++num_dp;
            }
            dp = dp->next_for_face;
        }
        num_verts_ += (1 + num_dp) * num_fverts;
        num_inds_ += (1 + num_dp) * (num_fverts - 2) * 3;
    }

    GRenderCache GRenderCacheBuilder::build(ID3D11Device* device)
    {
        SolidBatches batches;
        std::vector<GpuVertex> vb_data;
        std::vector<ushort> ib_data;
        vb_data.reserve(num_verts_);
        ib_data.reserve(num_inds_);

        for (auto& e : batched_faces_) {
            const GRenderCacheBuilder::FaceBatchKey& key = e.first;
            auto& faces = e.second;
            auto [render_type, texture_1, texture_2] = key;
            std::size_t start_index = ib_data.size();
            std::size_t base_vertex = vb_data.size();

            for (GFace* face : faces) {
                auto fvert = face->edge_loop;
                GTextureMover* texture_mover = face->attributes.texture_mover;
                float u_pan_speed = texture_mover ? texture_mover->u_pan_speed : 0.0f;
                float v_pan_speed = texture_mover ? texture_mover->v_pan_speed : 0.0f;
                auto face_start_index = static_cast<ushort>(vb_data.size() - base_vertex);
                int fvert_index = 0;
                while (fvert) {
                    auto& gpu_vert = vb_data.emplace_back();
                    gpu_vert.x = fvert->vertex->pos.x;
                    gpu_vert.y = fvert->vertex->pos.y;
                    gpu_vert.z = fvert->vertex->pos.z;
                    Vector3 normal = calculate_face_vertex_normal(fvert, face);
                    gpu_vert.norm = {normal.x, normal.y, normal.z};
                    gpu_vert.diffuse = 0xFFFFFFFF;
                    gpu_vert.u0 = fvert->texture_u;
                    gpu_vert.v0 = fvert->texture_v;
                    gpu_vert.u1 = fvert->lightmap_u;
                    gpu_vert.v1 = fvert->lightmap_v;
                    gpu_vert.u0_pan_speed = u_pan_speed;
                    gpu_vert.v0_pan_speed = v_pan_speed;

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
            std::size_t num_indices = ib_data.size() - start_index;
            std::array<int, 2> textures = {texture_1, texture_2};
            gr::Mode mode = determine_face_mode(render_type, texture_2 != -1, is_sky_);
            batches.get_batches(render_type).emplace_back(
                start_index, num_indices, base_vertex, textures, mode
            );
        }
        for (auto& e : batched_decal_polys_) {
            const GRenderCacheBuilder::DecalPolyBatchKey& key = e.first;
            auto& dps = e.second;
            auto [render_type, texture_1, texture_2, mode] = key;
            std::size_t start_index = ib_data.size();
            std::size_t base_vertex = vb_data.size();
            for (DecalPoly* dp : dps) {
                auto face = dp->face;
                auto fvert = face->edge_loop;
                auto face_start_index = static_cast<ushort>(vb_data.size() - base_vertex);
                int fvert_index = 0;
                while (fvert) {
                    auto& gpu_vert = vb_data.emplace_back();
                    gpu_vert.x = fvert->vertex->pos.x;
                    gpu_vert.y = fvert->vertex->pos.y;
                    gpu_vert.z = fvert->vertex->pos.z;
                    Vector3 normal = calculate_face_vertex_normal(fvert, face);
                    gpu_vert.norm = {normal.x, normal.y, normal.z};
                    gpu_vert.diffuse = 0xFFFFFFFF;
                    gpu_vert.u0 = dp->uvs[fvert_index].x;
                    gpu_vert.v0 = dp->uvs[fvert_index].y;
                    gpu_vert.u0_pan_speed = 0.0f;
                    gpu_vert.v0_pan_speed = 0.0f;
                    gpu_vert.u1 = fvert->lightmap_u;
                    gpu_vert.v1 = fvert->lightmap_v;

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
            int num_indices = ib_data.size() - start_index;
            std::array<int, 2> textures = {texture_1, texture_2};
            batches.get_batches(render_type).emplace_back(
                start_index, num_indices, base_vertex, textures, mode
            );
        }

        SolidGeometryBuffers geometry_buffers{vb_data, ib_data, device};
        return GRenderCache{batches, geometry_buffers};
    }

    class RoomRenderCache
    {
    public:
        RoomRenderCache(rf::GSolid* solid, rf::GRoom* room, ID3D11Device* device);
        ~RoomRenderCache() {}
        void render(FaceRenderType render_type, ID3D11Device* device, RenderContext& context);

        rf::GRoom* room() const
        {
            return room_;
        }

    private:
        char padding_[0x20];
        int state_ = 0; // modified by the game engine during geomod operation
        rf::GRoom* room_;
        rf::GSolid* solid_;
        std::optional<GRenderCache> cache_;

        void update(ID3D11Device* device);
        bool invalid() const;
    };

    inline bool RoomRenderCache::invalid() const
    {
        static_assert(offsetof(RoomRenderCache, state_) == 0x20, "Bad state_ offset");
        if (state_ != 0) {
            xlog::trace("room %d state %d", room_->room_index, state_);
        }
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
            xlog::debug("Skipping empty room %d", room_->room_index);
            return;
        }

        xlog::debug("Creating render cache for room %d - verts %d inds %d batches %d", room_->room_index,
            builder.get_num_verts(), builder.get_num_inds(), builder.get_num_batches());

        cache_ = std::optional{builder.build(device)};

        state_ = 0;
    }

    void RoomRenderCache::render(FaceRenderType render_type, ID3D11Device* device, RenderContext& context)
    {
        if (invalid()) {
            xlog::debug("Room %d render cache invalidated!", room_->room_index);
            cache_.reset();
            update(device);
        }

        if (cache_) {
            cache_.value().render(render_type, context);
        }
    }

    SolidRenderer::SolidRenderer(ComPtr<ID3D11Device> device, ShaderManager& shader_manager,
        [[maybe_unused]] StateManager& state_manager, DynamicGeometryRenderer& dyn_geo_renderer,
        RenderContext& render_context) :
        device_{std::move(device)}, context_{render_context.device_context()}, dyn_geo_renderer_{dyn_geo_renderer}, render_context_(render_context)
    {
        vertex_shader_ = shader_manager.get_vertex_shader(VertexShaderId::standard);
        pixel_shader_ = shader_manager.get_pixel_shader(PixelShaderId::standard);
    }

    SolidRenderer::~SolidRenderer()
    {}

    static gr::Mode dynamic_decal_mode{
        gr::TEXTURE_SOURCE_CLAMP,
        gr::COLOR_SOURCE_VERTEX,
        gr::ALPHA_SOURCE_VERTEX_TIMES_TEXTURE,
        gr::ALPHA_BLEND_ALPHA,
        gr::ZBUFFER_TYPE_READ,
        gr::FOG_ALLOWED,
    };

    static void render_face_dynamic_decals(GFace* face)
    {
        constexpr int max_decal_poly_vertices = 25;
        Vector3 verts[max_decal_poly_vertices];
        Vector2 uvs[max_decal_poly_vertices];

        auto dp = face->decal_list;
        while (dp) {
            GDecal* decal = dp->my_decal;
            if (!(decal->flags & DF_LEVEL_DECAL)) {
                int nv = dp->nv;
                for (int i = 0; i < nv; ++i) {
                    verts[i] = dp->verts[i].pos;
                    uvs[i] = dp->verts[i].uv;
                }
                Color color{255, 255, 255, decal->alpha};
                // TODO: lightmap_uv
                gr::world_poly(decal->bitmap_id, dp->nv, verts, uvs, dynamic_decal_mode, color);
            }
            dp = dp->next_for_face;
        }
    }

    void SolidRenderer::render_dynamic_decals(rf::GRoom** rooms, int num_rooms)
    {
        before_render_decals();

        for (int i = 0; i < num_rooms; ++i) {
            GRoom* room = rooms[i];
            for (GFace& face: room->face_list) {
                if (should_render_face(&face) && !face.attributes.is_see_thru()) {
                    render_face_dynamic_decals(&face);
                }
            }
            for (GRoom* detail_room : room->detail_rooms) {
                if (detail_room->room_to_render_with == room) {
                    for (GFace& face: detail_room->face_list) {
                        if (should_render_face(&face) && !face.attributes.is_see_thru()) {
                            render_face_dynamic_decals(&face);
                        }
                    }
                }
            }
        }
        after_render_decals();
    }

    void SolidRenderer::render_alpha_detail_dynamic_decals(rf::GRoom* detail_room)
    {
        before_render_decals();
        for (GFace& face: detail_room->face_list) {
            if (should_render_face(&face) && face.attributes.is_see_thru()) {
                render_face_dynamic_decals(&face);
            }
        }
        after_render_decals();
    }

    void SolidRenderer::render_movable_solid_dynamic_decals(rf::GSolid* solid, const rf::Vector3& pos, const rf::Matrix3& orient)
    {
        gr::start_instance(pos, orient);
        before_render_decals();
        for (GFace& face: solid->face_list) {
            if (should_render_face(&face)) {
                render_face_dynamic_decals(&face);
            }
        }
        after_render_decals();
        gr::stop_instance();
    }

    void SolidRenderer::before_render_decals()
    {
        constexpr int decal_zbias = 1000;
        render_context_.set_zbias(decal_zbias);
    }

    void SolidRenderer::after_render_decals()
    {
        dyn_geo_renderer_.flush();
        render_context_.set_zbias(0);
    }

    void SolidRenderer::render_room_faces(rf::GSolid* solid, rf::GRoom* room, FaceRenderType render_type)
    {
        auto cache = get_or_create_normal_room_cache(solid, room);
        cache->render(render_type, device_, render_context_);
    }

    RoomRenderCache* SolidRenderer::get_or_create_normal_room_cache(rf::GSolid* solid, rf::GRoom* room)
    {
        auto cache = reinterpret_cast<RoomRenderCache*>(room->geo_cache);
        if (!cache) {
            xlog::debug("Creating render cache for room %d", room->room_index);
            room_cache_.push_back(std::make_unique<RoomRenderCache>(solid, room, device_));
            cache = room_cache_.back().get();
            room->geo_cache = reinterpret_cast<GCache*>(cache);
            geo_cache_rooms[geo_cache_num_rooms++] = room;
        }
        return cache;
    }

    void SolidRenderer::render_detail(rf::GSolid* solid, GRoom* room, bool alpha)
    {
        GRenderCache* cache = get_or_create_detail_room_cache(solid, room);
        FaceRenderType render_type = alpha ? FaceRenderType::alpha : FaceRenderType::opaque;
        cache->render(render_type, render_context_);
    }

    GRenderCache* SolidRenderer::get_or_create_detail_room_cache(rf::GSolid* solid, rf::GRoom* room)
    {
        auto cache = reinterpret_cast<GRenderCache*>(room->geo_cache);
        if (!cache) {
            xlog::debug("Creating render cache for detail room %d", room->room_index);
            GRenderCacheBuilder builder;
            builder.add_room(room, solid);
            detail_render_cache_.push_back(std::make_unique<GRenderCache>(builder.build(device_)));
            cache = detail_render_cache_.back().get();
            room->geo_cache = reinterpret_cast<GCache*>(cache);
        }
        return cache;
    }

    void SolidRenderer::clear_cache()
    {
        xlog::debug("Room render cache clear");

        if (rf::level.geometry) {
            for (rf::GRoom* room: rf::level.geometry->all_rooms) {
                room->geo_cache = nullptr;
            }
        }

        room_cache_.clear();
        detail_render_cache_.clear();
        mover_render_cache_.clear();
        geo_cache_num_rooms = 0;
    }

    void SolidRenderer::render_sky_room(GRoom *room)
    {
        xlog::trace("Rendering sky room %d cache %p", room->room_index, room->geo_cache);
        before_render(sky_room_offset, rf::identity_matrix);
        render_room_faces(rf::level.geometry, room, FaceRenderType::opaque);
        render_room_faces(rf::level.geometry, room, FaceRenderType::alpha);
    }

    void SolidRenderer::render_movable_solid(GSolid* solid, const Vector3& pos, const Matrix3& orient)
    {
        xlog::trace("Rendering movable solid %p", solid);
        GRenderCache* cache = get_or_create_movable_solid_cache(solid);
        before_render(pos, orient);
        cache->render(FaceRenderType::opaque, render_context_);
        cache->render(FaceRenderType::alpha, render_context_);
        if (decals_enabled) {
            render_movable_solid_dynamic_decals(solid, pos, orient);
        }
    }

    GRenderCache* SolidRenderer::get_or_create_movable_solid_cache(rf::GSolid* solid)
    {
        auto it = mover_render_cache_.find(solid);
        if (it == mover_render_cache_.end()) {
            xlog::debug("Creating render cache for a mover %p", solid);
            GRenderCacheBuilder cache_builder;
            cache_builder.add_solid(solid);
            GRenderCache cache = cache_builder.build(device_);
            auto p = mover_render_cache_.emplace(std::make_pair(solid, std::make_unique<GRenderCache>(cache)));
            it = p.first;
        }
        return it->second.get();
    }

    void SolidRenderer::render_alpha_detail(GRoom *room, GSolid *solid)
    {
        xlog::trace("Rendering alpha detail room %d", room->room_index);
        if (room->face_list.empty()) {
            // Happens when glass is killed
            return;
        }
        before_render(rf::zero_vector, rf::identity_matrix);
        render_detail(solid, room, true);
        if (decals_enabled) {
            render_alpha_detail_dynamic_decals(room);
        }
    }

    void SolidRenderer::render_room_liquid_surface(GSolid* solid, GRoom* room)
    {
        before_render(rf::zero_vector, rf::identity_matrix);
        render_room_faces(solid, room, FaceRenderType::liquid);
    }

    void SolidRenderer::render_solid(rf::GSolid* solid, rf::GRoom** rooms, int num_rooms)
    {
        xlog::trace("Rendering level solid");
        rf::gr::light_filter_set_solid(solid, 1, 0);
        render_context_.update_lights();

        before_render(rf::zero_vector, rf::identity_matrix);

        for (int i = 0; i < num_rooms; ++i) {
            auto room = rooms[i];

            render_room_faces(solid, room, FaceRenderType::opaque);

            // Note: calling set_currently_rendered_room could improve culling here but it breaks some levels
            // if a detail brush is contained in multiple normal rooms
            for (GRoom* detail_room : room->detail_rooms) {
                if (detail_room->room_to_render_with == room && !gr::cull_bounding_box(detail_room->bbox_min, detail_room->bbox_max)) {
                    render_detail(solid, detail_room, false);
                }
            }
        }

        if (decals_enabled) {
            render_dynamic_decals(rooms, num_rooms);
        }

        rf::gr::light_filter_reset();
        render_context_.update_lights();
    }

    void SolidRenderer::before_render(const rf::Vector3& pos, const rf::Matrix3& orient)
    {
        render_context_.set_vertex_shader(vertex_shader_);
        render_context_.set_pixel_shader(pixel_shader_);
        render_context_.set_model_transform(pos, orient);
        render_context_.set_cull_mode(D3D11_CULL_BACK);
        render_context_.set_primitive_topology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    }

    void SolidRenderer::page_in_solid(rf::GSolid* solid)
    {
        for (rf::GRoom* room: solid->cached_normal_room_list) {
            get_or_create_normal_room_cache(solid, room);
        }
        for (rf::GRoom* room: solid->cached_detail_room_list) {
            get_or_create_detail_room_cache(solid, room);
        }
    }
}
