#pragma once

#include <patch_common/MemUtils.h>
#include "object.h"
#include "geometry.h"

namespace rf
{
    // Forward declarations
    struct Player;
    struct ParticleEmitter;

    using EntityFlags = int;
    using EntityPowerups = int;

    struct EntityCollisionSphereOverride
    {
        float radius;
        int field_4;
        float dmg_factor_single;
        float dmg_factor_multi;
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

    struct Skin
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
        V3DType v3d_type;
        int material;
        Vector3 field_9C;
        Vector3 field_A8;
        char allowed_weapons[64];
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
        int primary_prop_ids[3];
        int secondary_prop_ids[3];
        int primary_weapon_prop_ids[3];
        int secondary_weapon_prop_ids[3];
        int unk_prop_id_1ec;
        int thruster_prop_ids[33];
        char thruster_vfx[1024];
        int thruster_vfx_count;
        int corona_prop_ids[9];
        int corona_glare[8];
        int field_6bc;
        int corona_glare_headlamp[8];
        int field_6e0;
        int helmet_prop_id;
        int shell_eject_prop_id;
        int hand_left_prop_id;
        int hand_right_prop_id;
        int persona;
        int explode_anim_vclip;
        float explode_anim_radius;
        Vector3 explode_offset;
        int num_emitters;
        int emitters[2];
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
        Skin alt_skins[10];
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
    static auto& num_entity_types = AddrAsRef<int>(0x0062F2D0);
    static auto& entity_types = AddrAsRef<EntityInfo[MAX_ENTITY_TYPES]>(0x005CC500);

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

    struct ControlInfo
    {
        Vector3 rot_change;
        Vector3 pos_change;
        int field_18;
        float field_1c;
        float field_20;
    };

    struct AiPathInfo
    {
        int field_0;
        Vector3 *target_pos_unk_slots[4];
        int field_14;
        int current_slot;
        GPathNode field_1C;
        GPathNode field_98;
        GPathNode *field_114;
        GPathNode *field_118;
        Timestamp timer_11C;
        int waypoint_list_idx;
        int waypoint_idx;
        int field_128;
        int waypoint_method;
        int hEntityUnk;
        Timestamp timer_134;
        Vector3 orient_target;
        int moving_grp_handle_unk;
        int field_148;
        Vector3 field_14C;
        int field_158;
        char field_15C;
        int field_160;
        Vector3 unk_target_pos;
        int field_170;
        Vector3 target_pos0;
    };
    static_assert(sizeof(AiPathInfo) == 0x180);

    enum AiMode
    {
        AIM_NONE = 0x0,
        AIM_CATATONIC = 0x1,
        AIM_WAITING = 0x2,
        AIM_ATTACK = 0x3,
        AIM_WAYPOINTS = 0x4,
        AIM_COLLECTING = 0x5,
        AIM_AFTER_NOISE = 0x6,
        AIM_FLEE = 0x7,
        AIM_LOOK_AT = 0x8,
        AIM_SHOOT_AT = 0x9,
        AIM_WATCHFUL = 0xA,
        AIM_MOTION_DETECTION = 0xB,
        AIM_C = 0xC,
        AIM_TURRET_UNK = 0xD,
        AIM_HEALING = 0xE,
        AIM_CAMERA_UNK = 0xF,
        AIM_ACTIVATE_ALARM = 0x10,
        AIM_PANIC = 0x11,
    };

    enum AiAttackStyle
    {
        AIAS_DEFAULT = 0xFFFFFFFF,
        AIAS_EVASIVE = 0x1,
        AIAS_STAND_GROUND = 0x2,
        AIAS_DIRECT = 0x3,
    };

