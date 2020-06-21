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
            mesh_type = 1;
        }
        else if (StringEndsWithIgnoreCase(mesh_name_sv, ".v3c")) {
            mesh_type = 2;
        }
        else if (StringEndsWithIgnoreCase(mesh_name_sv, ".vfx")) {
            mesh_type = 3;
        }
    },
};

CodeInjection switch_model_event_obj_lighting_and_physics_fix{
    0x004BB940,
    [](auto& regs) {
        auto obj = reinterpret_cast<rf::Object*>(regs.edi);
        obj->mesh_lighting_data = nullptr;
        // Try to fix physics
        assert(obj->phys_info.colliders.Size() >= 1);
        auto& csphere = obj->phys_info.colliders.Get(0);
        csphere.center = rf::Vector3(.0f, .0f, .0f);
        csphere.radius = obj->radius + 1000.0f;
        auto PhysUpdateSizeFromColliders = AddrAsRef<void(rf::PhysicsInfo& pi)>(0x004A0CB0);
        PhysUpdateSizeFromColliders(obj->phys_info);
    },
};

struct EventSetLiquidDepthHook : rf::EventObj
{
    float depth;
    float duration;
};

void __fastcall EventSetLiquidDepth__HandleOnMsg_New(EventSetLiquidDepthHook* this_)
{
    auto AddLiquidDepthUpdate =
        AddrAsRef<void(rf::RflRoom* room, float target_liquid_depth, float duration)>(0x0045E640);
    auto RoomGetByUid = AddrAsRef<rf::RflRoom*(int uid)>(0x0045E7C0);

    xlog::info("Processing Set_Liquid_Depth event: uid %d depth %.2f duration %.2f", this_->uid, this_->depth, this_->duration);
    if (this_->links.Size() == 0) {
        xlog::trace("no links");
        AddLiquidDepthUpdate(this_->room, this_->depth, this_->duration);
    }
    else {
        for (int i = 0; i < this_->links.Size(); ++i) {
            auto room_uid = this_->links.Get(i);
            auto room = RoomGetByUid(room_uid);
            xlog::trace("link %d %p", room_uid, room);
            if (room) {
                AddLiquidDepthUpdate(room, this_->depth, this_->duration);
            }
        }
    }
}

extern CallHook<void __fastcall (rf::RflRoom* room, int edx, void* geo)> RflRoom_SetupLiquidRoom_EventSetLiquid_hook;

void __fastcall RflRoom_SetupLiquidRoom_EventSetLiquid(rf::RflRoom* room, int edx, void* geo) {
    RflRoom_SetupLiquidRoom_EventSetLiquid_hook.CallTarget(room, edx, geo);

    auto EntityCheckIsInLiquid = AddrAsRef<void(rf::EntityObj* entity)>(0x00429100);
    auto ObjCheckIsInLiquid = AddrAsRef<void(rf::Object* obj)>(0x00486C30);
    auto EntityCanSwim = AddrAsRef<bool(rf::EntityObj* entity)>(0x00427FF0);

    // check objects in room if they are in water
    auto& object_list = AddrAsRef<rf::Object>(0x0073D880);
    auto obj = object_list.next_obj;
    while (obj != &object_list) {
        if (obj->room == room) {
            if (obj->type == rf::OT_ENTITY) {
                auto entity = reinterpret_cast<rf::EntityObj*>(obj);
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

CallHook<void __fastcall (rf::RflRoom* room, int edx, void* geo)> RflRoom_SetupLiquidRoom_EventSetLiquid_hook{
    0x0045E4AC,
    RflRoom_SetupLiquidRoom_EventSetLiquid,
};

CallHook<int(void*)> AiNavClear_on_load_level_event_crash_fix{
    0x004BBD99,
    [](void* nav) {
        // Clear NavPoint pointers before level load
        StructFieldRef<void*>(nav, 0x114) = nullptr;
        StructFieldRef<void*>(nav, 0x118) = nullptr;
        return AiNavClear_on_load_level_event_crash_fix.CallTarget(nav);
    },
};

void DoLevelSpecificEventHacks(const char* level_filename)
{
    if (_stricmp(level_filename, "L5S2.rfl") == 0) {
        // HACKFIX: make Set_Liquid_Depth events properties in lava control room more sensible
        auto event1 = reinterpret_cast<EventSetLiquidDepthHook*>(rf::EventGetByUid(3940));
        auto event2 = reinterpret_cast<EventSetLiquidDepthHook*>(rf::EventGetByUid(4132));
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
    RflRoom_SetupLiquidRoom_EventSetLiquid_hook.Install();

    // Fix crash after level change (Load_Level event) caused by NavPoint pointers in AiNav not being cleared for entities
    // being taken from the previous level
    AiNavClear_on_load_level_event_crash_fix.Install();
}
