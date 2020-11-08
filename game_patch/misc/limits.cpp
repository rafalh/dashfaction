#include <patch_common/StaticBufferResizePatch.h>
#include <patch_common/AsmWriter.h>
#include <patch_common/CodeInjection.h>
#include "../rf/object.h"
#include "../rf/network.h"
#include "misc.h"

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

void SimObjArrayAddHook(rf::Object* obj)
{
    g_sim_obj_array.objects[g_sim_obj_array.count++] = obj;
}

CodeInjection obj_create_find_slot_patch{
    0x00486DFC,
    [](auto& regs) {
        auto obj_type = regs.ebx;

        static int low_index_hint = 0;
        static int high_index_hint = old_obj_limit;
        auto objects = obj_ptr_array_resize_patch.GetBuffer();

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

void ApplyLimitsPatches()
{
    // Change object limit
    //obj_free_slot_buffer_resize_patch.Install();
    obj_ptr_array_resize_patch.Install();
    obj_multi_handle_mapping_resize_patch.Install();
    WriteMem<u32>(0x0040A0F0 + 1, obj_limit);
    WriteMem<u32>(0x0047D8C1 + 1, obj_limit);

    // Change object index allocation strategy
    obj_create_find_slot_patch.Install();
    AsmWriter(0x00486E3F, 0x00486E61).nop();
    AsmWriter(0x0048685F, 0x0048687B).nop();
    AsmWriter(0x0048687C, 0x00486895).nop();

    // Remap simulated objects array
    AsmWriter(0x00487A6B, 0x00487A74).mov(asm_regs::ecx, &g_sim_obj_array);
    AsmWriter(0x00487C02, 0x00487C0B).mov(asm_regs::ecx, &g_sim_obj_array);
    AsmWriter(0x00487AD2, 0x00487ADB).call(SimObjArrayAddHook).add(asm_regs::esp, 4);
    AsmWriter(0x00487BBA, 0x00487BC3).call(SimObjArrayAddHook).add(asm_regs::esp, 4);

    // Allow pool allocation beyond the limit
    WriteMem<u8>(0x0048B5BB, asm_opcodes::jmp_rel_short); // weapon
    WriteMem<u8>(0x0048B72B, asm_opcodes::jmp_rel_short); // debris
    WriteMem<u8>(0x0048B89B, asm_opcodes::jmp_rel_short); // corpse
    WriteMem<u8>(0x004D7EEB, asm_opcodes::jmp_rel_short); // decal poly
    WriteMem<u8>(0x004E3C5B, asm_opcodes::jmp_rel_short); // face
    WriteMem<u8>(0x004E3DEB, asm_opcodes::jmp_rel_short); // face vertex
    WriteMem<u8>(0x004F97DB, asm_opcodes::jmp_rel_short); // bbox
    WriteMem<u8>(0x004F9C5B, asm_opcodes::jmp_rel_short); // vertex
    WriteMem<u8>(0x005047AB, asm_opcodes::jmp_rel_short); // vmesh

    // Remove object type-specific limits
    AsmWriter(0x0048712A, 0x00487137).nop(); // corpse
    AsmWriter(0x00487173, 0x00487180).nop(); // debris
    AsmWriter(0x004871D9, 0x004871E9).nop(); // item
    AsmWriter(0x00487271, 0x0048727A).nop(); // weapon
}
