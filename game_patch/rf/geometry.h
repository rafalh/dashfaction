#pragma once

#include "math/vector.h"
#include "math/matrix.h"
#include "math/plane.h"
#include "os/array.h"
#include "os/linklist.h"
#include "gr/gr.h"

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
    struct GPathNode;
    struct GLightmap;
    struct GrLight;
    struct DecalPoly;

    using GPathNodeType = int;

    struct GNodeNetwork
    {
        VArray<GPathNode*> nodes;
    };

    struct GClipWnd
    {
        float left;
        float top;
        float right;
        float bot;
    };

    enum GFaceListId
    {
        FACE_LIST_SOLID = 0,
        FACE_LIST_BBOX = 1,
        FACE_LIST_ROOM = 2,
        FACE_LIST_NUM = 3,
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
        VList<GFace, FACE_LIST_SOLID> face_list;
        VArray<GVertex*> vertices;
        VArray<GRoom*> children;
        VArray<GRoom*> all_rooms;
        VArray<GRoom*> cached_normal_room_list;
        VArray<GRoom*> cached_detail_room_list;
        VArray<GPortal*> portals;
        VArray<GSurface*> surfaces;
        VArray<GVertex*> sel_vertices;
        VArray<GFace*> sel_faces;
        VArray<GFace*> last_sel_faces;
        FArray<GDecal*, 128> decals;
        VArray<GTextureMover*> texture_movers;
        GNodeNetwork nodes;
        ubyte cubes[64];
        float cube_size;
        float cube_zero_pos[3];
        int current_frame; // used for caching
        VArray<GrLight*> lights_affecting_me;
        int last_light_state;
        int field_370;
        int field_374;

        void collide(GCollisionInput *in, GCollisionOutput *out, bool clear_fraction)
        {
            AddrCaller{0x004DF1C0}.this_call(this, in, out, clear_fraction);
        }
    };
    static_assert(sizeof(GSolid) == 0x378);

    struct GRoom
    {
        bool is_detail;
        bool is_sky;
        bool is_invisible;
        GCache *geo_cache;
        Vector3 bbox_min;
        Vector3 bbox_max;
        int room_index;
        int uid;
        VList<GFace, FACE_LIST_ROOM> face_list;
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
        GRoom *room_to_render_with;
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
        GClipWnd clip_wnd;
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
        int light_state;
    };
    static_assert(sizeof(GRoom) == 0x1CC);

    struct GFaceAttributes
    {
        uint flags;
        union {
            int group_id; // temporarily used for face grouping/sorting
            GTextureMover* texture_mover; // temporarily used by room render cache code
        };
        int bitmap_id;
        short portal_id; // portal index + 2 or 0
        short surface_index;
        int face_id;
        int smoothing_groups; // bitfield of smoothing groups
    };
    static_assert(sizeof(GFaceAttributes) == 0x18);

    struct GFace
    {
        Plane plane;
        Vector3 bounding_box_min;
        Vector3 bounding_box_max;
        GFaceAttributes attributes;
        GFaceVertex *edge_loop;
        GRoom *which_room;
        GBBox *which_bbox;
        DecalPoly *decal_list;
        short unk_cache_index;
        GFace* next[FACE_LIST_NUM];
    };
    static_assert(sizeof(GFace) == 0x60);

    struct GVertex
    {
        Vector3 pos;
        Vector3 rotated_pos;
        int last_frame; // last frame when vertex was transformed and possibly rendered
        int clip_codes; // also used for other things by geometry cache
        VArray<GFace*> adjacent_faces;
    };
    static_assert(sizeof(GVertex) == 0x2C);

    struct GFaceVertex
    {
        GVertex *vertex;
        float texture_u;
        float texture_v;
        float lightmap_u;
        float lightmap_v;
        GFaceVertex *next;
        GFaceVertex *prev;
    };
    static_assert(sizeof(GFaceVertex) == 0x1C);

    struct GSurface
    {
        int index;
        int lightstate;
        ubyte flags;
        bool should_smooth;
        bool fullbright;
        GLightmap *lightmap;
        int xstart;
        int ystart;
        int width;
        int height;
        VArray<> border_info; // unknown
        float x_pixels_per_meter;
        float y_pixels_per_meter;
        Vector3 bbox_mn;
        Vector3 bbox_mx;
        Vector2 uv_scale;
        Vector2 uv_add;
        int dropped_coefficient;
        int u_coefficient;
        int v_coefficient;
        int room_index;
        Plane plane;
    };
    static_assert(sizeof(GSurface) == 0x7C);

    struct GTextureMover
    {
        int face_id;
        float u_pan_speed;
        float v_pan_speed;
        VArray<GFace*> faces;

        void update_solid(GSolid* solid)
        {
            AddrCaller{0x004E60C0}.this_call(this, solid);
        }
    };
    static_assert(sizeof(GTextureMover) == 0x18);

    struct GPathNode
    {
        Vector3 pos;
        Vector3 use_pos;
        float original_radius;
        float radius;
        float height;
        float pause_time_seconds;
        VArray<GPathNode*> visible_nodes;
        bool visited;
        bool unusable;
        short adjacent;
        float distance;
        GPathNode *backptr;
        GPathNodeType type;
        bool directional;
        Matrix3 orient;
        int index;
        VArray<int> linked_uids;
    };
    static_assert(sizeof(GPathNode) == 0x7C);

    struct GDecal
    {
        Vector3 pos;
        Matrix3 orient;
        Vector3 width;
        int bitmap_id;
        GRoom *room;
        GRoom *room2;
        GSolid *solid;
        ubyte alpha;
        int flags;
        int object_handle;
        float tiling_scale;
        Plane decal_poly_planes[6];
        Vector3 bb_min;
        Vector3 bb_max;
        DecalPoly *poly_list;
        int num_decal_polys;
        float lifetime_sec;
        GDecal *next;
        GDecal *prev;
        GSolid *editor_geometry;
    };
    static_assert(sizeof(GDecal) == 0xEC);

    struct DecalVertex
    {
        Vector2 uv;
        Vector2 lightmap_uv;
        Vector3 pos;
        Vector3 rotated_pos;
        int field_28;
    };
    static_assert(sizeof(DecalVertex) == 0x2C);

    struct DecalPoly
    {
        Vector2 uvs[25];
        int face_priority;
        int lightmap_bm_handle;
        int nv;
        DecalVertex verts[25];
        GFace *face;
        GDecal *my_decal;
        DecalPoly *next;
        DecalPoly *prev;
        DecalPoly *next_for_face;
    };
    static_assert(sizeof(DecalPoly) == 0x534);

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
        ubyte *unk;
        int w;
        int h;
        ubyte *buf;
        int bm_handle;
        int index;
    };
    static_assert(sizeof(GLightmap) == 0x18);

    using ProcTexType = int;
    struct GProceduralTexture
    {
        int last_frame_updated;
        int last_frame_needs_updating;
        GProceduralTexture *next;
        GProceduralTexture *prev;
        int width;
        int height;
        int user_bm_handle;
        ProcTexType type;
        void (*update_function)(GProceduralTexture *pt);
        int base_bm_handle;
        float slide_pos_xc; // unused?
        float slide_pos_xt;
        float slide_pos_yc;
        float slide_pos_yt;
    };
    static_assert(sizeof(GProceduralTexture) == 0x38);

    struct GPortalObject
    {
        unsigned int id;
        Vector3 pos;
        float radius;
        bool has_alpha;
        bool did_draw;
        bool is_behind_brush;
        bool lights_enabled;
        bool use_static_lights;
        Plane *object_plane;
        Vector3 *bbox_min;
        Vector3 *bbox_max;
        float z_value;
        void (*render_function)(int, GSolid *);
    };
    static_assert(sizeof(GPortalObject) == 0x30);

    static auto& g_cache_clear = addr_as_ref<void()>(0x004F0B90);

    static auto& bbox_intersect = addr_as_ref<bool(const Vector3& bbox1_min, const Vector3& bbox1_max, const Vector3& bbox2_min, const Vector3& bbox2_max)>(0x0046C340);
}
