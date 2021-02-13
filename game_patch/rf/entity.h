#pragma once

#include <patch_common/MemUtils.h>
#include "object.h"
#include "geometry.h"
#include "ai.h"
#include "gr/gr.h"

namespace rf
{
    // Forward declarations
    struct Player;
    struct ParticleEmitter;
    struct EntityFireInfo;
    struct ClimbRegion;

    using EntityFlags = int;
    using EntityFlags2 = int;
    using EntitySpeed = int;

    constexpr int EF_JUMP_START_ANIM = 2;

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

    struct EntityAnim
    {
        int vmesh_anim_index;
        int info_index;
        int sound;
        int sound_instance;
    };
    static_assert(sizeof(EntityAnim) == 0x10);


    struct Entity : Object
    {
        Entity *next;
        Entity *prev;
        EntityInfo *info;
        int info_index;
        EntityInfo *info2;
        AiInfo ai;
        Vector3 eye_pos;
        Matrix3 eye_orient;
        int fly_sound_ambient_handle;
        int pain_sound_handle;
        int move_sound_handle;
        EntityFlags entity_flags;
        EntityFlags2 entity_flags2;
        Timestamp weapon_looping_timestamp;
        int weapon_loop_sound_handle;
        int weapon_stop_sound_handle;
        int death_anim_index;
        int flinch_anim_index;
        int drop_item_class;
        Timestamp can_play_flinch_timestamp;
        int force_state_anim_index;
        int npc_trigger_handle;
        int custom_death_anim_index;
        float fov_cos;
        int engine_rev_sound_handle;
        VMesh *primary_warmup_vfx_handle;
        VMesh *cockpit_vfx_handle;
        int nano_shield;
        int damage_sound_instance;
        MoveModeInfo *move_mode;
        Matrix3 *move_parent_orient;
        EntityControlData control_data;
        float max_vel;
        EntitySpeed current_speed;
        int max_speed_scale;
        VArray<> interface_points;
        VArray<> thruster_fx_handles;
        EntityAnim state_anims[23];
        EntityAnim action_anims[145];
        int last_idle_anim_index_maybe_unused;
        int last_cust_anim_index_maybe_unused;
        Timestamp primary_muzzle_timestamp;
        Timestamp play_idle_stand_animation_timestamp;
        Timestamp shell_eject_timestamp;
        int muzzle_light_handle;
        Timestamp next_footstep_timestamp;
        int ground_material;
        int debug_anim_current_state;
        int debug_anim_current_action;
        int current_state_anim;
        int next_state_anim;
        float total_transition_time;
        float elapsed_transition_time;
        Timestamp reload_done_timestamp;
        Timestamp drop_clip_timestamp;
        Timestamp zero_ammo_timestamp;
        int reload_clip_ammo;
        int reload_reserve_ammo;
        int reload_weapon_type;
        float handgun_spread_factor;
        Vector3 stationary_fvec;
        int driller_rot_angle;
        float driller_rot_speed;
        Timestamp driller_allow_geomod_timestamp;
        int driller_sound_handle;
        int flame_handle;
        EntityFireInfo* entity_fire_handle;
        Vector3 *collision_sphere_current_pos;
        Vector3 board_vehicle_local_pos;
        ClimbRegion *ignored_climb_region;
        ClimbRegion *current_climb_region;
        int mouth_tag;
        ParticleEmitter *breath_emitter;
        Timestamp last_fired_timestamp;
        int voice_handle;
        int last_voice_anim_index;
        float speech_duration;
        Timestamp unk_timer140c;
        VMesh *helmet_v3d_handle;
        Timestamp next_splash_allowed;
        VArray<> glares;
        int tracer_countdown;
        int weapon_custom_mode_bitmap;
        int weapon_silencer_bitfield;
        Player *local_player;
        Vector3 min_rel_eye_phb;
        Vector3 max_rel_eye_phb;
        int killer_handle;
        int riot_shield_handle;
        int jeep_gun_tag;
        Timestamp pain_sound_ok_timestamp;
        int hand_clutter_handles[2];
        float time;
        int field_1468;
        int entity_masako_in_fighter_hobj;
        int field_1470;
        Color ambient_color;
        int sub_hit_sound;
        int field_147c;
        int mp_character_id;
        int light_1484;
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
        bool time_limited;
        float time;
        int field_34;
        EntityFireInfo *next;
        EntityFireInfo *prev;
    };

