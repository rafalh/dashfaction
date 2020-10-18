#include <patch_common/FunHook.h>
#include <patch_common/CallHook.h>
#include <patch_common/CodeInjection.h>
#include <patch_common/ShortTypes.h>
#include <xlog/xlog.h>
#include "../rf/common.h"
#include "../rf/player.h"
#include "../rf/weapon.h"
#include "../rf/entity.h"
#include "../rf/network.h"
#include "../rf/geometry.h"
#include "../console/console.h"
#include "../main.h"

bool ShouldSwapWeaponAltFire(rf::Player* player)
{
    if (!g_game_config.swap_assault_rifle_controls) {
        return false;
    }

    auto entity = rf::EntityFromHandle(player->entity_handle);
    if (!entity) {
        return false;
    }

    // Check if local entity is attached to a parent (vehicle or torret)
    auto parent = rf::EntityFromHandle(entity->host_handle);
    if (parent) {
        return false;
    }

    static auto& assault_rifle_cls_id = AddrAsRef<int>(0x00872470);
    return entity->ai_info.current_primary_weapon == assault_rifle_cls_id;
}

bool IsPlayerWeaponInContinousFire(rf::Player* player, bool alt_fire) {
    if (!player) {
        player = rf::local_player;
    }
    auto entity = rf::EntityFromHandle(player->entity_handle);
    bool is_continous_fire = rf::IsEntityWeaponInContinousFire(entity->handle, entity->ai_info.current_primary_weapon);
    bool is_alt_fire_flag_set = (entity->ai_info.flags & 0x2000) != 0; // EWF_ALT_FIRE = 0x2000
    if (ShouldSwapWeaponAltFire(player)) {
        is_alt_fire_flag_set = !is_alt_fire_flag_set;
    }
    return is_continous_fire && is_alt_fire_flag_set == alt_fire;
}

FunHook<void(rf::Player*, bool, bool)> PlayerLocalFireControl_hook{
    0x004A4E80,
    [](rf::Player* player, bool alt_fire, bool was_pressed) {
        if (ShouldSwapWeaponAltFire(player)) {
            alt_fire = !alt_fire;
        }
        PlayerLocalFireControl_hook.CallTarget(player, alt_fire, was_pressed);
    },
};

