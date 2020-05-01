#pragma once

#include <patch_common/MemUtils.h>
#include "object.h"

namespace rf
{
    /* Weapons */

    struct WeaponStateAction
    {
        String name;
        String anim_name;
        int anim_id;
        int sound_id;
        int sound_handle;
    };
    static_assert(sizeof(WeaponStateAction) == 0x1C);

    struct WeaponClass
    {
        String name;
        String display_name;
        String v3d_filename;
        int v3d_type;
        String embedded_v3d_filename;
        int ammo_type;
        String third_person_v3d;
        AnimMesh *third_person_mesh_not_sure;
        int muzzle1_prop_id;
        int third_person_grip1_mesh_prop;
        int third_person_muzzle_flash_glare;
        String first_person_mesh;
        void *mesh;
        Vector3 first_person_offset;
        Vector3 first_person_offset_ss;
        int first_person_muzzle_flash_bitmap;
        float first_person_muzzle_flash_radius;
        int first_person_alt_muzzle_flash_bitmap;
        float first_person_alt_muzzle_flash_radius;
        float first_person_fov;
        float splitscreen_fov;
        int num_projectiles;
        int clip_size_sp;
        int clip_size_mp;
        int clip_size;
        float clip_reload_time;
        float clip_drain_time;
        int bitmap;
        int trail_emitter;
        float head_radius;
        float tail_radius;
        float head_len;
        int is_secondary;
        float collision_radius;
        int lifetime;
        float lifetime_sp;
        float lifetime_multi;
        float mass;
        float velocity;
        float velocity_multi;
        float velocity_sp;
        float lifetime_mul_vel;
        float fire_wait;
        float alt_fire_wait;
        float spread_degrees_sp;
        float spread_degrees_multi;
        int spread_degrees;
        float alt_spread_degrees_sp;
        float alt_spread_degrees_multi;
        int alt_spread_degrees;
        float ai_spread_degrees_sp;
        float ai_spread_degrees_multi;
        int ai_spread_degrees;
        float ai_alt_spread_degrees_sp;
        float ai_alt_spread_degrees_multi;
        int ai_alt_spread_degrees;
        float damage;
        float damage_multi;
        float alt_damage;
        float alt_damage_multi;
        float ai_dmg_scale_sp;
        float ai_dmg_scale_mp;
        int ai_dmg_scale;
        int field_124;
        int field_128;
        int field_12c;
        float turn_time;
        float view_cone;
        float scanning_range;
        float wakeup_time;
        float drill_time;
        float drill_range;
        float drill_charge;
        float crater_radius;
        float impact_delay[2];
        float alt_impact_delay[2];
        int launch;
        int alt_launch;
        int silent_launch;
        int underwater_launch;
        int launch_fail;
        int fly_sound;
        int material_impact_sound[30];
        int near_miss_snd;
        int near_miss_underwater_snd;
        int geomod_sound;
        int start_sound;
        float start_delay;
        int stop_sound;
        float damage_radius;
        float damage_radius_sp;
        float damage_radius_multi;
        float glow;
        float field_218;
        int glow2;
        int field_220;
        int field_224;
        int field_228;
        int muzzle_flash_light;
        int field_230;
        int muzzle_flash_light2;
        int field_238;
        int field_23c;
        int field_240;
        int hud_icon;
        int hud_reticle_texture;
        int hud_zoomed_reticle_texture;
        int hud_locked_reticle_texture;
        int zoom_sound;
        int max_ammo_sp;
        int max_ammo_mp;
        int max_ammo;
        int flags;
        int flags2;
        int field_26c;
        int field_270;
        int tracer_frequency;
        int field_278;
        float piercing_power;
        float ricochet_angle;
        int ricochet_bitmap;
        Vector3 ricochet_size;
        float thrust_lifetime;
        int num_states;
        int num_actions;
        WeaponStateAction states[3];
        WeaponStateAction actions[7];
        int field_3b8[35];
        int burst_count;
        float burst_delay;
        int burst_launch_sound;
        int impact_vclips_count;
        int impact_vclips[3];
        String impact_vclip_names[3];
        float impact_vclips_radius_list[3];
        int decal_texture;
        Vector3 decal_size;
        int glass_decal_texture;
        Vector3 glass_decal_size;
        int fpgun_shell_eject_prop_id;
        Vector3 shells_ejected_base_dir;
        float shell_eject_velocity;
        String shells_ejected_v3d;
        int shells_ejected_custom_sound_set;
        float primary_pause_time_before_eject;
        int fpgun_clip_eject_prop_id;
        String clips_ejected_v3d;
        float clips_ejected_drop_pause_time;
        int clips_ejected_custom_sound_set;
        float reload_zero_drain;
        float camera_shake_dist;
        float camera_shake_time;
        String silencer_v3d;
        int field_4f0_always0;
        int fpgun_silencer_prop_id;
        String spark_vfx;
        int field_500always0;
        int fpgun_thruster_prop_id;
        int field_508;
        float ai_attack_range1;
        float ai_attack_range2;
        float field_514;
        int fpgun_weapon_prop_id;
        int field_51c_always1neg;
        int weapon_type;
        String weapon_icon;
        int damage_type;
        int cycle_pos;
        int pref_pos;
        float fine_aim_reg_size;
        float fine_aim_reg_size_ss;
        String tracer_effect;
        int field_548_always_0;
        float multi_bbox_size_factor;
    };
    static_assert(sizeof(WeaponClass) == 0x550);

