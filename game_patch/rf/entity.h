#pragma once

#include <patch_common/MemUtils.h>
#include "object.h"
#include "geometry.h"
#include "ai.h"

namespace rf
{
    // Forward declarations
    struct Player;
    struct ParticleEmitter;

    using EntityFlags = int;
    using EntityPowerups = int;
    using EntitySpeed = int;

    struct EntityCollisionSphereOverride
    {
        float radius;
        float damage_factor;
        float damage_factor_single;
        float damage_factor_multi;
        float spring_constant;
        float spring_length;
        char name[24];
    };

    struct EntityCollisionSphere
    {
        float radius;
        float damage_factor;
        float damage_factor_single;
        float damage_factor_multi;
        float spring_constant;
        float spring_length;
        Vector3 center;
        int index;
    };

    struct EntityAnimTrigger
    {
        String name;
        float frame_num;
    };

    struct EntityAnimInfo
    {
        String name;
        String anim_filename;
        int field_10;
        int num_triggers;
        EntityAnimTrigger triggers[2];
        int field_30;
    };

    struct ObjSkinInfo
    {
        String name;
        int num_textures;
        String textures[12];
    };

    struct EntityInfo
    {
        String name;
        String v3d_filename;
        String debris_filename;
        String corpse_v3d_filename;
        String death_anim;
        int corpse_emitter;
        float corpse_emitter_lifetime;
        String move_mode;
        String helmet_v3d_filename;
        float col_radius;
        float life;
        float envirosuit;
        float col_damage_given;
        float max_vel;
        float slow_factor;
        float fast_factor;
        float acceleration;
        float max_rot_vel;
        float rot_acceleration;
        float mass;
        Vector3 min_rel_eye_phb;
        Vector3 max_rel_eye_phb;
        int field_84;
        int default_primary_weapon;
        int default_secondary_weapon;
        int default_melee_weapon;
        VMeshType vmesh_class;
        int material;
        Vector3 field_9C;
        Vector3 field_A8;
        bool allowed_weapons[64];
        int muzzle1_prop_id;
        int led1prop_id;
        int primary_muzzle_glare;
        int fly_snd_not_sure;
        String fly_snd;
        float fly_snd_unk1;
        float fly_snd_unk2;
        int attach_snd;
        int detach_snd;
        int rotate_snd;
        int jump_snd;
        int death_snd;
        int impact_death_sound;
        int head_lamp_on_snd;
        int head_lamp_off_snd;
        int engine_rev_fwd_sound;
        int engine_rev_back_sound;
        int move_sound;
        int land_sounds[10];
        int startle_snd;
        int low_pain_snd;
        int med_pain_snd;
        int squash_snd;
        int footstep_sounds[10];
        int climb_footstep_sounds[3];
        int crawl_footstep_sound;
        float min_fly_snd_volume;
        ObjectUse use;
        float use_radius;
        FArray<int, 2> primary_prop_ids;
        FArray<int, 2> secondary_prop_ids;
        FArray<int, 2> primary_weapon_prop_ids;
        FArray<int, 2> secondary_weapon_prop_ids;
        int eye_tag;
        FArray<int, 32> thruster_prop_ids;
        char thruster_vfx[1024];
        int thruster_vfx_count;
        FArray<int, 8> corona_prop_ids;
        FArray<int, 8> corona_glare;
        FArray<int, 8> corona_glare_headlamp;
        int helmet_prop_id;
        int shell_eject_prop_id;
        int hand_left_prop_id;
        int hand_right_prop_id;
        int persona;
        int explode_anim_vclip;
        float explode_anim_radius;
        Vector3 explode_offset;
        FArray<int, 2> emitters;
        float blind_pursuit_time;
        int field_71C;
        int attack_style;
        int entity_flags;
        int entity_flags2;
        int field_72C[6];
        String primary_warmup_fx;
        float movement_radius;
        int force_launch_sound;
        int num_state_anims;
        int num_action_anims;
        EntityAnimInfo *state_anims;
        EntityAnimInfo *action_anims;
        float fov;
        int weapon_specific_states_count[64];
        int weapon_specific_action_count[64];
        EntityAnimInfo *num_state_weapon_anims[64];
        EntityAnimInfo *num_action_weapon_anims[64];
        FArray<EntityCollisionSphereOverride, 8> csphere_overrides;
        FArray<EntityCollisionSphere, 8> collision_spheres;
        FArray<EntityCollisionSphere, 8> crouch_collision_spheres;
        float crouch_dist;
        float unholster_weapon_delay_seconds;
        int num_alt_skins;
        ObjSkinInfo alt_skins[10];
        int num_detail_distances;
        float detail_distances[4];
        String cockpit_vfx;
        int corpse_carry_tag;
        int lower_spine_tag;
        int upper_spine_tag;
        int head_tag;
        float body_temp;
        float damage_factors[11];
        float weapon_specific_spine_adjustments[64];
    };
    static_assert(sizeof(EntityInfo) == 0x1514);

    constexpr int MAX_ENTITY_TYPES = 75;
    static auto& num_entity_types = addr_as_ref<int>(0x0062F2D0);
    static auto& entity_types = addr_as_ref<EntityInfo[MAX_ENTITY_TYPES]>(0x005CC500);

    struct MoveModeInfo
    {
        char available;
        char unk, unk2, unk3;
        int id;
        int move_ref_x;
        int move_ref_y;
        int move_ref_z;
        int rot_ref_x;
        int rot_ref_y;
        int rot_ref_z;
    };

