#include <patch_common/CodeInjection.h>
#include <patch_common/CallHook.h>
#include <patch_common/AsmWriter.h>
#include <xlog/xlog.h>
#include <cassert>
#include <cstring>
#include "../rf/common.h"
#include "../rf/object.h"
#include "../rf/event.h"
#include "../rf/entity.h"
#include "../graphics/graphics.h"

CodeInjection switch_model_event_custom_mesh_patch{
    0x004BB921,
    [](auto& regs) {
        auto& mesh_type = regs.ebx;
        if (mesh_type) {
            return;
        }
        auto& mesh_name = *reinterpret_cast<rf::String*>(regs.esi);
        std::string_view mesh_name_sv{mesh_name.CStr()};
        if (StringEndsWithIgnoreCase(mesh_name_sv, ".v3m")) {
            mesh_type = rf::MESH_TYPE_STATIC;
        }
        else if (StringEndsWithIgnoreCase(mesh_name_sv, ".v3c")) {
            mesh_type = rf::MESH_TYPE_CHARACTER;
        }
        else if (StringEndsWithIgnoreCase(mesh_name_sv, ".vfx")) {
            mesh_type = rf::MESH_TYPE_ANIM_FX;
        }
    },
};

CodeInjection switch_model_event_obj_lighting_and_physics_fix{
    0x004BB940,
    [](auto& regs) {
        auto obj = reinterpret_cast<rf::Object*>(regs.edi);
        obj->mesh_lighting_data = nullptr;
        // Fix physics
        if (obj->vmesh) {
            rf::ObjectCreateInfo oci;
            oci.pos = obj->p_data.pos;
            oci.orient = obj->p_data.orient;
            oci.material = obj->material;
            oci.drag = obj->p_data.drag;
            oci.mass = obj->p_data.mass;
            oci.physics_flags = obj->p_data.flags;
            oci.radius = obj->radius;
            oci.vel = obj->p_data.vel;
            int num_vmesh_cspheres = rf::VMeshGetNumCSpheres(obj->vmesh);
            for (int i = 0; i < num_vmesh_cspheres; ++i) {
                rf::PCollisionSphere csphere;
                rf::VMeshGetCSphere(obj->vmesh, i, &csphere.center, &csphere.radius);
                csphere.spring_const = -1.0f;
                rf::VArray_PCollisionSphere__Add(&oci.spheres, csphere);
            }
            rf::PhysicsDeleteObject(&obj->p_data);
            rf::PhysicsCreateObject(&obj->p_data, &oci);

            if (obj->type == rf::OT_CLUTTER) {
                // Note: ObjDeleteMesh frees mesh_lighting_data
                ObjMeshLightingAllocOne(obj);
                ObjMeshLightingUpdateOne(obj);
            }
        }
    },
};

struct EventSetLiquidDepthHook : rf::Event
{
    float depth;
    float duration;
};

void __fastcall EventSetLiquidDepth__HandleOnMsg_New(EventSetLiquidDepthHook* this_)
{
    auto AddLiquidDepthUpdate =
        AddrAsRef<void(rf::GRoom* room, float target_liquid_depth, float duration)>(0x0045E640);
    auto RoomLookupFromUid = AddrAsRef<rf::GRoom*(int uid)>(0x0045E7C0);

    xlog::info("Processing Set_Liquid_Depth event: uid %d depth %.2f duration %.2f", this_->uid, this_->depth, this_->duration);
    if (this_->links.Size() == 0) {
        xlog::trace("no links");
        AddLiquidDepthUpdate(this_->room, this_->depth, this_->duration);
    }
    else {
        for (int i = 0; i < this_->links.Size(); ++i) {
            auto room_uid = this_->links.Get(i);
            auto room = RoomLookupFromUid(room_uid);
            xlog::trace("link %d %p", room_uid, room);
            if (room) {
                AddLiquidDepthUpdate(room, this_->depth, this_->duration);
            }
        }
    }
}

extern CallHook<void __fastcall (rf::GRoom* room, int edx, rf::GSolid* geo)> GRoom_SetupLiquidRoom_EventSetLiquid_hook;

void __fastcall GRoom_SetupLiquidRoom_EventSetLiquid(rf::GRoom* room, int edx, rf::GSolid* geo) {
    GRoom_SetupLiquidRoom_EventSetLiquid_hook.CallTarget(room, edx, geo);

    auto EntityCheckIsInLiquid = AddrAsRef<void(rf::Entity* entity)>(0x00429100);
    auto ObjCheckIsInLiquid = AddrAsRef<void(rf::Object* obj)>(0x00486C30);
    auto EntityCanSwim = AddrAsRef<bool(rf::Entity* entity)>(0x00427FF0);

    // check objects in room if they are in water
    auto& object_list = AddrAsRef<rf::Object>(0x0073D880);
    auto obj = object_list.next_obj;
    while (obj != &object_list) {
        if (obj->room == room) {
            if (obj->type == rf::OT_ENTITY) {
                auto entity = reinterpret_cast<rf::Entity*>(obj);
                EntityCheckIsInLiquid(entity);
                bool is_in_liquid = entity->obj_flags & 0x80000;
                // check if entity doesn't have 'swim' flag
                if (is_in_liquid && !EntityCanSwim(entity)) {
                    // he does not have swim animation - kill him
                    obj->life = 0.0f;
                }
            }
            else {
                ObjCheckIsInLiquid(obj);
            }
        }
        obj = obj->next_obj;
    }
}

CallHook<void __fastcall (rf::GRoom* room, int edx, rf::GSolid* geo)> GRoom_SetupLiquidRoom_EventSetLiquid_hook{
    0x0045E4AC,
    GRoom_SetupLiquidRoom_EventSetLiquid,
};

CallHook<int(rf::AiPathInfo*)> AiPathRelease_on_load_level_event_crash_fix{
    0x004BBD99,
    [](rf::AiPathInfo* pathp) {
        // Clear GNavNode pointers before level load
        StructFieldRef<void*>(pathp, 0x114) = nullptr;
        StructFieldRef<void*>(pathp, 0x118) = nullptr;
        return AiPathRelease_on_load_level_event_crash_fix.CallTarget(pathp);
    },
};

void DoLevelSpecificEventHacks(const char* level_filename)
{
    if (_stricmp(level_filename, "L5S2.rfl") == 0) {
        // HACKFIX: make Set_Liquid_Depth events properties in lava control room more sensible
        auto event1 = reinterpret_cast<EventSetLiquidDepthHook*>(rf::EventLookupFromUid(3940));
        auto event2 = reinterpret_cast<EventSetLiquidDepthHook*>(rf::EventLookupFromUid(4132));
        if (event1 && event2 && event1->duration == 0.15f && event2->duration == 0.15f) {
            event1->duration = 1.5f;
            event2->duration = 1.5f;
        }
    }
}

void ApplyEventPatches()
{
    // Allow custom mesh (not used in clutter.tbl or items.tbl) in Switch_Model event
    switch_model_event_custom_mesh_patch.Install();
    switch_model_event_obj_lighting_and_physics_fix.Install();

    // Fix Set_Liquid_Depth event
    AsmWriter(0x004BCBE0).jmp(&EventSetLiquidDepth__HandleOnMsg_New);
    GRoom_SetupLiquidRoom_EventSetLiquid_hook.Install();

    // Fix crash after level change (Load_Level event) caused by GNavNode pointers in AiPathInfo not being cleared for entities
    // being taken from the previous level
    AiPathRelease_on_load_level_event_crash_fix.Install();
}
