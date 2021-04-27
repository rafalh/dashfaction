#include <patch_common/FunHook.h>
#include <patch_common/CallHook.h>
#include <patch_common/CodeInjection.h>
#include <patch_common/AsmWriter.h>
#include <patch_common/StaticBufferResizePatch.h>
#include <xlog/xlog.h>
#include "../rf/object.h"
#include "../rf/item.h"
#include "../rf/clutter.h"
#include "../rf/multi.h"
#include "../rf/level.h"
#include "object.h"
#include "object_private.h"

FunHook<rf::Object*(int, int, int, rf::ObjectCreateInfo*, int, rf::GRoom*)> obj_create_hook{
    0x00486DA0,
    [](int type, int sub_type, int parent, rf::ObjectCreateInfo* create_info, int flags, rf::GRoom* room) {
        auto obj = obj_create_hook.call_target(type, sub_type, parent, create_info, flags, room);
        if (!obj) {
            xlog::info("Failed to create object (type %d)", type);
        }
        return obj;
    },
};

StaticBufferResizePatch<rf::Object*> obj_ptr_array_resize_patch{
    0x007394CC,
    old_obj_limit,
    obj_limit,
    {
         {0x0040A0F7},
         {0x004867B5},
         {0x00486854},
         {0x00486D51},
         {0x00486E38},
         {0x00487572},
         {0x0048759E},
         {0x004875B4},
         {0x0048A78A},
         {0x00486D70, true},
         {0x0048A7A1, true},
    },
};

StaticBufferResizePatch<int> obj_multi_handle_mapping_resize_patch{
    0x006FB428,
    old_obj_limit,
    obj_limit,
    {
        {0x0047D8C9},
        {0x00484B11},
        {0x00484B41},
        {0x00484B82},
        {0x00484BAE},
    },
};

static struct {
    int count;
    rf::Object* objects[obj_limit];
} g_sim_obj_array;

void sim_obj_array_add_hook(rf::Object* obj)
{
    g_sim_obj_array.objects[g_sim_obj_array.count++] = obj;
}

CodeInjection obj_create_find_slot_patch{
    0x00486DFC,
    [](auto& regs) {
        rf::ObjectType obj_type = regs.ebx;

        static int low_index_hint = 0;
        static int high_index_hint = old_obj_limit;
        auto objects = obj_ptr_array_resize_patch.get_buffer();

        int index_hint, min_index, max_index;
        bool use_low_index;
        if (rf::is_server && (obj_type == rf::OT_ENTITY || obj_type == rf::OT_ITEM)) {
            // Use low object numbers server-side for entities and items for better client compatibility
            use_low_index = true;
            index_hint = low_index_hint;
            min_index = 0;
            max_index = old_obj_limit - 1;
        }
        else {
            use_low_index = false;
            index_hint = high_index_hint;
            min_index = rf::is_server ? old_obj_limit : 0;
            max_index = obj_limit - 1;
        }

        int index = index_hint;
        while (objects[index]) {
            ++index;
            if (index > max_index) {
                index = min_index;
            }

            if (index == index_hint) {
                // failure: no free slots
                regs.eip = 0x00486E08;
                return;
            }
        }

        // success: current index is free
        xlog::trace("Using index %d for object type %d", index, obj_type);
        index_hint = index + 1;
        if (index_hint > max_index) {
            index_hint = min_index;
        }

        if (use_low_index) {
            low_index_hint = index_hint;
        }
        else {
            high_index_hint = index_hint;
        }

        regs.edi = index;
        regs.eip = 0x00486E15;
    },
};

CallHook<void*(size_t)> GPool_allocate_new_hook{
    {
        0x0048B5C6,
        0x0048B736,
        0x0048B8A6,
        0x0048BA16,
        0x004D7EF6,
        0x004E3C63,
        0x004E3DF3,
        0x004F97E3,
        0x004F9C63,
        0x005047B3,
    },
    [](size_t s) -> void* {
        void* result = GPool_allocate_new_hook.call_target(s);
        if (result) {
            // Zero memory allocated dynamically (static memory is zeroed by operating system automatically)
            std::memset(result, 0, s);
        }
        return result;
    },
};

CodeInjection sort_items_patch{
    0x004593AC,
    [](auto& regs) {
        rf::Item* item = regs.esi;
        auto vmesh = item->vmesh;
        auto mesh_name = vmesh ? rf::vmesh_get_name(vmesh) : nullptr;
        if (!mesh_name) {
            // Sometimes on level change some objects can stay and have only vmesh destroyed
            return;
        }
        std::string_view mesh_name_sv = mesh_name;

        // HACKFIX: enable alpha sorting for Invulnerability Powerup and Riot Shield
        // Note: material used for alpha-blending is flare_blue1.tga - it uses non-alpha texture
        // so information about alpha-blending cannot be taken from material alone - it must be read from VFX
        static const char* force_alpha_mesh_names[] = {
            "powerup_invuln.vfx",
            "Weapon_RiotShield.V3D",
        };
        for (auto alpha_mesh_name : force_alpha_mesh_names) {
            if (mesh_name_sv == alpha_mesh_name) {
                item->obj_flags |= rf::OF_HAS_ALPHA;
                break;
            }
        }

        rf::Item* current = rf::item_list.next;
        while (current != &rf::item_list) {
            auto current_anim_mesh = current->vmesh;
            auto current_mesh_name = current_anim_mesh ? rf::vmesh_get_name(current_anim_mesh) : nullptr;
            if (current_mesh_name && mesh_name_sv == current_mesh_name) {
                break;
            }
            current = current->next;
        }
        item->next = current;
        item->prev = current->prev;
        item->next->prev = item;
        item->prev->next = item;
        // Set up needed registers
        regs.ecx = regs.esp + 0xC0 - 0xB0; // create_info
        regs.eip = 0x004593D1;
    },
};