    enum WeaponTypeFlags
    {
        WTF_ALT_FIRE = 0x1,
        WTF_CONTINUOUS_FIRE = 0x2,
        WTF_ALT_CONTINUOUS_FIRE = 0x4,
        WTF_FLICKERS = 0x8,
        WTF_THRUSTER = 0x10,
        WTF_MELEE = 0x20,
        WTF_REMOTE_CHARGE = 0x40,
        WTF_PLAYER_WEP = 0x80,
        WTF_ALT_ZOOM = 0x100,
        WTF_UNDERWATER = 0x200,
        WTF_FROM_EYE = 0x400,
        WTF_INFRARED = 0x800,
        WTF_GRAVITY = 0x1000,
        WTF_FIXED_MUZ_FLASH = 0x2000,
        WTF_ALT_LOCK = 0x4000,
        WTF_SILENT = 0x8000,
        WTF_TORPEDO = 0x10000,
        WTF_DRILL = 0x20000,
        WTF_DETONATOR = 0x40000,
        WTF_ALT_CUSTOM_MODE = 0x80000,
        WTF_SEMI_AUTOMATIC = 0x100000,
        WTF_AUTOAIM = 0x200000,
        WTF_PS2_FP_FULL_CLIP = 0x400000,
        WTF_HOMING = 0x800000,
        WTF_STICKY = 0x1000000,
        WTF_HEAD_LENGTH_AND_TAIL_RADIUS = 0x4000000,
        WTF_BURST_MODE = 0x8000000,
        WTF_BURST_MODE_ALT_FIRE = 0x10000000,
        WTF_CAMERA_SHAKE = 0x20000000,
        WTF_TRACERS = 0x40000000,
        WTF_PIERCING = 0x80000000,
    };
    struct WeaponObj : Object
    {
        struct WeaponObj *next;
        struct WeaponObj *prev;
        WeaponClass *weapon_cls;
        int weapon_cls_id;
        float lifetime;
        int fly_sound;
        int glow_light;
        int weapon_flags;
        float field_2AC;
        int field_2B0;
        Vector3 field_2B4;
        Matrix3 field_2C0;
        Friendliness entity_friendliness;
        int field_2E8;
        Timer field_2EC;
        float piercing_power;
        float thrust_lifetime;
        int weapon_flags2;
        Vector3 last_hit_point;
        Vector3 field_308;
    };
    static_assert(sizeof(WeaponObj) == 0x314);

    static auto& weapon_classes = AddrAsRef<WeaponClass[64]>(0x0085CD08);
    static auto& riot_stick_cls_id = AddrAsRef<int32_t>(0x00872468);
    static auto& remote_charge_cls_id = AddrAsRef<int32_t>(0x0087210C);
    static auto& hide_enemy_bullets = AddrAsRef<bool>(0x005A24D0);

    static auto& WeaponClsIsDetonator = AddrAsRef<bool(int weapon_cls_id)>(0x004C9070);
    static auto& WeaponClsIsRiotStick = AddrAsRef<bool(int weapon_cls_id)>(0x004C90D0);
    static auto& PlayerSwitchWeaponInstant = AddrAsRef<void(rf::Player *player, int weapon_cls_id)>(0x004A4980);
    static auto& EntityIsReloading = AddrAsRef<bool(EntityObj* entity)>(0x00425250);
    static auto& IsEntityWeaponInContinousFire = AddrAsRef<bool(int entity_handle, int weapon_cls_id)>(0x0041A830);
}
