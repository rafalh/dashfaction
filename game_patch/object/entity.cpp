#include <patch_common/CodeInjection.h>
#include <patch_common/FunHook.h>
#include <patch_common/CallHook.h>
#include <patch_common/AsmWriter.h>
#include "../rf/entity.h"
#include "../rf/corpse.h"
#include "../rf/player/player.h"
#include "../rf/particle_emitter.h"
#include "../rf/os/frametime.h"
#include "../main/main.h"

rf::Timestamp g_player_jump_timestamp;

CodeInjection stuck_to_ground_when_jumping_fix{
    0x0042891E,
    [](auto& regs) {
        rf::Entity* entity = regs.esi;
        if (entity->local_player) {
            // Skip land handling code for next 64 ms (like in PF)
            g_player_jump_timestamp.set(64);
        }
    },
};

CodeInjection stuck_to_ground_when_using_jump_pad_fix{
    0x00486B60,
    [](auto& regs) {
        rf::Entity* entity = regs.esi;
        if (entity->local_player) {
            // Skip land handling code for next 64 ms
            g_player_jump_timestamp.set(64);
        }
    },
};

CodeInjection stuck_to_ground_fix{
    0x00487F82,
    [](auto& regs) {
        rf::Entity* entity = regs.esi;
        if (entity->local_player && g_player_jump_timestamp.valid() && !g_player_jump_timestamp.elapsed()) {
            // Jump to jump handling code that sets entity to falling movement mode
            regs.eip = 0x00487F7B;
        }
    },
};

CodeInjection entity_water_decelerate_fix{
    0x0049D82A,
    [](auto& regs) {
        rf::Entity* entity = regs.esi;
        float vel_factor = 1.0f - (rf::frametime * 4.5f);
        entity->p_data.vel.x *= vel_factor;
        entity->p_data.vel.y *= vel_factor;
        entity->p_data.vel.z *= vel_factor;
        regs.eip = 0x0049D835;
    },
};

FunHook<void(rf::Entity&, rf::Vector3&)> entity_on_land_hook{
    0x00419830,
    [](rf::Entity& entity, rf::Vector3& pos) {
        entity_on_land_hook.call_target(entity, pos);
        entity.p_data.vel.y = 0.0f;
    },
};

CallHook<void(rf::Entity&)> entity_make_run_after_climbing_patch{
    0x00430D5D,
    [](rf::Entity& entity) {
        entity_make_run_after_climbing_patch.call_target(entity);
        entity.p_data.vel.y = 0.0f;
    },
};

FunHook<void(rf::EntityFireInfo&, int)> entity_fire_switch_parent_to_corpse_hook{
    0x0042F510,
    [](rf::EntityFireInfo& fire_info, int corpse_handle) {
        auto corpse = rf::corpse_from_handle(corpse_handle);
        fire_info.parent_hobj = corpse_handle;
        rf::entity_fire_init_bones(&fire_info, corpse);
        for (auto& emitter_ptr : fire_info.emitters) {
            if (emitter_ptr) {
                emitter_ptr->parent_handle = corpse_handle;
            }
        }
        fire_info.time_limited = true;
        fire_info.time = 0.0f;
        corpse->corpse_flags |= 0x200;
    },
};

CallHook<bool(rf::Object*)> entity_update_liquid_status_obj_is_player_hook{
    {
        0x004292E3,
        0x0042932A,
        0x004293F4,
    },
    [](rf::Object* obj) {
        return obj == rf::local_player_entity;
    },
};

CallHook<bool(const rf::Vector3&, const rf::Vector3&, rf::PhysicsData*, rf::PCollisionOut*)> entity_maybe_stop_crouching_collide_spheres_world_hook{
    0x00428AB9,
    [](const rf::Vector3& p1, const rf::Vector3& p2, rf::PhysicsData* pd, rf::PCollisionOut* collision) {
        // Temporarly disable collisions with liquid faces
        auto collision_flags = pd->collision_flags;
        pd->collision_flags &= ~0x1000;
        bool result = entity_maybe_stop_crouching_collide_spheres_world_hook.call_target(p1, p2, pd, collision);
        pd->collision_flags = collision_flags;
        return result;
    },
};

void entity_do_patch()
{
    // Fix player being stuck to ground when jumping, especially when FPS is greater than 200
    stuck_to_ground_when_jumping_fix.install();
    stuck_to_ground_when_using_jump_pad_fix.install();
    stuck_to_ground_fix.install();

    // Fix water deceleration on high FPS
    AsmWriter(0x0049D816).nop(5);
    entity_water_decelerate_fix.install();

    // Fix flee AI mode on high FPS by avoiding clearing velocity in Y axis in EntityMakeRun
    AsmWriter(0x00428121, 0x0042812B).nop();
    AsmWriter(0x0042809F, 0x004280A9).nop();
    entity_on_land_hook.install();
    entity_make_run_after_climbing_patch.install();

    // Increase entity simulation max distance
    // TODO: create a config property for this
    if (g_game_config.disable_lod_models) {
        write_mem<float>(0x00589548, 100.0f);
    }

    // Fix crash when particle emitter allocation fails during entity ignition
    entity_fire_switch_parent_to_corpse_hook.install();

    // Fix buzzing sound when some player is floating in water
    entity_update_liquid_status_obj_is_player_hook.install();

    // Fix entity staying in crouched state after entering liquid
    entity_maybe_stop_crouching_collide_spheres_world_hook.install();

    // Use local_player variable for weapon shell distance calculation instead of local_player_entity
    // in entity_eject_shell. Fixed debris pool being exhausted when local player is dead.
    AsmWriter(0x0042A223, 0x0042A232).mov(asm_regs::ecx, {&rf::local_player});
}