CodeInjection sort_clutter_patch{
    0x004109D4,
    [](auto& regs) {
        rf::Clutter* clutter = regs.esi;
        auto vmesh = clutter->vmesh;
        auto mesh_name = vmesh ? rf::vmesh_get_name(vmesh) : nullptr;
        if (!mesh_name) {
            // Sometimes on level change some objects can stay and have only vmesh destroyed
            return;
        }
        std::string_view mesh_name_sv = mesh_name;

        auto current = rf::clutter_list.next;
        while (current != &rf::clutter_list) {
            auto current_anim_mesh = current->vmesh;
            auto current_mesh_name = current_anim_mesh ? rf::vmesh_get_name(current_anim_mesh) : nullptr;
            if (current_mesh_name && mesh_name_sv == current_mesh_name) {
                break;
            }
            if (current_mesh_name && std::string_view{current_mesh_name} == "LavaTester01.v3d") {
                // HACKFIX: place LavaTester01 at the end to fix alpha draw order issues in L5S2 (Geothermal Plant)
                // Note: OF_HAS_ALPHA cannot be used because it causes another draw-order issue when lava goes up
                break;
            }
            current = current->next;
        }
        // insert before current
        clutter->next = current;
        clutter->prev = current->prev;
        clutter->next->prev = clutter;
        clutter->prev->next = clutter;
        // Set up needed registers
        regs.eax = addr_as_ref<int>(regs.esp + 0xD0 + 0x18); // killable
        regs.ecx = addr_as_ref<int>(0x005C9358) + 1; // num_clutter_objs
        regs.eip = 0x00410A03;
    },
};

FunHook<rf::VMesh*(rf::Object*, const char*, rf::VMeshType)> obj_create_mesh_hook{
    0x00489FE0,
    [](rf::Object* objp, const char* name, rf::VMeshType type) {
        auto mesh = obj_create_mesh_hook.call_target(objp, name, type);
        if (mesh && (rf::level.flags & rf::LEVEL_LOADED) != 0) {
            obj_mesh_lighting_maybe_update(objp);
        }
        return mesh;
    },
};

FunHook<void(rf::Object*)> obj_delete_mesh_hook{
    0x00489FC0,
    [](rf::Object* objp) {
        obj_delete_mesh_hook.call_target(objp);
        obj_mesh_lighting_free_one(objp);
    },
};

void object_do_patch()
{
    // Log error when object cannot be created
    obj_create_hook.install();

    // Change object limit
    //obj_free_slot_buffer_resize_patch.install();
    obj_ptr_array_resize_patch.install();
    obj_multi_handle_mapping_resize_patch.install();
    write_mem<u32>(0x0040A0F0 + 1, obj_limit);
    write_mem<u32>(0x0047D8C1 + 1, obj_limit);

    // Change object index allocation strategy
    obj_create_find_slot_patch.install();
    AsmWriter(0x00486E3F, 0x00486E61).nop();
    AsmWriter(0x0048685F, 0x0048687B).nop();
    AsmWriter(0x0048687C, 0x00486895).nop();

    // Remap simulated objects array
    AsmWriter(0x00487A6B, 0x00487A74).mov(asm_regs::ecx, &g_sim_obj_array);
    AsmWriter(0x00487C02, 0x00487C0B).mov(asm_regs::ecx, &g_sim_obj_array);
    AsmWriter(0x00487AD2, 0x00487ADB).call(sim_obj_array_add_hook).add(asm_regs::esp, 4);
    AsmWriter(0x00487BBA, 0x00487BC3).call(sim_obj_array_add_hook).add(asm_regs::esp, 4);

    // Allow pool allocation beyond the limit
    write_mem<u8>(0x0048B5BB, asm_opcodes::jmp_rel_short); // weapon
    write_mem<u8>(0x0048B72B, asm_opcodes::jmp_rel_short); // debris
    write_mem<u8>(0x0048B89B, asm_opcodes::jmp_rel_short); // corpse
    write_mem<u8>(0x004D7EEB, asm_opcodes::jmp_rel_short); // decal poly
    write_mem<u8>(0x004E3C5B, asm_opcodes::jmp_rel_short); // face
    write_mem<u8>(0x004E3DEB, asm_opcodes::jmp_rel_short); // face vertex
    write_mem<u8>(0x004F97DB, asm_opcodes::jmp_rel_short); // bbox
    write_mem<u8>(0x004F9C5B, asm_opcodes::jmp_rel_short); // vertex
    write_mem<u8>(0x005047AB, asm_opcodes::jmp_rel_short); // vmesh

    // Remove object type-specific limits
    AsmWriter(0x0048712A, 0x00487137).nop(); // corpse
    AsmWriter(0x00487173, 0x00487180).nop(); // debris
    AsmWriter(0x004871D9, 0x004871E9).nop(); // item
    AsmWriter(0x00487271, 0x0048727A).nop(); // weapon

    // Zero memory allocated from GPool dynamically
    GPool_allocate_new_hook.install();

    // Sort objects by anim mesh name to improve rendering performance
    sort_items_patch.install();
    sort_clutter_patch.install();

    // Calculate lighting when object mesh is changed
    obj_create_mesh_hook.install();
    obj_delete_mesh_hook.install();

    // Other files
    entity_do_patch();
    cutscene_apply_patches();
    apply_event_patches();
    glare_patches_patches();
    apply_weapon_patches();
    trigger_apply_patches();
    monitor_do_patch();
    particle_do_patch();
    obj_light_apply_patch();
}
