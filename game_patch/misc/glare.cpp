

#include <patch_common/FunHook.h>
#include "../rf/common.h"
#include "../rf/entity.h"
#include "../rf/misc.h"
#include "../rf/graphics.h"
#include "../rf/clutter.h"
#include "../rf/player.h"
#include "../rf/glare.h"
#include "../rf/mover.h"
#include "../rf/geometry.h"
#include "../utils/list-utils.h"

bool GlareCollideObject(rf::GlareObj* glare, rf::Object* obj, const rf::Vector3& eye_pos)
{
    if (!(obj->obj_flags & rf::OF_WAS_RENDERED)
        || !obj->anim_mesh
        || (obj == rf::local_entity && rf::local_player->camera && rf::local_player->camera->type == rf::CAM_FIRST_PERSON)
        || glare->owner_entity_unk_handle == obj->handle) {
        return false;
    }
    rf::Vector3 hit_pt;
    if (rf::CutsceneIsActive() && obj->type == rf::OT_ENTITY) {
        // Fix glares/coronas being visible through characters during cutscenes
        rf::Vector3 root_bone_pos;
        rf::EntityGetRootBonePos(static_cast<rf::EntityObj*>(obj), root_bone_pos);
        rf::Vector3 aabb_min = root_bone_pos - obj->phys_info.radius;
        rf::Vector3 aabb_max = root_bone_pos + obj->phys_info.radius;
        if (!rf::CollideBoxSegment(aabb_min, aabb_max, glare->pos, eye_pos, &hit_pt)) {
            return false;
        }
    }
    else {
        if (!rf::CollideBoxSegment(obj->phys_info.aabb_min, obj->phys_info.aabb_max, glare->pos, eye_pos, &hit_pt)) {
            return false;
        }
    }

    rf::ModelCollideInput col_in;
    rf::ModelCollideOutput col_out;
    col_in.flags = 1;
    col_in.start_pos = eye_pos;
    col_in.dir = glare->pos - eye_pos;
    col_in.mesh_pos = obj->pos;
    col_in.mesh_orient = obj->orient;
    col_in.radius = 0.0f;
    return rf::ModelCollide(obj->anim_mesh, &col_in, &col_out, true);
}

bool GlareCollideMover(rf::GlareObj* glare, rf::MoverObj* mover, const rf::Vector3& eye_pos)
{
    rf::GeometryCollideInput col_in;
    rf::GeometryCollideOutput col_out;
    col_in.start_pos = eye_pos;
    col_in.len = glare->pos - eye_pos;
    col_in.flags = 1;
    col_in.radius = 0.0f;
    col_in.geometry_pos = mover->pos;
    col_in.geometry_orient = mover->orient;
    mover->geometry->TestLineCollision(&col_in, &col_out, true);
    return col_out.num_hits != 0;
}

FunHook<bool(rf::GlareObj* glare, const rf::Vector3& eye_pos)> GlareIsInView_hook{
    0x00414E00,
    [](rf::GlareObj* glare, const rf::Vector3& eye_pos) {
        rf::GeometryCollideInput geo_collide_in;
        rf::GeometryCollideOutput geo_collide_out;
        geo_collide_in.start_pos = eye_pos;
        geo_collide_in.len = glare->pos - eye_pos;
        geo_collide_in.flags = 5;
        geo_collide_in.radius = 0.0f;

        if (glare->last_covering_face) {
            geo_collide_in.face = glare->last_covering_face;
            rf::rfl_static_geometry->TestLineCollision(&geo_collide_in, &geo_collide_out, true);
            if (geo_collide_out.num_hits != 0) {
                return false;
            }
            glare->last_covering_face = nullptr;
        }

        if (glare->last_covering_mover) {
            if (GlareCollideMover(glare, glare->last_covering_mover, eye_pos)) {
                return false;
            }
            glare->last_covering_mover = nullptr;
        }

        if (glare->last_covering_objh != -1) {
            auto obj = rf::ObjGetByHandle(glare->last_covering_objh);
            if (GlareCollideObject(glare, obj, eye_pos)) {
                return false;
            }
            glare->last_covering_objh = -1;
        }

        geo_collide_in.face = nullptr;
        rf::rfl_static_geometry->TestLineCollision(&geo_collide_in, &geo_collide_out, true);
        if (geo_collide_out.num_hits != 0) {
            glare->last_covering_face = geo_collide_out.face;
            return false;
        }

        geo_collide_in.flags = 1;
        for (auto& mover: DoublyLinkedList{rf::mover_obj_list}) {
            if (GlareCollideMover(glare, &mover, eye_pos)) {
                glare->last_covering_mover = &mover;
                return false;
            }
        }

        for (auto clutter: DoublyLinkedList{rf::clutter_obj_list}) {
            if (GlareCollideObject(glare, &clutter, eye_pos)) {
                glare->last_covering_objh = clutter.handle;
                return false;
            }
        }

        for (auto& entity: DoublyLinkedList{rf::entity_obj_list}) {
            if (GlareCollideObject(glare, &entity, eye_pos)) {
                glare->last_covering_objh = entity.handle;
                return false;
            }
        }

        return true;
    },
};

FunHook<void(rf::GlareObj*, int)> GlareRenderCorona_hook{
    0x00414860,
    [](rf::GlareObj *glare, int player_idx) {
        // check if corona is in view using dynamic radius dedicated for this effect
        // Note: object radius matches volumetric effect size and can be very large so this check helps
        // to speed up rendering
        auto& current_radius = glare->current_radius[player_idx];
        if (!rf::GrIsSphereOutsideView(glare->pos, current_radius)) {
            GlareRenderCorona_hook.CallTarget(glare, player_idx);
        }
    },
};

void ApplyGlarePatches()
{
    // Handle collisions with clutter in glare rendering
    GlareIsInView_hook.Install();

    // Corona rendering optimization
    GlareRenderCorona_hook.Install();
}
