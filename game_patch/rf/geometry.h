#pragma once

#include "common.h"

namespace rf
{
    struct GCollisionInput;
    struct GCollisionOutput;
    struct GVertex;
    struct GFaceVertex;
    struct GFace;
    struct GSurface;
    struct GCache;
    struct GBBox;
    struct GDecal;
    struct GPortal;
    struct GTextureMover;
    struct GRoom;
    struct GrLight;
    struct GPathNode;

    using GPathNodeType = int;

    struct GPathNode
    {
        Vector3 pos;
        Vector3 unk_mut_pos;
        float radius2;
        float radius1;
        float height;
        float pause_time;
        VArray<> linked_nav_points_ptrs;
        char skip;
        char skip0;
        __int16 field_36;
        float unk_dist;
        struct GPathNode *field_3C;
        GPathNodeType type;
        char is_directional;
        Matrix3 orient;
        int uid;
        VArray<> linked_uids;
    };
    static_assert(sizeof(GPathNode) == 0x7C);

    struct GNodeNetwork
    {
        VArray<GPathNode*> nodes;
    };

    struct GSolid
    {
        GBBox *bbox;
        char name[64];
        int modifiability;
        Vector3 bbox_min;
        Vector3 bbox_max;
        float bounding_sphere_radius;
        Vector3 bounding_sphere_center;
        VList<GFace> face_list;
        VArray<GVertex*> vertices;
        VArray<GRoom*> children;
        VArray<GRoom*> all_rooms;
        VArray<GRoom*> cached_normal_room_list;
        VArray<GRoom*> cached_detail_room_list;
        VArray<GPortal*> portals;
        VArray<GSurface*> surfaces;
        VArray<void*> sel_vertices;
        VArray<void*> sel_faces;
        VArray<void*> last_sel_faces;
        FArray<GDecal*, 128> decals;
        VArray<GTextureMover*> texture_movers;
        GNodeNetwork nodes;
        int field_30C;
        int field_310[16];
        int field_350[3];
        int transform_cache_key;
        VArray<GrLight*> lights_affecting_me;
        int num_total_lights;
        int field_370;
        int field_374;

        void TestLineCollision(GCollisionInput *in, GCollisionOutput *out, bool clear_fraction)
        {
            AddrCaller{0x004DF1C0}.this_call(this, in, out, clear_fraction);
        }
    };
    static_assert(sizeof(GSolid) == 0x378);

    struct GRoom
    {
        bool is_subroom;
        bool is_skyroom;
        bool is_invisible;
        GCache *geo_cache;
        Vector3 bbox_min;
        Vector3 bbox_max;
        int room_index;
        int uid;
        VList<GFace> face_list;
        VArray<GPortal*> portals;
        GBBox *bbox;
        bool is_blocked;
        bool is_cold;
        bool is_outside;
        bool is_airlock;
        bool is_pressurized;
        bool ambient_light_defined;
        Color ambient_light;
        char eax_effect[32];
        bool has_alpha;
        VArray<GRoom*> detail_rooms;
        struct GRoom *room_to_render_with;
        Plane room_plane;
        int last_frame_rendered_normal;
        int last_frame_rendered_alpha;
        float life;
        bool is_invincible;
        FArray<GDecal*, 48> decals;
        bool visited_this_frame;
        bool visited_this_search;
        int render_depth;
        int creation_id;
        float clip_wnd[4];
        int bfs_visited;
        int liquid_type;
        bool contains_liquid;
        float liquid_depth;
        Color liquid_color;
        float liquid_visibility;
        int liquid_surface_bitmap;
        int liquid_surface_proctex_id;
        int liquid_ppm_u;
        int liquid_ppm_v;
        float liquid_angle;
        int liquid_alpha;
        bool liquid_plankton;
        int liquid_waveform;
        float liquid_surface_pan_u;
        float liquid_surface_pan_v;
        VArray<GrLight*> cached_lights;
        int num_total_lights;
    };
    static_assert(sizeof(GRoom) == 0x1CC);

    struct GCollisionInput
    {
        GFace *face;
        Vector3 geometry_pos;
        Matrix3 geometry_orient;
        Vector3 start_pos;
        Vector3 len;
        float radius;
        int flags;
        Vector3 start_pos_transformed;
        Vector3 len_transformed;

        GCollisionInput()
        {
            AddrCaller{0x004161F0}.this_call(this);
        }
    };
    static_assert(sizeof(GCollisionInput) == 0x6C);

    struct GCollisionOutput
    {
        int num_hits;
        float fraction;
        Vector3 hit_point;
        Vector3 normal;
        int field_20;
        GFace *face;

        GCollisionOutput()
        {
            AddrCaller{0x00416230}.this_call(this);
        }
    };
    static_assert(sizeof(GCollisionOutput) == 0x28);

    struct GLightmap
    {
        uint8_t *unk;
        int w;
        int h;
        uint8_t *buf;
        int bm_handle;
        int index;
    };
    static_assert(sizeof(GLightmap) == 0x18);

    static auto& GeomClearCache = AddrAsRef<void()>(0x004F0B90);

    static auto& rfl_static_geometry = AddrAsRef<GSolid*>(0x006460E8);
}
