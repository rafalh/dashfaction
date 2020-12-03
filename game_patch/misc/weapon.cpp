#include <patch_common/FunHook.h>
#include <patch_common/CallHook.h>
#include <patch_common/CodeInjection.h>
#include <patch_common/ShortTypes.h>
#include <xlog/xlog.h>
#include "../rf/common.h"
#include "../rf/player.h"
#include "../rf/weapon.h"
#include "../rf/entity.h"
#include "../rf/multi.h"
#include "../rf/geometry.h"
#include "../console/console.h"
#include "../main.h"

bool should_swap_weapon_alt_fire(rf::Player* player)
{
    if (!g_game_config.swap_assault_rifle_controls) {
        return false;
    }

    auto entity = rf::entity_from_handle(player->entity_handle);
    if (!entity) {
        return false;
    }

    // Check if local entity is attached to a parent (vehicle or torret)
    auto parent = rf::entity_from_handle(entity->host_handle);
    if (parent) {
        return false;
    }

    static auto& assault_rifle_cls_id = addr_as_ref<int>(0x00872470);
    return entity->ai.current_primary_weapon == assault_rifle_cls_id;
}

bool is_player_weapon_on(rf::Player* player, bool alt_fire) {
    if (!player) {
        player = rf::local_player;
    }
    auto entity = rf::entity_from_handle(player->entity_handle);
    bool is_continous_fire = rf::entity_weapon_is_on(entity->handle, entity->ai.current_primary_weapon);
    bool is_alt_fire_flag_set = (entity->ai.flags & 0x2000) != 0; // EWF_ALT_FIRE = 0x2000
    if (should_swap_weapon_alt_fire(player)) {
        is_alt_fire_flag_set = !is_alt_fire_flag_set;
    }
    return is_continous_fire && is_alt_fire_flag_set == alt_fire;
}

FunHook<void(rf::Player*, bool, bool)> PlayerFirePrimaryWeapon_hook{
    0x004A4E80,
    [](rf::Player* player, bool alt_fire, bool was_pressed) {
        if (should_swap_weapon_alt_fire(player)) {
            alt_fire = !alt_fire;
        }
        PlayerFirePrimaryWeapon_hook.call_target(player, alt_fire, was_pressed);
    },
};

CodeInjection stop_continous_primary_fire_patch{
    0x00430EC5,
    [](auto& regs) {
        auto entity = reinterpret_cast<rf::Entity*>(regs.esi);
        if (is_player_weapon_on(entity->local_player, false)) {
            regs.eip = 0x00430EDF;
        }
        else {
            regs.eip = 0x00430EF2;
        }
    }
};

CodeInjection stop_continous_alternate_fire_patch{
    0x00430F09,
    [](auto& regs) {
        auto entity = reinterpret_cast<rf::Entity*>(regs.esi);
        if (is_player_weapon_on(entity->local_player, true)) {
            regs.eip = 0x00430F23;
        }
        else {
            regs.eip = 0x00430F36;
        }
    }
};

ConsoleCommand2 swap_assault_rifle_controls_cmd{
    "swap_assault_rifle_controls",
    []() {
        g_game_config.swap_assault_rifle_controls = !g_game_config.swap_assault_rifle_controls;
        g_game_config.save();
        rf::console_printf("Swap assault rifle controls: %s",
                     g_game_config.swap_assault_rifle_controls ? "enabled" : "disabled");
    },
    "Swap Assault Rifle controls",
};

bool entity_is_reloading_player_select_weapon_new(rf::Entity* entity)
{
    if (rf::entity_is_reloading(entity))
        return true;

    int weapon_type = entity->ai.current_primary_weapon;
    if (weapon_type >= 0) {
        rf::WeaponInfo* weapon_cls = &rf::weapon_types[weapon_type];
        if (entity->ai.clip_ammo[weapon_type] == 0 && entity->ai.ammo[weapon_cls->ammo_type] > 0)
            return true;
    }
    return false;
}

CodeInjection weapons_tbl_buffer_overflow_fix_1{
    0x004C6855,
    [](auto& regs) {
        if (addr_as_ref<u32>(0x00872448) == 64) {
            xlog::warn("weapons.tbl limit of 64 definitions has been reached!");
            regs.eip = 0x004C6881;
        }
    },
};