    struct ShadowInfo
    {
        float radius;
        VMesh *vmesh_handle;
        int root_bone_index;
        Vector3 pos;
        Matrix3 orient;
    };

    struct PersonaInfo
    {
        String name;
        int alert_snd;
        int under_alert_snd;
        int cower_snd;
        int flee_snd;
        int use_key_snd;
        int healing_gone_snd;
        int heal_ignore_snd;
        int timeout_snd;
        int humming_snd;
        int fight_single_snd;
        int fight_multi_snd;
        int panic_snd;
    };

    static auto& entity_from_handle = addr_as_ref<Entity*(int handle)>(0x00426FC0);
    static auto& entity_create =
        addr_as_ref<Entity*(int entity_type, const char* name, int parent_handle, const Vector3& pos,
        const Matrix3& orient, int create_flags, int mp_character)>(0x00422360);
    static auto& entity_is_dying = addr_as_ref<bool(Entity *ep)>(0x00427020);
    static auto& entity_is_attached_to_vehicle = addr_as_ref<bool(Entity *ep)>(0x004290D0);
    static auto& entity_is_jeep_driver = addr_as_ref<bool(Entity *ep)>(0x0042AC80);
    static auto& entity_is_jeep_shooter = addr_as_ref<bool(Entity *ep)>(0x0042ACD0);
    static auto& entity_is_driller = addr_as_ref<bool(Entity *ep)>(0x0042D780);
    static auto& entity_is_sub = addr_as_ref<bool(Entity *ep)>(0x0040A270);
    static auto& entity_is_jeep = addr_as_ref<bool(Entity *ep)>(0x0040A2F0);
    static auto& entity_is_fighter = addr_as_ref<bool(Entity *ep)>(0x0040A210);
    static auto& entity_is_holding_body = addr_as_ref<bool(Entity *ep)>(0x00429D20);
    static auto& entity_fire_init_bones = addr_as_ref<bool(EntityFireInfo *efi, Object *objp)>(0x0042EB20);
    static auto& entity_is_swimming = addr_as_ref<bool(Entity* ep)>(0x0042A0A0);
    static auto& entity_is_falling = addr_as_ref<bool(Entity* ep)>(0x0042A020);
    static auto& entity_can_swim = addr_as_ref<bool(Entity* ep)>(0x00427FF0);
    static auto& entity_update_liquid_status = addr_as_ref<void(Entity* ep)>(0x00429100);
    static auto& entity_is_playing_action_animation = addr_as_ref<bool(Entity* entity, int action_idx)>(0x00428D10);
    static auto& entity_set_next_state_anim = addr_as_ref<void(Entity* entity, int state, float transition_time)>(0x0042A580);
    static auto& entity_play_action_animation = addr_as_ref<void(Entity* entity, int action, float transition_time, bool hold_last_frame, bool with_sound)>(0x00428C90);
    static auto& entity_is_reloading = addr_as_ref<bool(Entity* entity)>(0x00425250);
    static auto& entity_weapon_is_on = addr_as_ref<bool(int entity_handle, int weapon_type)>(0x0041A830);
    static auto& entity_reload_current_primary = addr_as_ref<bool __cdecl(Entity *entity, bool no_sound, bool is_reload_packet)>(0x00425280);
    static auto& entity_turn_weapon_on = addr_as_ref<void __cdecl(int entity_handle, int weapon_type, bool alt_fire)>(0x0041A870);
    static auto& entity_turn_weapon_off = addr_as_ref<void __cdecl(int entity_handle, int weapon_type)>(0x0041AE70);

    static auto& multi_entity_is_female = addr_as_ref<bool(int mp_character_idx)>(0x004762C0);

    static auto& entity_list = addr_as_ref<Entity>(0x005CB060);
    static auto& local_player_entity = addr_as_ref<Entity*>(0x005CB054);

    static auto& persona_info = addr_as_ref<PersonaInfo[0x10]>(0x0062F998);
}
