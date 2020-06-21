

#include <patch_common/FunHook.h>
#include <patch_common/CodeInjection.h>
#include "../rf/common.h"
#include "../rf/entity.h"
#include "../rf/misc.h"
#include "../rf/graphics.h"

CodeInjection CoronaEntityCollisionTestFix{
    0x004152F1,
    [](auto& regs) {
        auto get_entity_root_bone_pos = AddrAsRef<void(rf::EntityObj*, rf::Vector3&)>(0x48AC70);
        using IntersectLineWithAabbType = bool(rf::Vector3 * aabb1, rf::Vector3 * aabb2, rf::Vector3 * pos1,
                                               rf::Vector3 * pos2, rf::Vector3 * out_pos);
        auto& intersect_line_with_aabb = AddrAsRef<IntersectLineWithAabbType>(0x00508B70);

        rf::EntityObj* entity = reinterpret_cast<rf::EntityObj*>(regs.esi);
        if (!rf::CutsceneIsActive()) {
            return;
        }

        rf::Vector3 root_bone_pos;
        get_entity_root_bone_pos(entity, root_bone_pos);
        rf::Vector3 aabb_min = root_bone_pos - entity->phys_info.radius;
        rf::Vector3 aabb_max = root_bone_pos + entity->phys_info.radius;
        auto corona_pos = reinterpret_cast<rf::Vector3*>(regs.edi);
        auto eye_pos = reinterpret_cast<rf::Vector3*>(regs.ebx);
        auto tmp_vec = reinterpret_cast<rf::Vector3*>(regs.ecx);
        regs.eax = intersect_line_with_aabb(&aabb_min, &aabb_max, corona_pos, eye_pos, tmp_vec);
        regs.eip = 0x004152F6;
    },
};

FunHook<void(rf::Object*, int)> GlareRenderCorona_hook{
    0x00414860,
    [](rf::Object *glare, int player_idx) {
        // check if corona is in view using dynamic radius dedicated for this effect
        // Note: object radius matches volumetric effect size and can be very large so this check helps
        // to speed up rendering
        auto& current_radius = StructFieldRef<float[2]>(glare, 0x2A4);
        if (!rf::GrIsSphereOutsideView(glare->pos, current_radius[player_idx])) {
            GlareRenderCorona_hook.CallTarget(glare, player_idx);
        }
    },
};

void ApplyGlarePatches()
{
    // Fix glares/coronas being visible through characters
    CoronaEntityCollisionTestFix.Install();

    // Corona rendering optimization
    GlareRenderCorona_hook.Install();
}