CodeInjection weapons_tbl_buffer_overflow_fix_2{
    0x004C68AD,
    [](auto& regs) {
        if (addr_as_ref<u32>(0x00872448) == 64) {
            xlog::warn("weapons.tbl limit of 64 definitions has been reached!");
            regs.eip = 0x004C68D9;
        }
    },
};

FunHook<void(rf::Weapon *weapon)> weapon_move_one_hook{
    0x004C69A0,
    [](rf::Weapon* weapon) {
        weapon_move_one_hook.call_target(weapon);
        auto& level_aabb_min = rf::level.geometry->bbox_min;
        auto& level_aabb_max = rf::level.geometry->bbox_max;
        float margin = weapon->vmesh ? 275.0f : 10.0f;
        bool has_gravity_flag = weapon->p_data.flags & 1;
        bool check_y_axis = !(has_gravity_flag || weapon->info->thrust_lifetime > 0.0f);
        auto& pos = weapon->pos;
        if (pos.x < level_aabb_min.x - margin || pos.x > level_aabb_max.x + margin
        || pos.z < level_aabb_min.z - margin || pos.z > level_aabb_max.z + margin
        || (check_y_axis && (pos.y < level_aabb_min.y - margin || pos.y > level_aabb_max.y + margin))) {
            // Weapon is outside the level - delete it
            rf::obj_flag_dead(weapon);
        }
    },
};

CodeInjection weapon_vs_obj_collision_fix{
    0x0048C803,
    [](auto& regs) {
        auto obj = reinterpret_cast<rf::Object*>(regs.edi);
        auto weapon = reinterpret_cast<rf::Object*>(regs.ebp);
        auto dir = obj->pos - weapon->pos;
        // Take into account weapon and object radius
        float rad = weapon->radius + obj->radius;
        if (dir.dot_prod(weapon->orient.fvec) < -rad) {
            // Ignore this pair
            regs.eip = 0x0048C82A;
        }
        else {
            // Continue processing this pair
            regs.eip = 0x0048C834;
        }
    },
};

FunHook<void(rf::Player*, int)> player_make_weapon_current_selection_hook{
    0x004A4980,
    [](rf::Player* player, int weapon_type) {
        player_make_weapon_current_selection_hook.call_target(player, weapon_type);
        auto entity = rf::entity_from_handle(player->entity_handle);
        if (entity && rf::is_multi) {
            // Reset impact delay timers when switching weapon (except in SP because of speedrunners)
            entity->ai.create_weapon_delay_timestamps[0].invalidate();
            entity->ai.create_weapon_delay_timestamps[1].invalidate();
        }
    },
};

bool weapon_uses_ammo(int weapon_type, bool alt_fire)
{
    if (rf::weapon_is_detonator(weapon_type)) {
         return false;
    }
    if (rf::weapon_is_riot_stick(weapon_type) && alt_fire) {
        return true;
    }
    auto info = &rf::weapon_types[weapon_type];
    if (info->flags & rf::WTF_MELEE) {
        return false;
    }
    return true;
}

bool is_entity_out_of_ammo(rf::Entity *entity, int weapon_type, bool alt_fire)
{
    if (!weapon_uses_ammo(weapon_type, alt_fire)) {
        return false;
    }
    auto info = &rf::weapon_types[weapon_type];
    if (info->clip_size == 0) {
        auto ammo = entity->ai.ammo[info->ammo_type];
        return ammo == 0;
    }
    else {
        auto clip_ammo = entity->ai.clip_ammo[weapon_type];
        return clip_ammo == 0;
    }
}

FunHook<void(rf::Entity*, int, rf::Vector3*, rf::Matrix3*, bool)> multi_process_remote_weapon_fire_hook{
    0x0047D220,
    [](rf::Entity *entity, int weapon_type, rf::Vector3 *pos, rf::Matrix3 *orient, bool alt_fire) {
        if (entity->ai.current_primary_weapon != weapon_type) {
            xlog::info("Weapon mismatch when processing weapon fire packet");
            auto player = rf::player_from_entity_handle(entity->handle);
            rf::player_make_weapon_current_selection(player, weapon_type);
        }

        if (rf::is_server && is_entity_out_of_ammo(entity, weapon_type, alt_fire)) {
            xlog::info("Skipping weapon fire packet because player is out of ammunition");
        }
        else {
            multi_process_remote_weapon_fire_hook.call_target(entity, weapon_type, pos, orient, alt_fire);
        }
    },
};

