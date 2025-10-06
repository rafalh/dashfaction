#include <patch_common/FunHook.h>
#include <patch_common/CallHook.h>
#include <patch_common/CodeInjection.h>
#include <patch_common/ShortTypes.h>
#include <xlog/xlog.h>
#include "../rf/player/player.h"
#include "../rf/weapon.h"
#include "../rf/entity.h"
#include "../rf/multi.h"
#include "../rf/level.h"
#include "../os/console.h"
#include "../main/main.h"
#include "../multi/multi.h"

bool entity_is_reloading_player_select_weapon_new(rf::Entity* entity)
{
    if (rf::entity_is_reloading(entity))
        return true;

    int weapon_type = entity->ai.current_primary_weapon;
    if (weapon_type >= 0) {
        rf::WeaponInfo& wi = rf::weapon_types[weapon_type];
        if (entity->ai.clip_ammo[weapon_type] == 0 && entity->ai.ammo[wi.ammo_type] > 0)
            return true;
    }
    return false;
}

CodeInjection weapons_tbl_buffer_overflow_fix_1{
    0x004C6855,
    [](auto& regs) {
        if (rf::num_weapon_types == 64) {
            xlog::warn("weapons.tbl limit of 64 definitions has been reached!");
            regs.eip = 0x004C6881;
        }
    },
};

CodeInjection weapons_tbl_buffer_overflow_fix_2{
    0x004C68AD,
    [](auto& regs) {
        if (rf::num_weapon_types == 64) {
            xlog::warn("weapons.tbl limit of 64 definitions has been reached!");
            regs.eip = 0x004C68D9;
        }
    },
};

FunHook<void(rf::Weapon*)> weapon_move_one_hook{
    0x004C69A0,
    [](rf::Weapon* weapon) {
        weapon_move_one_hook.call_target(weapon);
        auto& level_aabb_min = rf::level.geometry->bbox_min;
        auto& level_aabb_max = rf::level.geometry->bbox_max;
        float margin = weapon->vmesh ? 275.0f : 10.0f;
        bool has_gravity_flag = weapon->p_data.flags & 1;
        bool check_y_axis = !(has_gravity_flag || weapon->info->thrust_lifetime_seconds > 0.0f);
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
        rf::Object* obj = regs.edi;
        rf::Object* weapon = regs.ebp;
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

CodeInjection muzzle_flash_light_not_disabled_fix{
    0x0041E806,
    [](auto& regs) {
        rf::Timestamp* primary_muzzle_timestamp = regs.ecx;
        if (!primary_muzzle_timestamp->valid()) {
            regs.eip = 0x0041E969;
        }
    },
};

CallHook<void(rf::Player*, int)> process_create_entity_packet_switch_weapon_fix{
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
        rf::console::print("Enemy bullets are {}", g_game_config.show_enemy_bullets ? "enabled" : "disabled");
    },
    "Toggles enemy bullets visibility",
};

CallHook<void(rf::Vector3&, float, float, int, int)> weapon_hit_wall_obj_apply_radius_damage_hook{
    0x004C53A8,
    [](rf::Vector3& epicenter, float damage, float radius, int killer_handle, int damage_type) {
        auto& collide_out = *reinterpret_cast<rf::PCollisionOut*>(&epicenter);
        auto new_epicenter = epicenter + collide_out.hit_normal * 0.0001f;
        weapon_hit_wall_obj_apply_radius_damage_hook.call_target(new_epicenter, damage, radius, killer_handle, damage_type);
    },
};

std::optional<int> remote_fire_wait_override() {
    return get_af_remote_info().and_then([] (const AlpineFactionRemoteInfo& af_remote_info) {
        return af_remote_info.semi_auto_cooldown;
    });
}

CodeInjection player_fire_primary_weapon_semi_auto_patch{
    0x004A50BB,
    [] (auto& regs) {
        const rf::Entity* const entity = regs.esi;
        if (get_af_remote_info().has_value()
            && get_af_remote_info().value().click_limit
            && !entity->ai.next_fire_primary.elapsed()) {
            regs.eip = 0x004A58B8;
        }
    },
};

CodeInjection entity_fire_primary_weapon_semi_auto_patch{
    0x004259B8,
    [] (auto& regs) {
        // HACKFIX: Override fire wait for stock semi auto weapons.
        auto& fire_wait = regs.eax;
        const int weapon_type = regs.ebx;
        const rf::Entity* const entity = regs.esi;
        if (remote_fire_wait_override().has_value()
            && rf::obj_is_player(entity)
            && rf::weapon_is_semi_automatic(weapon_type)
            && fire_wait == 500) {
            fire_wait = remote_fire_wait_override().value();
        }
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

    // Fix muzzle flash light sometimes not getting disabled (e.g. when weapon is switched during riot stick attack
    // in multiplayer)
    muzzle_flash_light_not_disabled_fix.install();

    // Fix weapon being auto-switched to previous one after respawn even when auto-switch is disabled
    process_create_entity_packet_switch_weapon_fix.install();

    // Show enemy bullets
    rf::hide_enemy_bullets = !g_game_config.show_enemy_bullets;
    show_enemy_bullets_cmd.register_cmd();

    // Fix rockets not making damage after hitting a detail brush
    weapon_hit_wall_obj_apply_radius_damage_hook.install();

    player_fire_primary_weapon_semi_auto_patch.install();
    entity_fire_primary_weapon_semi_auto_patch.install();
}
