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
    struct Glare;

    enum EntityFlags
    {
        EF_JUMP_START_ANIM = 2,
    };

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
        int sound;
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

    enum EntitySpeed
    {
        ENTITY_SPEED_SLOW = 0x0,
        ENTITY_SPEED_REGULAR = 0x1,
        ENTITY_SPEED_FAST = 0x2,
    };

    struct EntityInfo
    {
        String name;
        String vmesh_filename;
        String debris_filename;
        String corpse_vmesh_filename;
        String corpse_anim_string;
        int corpse_emitter_handle;
        float corpse_emitter_lifetime;
        String movemode_name;
        String helmet_v3d_filename;
        float collision_radius;
        float max_life;
        float max_armor;
        float collision_damage_given;
        float max_vel;
        float slow_vel_factor;
        float fast_vel_factor;
        float acceleration;
        float max_rot_vel;
        float rot_acceleration;
        float mass;
        Vector3 min_rel_eye_phb;
        Vector3 max_rel_eye_phb;
        float radius;
        int default_primary_weapon;
        int default_secondary_weapon;
        int default_melee_weapon;
        VMeshType vmesh_class;
        int material;
        Vector3 local_eye_offset;
        Vector3 local_crouch_eye_offset;
        bool allowed_weapons[64];
        int primary_muzzle_flash_tag;
        int motion_detect_led_tag;
        int primary_muzzle_flash_glare_index;
        int fly_sound_ambient;
        String fly_sound_ambient_name;
        float fly_sound_ambient_min_dist;
        float fly_sound_ambient_vol_scale;
        int attach_sound;
        int detach_sound;
        int rotate_sound;
        int jump_sound;
        int death_sound;
        int impact_death_sound;
        int headlamp_on_sound;
        int headlamp_off_sound;
        int engine_rev_fwd_sound;
        int engine_rev_back_sound;
        int move_sound;
        int land_sound_id[10];
        int startle_sound_id;
        int low_pain_sounds_id;
        int med_pain_sounds_id;
        int squash_sounds_id;
        int footstep_sound_id[10];
        int footstep_sound_climb_id[3];
        int crawl_footstep_sound_id;
        float min_fly_sound_volume;
        ObjectUseFunction use_function;
        float use_radius;
        FArray<int, 2> primary_fire_points;
        FArray<int, 2> secondary_fire_points;
        FArray<int, 2> primary_gun_points;
        FArray<int, 2> secondary_gun_points;
        int eye_tag;
        FArray<int, 32> thruster_points;
        char thruster_vfx_names[32][32];
        int num_thruster_vfx_names;
        FArray<int, 8> glare_points;
        FArray<int, 8> glare_indices;
        FArray<int, 8> glare_headlamps;
        int helmet_tag;
        int shell_eject_tag;
        int hand_tags[2];
        int persona;
        int explode_vclip_index;
        float explode_vclip_radius;
        Vector3 explode_local_offset;
        FArray<int, 2> emitter_indices;
        float blind_pursuit_time_seconds;
        int backoff_attack_distance;
        int default_attack_style;
        int flags;
        int flags2;
        int field_72C;
        FArray<int, 4> field_730;
        String primary_warmup_vfx_filename;
        float default_movement_radius;
        int force_launch_sound;
        int num_state_anims;
        int num_action_anims;
        EntityAnimInfo *state_anims;
        EntityAnimInfo *action_anims;
        float fov_dot;
        int num_state_weapon_anims[64];
        int num_action_weapon_anims[64];
        EntityAnimInfo *state_weapon_anims[64];
        EntityAnimInfo *action_weapon_anims[64];
        FArray<EntityCollisionSphereOverride, 8> csphere_overrides;
        FArray<EntityCollisionSphere, 8> collision_spheres;
        FArray<EntityCollisionSphere, 8> crouch_collision_spheres;
        float crouch_dist;
        float unholster_weapon_delay_seconds;
        int num_alt_skins;
        ObjSkinInfo alt_skins[10];
        int num_detail_distances;
        float detail_distances[4];
        String cockpit_vfx_filename;
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

    struct MoveMode
    {
        bool valid;
        int mode;
        int move_ref_x;
        int move_ref_y;
        int move_ref_z;
        int rot_ref_x;
        int rot_ref_y;
        int rot_ref_z;
    };

    struct EntityControlData
    {
        int field_0;
        Vector3 phb;
        Vector3 delta_phb;
        Vector3 eye_phb;
        Vector3 delta_eye_phb;
        Vector3 automobile_eye_phb;
        Vector3 local_vel;
        int standing_on_obj_handle;
        int field_8b0;
        float shake_max_amplitude;
        float shake_duration;
        Timestamp shake_timestamp;
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

    struct EntityInterfacePoint
    {
        int tag_handle;
        int leech_handle;
    };
    static_assert(sizeof(EntityInterfacePoint) == 0x8);

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
        unsigned entity_flags;
        unsigned entity_flags2;
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
        MoveMode *move_mode;
        Matrix3 *move_parent_orient;
        EntityControlData control_data;
        float max_vel;
        EntitySpeed current_speed;
        int max_speed_scale;
        VArray<EntityInterfacePoint> interface_points;
        VArray<VMesh*> thruster_fx_handles;
        EntityAnim state_anims[23];
        EntityAnim action_anims[45];
        EntityAnim default_state_anims[23];
        EntityAnim default_action_anims[45];
        EntityAnim *state_weapon_anims[64];
        EntityAnim *action_weapon_anims[64];
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
        Timestamp play_next_speech_anim;
        VMesh *helmet_v3d_handle;
        Timestamp next_splash_allowed;
        VArray<Glare*> glares;
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
        float time_since_spine_bend;
        int field_1468;
        int masako_in_fighter_entity_handle;
        int driller_max_geomods;
        Color ambient_color;
        int sub_hit_sound_instance;
        int bot_index;
        int mp_character_id;
        int powerup_light_handle;
        int field_1488;
        VMesh *respawn_vfx_handle;
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
    static auto& entity_in_vehicle = addr_as_ref<bool(Entity *ep)>(0x004290D0);
    static auto& entity_is_jeep_driver = addr_as_ref<bool(Entity *ep)>(0x0042AC80);
    static auto& entity_is_jeep_gunner = addr_as_ref<bool(Entity *ep)>(0x0042ACD0);
    static auto& entity_is_driller = addr_as_ref<bool(Entity *ep)>(0x0042D780);
    static auto& entity_is_sub = addr_as_ref<bool(Entity *ep)>(0x0040A270);
    static auto& entity_is_jeep = addr_as_ref<bool(Entity *ep)>(0x0040A2F0);
    static auto& entity_is_fighter = addr_as_ref<bool(Entity *ep)>(0x0040A210);
    static auto& entity_is_carrying_corpse = addr_as_ref<bool(Entity *ep)>(0x00429D20);
    static auto& entity_fire_init_bones = addr_as_ref<bool(EntityFireInfo *efi, Object *objp)>(0x0042EB20);
    static auto& entity_is_swimming = addr_as_ref<bool(Entity* ep)>(0x0042A0A0);
    static auto& entity_is_falling = addr_as_ref<bool(Entity* ep)>(0x0042A020);
    static auto& entity_can_swim = addr_as_ref<bool(Entity* ep)>(0x00427FF0);
    static auto& entity_update_liquid_status = addr_as_ref<void(Entity* ep)>(0x00429100);
    static auto& entity_is_playing_action_animation = addr_as_ref<bool(Entity* entity, int action)>(0x00428D10);
    static auto& entity_set_next_state_anim = addr_as_ref<void(Entity* entity, int state, float transition_time)>(0x0042A580);
    static auto& entity_play_action_animation = addr_as_ref<void(Entity* entity, int action, float transition_time, bool hold_last_frame, bool with_sound)>(0x00428C90);
    static auto& entity_is_reloading = addr_as_ref<bool(Entity* entity)>(0x00425250);
    static auto& entity_weapon_is_on = addr_as_ref<bool(int entity_handle, int weapon_type)>(0x0041A830);
    static auto& entity_reload_current_primary = addr_as_ref<bool __cdecl(Entity *entity, bool no_sound, bool is_reload_packet)>(0x00425280);
    static auto& entity_turn_weapon_on = addr_as_ref<void __cdecl(int entity_handle, int weapon_type, bool alt_fire)>(0x0041A870);
    static auto& entity_turn_weapon_off = addr_as_ref<void __cdecl(int entity_handle, int weapon_type)>(0x0041AE70);
    static auto& entity_restore_mesh = addr_as_ref<void(Entity *ep, const char *mesh_name)>(0x0042C570);
    static auto& game_do_explosion = addr_as_ref<void(int vclip, GRoom* src_room, const Vector3* src_pos,
        const Vector3* pos, float radius, float damage_scale, const Vector3* dir)>(0x00436490);
    static auto& vclip_lookup = addr_as_ref<int(const char*)>(0x004C1D00);

    static auto& entity_list = addr_as_ref<Entity>(0x005CB060);
    static auto& local_player_entity = addr_as_ref<Entity*>(0x005CB054);

    static auto& persona_info = addr_as_ref<PersonaInfo[0x10]>(0x0062F998);
}