CodeInjection process_obj_update_packet_check_if_weapon_is_possessed_patch{
    0x0047E404,
    [](auto& regs) {
        auto entity = reinterpret_cast<rf::Entity*>(regs.edi);
        auto weapon_type = regs.ebx;
        if (rf::is_server && !rf::ai_has_weapon(&entity->ai, weapon_type)) {
            // skip switching player weapon
            xlog::info("Skipping weapon switch because player does not possess the weapon");
            regs.eip = 0x0047E467;
        }
    },
};

CodeInjection muzzle_flash_light_not_disabled_fix{
    0x0041E806,
    [](auto& regs) {
        auto muzzle_flash_timer = addr_as_ref<rf::Timestamp>(regs.ecx);
        if (!muzzle_flash_timer.valid()) {
            regs.eip = 0x0041E969;
        }
    },
};

CallHook<void(rf::Player* player, int weapon_type)> process_create_entity_packet_switch_weapon_fix{
    0x004756B7,
    [](rf::Player* player, int weapon_type) {
        process_create_entity_packet_switch_weapon_fix.call_target(player, weapon_type);
        // Check if local player is being spawned
        if (!rf::is_server && player == rf::local_player) {
            // Update requested weapon to make sure server does not auto-switch the weapon during item pickup
            rf::multi_set_next_weapon(weapon_type);
        }
    },
};

ConsoleCommand2 show_enemy_bullets_cmd{
    "show_enemy_bullets",
    []() {
        g_game_config.show_enemy_bullets = !g_game_config.show_enemy_bullets;
        g_game_config.save();
        rf::hide_enemy_bullets = !g_game_config.show_enemy_bullets;
        rf::console_printf("Enemy bullets are %s", g_game_config.show_enemy_bullets ? "enabled" : "disabled");
    },
    "Toggles enemy bullets visibility",
};

CallHook<void(rf::Vector3&, float, float, int, int)> weapon_hit_wall_obj_apply_radius_damage_hook{
    0x004C53A8,
    [](rf::Vector3& epicenter, float damage, float radius, int killer_handle, int damage_type) {
        auto& collide_out = *reinterpret_cast<rf::PCollisionOut*>(&epicenter);
        auto new_epicenter = epicenter + collide_out.normal * 0.000001f;
        weapon_hit_wall_obj_apply_radius_damage_hook.call_target(new_epicenter, damage, radius, killer_handle, damage_type);
    },
};

void apply_weapon_patches()
{
    // Fix crashes caused by too many records in weapons.tbl file
    weapons_tbl_buffer_overflow_fix_1.install();
    weapons_tbl_buffer_overflow_fix_2.install();

#if 0
    // Fix weapon switch glitch when reloading (should be used on Match Mode)
    AsmWriter(0x004A4B4B).call(entity_is_reloading_player_select_weapon_new);
    AsmWriter(0x004A4B77).call(entity_is_reloading_player_select_weapon_new);
#endif

    // Delete weapons (projectiles) that reach bounding box of the level
    weapon_move_one_hook.install();

    // Fix weapon vs object collisions for big objects
    weapon_vs_obj_collision_fix.install();

    // Reset impact delay timers when switching weapon to avoid delayed fire after switching
    player_make_weapon_current_selection_hook.install();

    // Check ammo server-side when handling weapon fire packets
    multi_process_remote_weapon_fire_hook.install();

    // Verify if player possesses a weapon before switching during obj_update packet handling
    process_obj_update_packet_check_if_weapon_is_possessed_patch.install();

    // Fix muzzle flash light sometimes not getting disabled (e.g. when weapon is switched during riot stick attack
    // in multiplayer)
    muzzle_flash_light_not_disabled_fix.install();

    // Fix weapon being auto-switched to previous one after respawn even when auto-switch is disabled
    process_create_entity_packet_switch_weapon_fix.install();

    // Allow swapping Assault Rifle primary and alternate fire controls
    PlayerFirePrimaryWeapon_hook.install();
    stop_continous_primary_fire_patch.install();
    stop_continous_alternate_fire_patch.install();
    swap_assault_rifle_controls_cmd.register_cmd();

    // Show enemy bullets
    rf::hide_enemy_bullets = !g_game_config.show_enemy_bullets;
    show_enemy_bullets_cmd.register_cmd();

    // Fix rockets not making damage after hitting a detail brush
    weapon_hit_wall_obj_apply_radius_damage_hook.install();
}
