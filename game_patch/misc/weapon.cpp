#include <patch_common/FunHook.h>
#include <patch_common/CallHook.h>
#include <patch_common/CodeInjection.h>
#include "../rf/common.h"
#include "../rf/player.h"
#include "../rf/weapon.h"
#include "../rf/entity.h"
#include "../rf/network.h"
#include "../console/console.h"
#include "../main.h"

bool IsHoldingAssaultRifle()
{
    static auto& assault_rifle_cls_id = AddrAsRef<int>(0x00872470);
    rf::EntityObj* entity = rf::EntityGetFromHandle(rf::local_player->entity_handle);
    return entity && entity->ai_info.weapon_cls_id == assault_rifle_cls_id;
}

FunHook<void(rf::Player*, bool, bool)> PlayerLocalFireControl_hook{
    0x004A4E80,
    [](rf::Player* player, bool secondary, bool was_pressed) {
        if (g_game_config.swap_assault_rifle_controls && IsHoldingAssaultRifle())
            secondary = !secondary;
        PlayerLocalFireControl_hook.CallTarget(player, secondary, was_pressed);
    },
};

extern CallHook<bool(rf::ControlConfig*, rf::GameCtrl, bool*)> IsEntityCtrlActive_hook1;
bool IsEntityCtrlActive_New(rf::ControlConfig* control_config, rf::GameCtrl game_ctrl, bool* was_pressed)
{
    if (g_game_config.swap_assault_rifle_controls && IsHoldingAssaultRifle()) {
        if (game_ctrl == rf::GC_PRIMARY_ATTACK)
            game_ctrl = rf::GC_SECONDARY_ATTACK;
        else if (game_ctrl == rf::GC_SECONDARY_ATTACK)
            game_ctrl = rf::GC_PRIMARY_ATTACK;
    }
    return IsEntityCtrlActive_hook1.CallTarget(control_config, game_ctrl, was_pressed);
}
CallHook<bool(rf::ControlConfig*, rf::GameCtrl, bool*)> IsEntityCtrlActive_hook1{0x00430E65, IsEntityCtrlActive_New};
CallHook<bool(rf::ControlConfig*, rf::GameCtrl, bool*)> IsEntityCtrlActive_hook2{0x00430EF7, IsEntityCtrlActive_New};

DcCommand2 swap_assault_rifle_controls_cmd{
    "swap_assault_rifle_controls",
    []() {
        g_game_config.swap_assault_rifle_controls = !g_game_config.swap_assault_rifle_controls;
        g_game_config.save();
        rf::DcPrintf("Swap assault rifle controls: %s",
                     g_game_config.swap_assault_rifle_controls ? "enabled" : "disabled");
    },
    "Swap Assault Rifle controls",
};

bool EntityIsReloading_SwitchWeapon_New(rf::EntityObj* entity)
{
    if (rf::EntityIsReloading(entity))
        return true;

    int weapon_cls_id = entity->ai_info.weapon_cls_id;
    if (weapon_cls_id >= 0) {
        rf::WeaponClass* weapon_cls = &rf::weapon_classes[weapon_cls_id];
        if (entity->ai_info.clip_ammo[weapon_cls_id] == 0 && entity->ai_info.ammo[weapon_cls->ammo_type] > 0)
            return true;
    }
    return false;
}

CodeInjection weapons_tbl_buffer_overflow_fix_1{
    0x004C6855,
    [](auto& regs) {
        if (AddrAsRef<u32>(0x00872448) == 64) {
            WARN("weapons.tbl limit of 64 definitions has been reached!");
            regs.eip = 0x004C6881;
        }
    },
};

CodeInjection weapons_tbl_buffer_overflow_fix_2{
    0x004C68AD,
    [](auto& regs) {
        if (AddrAsRef<u32>(0x00872448) == 64) {
            WARN("weapons.tbl limit of 64 definitions has been reached!");
            regs.eip = 0x004C68D9;
        }
    },
};

