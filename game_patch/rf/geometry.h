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
    struct GPathNode;
    struct GrLight;
    struct ParticleEmitter;
    struct BoltEmitter;
    struct LevelLight;
    struct GeoRegion;
    struct ClimbRegion;
    struct PushRegion;

    using GPathNodeType = int;

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
        __int16 adjacent;
        float distance;
        GPathNode *backptr;
        GPathNodeType type;
        bool directional;
        Matrix3 orient;
        int index;
        VArray<int> linked_uids;
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
        VArray<GVertex*> sel_vertices;
        VArray<GFace*> sel_faces;
        VArray<GFace*> last_sel_faces;
        FArray<GDecal*, 128> decals;
        VArray<GTextureMover*> texture_movers;
        GNodeNetwork nodes;
        int field_30C;
        int field_310[16];
        int field_350[3];
        int transform_cache_key;
        VArray<GrLight*> lights_affecting_me;
        int light_state;
        int field_370;
        int field_374;

        void Collide(GCollisionInput *in, GCollisionOutput *out, bool clear_fraction)
        {
            AddrCaller{0x004DF1C0}.this_call(this, in, out, clear_fraction);
        }
    };
    static_assert(sizeof(GSolid) == 0x378);

    struct GClipWnd
    {
        float left;
        float top;
        float right;
        float bot;
    };

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

    struct EmitterPair
    {
        ParticleEmitter *pemitter;
        int sound_emitter_handle;
        Vector3 emitting_pos;
        EmitterPair *next;
        EmitterPair *prev;
        GRoom *room;
    };
    static_assert(sizeof(EmitterPair) == 0x20);

    struct EmitterPairSet
    {
        EmitterPair head;
    };
    static_assert(sizeof(EmitterPairSet) == 0x20);

    struct LevelInfo
    {
        int version;
        String name;
        String filename;
        String author;
        String level_date;
        int level_timestamp;
        int default_rock_texture;
        int default_rock_hardness;
        Color ambient_light;
        Color distance_fog_color;
        float distance_fog_near_clip;
        float distance_fog_far_clip;
        float old_distance_fog_far_clip;
        char has_mirrored_faces;
        char has_skyroom;
        char skybox_rotation_axis;
        float skybox_rotation_velocity;
        float skybox_current_rotation;
        Matrix3 skybox_current_orientation;
        Matrix3 skybox_inverse_orientation;
        int checksum;
        String next_level_filename;
        bool hard_level_break;
        VArray<ParticleEmitter*> pemitters;
        VArray<BoltEmitter*> bemitters;
        VArray<LevelLight*> lights;
        VArray<GeoRegion*> regions;
        VArray<ClimbRegion*> ladders;
        VArray<PushRegion*> pushers;
        EmitterPairSet ep_set;
        GSolid *geometry;
        int flags;
        float time;
        float global_time;
        float time_left;
        Vector3 player_start_pos;
        Matrix3 player_start_orient;
    };
    static_assert(sizeof(LevelInfo) == 0x154);

    static auto& GeomClearCache = AddrAsRef<void()>(0x004F0B90);

    static auto& level = AddrAsRef<LevelInfo>(0x00645FD8);
}
