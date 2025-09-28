#include <patch_common/CodeInjection.h>
#include <patch_common/FunHook.h>
#include <patch_common/CallHook.h>
#include <patch_common/AsmWriter.h>
#include <xlog/xlog.h>
#include "../os/console.h"
#include "../rf/entity.h"
#include "../rf/corpse.h"
#include "../rf/weapon.h"
#include "../rf/player/player.h"
#include "../rf/particle_emitter.h"
#include "../rf/os/frametime.h"
#include "../rf/sound/sound.h"
#include "../rf/debris.h"
#include "../rf/misc.h"
#include "../main/main.h"
#include "../multi/multi.h"

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
        rf::Corpse* corpse = rf::corpse_from_handle(corpse_handle);
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

CodeInjection entity_process_post_hidden_injection{
    0x0041E4C8,
    [](auto& regs) {
        rf::Entity* ep = regs.esi;
        // Make sure move sound is muted
        if (ep->move_sound_handle != -1) {
            rf::snd_change_3d(ep->move_sound_handle, ep->pos, rf::zero_vector, 0.0f);
        }
    },
};

CodeInjection entity_render_weapon_in_hands_silencer_visibility_injection{
    0x00421D39,
    [](auto& regs) {
        rf::Entity* ep = regs.esi;
        if (!rf::weapon_is_glock(ep->ai.current_primary_weapon)) {
            regs.eip = 0x00421D3F;
        }
    },
};

CodeInjection waypoints_read_lists_oob_fix{
    0x00468E54,
    [](auto& regs) {
        constexpr int max_waypoint_lists = 32;
        int& num_waypoint_lists = addr_as_ref<int>(0x0064E398);
        int index = regs.eax;
        int num_lists = regs.ecx;
        if (index >= max_waypoint_lists && index < num_lists) {
            xlog::error("Too many waypoint lists (limit is {})! Overwritting the last list.", max_waypoint_lists);
            // reduce count by one and keep index unchanged
            --num_waypoint_lists;
            regs.ecx = num_waypoint_lists;
            // skip EBP update to fix OOB write
            regs.eip = 0x00468E5B;
        }
    },
};

CodeInjection waypoints_read_nodes_oob_fix{
    0x00468DB1,
    [](auto& regs) {
        constexpr int max_waypoint_list_nodes = 128;
        int node_index = regs.eax + 1;
        int& num_nodes = *static_cast<int*>(regs.ebp);
        if (node_index >= max_waypoint_list_nodes && node_index < num_nodes) {
            xlog::error("Too many waypoint list nodes (limit is {})! Overwritting the last endpoint.", max_waypoint_list_nodes);
            // reduce count by one and keep node index unchanged
            --num_nodes;
            // Set EAX and ECX based on skipped instructions but do not update EBX to fix OOB write
            regs.eax = node_index - 1;
            regs.ecx = num_nodes;
            regs.eip = 0x00468DB8;
        }
    },
};

CodeInjection entity_fire_update_all_freeze_fix{
    0x0042EF31,
    [](auto& regs) {
        void* fire = regs.esi;
        void* next_fire = regs.ebp;
        if (fire == next_fire) {
            // only one object was on the list and it got deleted so exit the loop
            regs.eip = 0x0042F2AF;
        } else {
            // go to the next object
            regs.esi = next_fire;
        }
    },
};

CodeInjection entity_process_pre_hide_riot_shield_injection{
    0x0041DAFF,
    [](auto& regs) {
        rf::Entity* ep = regs.esi;
        int hidden = regs.eax;
        if (hidden) {
            auto shield = rf::obj_from_handle(ep->riot_shield_handle);
            if (shield) {
                rf::obj_hide(shield);
            }
        }
    },
};

constexpr int gore_setting_gibs = 2;