    struct EntityControlData
    {
        int field_860;
        Vector3 rot_yaw;
        Vector3 rot_yaw_delta;
        Vector3 rot_pitch;
        Vector3 rot_pitch_delta;
        Vector3 field_894;
        Vector3 field_8a0;
        int field_8ac;
        int field_8b0;
        float camera_shake_dist;
        float camera_shake_time;
        Timestamp camera_shake_timestamp;
    };
    static_assert(sizeof(EntityControlData) == 0x60);

    struct Entity : Object
    {
        Entity *next;
        Entity *prev;
        EntityInfo *info;
        int info_index;
        EntityInfo *info2;
        AiInfo ai;
        Vector3 view_pos;
        Matrix3 view_orient;
        int unk_ambient_sound;
        int field_808;
        int move_snd;
        EntityFlags entity_flags;
        EntityPowerups powerups;
        Timestamp unk_timer818;
        int launch_snd;
        int field_820;
        int16_t kill_type;
        int16_t field_826;
        int field_828;
        int field_82c;
        Timestamp field_830;
        int force_state_anim;
        int field_834[4];
        VMesh *field_848;
        VMesh *field_84c;
        int nano_shield;
        int snd854;
        MoveModeInfo *movement_mode;
        Matrix3 *field_85c;
        EntityControlData control_data;
        float max_vel;
        EntitySpeed entity_speed;
        int field_8c8;
        VArray<> interface_props;
        VArray<> unk_anim_mesh_array8_d8;
        int field_8e4[92];
        int unk_anims[180];
        int field_d24[129];
        int field_f28[200];
        int field_1248[6];
        int field_1260;
        int field_1264;
        int field_1268[65];
        Timestamp muzzle_flash_timestamp;
        Timestamp last_state_action_change_timestamp;
        Timestamp shell_eject_timestamp;
        int field_1378;
        Timestamp field_137c;
        int field_1380;
        int field_1384;
        int field_1388;
        int current_state;
        int next_state;
        float field_1394;
        float field_1398;
        Timestamp field_139c;
        Timestamp field_13a0;
        Timestamp weapon_drain_timestamp;
        int reload_ammo;
        int reload_clip_ammo;
        int reload_weapon_cls_id;
        float field_13b4;
        Vector3 field_13b8;
        int field_13c4[2];
        Timestamp field_13cc;
        int field_13d0[2];
        int field_13d8;
        int field_13dc;
        Vector3 field_13e0;
        int field_13ec;
        int field_13f0;
        int unk_bone_id13_f4;
        void *field_13f8;
        Timestamp timer13_fc;
        int speaker_sound;
        int field_1404;
        float unk_count_down_time1408;
        Timestamp unk_timer140_c;
        VMesh *field_1410;
        Timestamp splash_in_counter;
        VArray<> field_1418;
        int field_1424;
        int field_1428;
        int field_142c;
        Player *local_player;
        Vector3 pitch_min;
        Vector3 pitch_max;
        int killer_entity_handle;
        int riot_shield_clutter_handle;
        int field_1454;
        Timestamp field_1458;
        int unk_clutter_handles[2];
        float time;
        int field_1468;
        int unk_entity_handle;
        int field_1470;
        Color ambient_color;
        int field_1478;
        int field_147c;
        int field_1480;
        int field_1484;
        int field_1488;
        VMesh *respawn_vfx;
        Timestamp field_1490;
    };
    static_assert(sizeof(Entity) == 0x1494);

    struct EntityFireInfo
    {
        ParticleEmitter *emitters[4];
        int parent_handle;
        int lower_leg_l_bone;
        int lower_leg_r_bone;
        int spine_bone;
        int head_bone;
        int sound_handle;
        float byte28;
        char field_2C;
        float field_30;
        int field_34;
        EntityFireInfo *next;
        EntityFireInfo *prev;
    };

    struct ShadowInfo
    {
        float radius;
        VMesh *vmesh;
        int bone_id_unk;
        Vector3 pos;
        Matrix3 orient;
    };

    static auto& entity_from_handle = addr_as_ref<Entity*(int handle)>(0x00426FC0);
    static auto& entity_create =
        addr_as_ref<Entity*(int entity_type, const char* name, int parent_handle, const Vector3& pos,
        const Matrix3& orient, int create_flags, int mp_character)>(0x00422360);
    static auto& entity_is_dying = addr_as_ref<bool(Entity *entity)>(0x00427020);
    static auto& entity_is_attached_to_vehicle = addr_as_ref<bool(Entity *entity)>(0x004290D0);
    static auto& entity_is_jeep_driver = addr_as_ref<bool(Entity *entity)>(0x0042AC80);
    static auto& entity_is_jeep_shooter = addr_as_ref<bool(Entity *entity)>(0x0042ACD0);
    static auto& entity_is_driller = addr_as_ref<bool(Entity *entity)>(0x0042D780);
    static auto& entity_is_sub = addr_as_ref<bool(Entity *entity)>(0x0040A270);
    static auto& entity_is_jeep = addr_as_ref<bool(Entity *entity)>(0x0040A2F0);
    static auto& entity_is_fighter = addr_as_ref<bool(Entity *entity)>(0x0040A210);
    static auto& entity_is_holding_body = addr_as_ref<bool(Entity *entity)>(0x00429D20);
    static auto& entity_fire_init_bones = addr_as_ref<bool(EntityFireInfo *burn_info, Object *obj)>(0x0042EB20);
    static auto& entity_is_swimming = addr_as_ref<bool(Entity* entity)>(0x0042A0A0);
    static auto& entity_is_falling = addr_as_ref<bool(Entity* entit)>(0x0042A020);
    static auto& multi_entity_is_female = addr_as_ref<bool(int mp_character_idx)>(0x004762C0);

    static auto& entity_list = addr_as_ref<Entity>(0x005CB060);
    static auto& local_player_entity = addr_as_ref<Entity*>(0x005CB054);
}