    struct AiInfo
    {
        struct Entity *entity;
        int current_primary_weapon;
        int current_secondary_weapon;
        int ammo[32];
        int clip_ammo[64];
        bool has_weapon[64];
        bool weapon_is_on[64];
        VArray<int> hate_list;
        Timestamp next_fire_primary;
        Timestamp next_fire_secondary;
        Timestamp create_weapon_delay_timestamps[2];
        bool delayed_alt_fire;
        Timestamp timer_22C;
        Timestamp watchful_mode_update_target_pos_timestamp;
        Timestamp timer_234;
        Timestamp timer_238;
        Timestamp timer_23C;
        Vector3 field_240;
        int field_24C;
        Timestamp flee_timer_unk;
        Timestamp timer_254;
        Timestamp timer_258;
        Timestamp primary_burst_delay_timestamp;
        int primary_burst_count_left;
        Timestamp seconary_burst_delay_timestamp;
        int secondary_burst_count_left;
        int field_26C;
        int field_270;
        Timestamp unholster_timestamp;
        Timestamp field_278;
        int cooperation;
        AiMode mode;
        AiMode mode_default;
        int mode_change_time;
        int mode_parm_0;
        int mode_parm_1;
        Timestamp timer_294;
        float unk_time;
        char field_29C;
        char field_29D;
        __int16 field_29E;
        Timestamp timer_2A0;
        Timestamp timer_2A4;
        Timestamp timer_2A8;
        int field_2AC;
        int field_2B0;
        int submode;
        int submode_default;
        int submode_change_time;
        int target_obj_handle;
        Vector3 target_obj_pos;
        Timestamp field_2D0;
        int look_at_handle;
        int shoot_at_handle;
        Timestamp field_2DC;
        Timestamp field_2E0;
        int field_2E4;
        AiPathInfo current_path;
        ControlInfo motion_change;
        Timestamp field_48C;
        Vector3 field_490;
        float last_dmg_time;
        AiAttackStyle ai_attack_style;
        Timestamp field_4A4;
        int waypoint_list_idx;
        int waypoint_method;
        int turret_uid;
        Timestamp field_4B4;
        Timestamp field_4B8;
        int held_corpse_handle;
        float field_4C0;
        Timestamp field_4C4;
        int alert_camera_uid;
        int alarm_event_uid;
        Timestamp field_4D0;
        Timestamp field_4D4;
        Timestamp field_4D8;
        Timestamp waiting_to_waypoints_mode_timestamp;
        Timestamp timeout_timestamp;
        Timestamp field_4E4;
        Timestamp field_4E8;
        Vector3 unk_target_pos;
        Timestamp timer_4F8;
        Timestamp timer_4FC;
        Vector3 pos_delta;
        int field_50C;
        float last_rot_time;
        float last_move_time;
        float last_fire_time;
        int field_51C_;
        float movement_radius;
        float field_524_;
        char use_custom_attack_range;
        float custom_attack_range;
        int flags;
    };
    static_assert(sizeof(AiInfo) == 0x534);

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
        AiInfo ai_info;
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
        int max_vel_modifier_type;
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
        int parent_hobj;
        int lower_leg_l_bone;
        int lower_leg_r_bone;
        int spine_bone;
        int head_bone;
        int sound_handle;
        float byte28;
        char field_2C;
        float field_30;
        int field_34;
        struct EntityFireInfo *next;
        struct EntityFireInfo *prev;
    };

    struct ShadowInfo
    {
        float radius;
        VMesh *vmesh;
        int bone_id_unk;
        Vector3 pos;
        Matrix3 orient;
    };

    struct Corpse : Object
    {
        struct Corpse *next;
        struct Corpse *prev;
        float creation_level_time;
        float seconds_left_before_delete;
        int flags;
        int entity_cls_id;
        String corpse_pose_name;
        Timestamp emitter_lifetime_timestamp;
        float body_temp;
        int entity_idle_state_anim_id;
        int pose_action_anim;
        int corpse_drop_anim;
        int corpse_carry_anim;
        int corpse_pose;
        VMesh *helmet_anim_mesh;
        int unk_item_handle;
        EntityFireInfo *burn_info;
        int field_2D4;
        Color ambient_clr;
        ShadowInfo shadow_info;
    };
    static_assert(sizeof(Corpse) == 0x318);

    static auto& EntityFromHandle = AddrAsRef<Entity*(int handle)>(0x00426FC0);
    static auto& CorpseFromHandle = AddrAsRef<Corpse*(int handle)>(0x004174C0);
    static auto& EntityCreate =
        AddrAsRef<Entity*(int entity_type, const char* name, int parent_handle, const Vector3& pos,
        const Matrix3& orient, int create_flags, int mp_character)>(0x00422360);
    static auto& AiPossessesWeapon = AddrAsRef<bool(AiInfo *ai_info, int weapon_type)>(0x00403250);
    static auto& EntityIsDying = AddrAsRef<bool(Entity *entity)>(0x00427020);
    static auto& EntityIsAttachedToVehicle = AddrAsRef<bool(Entity *entity)>(0x004290D0);
    static auto& EntityIsJeepDriver = AddrAsRef<bool(Entity *entity)>(0x0042AC80);
    static auto& EntityIsJeepShooter = AddrAsRef<bool(Entity *entity)>(0x0042ACD0);
    static auto& EntityIsDriller = AddrAsRef<bool(Entity *entity)>(0x0042D780);
    static auto& EntityIsSub = AddrAsRef<bool(Entity *entity)>(0x0040A270);
    static auto& EntityIsJeep = AddrAsRef<bool(Entity *entity)>(0x0040A2F0);
    static auto& EntityIsFighter = AddrAsRef<bool(Entity *entity)>(0x0040A210);
    static auto& EntityIsHoldingBody = AddrAsRef<bool(Entity *entity)>(0x00429D20);
    static auto& EntityBurnInitBones = AddrAsRef<bool(EntityFireInfo *burn_info, Object *obj)>(0x0042EB20);
    static auto& MultiEntityIsFemale = AddrAsRef<bool(int mp_character_idx)>(0x004762C0);
    static auto& ObjectFindRootBonePos = AddrAsRef<void(Object*, Vector3&)>(0x48AC70);

    static auto& entity_list = AddrAsRef<Entity>(0x005CB060);
    static auto& local_player_entity = AddrAsRef<Entity*>(0x005CB054);
    static auto& corpse_list = AddrAsRef<Corpse>(0x005CABB8);
}
