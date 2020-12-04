#include "common.h"

namespace rf
{
    struct GRoom;
    struct GSolid;
    struct ParticleEmitter;
    struct BoltEmitter;
    struct LevelLight;
    struct GeoRegion;
    struct ClimbRegion;
    struct PushRegion;

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

    static auto& add_liquid_depth_update =
        addr_as_ref<void(GRoom* room, float target_liquid_depth, float duration)>(0x0045E640);
    static auto& level_room_from_uid = addr_as_ref<GRoom*(int uid)>(0x0045E7C0);

    static auto& level = addr_as_ref<LevelInfo>(0x00645FD8);
}
