#include <patch_common/FunHook.h>
#include <patch_common/CallHook.h>
#include <patch_common/AsmOpcodes.h>
#include <patch_common/CodeInjection.h>
#include <patch_common/AsmWriter.h>
#include <xlog/xlog.h>
#include "../rf/particle_emitter.h"
#include "../rf/geometry.h"
#include "../rf/multi.h"

FunHook<rf::ParticleEmitter*(int, rf::ParticleEmitterType&, rf::GRoom*, rf::Vector3&, bool)>
    particle_emitter_create_hook{
        0x00497CA0,
        [](int objh, rf::ParticleEmitterType& type, rf::GRoom* room, rf::Vector3& pos, bool is_on) {
            float min_spawn_delay = 1.0f / 60.0f;
            if ((type.flags & rf::PEF_CONTINOUS) || type.min_spawn_delay < min_spawn_delay) {
                xlog::debug(
                    "Particle emitter spawn delay is too small: {:.2f}. Increasing to {:.2f}...", type.min_spawn_delay,
                    min_spawn_delay
                );
                rf::ParticleEmitterType new_type = type;
                new_type.flags &= ~rf::PEF_CONTINOUS;
                new_type.min_spawn_delay = std::max(new_type.min_spawn_delay, min_spawn_delay);
                new_type.max_spawn_delay = std::max(new_type.max_spawn_delay, min_spawn_delay);
                return particle_emitter_create_hook.call_target(objh, new_type, room, pos, is_on);
            }
            return particle_emitter_create_hook.call_target(objh, type, room, pos, is_on);
        },
    };

CodeInjection particle_update_accel_patch{
    0x00495282,
    [](auto& regs) {
        rf::Vector3* vel = regs.ecx;
        if (vel->len() < 0.0001f) {
            regs.eip = 0x00495301;
        }
    },
};

FunHook<void(int, rf::ParticleCreateInfo&, rf::GRoom*, rf::Vector3*, int, rf::Particle**, rf::ParticleEmitter*)>
    particle_create_hook{
        0x00496840,
        [](int pool_id, rf::ParticleCreateInfo& pci, rf::GRoom* room, rf::Vector3* a4, int parent_obj,
           rf::Particle** result, rf::ParticleEmitter* emitter) {
            bool damages_flag = pci.flags2 & 1;
            // Do not create not damaging particles on a dedicated server
            if (damages_flag || !rf::is_dedicated_server) {
                particle_create_hook.call_target(pool_id, pci, room, a4, parent_obj, result, emitter);
            }
        },
    };

CodeInjection particle_should_take_damage_injection{0x00494DE9, [](auto& regs) {
                                                        if (rf::is_dedicated_server) {
                                                            regs.eip = 0x00494DF8;
                                                        }
                                                    }};

CodeInjection particle_emitter_update_cull_radius_injection{
    0x00497E08,
    [](auto& regs) {
        rf::ParticleEmitter* emitter = regs.esi;
        rf::Vector3 pos, dir;
        emitter->get_pos_and_dir(&pos, &dir);

        rf::Particle* p = emitter->particle_list.next;
        float max_dist_sq = 0.0f;
        float max_pradius = 0.0;

        while (p != &emitter->particle_list) {
            float dist_sq = (p->pos - pos).len_sq();
            max_dist_sq = std::max(max_dist_sq, dist_sq);
            max_pradius = std::max(max_pradius, p->radius);
            p = p->next;
        }

        emitter->cull_radius = std::sqrt(max_dist_sq) + max_pradius;
        regs.eip = 0x00497E2E;
    },
};

CallHook<void(
    rf::ParticleEmitter*, const rf::Vector3*, const rf::Vector3*, float, void (*)(int, rf::GSolid*), bool,
    const rf::Plane*, const rf::Vector3*, const rf::Vector3*, bool, bool
)>
    particle_emitter_g_portal_object_add_hook{
        0x00497C82,
        [](rf::ParticleEmitter* emitter, const rf::Vector3* pos, const rf::Vector3* cull_pos, float radius,
           void (*render_function)(int, rf::GSolid*), bool has_alpha, const rf::Plane*, const rf::Vector3*,
           const rf::Vector3*, bool lights_enabled, bool use_static_lights) {
            // Use world_pos field to hold the pos because pos points to a local variable and g_portal_object_add saves
            // the pointer for a later use. Note that world_pos is not used anywhere else except for a code after
            // 0x00495216 that is skipped in DF.
            emitter->world_pos = *pos;
            // Use world_pos as bbox_min and bbox_max. Those fields are not used for culling, but for
            // sorting objects in respect to the liquid surface.
            // If the player is above liquid surface the engine renders first objects that are partially or fully under
            // the liquid surface, then the surface itself and ends with rendering objects that are fully above the
            // surface. If the player is below the surface the engine first renders objects partially above the surface,
            // then the surface and ends with objects fully below the surface. To determine if object is fully or
            // partially above/below the surface the engine estimates max and min Y for the object. Normally pos.y +/-
            // radius are used for that estimation, but for particle emitters this method if faulty, because they are
            // often far from a ball shape (e.g. flamethrower). Because of that flame coming from a flamethrower can be
            // rendered as underwater, when the player is close to the water surface, even if particles don't touch the
            // water. To mitigate this issue use particle emitter center point for sorting.
            particle_emitter_g_portal_object_add_hook.call_target(
                emitter, pos, cull_pos, radius, render_function, has_alpha, nullptr, &emitter->world_pos,
                &emitter->world_pos, lights_enabled, use_static_lights
            );
        },
    };

void particle_do_patch()
{
    // Fix particle damage on dedicated servers, e.g., flame thrower.
    particle_should_take_damage_injection.install();

    // Skip broken code that was supposed to skip particle emulation when particle emitter is in non-rendered room
    // RF code is broken here because level emitters have object handle set to 0 and other emitters are not added to
    // the searched list
    write_mem<uint8_t>(0x00495158, asm_opcodes::jmp_rel_short);

    // Recude particle emitters maximal spawn rate
    particle_emitter_create_hook.install();

    // Avoid doing calculations on velocity that is close to 0
    // Related to flamethrower issues on high framerate
    particle_update_accel_patch.install();

    // Do not create not damaging particles on a dedicated server
    particle_create_hook.install();

    // Fix cull radius calculation for particle emitters
    particle_emitter_update_cull_radius_injection.install();
    AsmWriter{0x00495216}.jmp(0x0049525B);

    // Improve sorting in respect to the liquid surface
    particle_emitter_g_portal_object_add_hook.install();
}
