#include <patch_common/CodeInjection.h>
#include <patch_common/CallHook.h>
#include <patch_common/FunHook.h>
#include <patch_common/AsmWriter.h>
#include <xlog/xlog.h>
#include <cassert>
#include <unordered_map>
#include <unordered_set>
#include "../rf/object.h"
#include "../rf/event.h"
#include "../rf/entity.h"
#include "../rf/multi.h"
#include "../rf/level.h"
#include "../rf/file/file.h"
#include "../rf/player/player.h"
#include "../rf/os/console.h"
#include "../os/console.h"
#include "../misc/level.h"

class EventNameMapper {
    std::unordered_map<std::string, int> event_name_map;

public:
    int register_event(std::string_view name)
    {
        int index = static_cast<int>(event_name_map.size());
        auto name_lower = string_to_lower(name);
        event_name_map.emplace(name_lower, index);
        return index;
    }

    void force_event_mapping(std::string_view name, int index)
    {
        auto name_lower = string_to_lower(name);
        event_name_map[name_lower] = index;
    }

    std::optional<int> find(std::string_view name)
    {
        auto name_lower = string_to_lower(name);
        auto it = event_name_map.find(name_lower);
        if (it == event_name_map.end()) {
            return {};
        }
        return {it->second};
    }
};

bool event_debug_enabled;
EventNameMapper event_name_mapper;

const char* custom_event_names[] = {
    "AF_Teleport_Player",
};

enum EventType : int {
    Teleport = 4,
    Teleport_Player = 0x3F,
    PlayVclip = 0x46,
    Alarm = 0x2E,
    // There are 90 builtin events so start from 0x5A (90)
    AF_Teleport_Player = 0x5A,
    Clone_Entity,
    Anchor_Marker_Orient,
};

CodeInjection switch_model_event_custom_mesh_patch{
    0x004BB921,
    [](auto& regs) {
        auto& mesh_type = regs.ebx;
        if (mesh_type != rf::MESH_TYPE_UNINITIALIZED) {
            return;
        }
        auto& mesh_name = *static_cast<rf::String*>(regs.esi);
        std::string_view mesh_name_sv{mesh_name.c_str()};
        if (string_ends_with_ignore_case(mesh_name_sv, ".v3m")) {
            mesh_type = rf::MESH_TYPE_STATIC;
        }
        else if (string_ends_with_ignore_case(mesh_name_sv, ".v3c")) {
            mesh_type = rf::MESH_TYPE_CHARACTER;
        }
        else if (string_ends_with_ignore_case(mesh_name_sv, ".vfx")) {
            mesh_type = rf::MESH_TYPE_ANIM_FX;
        }
    },
};

CodeInjection switch_model_event_obj_lighting_and_physics_fix{
    0x004BB940,
    [](auto& regs) {
        rf::Object* obj = regs.edi;
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
            int num_vmesh_cspheres = rf::vmesh_get_num_cspheres(obj->vmesh);
            for (int i = 0; i < num_vmesh_cspheres; ++i) {
                rf::PCollisionSphere csphere;
                rf::vmesh_get_csphere(obj->vmesh, i, &csphere.center, &csphere.radius);
                csphere.spring_const = -1.0f;
                oci.spheres.add(csphere);
            }
            rf::physics_delete_object(&obj->p_data);
            rf::physics_create_object(&obj->p_data, &oci);
        }
    },
};

struct EventSetLiquidDepthHook : rf::Event
{
    float depth;
    float duration;
};

void __fastcall EventSetLiquidDepth_turn_on_new(EventSetLiquidDepthHook* this_)
{
    xlog::info("Processing Set_Liquid_Depth event: uid {} depth {:.2f} duration {:.2f}", this_->uid, this_->depth, this_->duration);
    if (this_->links.size() == 0) {
        xlog::trace("no links");
        rf::add_liquid_depth_update(this_->room, this_->depth, this_->duration);
    }
    else {
        for (auto room_uid : this_->links) {
            rf::GRoom* room = rf::level_room_from_uid(room_uid);
            xlog::trace("link {} {}", room_uid, room);
            if (room) {
                rf::add_liquid_depth_update(room, this_->depth, this_->duration);
            }
        }
    }
}

extern CallHook<void __fastcall (rf::GRoom*, int, rf::GSolid*)> liquid_depth_update_apply_all_GRoom_reset_liquid_hook;