FunHook<void(rf::WeaponObj *weapon)> WeaponMoveOne_hook{
    0x004C69A0,
    [](rf::WeaponObj* weapon) {
        WeaponMoveOne_hook.CallTarget(weapon);
        auto& level_aabb_min = StructFieldRef<rf::Vector3>(rf::rfl_static_geometry, 0x48);
        auto& level_aabb_max = StructFieldRef<rf::Vector3>(rf::rfl_static_geometry, 0x54);
        float margin = weapon->_super.anim_mesh ? 275.0f : 10.0f;
        bool has_gravity_flag = weapon->_super.phys_info.flags & 1;
        bool check_y_axis = !(has_gravity_flag || weapon->weapon_cls->thrust_lifetime > 0.0f);
        auto& pos = weapon->_super.pos;
        if (pos.x < level_aabb_min.x - margin || pos.x > level_aabb_max.x + margin
        || pos.z < level_aabb_min.z - margin || pos.z > level_aabb_max.z + margin
        || (check_y_axis && (pos.y < level_aabb_min.y - margin || pos.y > level_aabb_max.y + margin))) {
            // Weapon is outside the level - delete it
            rf::ObjQueueDelete(&weapon->_super);
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
        if (dir.DotProd(weapon->orient.n.fvec) < -rad) {
            // Ignore this pair
            regs.eip = 0x0048C82A;
        }
        else {
            // Continue processing this pair
            regs.eip = 0x0048C834;
        }
    },
};

FunHook<void(rf::Player*, int)> PlayerSwitchWeaponInstant_hook{
    0x004A4980,
    [](rf::Player* player, int weapon_cls_id) {
        PlayerSwitchWeaponInstant_hook.CallTarget(player, weapon_cls_id);
        auto entity = rf::EntityGetFromHandle(player->entity_handle);
        if (entity && rf::is_net_game) {
            // Reset impact delay timers when switching weapon (except in SP because of speedrunners)
            entity->ai_info.impact_delay_timer[0].Unset();
            entity->ai_info.impact_delay_timer[1].Unset();
        }
    },
};

bool WeaponClsUsesAmmo(int weapon_cls_id, bool alt_fire)
{
    if (rf::WeaponClsIsDetonator(weapon_cls_id)) {
         return false;
    }
    if (rf::WeaponClsIsRiotStick(weapon_cls_id) && alt_fire) {
        return true;
    }
    auto weapon_cls = &rf::weapon_classes[weapon_cls_id];
    if (weapon_cls->flags & rf::WTF_MELEE) {
        return false;
    }
    return true;
}

bool IsEntityOutOfAmmo(rf::EntityObj *entity, int weapon_cls_id, bool alt_fire)
{
    if (!WeaponClsUsesAmmo(weapon_cls_id, alt_fire)) {
        return false;
    }
    auto weapon_cls = &rf::weapon_classes[weapon_cls_id];
    if (weapon_cls->clip_size == 0) {
        auto ammo = entity->ai_info.ammo[weapon_cls->ammo_type];
        return ammo == 0;
    }
    else {
        auto clip_ammo = entity->ai_info.clip_ammo[weapon_cls_id];
        return clip_ammo == 0;
    }
}

FunHook<void(rf::EntityObj*, int, rf::Vector3*, rf::Matrix3*, bool)> MultiProcessRemoteWeaponFire_hook{
    0x0047D220,
    [](rf::EntityObj *entity, int weapon_cls_id, rf::Vector3 *pos, rf::Matrix3 *orient, bool alt_fire) {
        if (entity->ai_info.weapon_cls_id != weapon_cls_id) {
            INFO("Weapon mismatch when processing weapon fire packet");
            auto player = rf::GetPlayerFromEntityHandle(entity->_super.handle);
            rf::PlayerSwitchWeaponInstant(player, weapon_cls_id);
        }

        if (rf::is_local_net_game && IsEntityOutOfAmmo(entity, weapon_cls_id, alt_fire)) {
            INFO("Skipping weapon fire packet because player is out of ammunition");
        }
        else {
            MultiProcessRemoteWeaponFire_hook.CallTarget(entity, weapon_cls_id, pos, orient, alt_fire);
        }
    },
};

CodeInjection ProcessObjUpdatePacket_check_if_weapon_is_possessed_patch{
    0x0047E404,
    [](auto& regs) {
        auto entity = reinterpret_cast<rf::EntityObj*>(regs.edi);
        auto weapon_cls_id = regs.ebx;
        if (rf::is_local_net_game && !rf::AiPossessesWeapon(&entity->ai_info, weapon_cls_id)) {
            // skip switching player weapon
            INFO("Skipping weapon switch because player does not possess the weapon");
            regs.eip = 0x0047E467;
        }
    },
};

CodeInjection muzzle_flash_light_not_disabled_fix{
    0x0041E806,
    [](auto& regs) {
        auto muzzle_flash_timer = AddrAsRef<rf::Timer>(regs.ecx);
        if (!muzzle_flash_timer.IsSet()) {
            regs.eip = 0x0041E969;
        }
    },
};

CallHook<void(rf::Player* player, int weapon_cls_id)> ProcessCreateEntityPacket_switch_weapon_fix{
    0x004756B7,
    [](rf::Player* player, int weapon_cls_id) {
        ProcessCreateEntityPacket_switch_weapon_fix.CallTarget(player, weapon_cls_id);
        // Check if local player is being spawned
        if (!rf::is_local_net_game && player == rf::local_player) {
            // Update requested weapon to make sure server does not auto-switch the weapon during item pickup
            rf::MultiSetRequestedWeapon(weapon_cls_id);
        }
    },
};

DcCommand2 show_enemy_bullets_cmd{
    "show_enemy_bullets",
    []() {
        g_game_config.show_enemy_bullets = !g_game_config.show_enemy_bullets;
        g_game_config.save();
        rf::hide_enemy_bullets = !g_game_config.show_enemy_bullets;
        rf::DcPrintf("Enemy bullets are %s", g_game_config.show_enemy_bullets ? "enabled" : "disabled");
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
    PlayerSwitchWeaponInstant_hook.Install();

    // Check ammo server-side when handling weapon fire packets
    MultiProcessRemoteWeaponFire_hook.Install();

    // Verify if player possesses a weapon before switching during obj_update packet handling
    ProcessObjUpdatePacket_check_if_weapon_is_possessed_patch.Install();

    // Fix muzzle flash light sometimes not getting disabled (e.g. when weapon is switched during riot stick attack
    // in multiplayer)
    muzzle_flash_light_not_disabled_fix.Install();

    // Fix weapon being auto-switched to previous one after respawn even when auto-switch is disabled
    ProcessCreateEntityPacket_switch_weapon_fix.Install();

    // Swap Assault Rifle fire controls
    PlayerLocalFireControl_hook.Install();
    IsEntityCtrlActive_hook1.Install();
    IsEntityCtrlActive_hook2.Install();
    swap_assault_rifle_controls_cmd.Register();

    // Show enemy bullets
    rf::hide_enemy_bullets = !g_game_config.show_enemy_bullets;
    show_enemy_bullets_cmd.Register();
}
