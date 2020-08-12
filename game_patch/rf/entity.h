#pragma once

#include <patch_common/MemUtils.h>
#include "object.h"

namespace rf
{
    // Forward declarations
    struct Player;
    struct ParticleEmitter;

    using EntityFlags = int;
    using EntityPowerups = int;

    struct EntityColSphere
    {
        float radius;
        int field_4;
        float dmg_factor_single;
        float dmg_factor_multi;
        float spring_constant;
        float spring_length;
        char name[24];
    };

    struct EntityColSphereArray
    {
        int num;
        EntityColSphere col_spheres[8];
    };

    struct EntityClassFootstepTriggerInfo
    {
        String field_0;
        float value;
    };

    struct EntityClassStateActionInfo
    {
        String name;
        String anim_filename;
        int field_10;
        int num_footstep_triggers;
        EntityClassFootstepTriggerInfo footstep_triggers[2];
        int field_30;
    };

    struct Skin
    {
        String name;
        int num_textures;
        String textures[12];
    };

    struct EntityClass
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
        int state_count;
        int action_count;
        EntityClassStateActionInfo *state_array;
        EntityClassStateActionInfo *action_array;
        float fov;
        int weapon_specific_states_count[64];
        int weapon_specific_action_count[64];
        EntityClassStateActionInfo *weapon_specific_states[64];
        EntityClassStateActionInfo *weapon_specific_actions[64];
        EntityColSphereArray collision_spheres;
        int unk_col_spheres_unused[81];
        int unk_col_spheres2[81];
        float field_F74;
        float unholster_delay;
        int num_skins;
        Skin skins[10];
        int num_lod_distances;
        float lod_distances[4];
        String cockpit_vfx;
        int corpse_carry_prop_id;
        int spine_bone1;
        int spine_bone2;
        int head_bone;
        float body_temp;
        float damage_type_factors[11];
        float weapon_specific_spine_adjustments[64];
    };
    static_assert(sizeof(EntityClass) == 0x1514);

    static auto& num_entity_classes = AddrAsRef<int>(0x0062F2D0);
    static auto& entity_classes = AddrAsRef<EntityClass[75]>(0x005CC500);

    struct MoveModeTbl
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

    struct EntityMotion
    {
        Vector3 rot_change;
        Vector3 pos_change;
        int field_18;
        float field_1c;
        float field_20;
    };

    struct EntityWeapon2E8InnerUnk
    {
        Vector3 field_0;
        Vector3 field_c;
        int field_18;
        int field_1c;
        float field_20;
        int field_24;
        DynamicArray<> field_28;
        char field_34;
        char field_35;
        int16_t field_36;
        int field_38;
        int field_3c;
        int field_40;
        int field_44;
        Matrix3 field_48;
        int field_6c;
        DynamicArray<> field_70;
    };

    struct EntityWeapon2E8
    {
        int field_0;
        int field_4;
        int field_8[5];
        EntityWeapon2E8InnerUnk field_1c;
        EntityWeapon2E8InnerUnk field_98;
        int field_114;
        int field_118;
        Timer timer_11_c;
        int field_120;
        int field_124;
        int field_128;
        int field_12c;
        int entity_unk_handle;
        Timer timer_134;
        Vector3 field_138;
        int field_144;
        int field_148;
        Vector3 field_14c;
        int field_158;
        int field_15c;
        int field_160;
        Vector3 field_164;
        int field_170;
        Vector3 field_174;
    };

    using NavPointType = int;

    struct NavPoint
    {
        Vector3 pos;
        Vector3 unk_mut_pos;
        float radius2;
        float radius1;
        float height;
        float pause_time;
        DynamicArray<> linked_nav_points_ptrs;
        char skip;
        char skip0;
        __int16 field_36;
        float unk_dist;
        struct NavPoint *field_3C;
        NavPointType type;
        char is_directional;
        Matrix3 orient;
        int uid;
        DynamicArray<> linked_uids;
    };
    static_assert(sizeof(NavPoint) == 0x7C);

    struct AiNavInfo
    {
        int field_0;
        Vector3 *target_pos_unk_slots[4];
        int field_14;
        int current_slot;
        NavPoint field_1C;
        NavPoint field_98;
        NavPoint *field_114;
        NavPoint *field_118;
        Timer timer_11C;
        int waypoint_list_idx;
        int waypoint_idx;
        int field_128;
        int waypoint_method;
        int hEntityUnk;
        Timer timer_134;
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
    static_assert(sizeof(AiNavInfo) == 0x180);

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
        struct EntityObj *entity;
        int weapon_cls_id;
        int weapon_cls_id2;
        int ammo[32];
        int clip_ammo[64];
        char possesed_weapons_bitmap[64];
        char is_loop_fire[64];
        DynamicArray<int> target_obj_handles;
        Timer fire_wait_timer;
        Timer alt_fire_wait_in_veh_timer;
        Timer impact_delay_timer[2];
        char was_alt_impact_delay;
        Timer timer_22C;
        Timer watchful_mode_update_target_pos_timer;
        Timer timer_234;
        Timer timer_238;
        Timer timer_23C;
        Vector3 field_240;
        int field_24C;
        Timer flee_timer_unk;
        Timer timer_254;
        Timer timer_258;
        Timer primary_burst_delay_timer;
        int primary_burst_count_left;
        Timer seconary_burst_delay_timer;
        int secondary_burst_count_left;
        int field_26C;
        int field_270;
        Timer unholster_timer;
        Timer field_278;
        int cooperation;
        AiMode ai_mode;
        AiMode base_ai_mode;
        int mode_change_time;
        int field_28C;
        int field_290;
        Timer timer_294;
        float unk_time;
        char field_29C;
        char field_29D;
        __int16 field_29E;
        Timer timer_2A0;
        Timer timer_2A4;
        Timer timer_2A8;
        int field_2AC;
        int field_2B0;
        int ai_submode;
        int field_2B8;
        int submode_change_time;
        int target_obj_handle;
        Vector3 target_obj_pos;
        Timer field_2D0;
        int look_at_handle;
        int shoot_at_handle;
        Timer field_2DC;
        Timer field_2E0;
        int field_2E4;
        AiNavInfo nav;
        EntityMotion motion_change;
        Timer field_48C;
        Vector3 field_490;
        float last_dmg_time;
        AiAttackStyle ai_attack_style;
        Timer field_4A4;
        int waypoint_list_idx;
        int waypoint_method;
        int turret_uid;
        Timer field_4B4;
        Timer field_4B8;
        int held_corpse_handle;
        float field_4C0;
        Timer field_4C4;
        int alert_camera_uid;
        int alarm_event_uid;
        Timer field_4D0;
        Timer field_4D4;
        Timer field_4D8;
        Timer waiting_to_waypoints_mode_timer;
        Timer timeout_timer;
        Timer field_4E4;
        Timer field_4E8;
        Vector3 unk_target_pos;
        Timer timer_4F8;
        Timer timer_4FC;
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

    struct EntityCameraInfo
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
        Timer camera_shake_timer;
    };
    static_assert(sizeof(EntityCameraInfo) == 0x60);

    struct EntityObj : Object
    {
        EntityObj *next;
        EntityObj *prev;
        EntityClass *cls;
        int class_id;
        EntityClass *cls2;
        AiInfo ai_info;
        Vector3 view_pos;
        Matrix3 view_orient;
        int unk_ambient_sound;
        int field_808;
        int move_snd;
        EntityFlags entity_flags;
        EntityPowerups powerups;
        Timer unk_timer818;
        int launch_snd;
        int field_820;
        int16_t kill_type;
        int16_t field_826;
        int field_828;
        int field_82c;
        Timer field_830;
        int force_state_anim;
        int field_834[4];
        AnimMesh *field_848;
        AnimMesh *field_84c;
        int nano_shield;
        int snd854;
        MoveModeTbl *movement_mode;
        Matrix3 *field_85c;
        EntityCameraInfo camera_info;
        float max_vel;
        int max_vel_modifier_type;
        int field_8c8;
        DynamicArray<> interface_props;
        DynamicArray<> unk_anim_mesh_array8_d8;
        int field_8e4[92];
        int unk_anims[180];
        int field_d24[129];
        int field_f28[200];
        int field_1248[6];
        int field_1260;
        int field_1264;
        int field_1268[65];
        Timer muzzle_flash_timer;
        Timer last_state_action_change_timer;
        Timer shell_eject_timer;
        int field_1378;
        Timer field_137c;
        int field_1380;
        int field_1384;
        int field_1388;
        int current_state;
        int next_state;
        float field_1394;
        float field_1398;
        Timer field_139c;
        Timer field_13a0;
        Timer weapon_drain_timer;
        int reload_ammo;
        int reload_clip_ammo;
        int reload_weapon_cls_id;
        float field_13b4;
        Vector3 field_13b8;
        int field_13c4[2];
        Timer field_13cc;
        int field_13d0[2];
        int field_13d8;
        int field_13dc;
        Vector3 field_13e0;
        int field_13ec;
        int field_13f0;
        int unk_bone_id13_f4;
        void *field_13f8;
        Timer timer13_fc;
        int speaker_sound;
        int field_1404;
        float unk_count_down_time1408;
        Timer unk_timer140_c;
        AnimMesh *field_1410;
        Timer splash_in_counter;
        DynamicArray<> field_1418;
        int field_1424;
        int field_1428;
        int field_142c;
        Player *local_player;
        Vector3 pitch_min;
        Vector3 pitch_max;
        int killer_entity_handle;
        int riot_shield_clutter_handle;
        int field_1454;
        Timer field_1458;
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
        AnimMesh *respawn_vfx;
        Timer field_1490;
    };
    static_assert(sizeof(EntityObj) == 0x1494);

    struct EntityBurnInfo
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
        struct EntityBurnInfo *next;
        struct EntityBurnInfo *prev;
    };

    struct ShadowInfo
    {
        float radius;
        AnimMesh *anim_mesh;
        int bone_id_unk;
        Vector3 pos;
        Matrix3 orient;
    };

    struct CorpseObj : Object
    {
        struct CorpseObj *next;
        struct CorpseObj *prev;
        float creation_level_time;
        float seconds_left_before_delete;
        int flags;
        int entity_cls_id;
        String corpse_pose_name;
        Timer emitter_lifetime_timer;
        float body_temp;
        int entity_idle_state_anim_id;
        int pose_action_anim;
        int corpse_drop_anim;
        int corpse_carry_anim;
        int corpse_pose;
        AnimMesh *helmet_anim_mesh;
        int unk_item_handle;
        EntityBurnInfo *burn_info;
        int field_2D4;
        Color ambient_clr;
        ShadowInfo shadow_info;
    };
    static_assert(sizeof(CorpseObj) == 0x318);

    static auto& EntityGetByHandle = AddrAsRef<EntityObj*(int handle)>(0x00426FC0);
    static auto& CorpseGetByHandle = AddrAsRef<CorpseObj*(int handle)>(0x004174C0);
    static auto& EntityCreate =
        AddrAsRef<EntityObj*(int cls_id, const char* name, int owner_handle, const Vector3& pos,
        const Matrix3& orient, int create_flags, int mp_character)>(0x00422360);
    static auto& AiPossessesWeapon = AddrAsRef<bool(AiInfo *ai_info, int weapon_cls_id)>(0x00403250);
    static auto& IsFemaleMpCharacter = AddrAsRef<bool(int mp_character_idx)>(0x004762C0);
    static auto& EntityIsDying = AddrAsRef<bool(EntityObj *entity)>(0x00427020);
    static auto& EntityIsAttachedToVehicle = AddrAsRef<bool(EntityObj *entity)>(0x004290D0);
    static auto& EntityIsJeepDriver = AddrAsRef<bool(EntityObj *entity)>(0x0042AC80);
    static auto& EntityIsJeepShooter = AddrAsRef<bool(EntityObj *entity)>(0x0042ACD0);
    static auto& EntityIsDriller = AddrAsRef<bool(EntityObj *entity)>(0x0042D780);
    static auto& EntityIsSub = AddrAsRef<bool(EntityObj *entity)>(0x0040A270);
    static auto& EntityIsJeep = AddrAsRef<bool(EntityObj *entity)>(0x0040A2F0);
    static auto& EntityIsFighter = AddrAsRef<bool(EntityObj *entity)>(0x0040A210);
    static auto& EntityIsHoldingBody = AddrAsRef<bool(EntityObj *entity)>(0x00429D20);
    static auto& EntityBurnInitBones = AddrAsRef<bool(EntityBurnInfo *burn_info, Object *obj)>(0x0042EB20);
    static auto& EntityGetRootBonePos = AddrAsRef<void(rf::EntityObj*, rf::Vector3&)>(0x48AC70);

    static auto& entity_obj_list = AddrAsRef<EntityObj>(0x005CB060);
    static auto& local_entity = AddrAsRef<EntityObj*>(0x005CB054);
    static auto& corpse_obj_list = AddrAsRef<CorpseObj>(0x005CABB8);
}