CodeInjection stop_continous_primary_fire_patch{
    0x00430EC5,
    [](auto& regs) {
        auto entity = reinterpret_cast<rf::Entity*>(regs.esi);
        if (IsPlayerWeaponInContinousFire(entity->local_player, false)) {
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
        if (IsPlayerWeaponInContinousFire(entity->local_player, true)) {
            regs.eip = 0x00430F23;
        }
        else {
            regs.eip = 0x00430F36;
        }
    }
};

DcCommand2 swap_assault_rifle_controls_cmd{
    "swap_assault_rifle_controls",
    []() {
        g_game_config.swap_assault_rifle_controls = !g_game_config.swap_assault_rifle_controls;
        g_game_config.save();
        rf::ConsolePrintf("Swap assault rifle controls: %s",
                     g_game_config.swap_assault_rifle_controls ? "enabled" : "disabled");
    },
    "Swap Assault Rifle controls",
};

bool EntityIsReloading_SwitchWeapon_New(rf::Entity* entity)
{
    if (rf::EntityIsReloading(entity))
        return true;

    int weapon_type = entity->ai_info.current_primary_weapon;
    if (weapon_type >= 0) {
        rf::WeaponInfo* weapon_cls = &rf::weapon_types[weapon_type];
        if (entity->ai_info.clip_ammo[weapon_type] == 0 && entity->ai_info.ammo[weapon_cls->ammo_type] > 0)
            return true;
    }
    return false;
}

CodeInjection weapons_tbl_buffer_overflow_fix_1{
    0x004C6855,
    [](auto& regs) {
        if (AddrAsRef<u32>(0x00872448) == 64) {
            xlog::warn("weapons.tbl limit of 64 definitions has been reached!");
            regs.eip = 0x004C6881;
        }
    },
};

CodeInjection weapons_tbl_buffer_overflow_fix_2{
    0x004C68AD,
    [](auto& regs) {
        if (AddrAsRef<u32>(0x00872448) == 64) {
            xlog::warn("weapons.tbl limit of 64 definitions has been reached!");
            regs.eip = 0x004C68D9;
        }
    },
};

FunHook<void(rf::Weapon *weapon)> WeaponMoveOne_hook{
    0x004C69A0,
    [](rf::Weapon* weapon) {
        WeaponMoveOne_hook.CallTarget(weapon);
        auto& level_aabb_min = rf::rfl_static_geometry->bbox_min;
        auto& level_aabb_max = rf::rfl_static_geometry->bbox_max;
        float margin = weapon->vmesh ? 275.0f : 10.0f;
        bool has_gravity_flag = weapon->p_data.flags & 1;
        bool check_y_axis = !(has_gravity_flag || weapon->weapon_cls->thrust_lifetime > 0.0f);
        auto& pos = weapon->pos;
        if (pos.x < level_aabb_min.x - margin || pos.x > level_aabb_max.x + margin
        || pos.z < level_aabb_min.z - margin || pos.z > level_aabb_max.z + margin
        || (check_y_axis && (pos.y < level_aabb_min.y - margin || pos.y > level_aabb_max.y + margin))) {
            // Weapon is outside the level - delete it
            rf::ObjQueueDelete(weapon);
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
        if (dir.DotProd(weapon->orient.fvec) < -rad) {
            // Ignore this pair
            regs.eip = 0x0048C82A;
        }
        else {
            // Continue processing this pair
            regs.eip = 0x0048C834;
        }
    },
};

FunHook<void(rf::Player*, int)> PlayerMakeWeaponCurrentSelection_hook{
    0x004A4980,
    [](rf::Player* player, int weapon_type) {
        PlayerMakeWeaponCurrentSelection_hook.CallTarget(player, weapon_type);
        auto entity = rf::EntityFromHandle(player->entity_handle);
        if (entity && rf::is_multi) {
            // Reset impact delay timers when switching weapon (except in SP because of speedrunners)
            entity->ai_info.create_weapon_delay_timestamps[0].Invalidate();
            entity->ai_info.create_weapon_delay_timestamps[1].Invalidate();
        }
    },
};

bool WeaponClsUsesAmmo(int weapon_type, bool alt_fire)
{
    if (rf::WeaponIsDetonator(weapon_type)) {
         return false;
    }
    if (rf::WeaponIsRiotStick(weapon_type) && alt_fire) {
        return true;
    }
    auto weapon_cls = &rf::weapon_types[weapon_type];
    if (weapon_cls->flags & rf::WTF_MELEE) {
        return false;
    }
    return true;
}

bool IsEntityOutOfAmmo(rf::Entity *entity, int weapon_type, bool alt_fire)
{
    if (!WeaponClsUsesAmmo(weapon_type, alt_fire)) {
        return false;
    }
    auto weapon_cls = &rf::weapon_types[weapon_type];
    if (weapon_cls->clip_size == 0) {
        auto ammo = entity->ai_info.ammo[weapon_cls->ammo_type];
        return ammo == 0;
    }
    else {
        auto clip_ammo = entity->ai_info.clip_ammo[weapon_type];
        return clip_ammo == 0;
    }
}

FunHook<void(rf::Entity*, int, rf::Vector3*, rf::Matrix3*, bool)> MultiProcessRemoteWeaponFire_hook{
    0x0047D220,
    [](rf::Entity *entity, int weapon_type, rf::Vector3 *pos, rf::Matrix3 *orient, bool alt_fire) {
        if (entity->ai_info.current_primary_weapon != weapon_type) {
            xlog::info("Weapon mismatch when processing weapon fire packet");
            auto player = rf::GetPlayerFromEntityHandle(entity->handle);
            rf::PlayerSwitchWeaponInstant(player, weapon_type);
        }

        if (rf::is_server && IsEntityOutOfAmmo(entity, weapon_type, alt_fire)) {
            xlog::info("Skipping weapon fire packet because player is out of ammunition");
        }
        else {
            MultiProcessRemoteWeaponFire_hook.CallTarget(entity, weapon_type, pos, orient, alt_fire);
        }
    },
};

CodeInjection ProcessObjUpdatePacket_check_if_weapon_is_possessed_patch{
    0x0047E404,
    [](auto& regs) {
        auto entity = reinterpret_cast<rf::Entity*>(regs.edi);
        auto weapon_type = regs.ebx;
        if (rf::is_server && !rf::AiHasWeapon(&entity->ai_info, weapon_type)) {
            // skip switching player weapon
            xlog::info("Skipping weapon switch because player does not possess the weapon");
            regs.eip = 0x0047E467;
        }
    },
};

CodeInjection muzzle_flash_light_not_disabled_fix{
    0x0041E806,
    [](auto& regs) {
        auto muzzle_flash_timer = AddrAsRef<rf::Timestamp>(regs.ecx);
        if (!muzzle_flash_timer.Valid()) {
            regs.eip = 0x0041E969;
        }
    },
};

CallHook<void(rf::Player* player, int weapon_type)> ProcessCreateEntityPacket_switch_weapon_fix{
    0x004756B7,
    [](rf::Player* player, int weapon_type) {
        ProcessCreateEntityPacket_switch_weapon_fix.CallTarget(player, weapon_type);
        // Check if local player is being spawned
        if (!rf::is_server && player == rf::local_player) {
            // Update requested weapon to make sure server does not auto-switch the weapon during item pickup
            rf::MultiSetRequestedWeapon(weapon_type);
        }
    },
};

DcCommand2 show_enemy_bullets_cmd{
    "show_enemy_bullets",
    []() {
        g_game_config.show_enemy_bullets = !g_game_config.show_enemy_bullets;
        g_game_config.save();
        rf::hide_enemy_bullets = !g_game_config.show_enemy_bullets;
        rf::ConsolePrintf("Enemy bullets are %s", g_game_config.show_enemy_bullets ? "enabled" : "disabled");
    },
    "Toggles enemy bullets visibility",
};

void ApplyWeaponPatches()
{
    // Fix crashes caused by too many records in weapons.tbl file
    weapons_tbl_buffer_overflow_fix_1.Install();
    weapons_tbl_buffer_overflow_fix_2.Install();

#if 0
    // Fix weapon switch glitch when reloading (should be used on Match Mode)
    AsmWriter(0x004A4B4B).call(EntityIsReloading_SwitchWeapon_New);
    AsmWriter(0x004A4B77).call(EntityIsReloading_SwitchWeapon_New);
#endif

    // Delete weapons (projectiles) that reach bounding box of the level
    WeaponMoveOne_hook.Install();

    // Fix weapon vs object collisions for big objects
    weapon_vs_obj_collision_fix.Install();

    // Reset impact delay timers when switching weapon to avoid delayed fire after switching
    PlayerMakeWeaponCurrentSelection_hook.Install();

    // Check ammo server-side when handling weapon fire packets
    MultiProcessRemoteWeaponFire_hook.Install();

    // Verify if player possesses a weapon before switching during obj_update packet handling
    ProcessObjUpdatePacket_check_if_weapon_is_possessed_patch.Install();

    // Fix muzzle flash light sometimes not getting disabled (e.g. when weapon is switched during riot stick attack
    // in multiplayer)
    muzzle_flash_light_not_disabled_fix.Install();

    // Fix weapon being auto-switched to previous one after respawn even when auto-switch is disabled
    ProcessCreateEntityPacket_switch_weapon_fix.Install();

    // Allow swapping Assault Rifle primary and alternate fire controls
    PlayerLocalFireControl_hook.Install();
    stop_continous_primary_fire_patch.Install();
    stop_continous_alternate_fire_patch.Install();
    swap_assault_rifle_controls_cmd.Register();

    // Show enemy bullets
    rf::hide_enemy_bullets = !g_game_config.show_enemy_bullets;
    show_enemy_bullets_cmd.Register();
}
