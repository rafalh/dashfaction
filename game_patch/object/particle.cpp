#include <patch_common/FunHook.h>
#include <patch_common/AsmOpcodes.h>
#include <patch_common/CodeInjection.h>
#include <xlog/xlog.h>
#include "../rf/particle_emitter.h"
#include "../rf/geometry.h"
#include "../rf/multi.h"

FunHook<rf::ParticleEmitter*(int, rf::ParticleEmitterType&, rf::GRoom*, rf::Vector3&, bool)> particle_emitter_create_hook{
    0x00497CA0,
    [](int objh, rf::ParticleEmitterType& type, rf::GRoom* room, rf::Vector3& pos, bool is_on) {
        float min_spawn_delay = 1.0f / 60.0f;
        if ((type.flags & rf::PEF_CONTINOUS) || type.min_spawn_delay < min_spawn_delay) {
            xlog::debug("Particle emitter spawn delay is too small: %.2f. Increasing to %.2f...", type.min_spawn_delay, min_spawn_delay);
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
        auto& vel = addr_as_ref<rf::Vector3>(regs.ecx);
        if (vel.len() < 0.0001f) {
            regs.eip = 0x00495301;
        }
    },
};

FunHook<void(int, rf::ParticleCreateInfo&, rf::GRoom*, rf::Vector3*, int, rf::Particle**, rf::ParticleEmitter*)> particle_create_hook{
    0x00496840,
    [](int pool_id, rf::ParticleCreateInfo& pci, rf::GRoom* room, rf::Vector3 *a4, int parent_obj, rf::Particle** result, rf::ParticleEmitter* emitter) {
        bool damages_flag = pci.flags2 & 1;
        // Do not create not damaging particles on a dedicated server
        if (damages_flag || !rf::is_dedicated_server) {
            particle_create_hook.call_target(pool_id, pci, room, a4, parent_obj, result, emitter);
        }
    },
};

void particle_do_patch()
{
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
}
