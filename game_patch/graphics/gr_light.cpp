#include "../rf/gr/gr.h"

bool is_find_static_lights = false;

void gr_light_use_static(bool use_static)
{
    // Enable some experimental flag that causes static lights to be included in computations
    // auto& experimental_alloc_and_lighting = addr_as_ref<bool>(0x00879AF8);
    auto& gr_light_state = addr_as_ref<int>(0x00C96874);
    // experimental_alloc_and_lighting = use_static;
    is_find_static_lights = use_static;
    // Increment light cache key to trigger cache invalidation
    gr_light_state++;
}

void gr_light_apply_patch()
{
    // Change the variable that is used to determine what light list is searched (by default is_editor
    // variable determines that). It is needed for static mesh lighting.
    write_mem_ptr(0x004D96CD + 1, &is_find_static_lights); // gr_light_find_all_in_room
    write_mem_ptr(0x004D9761 + 1, &is_find_static_lights); // gr_light_find_all_in_room
    write_mem_ptr(0x004D98B3 + 1, &is_find_static_lights); // gr_light_find_all_by_gsolid
}