void __fastcall liquid_depth_update_apply_all_GRoom_reset_liquid(rf::GRoom* room, int edx, rf::GSolid* solid) {
    liquid_depth_update_apply_all_GRoom_reset_liquid_hook.call_target(room, edx, solid);

    // check objects in room if they are in water
    auto* objp = rf::object_list.next_obj;
    while (objp != &rf::object_list) {
        if (objp->room == room) {
            if (objp->type == rf::OT_ENTITY) {
                auto* ep = static_cast<rf::Entity*>(objp);
                rf::entity_update_liquid_status(ep);
                bool is_in_liquid = ep->obj_flags & rf::OF_IN_LIQUID;
                // check if entity doesn't have 'swim' flag
                if (is_in_liquid && !rf::entity_can_swim(ep)) {
                    // he does not have swim animation - kill him
                    objp->life = 0.0f;
                }
            }
            else {
                rf::obj_update_liquid_status(objp);
            }
        }
        objp = objp->next_obj;
    }
}

CallHook<void __fastcall (rf::GRoom* room, int edx, rf::GSolid* geo)> liquid_depth_update_apply_all_GRoom_reset_liquid_hook{
    0x0045E4AC,
    liquid_depth_update_apply_all_GRoom_reset_liquid,
};

CallHook<int(rf::AiPathInfo*)> ai_path_release_on_load_level_event_crash_fix{
    0x004BBD99,
    [](rf::AiPathInfo* pathp) {
        // Clear GPathNode pointers before level load
        pathp->adjacent_node1 = nullptr;
        pathp->adjacent_node2 = nullptr;
        return ai_path_release_on_load_level_event_crash_fix.call_target(pathp);
    },
};

FunHook<void()> event_level_init_post_hook{
    0x004BD890,
    []() {
        event_level_init_post_hook.call_target();
        if (string_equals_ignore_case(rf::level.filename, "L5S2.rfl")) {
            // HACKFIX: make Set_Liquid_Depth events properties in lava control room more sensible
            xlog::trace("Changing Set_Liquid_Depth events in this level...");
            auto* event1 = static_cast<EventSetLiquidDepthHook*>(rf::event_lookup_from_uid(3940));
            auto* event2 = static_cast<EventSetLiquidDepthHook*>(rf::event_lookup_from_uid(4132));
            if (event1 && event2 && event1->duration == 0.15f && event2->duration == 0.15f) {
                event1->duration = 1.5f;
                event2->duration = 1.5f;
            }
        }
        if (string_equals_ignore_case(rf::level.filename, "L5S3.rfl")) {
            // Fix submarine exploding - change delay of two events to make submarine physics enabled later
            xlog::trace("Fixing Submarine exploding bug...");
            int uids[] = {4679, 4680};
            for (int uid : uids) {
                auto* event = rf::event_lookup_from_uid(uid);
                if (event && event->delay_seconds == 1.5f) {
                    event->delay_seconds += 1.5f;
                }
            }
        }
    },
};

extern FunHook<void __fastcall(rf::Event *)> EventMessage__turn_on_hook;
void __fastcall EventMessage__turn_on_new(rf::Event *this_)
{
    if (!rf::is_dedicated_server) EventMessage__turn_on_hook.call_target(this_);
}
FunHook<void __fastcall(rf::Event *this_)> EventMessage__turn_on_hook{
    0x004BB210,
    EventMessage__turn_on_new,
};

CodeInjection event_activate_injection{
    0x004B8BF4,
    [](auto& regs) {
        if (event_debug_enabled) {
            rf::Event* event = regs.esi;
            bool on = addr_as_ref<bool>(regs.esp + 0xC + 0xC);
            rf::console::print("Processing {} message in event {} ({})",
            on ? "ON" : "OFF", event->name, event->uid);
        }
    },
};

CodeInjection event_activate_injection2{
    0x004B8BE3,
    [](auto& regs) {
        if (event_debug_enabled) {
            rf::Event* event = regs.esi;
            bool on = regs.cl;
            rf::console::print("Delaying {} message in event {} ({})",
                on ? "ON" : "OFF", event->name, event->uid);
        }
    },
};

CodeInjection event_process_injection{
    0x004B8CF5,
    [](auto& regs) {
        if (event_debug_enabled) {
            rf::Event* event = regs.esi;
            rf::console::print("Processing {} message in event {} ({}) (delayed)",
                event->delayed_msg ? "ON" : "OFF", event->name, event->uid);
        }
    },
};

CodeInjection event_load_level_turn_on_injection{
    0x004BB9C9,
    [](auto& regs) {
        if (rf::local_player->flags & (rf::PF_KILL_AFTER_BLACKOUT|rf::PF_END_LEVEL_AFTER_BLACKOUT)) {
            // Ignore level transition if the player was going to die or game was going to end after a blackout effect
            regs.eip = 0x004BBA71;
        }
    }
};

