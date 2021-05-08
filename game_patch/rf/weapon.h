#pragma once

#include <patch_common/MemUtils.h>
#include "object.h"
#include "os/timestamp.h"

namespace rf
{
    struct ImpactSoundSet;

    struct WeaponStateAction
    {
        String name;
        String anim_name;
        int anim_id;
        int sound_id;
        int sound_handle;
    };
    static_assert(sizeof(WeaponStateAction) == 0x1C);

    struct ObjLight
    {
        float inner_radius;
        float outer_radius;
        float enabled;
        float red;
        float green;
        float blue;
    };
    static_assert(sizeof(ObjLight) == 0x18);

    enum WeaponInfoType {
        WEAPON_PRIMARY = 0,
        WEAPON_SECONDARY = 1,
    };

    struct WeaponInfo
    {
        String name;
        String display_name;
        String vmesh_filename;
        int vmesh_type;
        String embedded_vmesh_filename;
        int ammo_type;
        String third_person_vmesh_filename;
        VMesh *third_person_vmesh_handle;
        int third_person_muzzle_tag;
        int third_person_grip_tag;
        int third_person_muzzle_flash_glare_index;
        String first_person_vmesh_filename;
        VMesh *mesh;
        Vector3 first_person_offset;
        Vector3 first_person_offset_ss;
        int first_person_muzzle_flash_bitmap;
        float first_person_muzzle_flash_radius;
        int first_person_alt_muzzle_flash_bitmap;
        float first_person_alt_muzzle_flash_radius;
        float first_person_fov;
        float splitscreen_fov;
        int num_projectiles;
        int clip_size_single;
        int clip_size_multi;
        int clip_size;
        float clip_reload_time;
        float clip_drain_time;
        int bitmap;
        int trail_emitter;
        float head_radius;
        float tail_radius;
        float length;
        WeaponInfoType type;
        float collision_radius;
        float lifetime_seconds;
        float lifetime_seconds_single;
        float lifetime_seconds_multi;
        float mass;
        float max_speed;
        float max_speed_multi;
        float max_speed_single;
        float lifetime_mul_vel;
        float fire_wait;
        float alt_fire_wait;
        float spread_degrees_single;
        float spread_degrees_multi;
        float spread_degrees;
        float alt_spread_degrees_single;
        float alt_spread_degrees_multi;
        float alt_spread_degrees;
        float ai_spread_degrees_single;
        float ai_spread_degrees_multi;
        float ai_spread_degrees;
        float ai_alt_spread_degrees_single;
        float ai_alt_spread_degrees_multi;
        float ai_alt_spread_degrees;
        float damage;
        float damage_multi;
        float alt_damage;
        float alt_damage_multi;
        float ai_damage_scale_single;
        float ai_damage_scale_multi;
        float ai_damage_scale;
        int blast_force_unused;
        int inner_radius_unused;
        int outer_radius_unused;
        float turn_time;
        float fov;
        float scanning_range;
        float wakeup_time;
        float drill_time_secs;
        float drill_range;
        float drill_charge_secs;
        float crater_radius;
        float create_weapon_delay_seconds[2];
        float alt_create_weapon_delay_seconds[2];
        int launch_foley_id;
        int alt_launch_foley_id;
        int silent_launch_foley_id;
        int underwater_launch_foley_id;
        int launch_fail_foley_id;
        int fly_sound;
        int impact_foley_id[10];
        int underwater_impact_foley_id[10];
        int stick_sound_unused[10];
        int near_miss_foley_id;
        int near_miss_underwater_foley_id;
        int geomod_foley_id;
        int start_sound;
        float start_delay;
        int stop_sound;
        float damage_radius;
        float damage_radius_single;
        float damage_radius_multi;
        ObjLight light;
        ObjLight muzzle_flash_light;
        int hud_icon_bitmap;
        int hud_reticle_bitmap;
        int hud_zoomed_reticle_bitmap;
        int hud_locked_reticle_bitmap;
        int zoom_in_sound;
        int max_ammo_single;
        int max_ammo_multi;
        int max_ammo;
        int flags;
        int flags2;
        String tracer_vmesh_filename;
        int tracer_frequency;
        int tracer_velocity_unused;
        float pierce_power;
        float ricochet_angle_cos;
        int ricochet_bitmap_id;
        Vector3 ricochet_size;
        float thrust_lifetime_seconds;
        int num_first_person_state_anims;
        int num_first_person_action_anims;
        WeaponStateAction first_person_state_anims[3];
        WeaponStateAction first_person_action_anims[12];
        int burst_count;
        float burst_delay_seconds;
        int burst_launch_sound;
        int num_impact_vclips;
        int impact_vclip_handles[3];
        String impact_vclip_names[3];
        float impact_vclip_scale_factors[3];
        int scorch_bitmap_id;
        Vector3 scorch_size;
        int glass_decal_bitmap_id;
        Vector3 glass_decal_size;
        int shell_eject_fp_tag;
        Vector3 shell_eject_dir;
        float shell_eject_vel_mag;
        String shell_eject_vmesh_filename;
        ImpactSoundSet* shell_eject_sound_set;
        float shell_eject_primary_pause;
        int clip_eject_fp_tag;
        String clip_eject_vmesh_filename;
        float clip_eject_pause;
        ImpactSoundSet* clip_eject_sound_set;
        float reload_zero_drain_seconds;
        float shake_camera_distance;
        float shake_camera_time;
        String silencer_v3d_name;
        VMesh *silencer_v3d;
        int silencer_fp_tag;
        String spark_vfx_name;
        VMesh *spark_vfx;
        int spark_fp_tag;
        int spark_fp_flash_bitmap;
        float ai_max_range_single;
        float ai_max_range_multi;
        float ai_max_range;
        int weapon_fp_tag;
        int weapon_select_icon_unused;
        int weapon_select_type;
        String weapon_select_icon_filename;
        int damage_type;
        int cycle_position;
        int pref_position;
        float fine_aim_region_factor;
        float fine_aim_region_factor_ss;
        String tracer_mesh_filename;
        VMesh *tracer_mesh_handle;
        float multi_bbox_size_factor;
    };
    static_assert(sizeof(WeaponInfo) == 0x550);

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
    struct Weapon : Object
    {
        Weapon *next;
        Weapon *prev;
        WeaponInfo *info;
        int info_index;
        float lifeleft_seconds;
        int fly_sound_handle;
        int light_handle;
        int weapon_flags;
        float flicker_index;
        int sticky_host_handle;
        Vector3 sticky_host_pos_offset;
        Matrix3 sticky_host_orient;
        ObjFriendliness friendliness;
        int target_handle;
        Timestamp scan_time;
        float pierce_power_left;
        float thrust_left;
        int t_flags;
        Vector3 water_hit_point;
        Vector3 firing_pos;
    };
    static_assert(sizeof(Weapon) == 0x314);

