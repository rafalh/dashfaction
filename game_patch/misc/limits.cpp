#include <patch_common/StaticBufferResizePatch.h>
#include <patch_common/AsmWriter.h>
#include "../rf/object.h"
#include "misc.h"

StaticBufferResizePatch obj_free_slot_buffer_resize_patch{
    0x0073A880,
    0xC,
    old_obj_limit,
    obj_limit,
    {
        {0x00486882},
        {0x00486D18},
        {0x00486E48},
        {0x00486E55},
        {0x004875C5},
        {0x004875E6},
        {0x004875EE},
        {0x004875FD},
        {0x0048687C},
        {0x00486E42},
        {0x00486E5B},
        {0x004875BF},
        {0x004875E0},
        {0x004875F4},
        {0x00487607},
        {0x00486D31, StaticBufferResizePatch::end_ref},
    },
};

StaticBufferResizePatch obj_ptr_array_resize_patch{
    0x007394CC,
    sizeof(rf::Object*),
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
         {0x00486D70, StaticBufferResizePatch::end_ref},
         {0x0048A7A1, StaticBufferResizePatch::end_ref},
    },
};

StaticBufferResizePatch obj_multi_handle_mapping_resize_patch{
    0x006FB428,
    sizeof(int),
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

void ApplyLimitsPatches()
{
    // Change object limit
    obj_free_slot_buffer_resize_patch.Install();
    obj_ptr_array_resize_patch.Install();
    obj_multi_handle_mapping_resize_patch.Install();
    WriteMem<u32>(0x0040A0F0 + 1, obj_limit);
    WriteMem<u32>(0x0047D8C1 + 1, obj_limit);

    // Remap simulated objects array
    AsmWriter(0x00487A6B, 0x00487A74).mov(asm_regs::ecx, &g_sim_obj_array);
    AsmWriter(0x00487C02, 0x00487C0B).mov(asm_regs::ecx, &g_sim_obj_array);
    AsmWriter(0x00487AD2, 0x00487ADB).call(SimObjArrayAddHook).add(asm_regs::esp, 4);
    AsmWriter(0x00487BBA, 0x00487BC3).call(SimObjArrayAddHook).add(asm_regs::esp, 4);

    // Allow pool allocation beyond the limit
    WriteMem<u8>(0x0048B5BB + 1, asm_opcodes::jmp_rel_short); // weapon
    WriteMem<u8>(0x0048B72B + 1, asm_opcodes::jmp_rel_short); // debris
    WriteMem<u8>(0x0048B89B + 1, asm_opcodes::jmp_rel_short); // corpse
    WriteMem<u8>(0x004D7EEB + 1, asm_opcodes::jmp_rel_short); // decal poly
    WriteMem<u8>(0x004E3C5B + 1, asm_opcodes::jmp_rel_short); // face
    WriteMem<u8>(0x004E3DEB + 1, asm_opcodes::jmp_rel_short); // face vertex
    WriteMem<u8>(0x004F97DB + 1, asm_opcodes::jmp_rel_short); // bbox
    WriteMem<u8>(0x004F9C5B + 1, asm_opcodes::jmp_rel_short); // vertex
    WriteMem<u8>(0x005047AB + 1, asm_opcodes::jmp_rel_short); // vmesh

    // Change object type-specific limits
    // WriteMem<u8>(0x0048712A + 6, 127); // corpse
    // WriteMem<u8>(0x00487173 + 6, 127); // debris
    // WriteMem<u32>(0x004871D9 + 6, 1024); // item
    // WriteMem<u8>(0x00487271 + 6, 127); // weapon

    // Remove object type-specific limits
    AsmWriter(0x0048712A, 0x00487137).nop(); // corpse
    AsmWriter(0x00487173, 0x00487180).nop(); // debris
    AsmWriter(0x004871D9, 0x004871E9).nop(); // item
    AsmWriter(0x00487271, 0x0048727A).nop(); // weapon
}