static FunHook entity_blood_throw_gibs_hook{
    0x0042E3C0,
    [](int handle) {
        static const char *gib_chunk_filenames[] = {
            "meatchunk1.v3d",
            "meatchunk2.v3d",
            "meatchunk3.v3d",
            "meatchunk4.v3d",
            "meatchunk5.v3d",
        };
        auto& gib_impact_sound_set = addr_as_ref<rf::ImpactSoundSet*>(0x0062F75C);

        if (rf::player_gore_setting < gore_setting_gibs) {
            return;
        }

        constexpr int eif_ambient = 0x800000;
        rf::Entity* ep = rf::entity_from_handle(handle);
        if (ep && ep->info->flags & eif_ambient) {
            // bats and fish
            return;
        }

        constexpr int flash_material = 3;
        rf::Object* objp = rf::obj_from_handle(handle);

        if (objp && (objp->type == rf::OT_ENTITY || objp->type == rf::OT_CORPSE) && objp->material == flash_material) {
            rf::DebrisCreateStruct dcs{};
            for (int i = 0; i < 10; ++i) {
                dcs.pos = objp->pos;
                dcs.vel.rand_quick();
                dcs.vel *= 10.0; // PS2 demo uses 15
                dcs.vel += objp->p_data.vel * 0.5;
                dcs.orient.rand_quick();
                dcs.spin.rand_quick();
                float scale = rf::fl_rand_range(10.0, 25.0);
                dcs.spin *= scale;
                dcs.lifespan_ms = 5000; // 5 sec. PS2 demo uses 2.7 sec.
                dcs.material = flash_material;
                dcs.explosion_index = -1;
                dcs.debris_flags = 4;
                dcs.obj_flags = 0x8000; // OF_START_HIDDEN
                dcs.iss = gib_impact_sound_set;
                int file_index = rand() % std::size(gib_chunk_filenames);
                auto dp = rf::debris_create(objp->handle, gib_chunk_filenames[file_index], 0.3, &dcs, 0, -1.0);
                if (dp) {
                    dp->obj_flags |= rf::OF_INVULNERABLE;
                }
            }
        }
    },
};

static FunHook<float(rf::Entity*, float, int, int, int)> entity_damage_hook{
    0x0041A350,
    [](rf::Entity *damaged_ep, float damage, int killer_handle, int damage_type, int killer_uid) {
        float result = entity_damage_hook.call_target(damaged_ep, damage, killer_handle, damage_type, killer_uid);
        if (
            rf::player_gore_setting >= gore_setting_gibs
            && damaged_ep->life < -100.0 // high damage taken
            && damaged_ep->material == 3 // flash material
            && (damage_type == 3 || damage_type == 9) // armor piercing bullet or scalding
            && damaged_ep->radius < 2.0 // make sure only small entities use gibs
        ) {
            damaged_ep->entity_flags = damaged_ep->entity_flags | 0x80; // throw gibs
        }
        return result;
    },
};

CallHook<void(rf::Entity&)> entity_update_muzzle_flash_light_hook{
    0x0041E814,
    [] (rf::Entity& ep) {
        const bool remote_force_muzzle_flash = get_af_server_info().has_value()
            && !get_af_server_info().value().allow_no_mf;
        if (g_game_config.muzzle_flash || remote_force_muzzle_flash) {
            entity_update_muzzle_flash_light_hook.call_target(ep);
        }
    },
};

ConsoleCommand2 muzzle_flash_cmd{
    "muzzle_flash",
    []() {
        g_game_config.muzzle_flash = !g_game_config.muzzle_flash;
        g_game_config.save();
        rf::console::print("Muzzle flash lights are {}", g_game_config.muzzle_flash ? "enabled" : "disabled");
    },
    "Toggle muzzle flash dynamic lights",
};

ConsoleCommand2 gibs_cmd{
    "gibs",
    []() {
        rf::player_gore_setting = rf::player_gore_setting == gore_setting_gibs ? 1 : gore_setting_gibs;
        rf::console::print("Gibs are {}", rf::player_gore_setting == gore_setting_gibs ? "enabled" : "disabled");
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

    // Fix move sound not being muted if entity is created hidden (example: jeep in L18S3)
    entity_process_post_hidden_injection.install();

    // Do not show glock with silencer in 3rd person view if current primary weapon is not a glock
    entity_render_weapon_in_hands_silencer_visibility_injection.install();

    // Fix OOB writes in waypoint list read code
    waypoints_read_lists_oob_fix.install();
    waypoints_read_nodes_oob_fix.install();

    // Fix possible freeze when burning entity is destroyed
    entity_fire_update_all_freeze_fix.install();

    // Hide riot shield third person model if entity is hidden (e.g. in cutscenes)
    entity_process_pre_hide_riot_shield_injection.install();

    // Add gibs support
    entity_blood_throw_gibs_hook.install();
    entity_damage_hook.install();

    // Don't create muzzle flash lights
    entity_update_muzzle_flash_light_hook.install();

    // Commands
    gibs_cmd.register_cmd();
    muzzle_flash_cmd.register_cmd();
}