    enum WeaponState
    {
        WS_IDLE = 0,
        WS_RUN = 1,
        WS_LOOP_FIRE = 2,
    };

    static auto& weapon_types = addr_as_ref<WeaponInfo[64]>(0x0085CD08);
    static auto& remote_charge_det_weapon_type = addr_as_ref<int>(0x0085CCE0);
    static auto& machine_pistol_special_weapon_type = addr_as_ref<int>(0x0085CD00);
    static auto& machine_pistol_weapon_type = addr_as_ref<int>(0x0085CCD8);
    static auto& num_weapon_types = addr_as_ref<int>(0x00872448);
    static auto& riot_stick_weapon_type = addr_as_ref<int>(0x00872468);
    static auto& remote_charge_weapon_type = addr_as_ref<int>(0x0087210C);
    static auto& shoulder_cannon_weapon_type = addr_as_ref<int>(0x0087244C);
    static auto& assault_rifle_weapon_type = addr_as_ref<int>(0x00872470);
    static auto& hide_enemy_bullets = addr_as_ref<bool>(0x005A24D0);

    static auto& weapon_lookup_type = addr_as_ref<int(const char*)>(0x004C81F0);
    static auto& weapon_is_detonator = addr_as_ref<bool(int weapon_type)>(0x004C9070);
    static auto& weapon_is_riot_stick = addr_as_ref<bool(int weapon_type)>(0x004C90D0);
    static auto& weapon_is_on_off_weapon = addr_as_ref<bool(int weapon_type, bool alt_fire)>(0x004C8350);
    static auto& weapon_is_semi_automatic = addr_as_ref<bool(int weapon_type)>(0x004C92C0);
    static auto& weapon_is_melee = addr_as_ref<bool(int weapon_type)>(0x004C91B0);
    static auto& weapon_uses_clip = addr_as_ref<bool(int weapon_type)>(0x004C86E0);
    static auto& weapon_get_fire_wait_ms = addr_as_ref<int __cdecl(int weapon_type, bool alt_fire)>(0x004C8710);
}
