

#include <patch_common/FunHook.h>
#include "../rf/common.h"
#include "../rf/entity.h"
#include "../rf/corpse.h"
#include "../rf/misc.h"
#include "../rf/graphics.h"
#include "../rf/clutter.h"
#include "../rf/item.h"
#include "../rf/player.h"
#include "../rf/glare.h"
#include "../rf/mover.h"
#include "../rf/geometry.h"
#include "../utils/list-utils.h"
#include "../main.h"

static std::vector<rf::Object*> g_objects_to_check;
static std::vector<rf::MoverBrush*> g_mover_brushes_to_check;

static bool GlareCollideObject(rf::Glare* glare, rf::Object* obj, const rf::Vector3& eye_pos)
{
    if (!(obj->obj_flags & rf::OF_WAS_RENDERED)
        || !obj->vmesh
        || (obj == rf::local_player_entity && rf::local_player->cam && rf::local_player->cam->mode == rf::CAM_FIRST_PERSON)
        || glare->parent_handle == obj->handle) {
        return false;
    }

    // Skip local entity vehicle
    if (rf::EntityIsAttachedToVehicle(rf::local_player_entity) && obj->handle == rf::local_player_entity->host_handle) {
        return false;
    }

    rf::Vector3 hit_pt;
    if (rf::CutsceneIsActive() && obj->type == rf::OT_ENTITY) {
        // Fix glares/coronas being visible through characters during cutscenes
        rf::Vector3 root_bone_pos;
        rf::ObjFindRootBonePos(static_cast<rf::Entity*>(obj), root_bone_pos);
        rf::Vector3 aabb_min = root_bone_pos - obj->p_data.radius;
        rf::Vector3 aabb_max = root_bone_pos + obj->p_data.radius;
        if (!rf::IxLineSegmentBoundingBox(aabb_min, aabb_max, glare->pos, eye_pos, &hit_pt)) {
            return false;
        }
    }
    else {
        if (!rf::IxLineSegmentBoundingBox(obj->p_data.bbox_min, obj->p_data.bbox_max, glare->pos, eye_pos, &hit_pt)) {
            return false;
        }
    }

    rf::VMeshCollisionInput col_in;
    rf::VMeshCollisionOutput col_out;
    col_in.flags = 1;
    col_in.start_pos = eye_pos;
    col_in.dir = glare->pos - eye_pos;
    col_in.mesh_pos = obj->pos;
    col_in.mesh_orient = obj->orient;
    col_in.radius = 0.0f;
    return rf::VMeshCollide(obj->vmesh, &col_in, &col_out, true);
}

static bool GlareCollideMoverBrush(rf::Glare* glare, rf::MoverBrush* mbp, const rf::Vector3& eye_pos)
{
    if (!(mbp->obj_flags & rf::OF_WAS_RENDERED)) {
        return false;
    }

    rf::Vector3 hit_pt;
    if (!rf::IxLineSegmentBoundingBox(mbp->p_data.bbox_min, mbp->p_data.bbox_max, glare->pos, eye_pos, &hit_pt)) {
        return false;
    }
    rf::GCollisionInput col_in;
    rf::GCollisionOutput col_out;
    col_in.start_pos = eye_pos;
    col_in.len = glare->pos - eye_pos;
    col_in.flags = 1;
    col_in.radius = 0.0f;
    col_in.geometry_pos = mbp->pos;
    col_in.geometry_orient = mbp->orient;
    mbp->geometry->Collide(&col_in, &col_out, true);
    return col_out.num_hits != 0;
}