FunHook event_lookup_type_hook{
    0x004BD700,
    [] (const rf::String& name) {
        const std::optional<int> id = event_name_mapper.find(name);
        if (!id) {
            xlog::warn("Unsupported event class: {}", name);
            g_level_has_unsupported_event_classes = true;  
            return -1;
        }
        return id.value();
    },
};

CodeInjection level_read_events_injection_orient{
    0x00462327,
    [](auto& regs) {
        int event_type = regs.ebp;
        rf::File* file = regs.edi;
        auto& orient = addr_as_ref<rf::Matrix3>(regs.esp + 0x98 - 0x30);

        int orient_ver = 0;
        switch (event_type) {
            case EventType::Teleport:
            case EventType::Teleport_Player:
            case EventType::PlayVclip:
                orient_ver = 145;
                break;
            case EventType::Alarm:
                orient_ver = 152;
                break;
            case EventType::AF_Teleport_Player:
            case EventType::Clone_Entity:
                orient_ver = 300;
                break;
            case EventType::Anchor_Marker_Orient:
                orient_ver = 301;
                break;
        }
        if (orient_ver) {
            orient = file->read_matrix(orient_ver);
        }

        auto color = file->read<std::uint32_t>(0xB0);

        // If this is a unknown event type in a level with an unsupported version
        // try to determine if color is actually the first value in a rotation matrix
        if (file->check_version(301) && event_type == -1) {
            float color_fl = 0.0f;
            std::memcpy(&color_fl, &color, sizeof(color_fl));
            // Rotation matrix elements are always between -1.0 and +1.0
            if (color_fl >= -1.0f && color_fl <= 1.0f) {
                // It is most likely a rotation matrix, so skip it together with the real color field
                xlog::warn("Skipping unexpected rotation matrix in event data");
                file->seek(sizeof(rf::Matrix3), rf::File::SeekOrigin::seek_cur);
            }
        }

        regs.eip = 0x0046237F;
    },
    false,
};

ConsoleCommand2 debug_event_msg_cmd{
    "debug_event_msg",
    []() {
        event_debug_enabled = !event_debug_enabled;
    }
};

CodeInjection trigger_activate_linked_objects_activate_event_in_multi_patch{
    0x004C038C,
    [] (auto& regs) {
        if (rf::level.version >= 300) {
            const rf::Object* const obj = regs.esi;
            const rf::Event* const event = static_cast<const rf::Event*>(obj);
            static const std::unordered_set blocked_event_types{{
                rf::EventType::Load_Level,
                rf::EventType::Drop_Point_Marker,
                rf::EventType::Go_Undercover,
                rf::EventType::Win_PS2_Demo,
                rf::EventType::Endgame,
                rf::EventType::Defuse_Nuke,
                rf::EventType::Play_Video,
            }};
            // RF does not allow triggers to activate events in multiplayer.
            // In AF levels, we allow it, unless it is an event that would be 
            // problematic in multiplayer. 
            const rf::EventType event_type = static_cast<rf::EventType>(event->event_type);
            if (!blocked_event_types.contains(event_type)) {
                // Allow activation.
                regs.eip = 0x004C03C2;
            } else {
                // Do not allow activation.
                regs.eip = 0x004C03D3;
            }
        }
    }
};

void event_process_handle_delay(rf::Event* const event) {
    if (rf::level.version >= 300 && event->delay_timestamp.elapsed()) {
        if (event_debug_enabled) {
            rf::console::print(
                "Processing {} message in event {} ({}) (delayed)",
                event->delayed_msg ? "ON" : "OFF",
                event->name,
                event->uid
            );
        }

        const rf::EventType event_type = static_cast<rf::EventType>(event->event_type);
        if (event->delayed_msg) {
            static const std::unordered_map<rf::EventType, uintptr_t> handlers{{
                { rf::EventType::UnHide, 0x004BCDD0 },
                { rf::EventType::Alarm_Siren, 0x004BA830 },
                { rf::EventType::Make_Invulnerable, 0x004BC7C0 },
                { rf::EventType::Cyclic_Timer, 0x004BB7A0 },
                { rf::EventType::Play_Sound, 0x004BA440 },
            }};
            if (const auto pair = handlers.find(event_type); pair != handlers.end()) {
                using Event_TurnOn = void(__thiscall)(rf::Event*);
                const auto& event_turn_on = addr_as_ref<Event_TurnOn>(pair->second);
                event_turn_on(event);
            } else {
                // xlog::error("{} turned on", event->uid);
            }
        } else {
            static const std::unordered_map<rf::EventType, uintptr_t> handlers{{
                { rf::EventType::UnHide, 0x004BCDE0 },
                { rf::EventType::Alarm_Siren, 0x004BA8C0 },
                { rf::EventType::Make_Invulnerable, 0x004BC880 },
                { rf::EventType::Cyclic_Timer, 0x004BB8A0 },
                { rf::EventType::Play_Sound, 0x004BA690 },
            }};
            if (const auto pair = handlers.find(event_type); pair != handlers.end()) {
                using Event_TurnOff = void(__thiscall)(rf::Event*);
                const auto& event_turn_off = addr_as_ref<Event_TurnOff>(pair->second);
                event_turn_off(event);
            } else {
                // xlog::error("{} turned off", event->uid);
            }
        }

        if (rf::event_type_forwards_messages(event_type)) {
            using Event_ActivateLinks = void(__thiscall)(rf::Event*, int, int, bool);
            const auto& event_activate_links = addr_as_ref<Event_ActivateLinks>(0x004B8B00);
            event_activate_links(
                event,
                event->trigger_handle,
                event->triggered_by_handle,
                event->delayed_msg
            );
        }

        event->delay_timestamp.invalidate();
    }
}