FunHook<void(bool)> GlareRenderAllCorona_hook{
    0x004154F0,
    [](bool reflections) {
        // check if glares were disabled by vli command
        if (!g_game_config.glares) {
            return;
        }

        g_objects_to_check.clear();
        for (auto& entity: DoublyLinkedList{rf::entity_list}) {
            if (entity.obj_flags & rf::OF_WAS_RENDERED) {
                g_objects_to_check.push_back(&entity);
            }
        }

        for (auto& corpse: DoublyLinkedList{rf::corpse_list}) {
            if (corpse.obj_flags & rf::OF_WAS_RENDERED) {
                g_objects_to_check.push_back(&corpse);
            }
        }

        for (auto& clutter: DoublyLinkedList{rf::clutter_list}) {
            if (clutter.obj_flags & rf::OF_WAS_RENDERED) {
                g_objects_to_check.push_back(&clutter);
            }
        }

        for (auto& item: DoublyLinkedList{rf::item_list}) {
            if (item.obj_flags & rf::OF_WAS_RENDERED) {
                g_objects_to_check.push_back(&item);
            }
        }

        g_mover_brushes_to_check.clear();
        for (auto& mb: DoublyLinkedList{rf::mover_brush_list}) {
            if (mb.obj_flags & rf::OF_WAS_RENDERED) {
                g_mover_brushes_to_check.push_back(&mb);
            }
        }

        GlareRenderAllCorona_hook.CallTarget(reflections);
    },
};

FunHook<bool(rf::Glare* glare, const rf::Vector3& eye_pos)> GlareIsInView_hook{
    0x00414E00,
    [](rf::Glare* glare, const rf::Vector3& eye_pos) {
        rf::GCollisionInput geo_collide_in;
        rf::GCollisionOutput geo_collide_out;
        geo_collide_in.start_pos = eye_pos;
        geo_collide_in.len = glare->pos - eye_pos;
        geo_collide_in.flags = 5;
        geo_collide_in.radius = 0.0f;

        if (glare->last_covering_face) {
            geo_collide_in.face = glare->last_covering_face;
            rf::level.geometry->Collide(&geo_collide_in, &geo_collide_out, true);
            if (geo_collide_out.num_hits != 0) {
                return false;
            }
            glare->last_covering_face = nullptr;
        }

        if (glare->last_covering_mover_brush) {
            if (GlareCollideMoverBrush(glare, glare->last_covering_mover_brush, eye_pos)) {
                return false;
            }
            glare->last_covering_mover_brush = nullptr;
        }

        if (glare->last_covering_objh != -1) {
            auto obj = rf::ObjFromHandle(glare->last_covering_objh);
            if (obj && GlareCollideObject(glare, obj, eye_pos)) {
                return false;
            }
            glare->last_covering_objh = -1;
        }

        geo_collide_in.face = nullptr;
        rf::level.geometry->Collide(&geo_collide_in, &geo_collide_out, true);
        if (geo_collide_out.num_hits != 0) {
            glare->last_covering_face = geo_collide_out.face;
            return false;
        }

        for (auto mbp : g_mover_brushes_to_check) {
            if (GlareCollideMoverBrush(glare, mbp, eye_pos)) {
                glare->last_covering_mover_brush = mbp;
                return false;
            }
        }
        for (auto obj_ptr : g_objects_to_check) {
            if (GlareCollideObject(glare, obj_ptr, eye_pos)) {
                glare->last_covering_objh = obj_ptr->handle;
                return false;
            }
        }

        return true;
    },
};

FunHook<void(rf::Glare*, int)> GlareRenderCorona_hook{
    0x00414860,
    [](rf::Glare *glare, int player_idx) {
        // check if corona is in view using dynamic radius dedicated for this effect
        // Note: object radius matches volumetric effect size and can be very large so this check helps
        // to speed up rendering
        auto& current_radius = glare->last_rendered_radius[player_idx];
        if (!rf::GrCullSphere(glare->pos, current_radius)) {
            GlareRenderCorona_hook.CallTarget(glare, player_idx);
        }
    },
};

void ApplyGlarePatches()
{
    // Support disabling glares and optimize rendering
    GlareRenderAllCorona_hook.Install();

    // Handle collisions with clutter in glare rendering
    GlareIsInView_hook.Install();

    // Corona rendering optimization
    GlareRenderCorona_hook.Install();

}