CodeInjection EventUnhide__process_patch {
    0x004BCDF0,
    [] (const auto& regs) {
        rf::Event* const event = regs.ecx;
        event_process_handle_delay(event);
    }
};

CodeInjection EventAlarmSiren__process_patch {
    0x004BA880,
    [] (const auto& regs) {
        rf::Event* const event = regs.ecx;
        event_process_handle_delay(event);
    }
};

CodeInjection EventMakeInvulnerable__process_patch {
    0x004BC8F0,
    [] (const auto& regs) {
        rf::Event* const event = regs.ecx;
        event_process_handle_delay(event);
    }
};

CodeInjection EventCyclicTimer__process_patch {
    0x004BB7B0,
    [] (const auto& regs) {
        rf::Event* const event = regs.ecx;
        event_process_handle_delay(event);
    }
};

CodeInjection EventPlaySound__process_patch {
    0x004BA570,
    [] (const auto& regs) {
        rf::Event* const event = regs.ecx;
        event_process_handle_delay(event);
    }
};

CodeInjection event_type_forwards_messages_patch{
    0x004B8C44,
    [] (auto& regs) {
        if (rf::level.version >= 300) {
            const rf::EventType event_type = static_cast<rf::EventType>(regs.eax);
            if (event_type == rf::EventType::Cyclic_Timer
                    && !AlpineLevelProps::instance().legacy_cyclic_timers) {
                // Do not forward messages.
                regs.al = false;
                regs.eip = 0x004B8C5D;
            }
        }
    }
};

void apply_event_patches()
{
    event_type_forwards_messages_patch.install();

    // In AF levels, fix events that are broken, if `delay_timestamp` is set.
    EventUnhide__process_patch.install();
    EventAlarmSiren__process_patch.install();
    EventMakeInvulnerable__process_patch.install();
    EventCyclicTimer__process_patch.install();
    EventPlaySound__process_patch.install();

    // In AF levels, allow triggers to activate events in multiplayer.
    trigger_activate_linked_objects_activate_event_in_multi_patch.install();

    // Allow custom mesh (not used in clutter.tbl or items.tbl) in Switch_Model event
    switch_model_event_custom_mesh_patch.install();
    switch_model_event_obj_lighting_and_physics_fix.install();

    // Fix Set_Liquid_Depth event
    AsmWriter(0x004BCBE0).jmp(&EventSetLiquidDepth_turn_on_new);
    liquid_depth_update_apply_all_GRoom_reset_liquid_hook.install();

    // Fix crash after level change (Load_Level event) caused by GNavNode pointers in AiPathInfo not being cleared for entities
    // being taken from the previous level
    ai_path_release_on_load_level_event_crash_fix.install();

    // Fix Message event crash on dedicated server
    EventMessage__turn_on_hook.install();

    // Level specific event fixes
    event_level_init_post_hook.install();

    // Debug event messages
    event_activate_injection.install();
    event_activate_injection2.install();
    event_process_injection.install();

    // Do not load next level if blackout is in progress
    event_load_level_turn_on_injection.install();

    // Support new event classes
    event_lookup_type_hook.install();
    for (auto& n : rf::event_names) {
        event_name_mapper.register_event(n);
    }
    for (auto& n : custom_event_names) {
        event_name_mapper.register_event(n);
    }
    // Temporarly handle AF_Teleport_Player as Teleport_Player
    event_name_mapper.force_event_mapping("AF_Teleport_Player", {EventType::Teleport_Player});
    level_read_events_injection_orient.install();

    // Register commands
    debug_event_msg_cmd.register_cmd();
}
