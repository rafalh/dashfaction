#include <cassert>
#include <xlog/xlog.h>
#include <patch_common/FunHook.h>
#include <patch_common/AsmWriter.h>
#include <patch_common/CodeInjection.h>
#include <common/utils/list-utils.h>
#include "../rf/object.h"
#include "../rf/item.h"
#include "../rf/clutter.h"
#include "../rf/gr/gr.h"
#include "../rf/multi.h"
#include "../rf/crt.h"
#include "../os/console.h"
#include "../main/main.h"
#include "../multi/multi.h"

float obj_light_scale = 1.0;

void gr_light_use_static(bool use_static);

void obj_light_alloc_one(rf::Object* objp)
{
    if ((objp->type != rf::OT_ITEM && objp->type != rf::OT_CLUTTER) ||
        !objp->vmesh ||
        (objp->obj_flags & rf::OF_DELAYED_DELETE) ||
        rf::vmesh_get_type(objp->vmesh) != rf::MESH_TYPE_STATIC
    ) {
        return;
    }

    // Note: obj_delete_mesh frees mesh_lighting_data
    assert(objp->mesh_lighting_data == nullptr);

    auto size = rf::vmesh_calc_lighting_data_size(objp->vmesh);
    objp->mesh_lighting_data = rf::rf_malloc(size);
}

void obj_light_free_one(rf::Object* objp)
{
    if (objp->mesh_lighting_data) {
        rf::rf_free(objp->mesh_lighting_data);
        objp->mesh_lighting_data = nullptr;
    }
}

void obj_light_update_one(rf::Object* objp)
{
    if (!objp->mesh_lighting_data) {
        return;
    }

    bool use_static_lights = g_game_config.mesh_static_lighting;
    if (objp->type == rf::OT_ITEM && rf::is_multi) {
        // In multi-player items spin so having baked lighting makes less sense
        use_static_lights = false;
    }

    // Do not strengthen lighting when static lights are used
    // In other cases behave like the base game (00504275)
    if (use_static_lights) {
        obj_light_scale = 1.0;
    } else if (rf::is_multi) {
        obj_light_scale = 3.2;
    } else {
        obj_light_scale = 2.0;
    }

    if (use_static_lights) {
        gr_light_use_static(true);
    }
    rf::vmesh_update_lighting_data(objp->vmesh, objp->room, objp->pos, objp->orient, objp->mesh_lighting_data);
    if (use_static_lights) {
        gr_light_use_static(false);
    }
}

void obj_light_init_object(rf::Object* objp)
{
    if (!objp->mesh_lighting_data) {
        obj_light_alloc_one(objp);
        obj_light_update_one(objp);
    }
}

FunHook<void()> obj_light_calculate_hook{
    0x0048B0E0,
    []() {
        xlog::trace("obj_light_calculate_hook");
        // Init transform for lighting calculation
        rf::gr::view_matrix.make_identity();
        rf::gr::view_pos.zero();
        rf::gr::light_matrix.make_identity();
        rf::gr::light_base.zero();

        for (auto& item : DoublyLinkedList{rf::item_list}) {
            obj_light_update_one(&item);
        }
        for (auto& clutter : DoublyLinkedList{rf::clutter_list}) {
            obj_light_update_one(&clutter);
        }
    },
};

FunHook<void()> obj_light_alloc_hook{
    0x0048B1D0,
    []() {
        for (auto& item : DoublyLinkedList{rf::item_list}) {
            obj_light_alloc_one(&item);
        }
        for (auto& clutter : DoublyLinkedList{rf::clutter_list}) {
            obj_light_alloc_one(&clutter);
        }
    },
};

FunHook<void()> obj_light_free_hook{
    0x0048B370,
    []() {
        for (auto& item : DoublyLinkedList{rf::item_list}) {
            obj_light_free_one(&item);
        }
        for (auto& clutter : DoublyLinkedList{rf::clutter_list}) {
            obj_light_free_one(&clutter);
        }
    },
};

void recalc_mesh_static_lighting()
{
    rf::obj_light_free();
    rf::obj_light_alloc();
    rf::obj_light_calculate();
}

ConsoleCommand2 mesh_static_lighting_cmd{
    "mesh_static_lighting",
    []() {
        g_game_config.mesh_static_lighting = !g_game_config.mesh_static_lighting;
        g_game_config.save();
        recalc_mesh_static_lighting();
        rf::console::print("Mesh static lighting is {}", g_game_config.mesh_static_lighting ? "enabled" : "disabled");
    },
    "Toggle mesh static lighting calculation",
};

CodeInjection init_mesh_dynamic_light_data_patch{
    0x0052DBD0,
    [] (const auto& regs) {
        const bool allow_full_bright = !(get_remote_server_info()
            && !get_remote_server_info().value().allow_full_bright_entities);
        if (g_game_config.full_bright_entities && allow_full_bright) {
            // For Direct3d 11, we set in `gr_d3d11_mesh.cpp`.
            auto& ambient_light = addr_as_ref<rf::Vector3>(0x1C3D548);
            ambient_light = rf::Vector3{255.f, 255.f, 255.f};
        }
    },
};

ConsoleCommand2 full_bright_entities_cmd{
    "full_bright_entities",
    [] {
        g_game_config.full_bright_entities = !g_game_config.full_bright_entities;
        g_game_config.save();
        rf::console::print(
            "Full bright entities is {}",
            g_game_config.full_bright_entities
                ? "enabled [multiplayer servers may ignore]"
                : "disabled"
        );
    },
    "Toggle full-bright entities [multiplayer servers may ignore]",
};

void obj_light_apply_patch()
{
    // Fix/improve items and clutters static lighting calculation: fix matrices being zero and use static lights
    obj_light_calculate_hook.install();
    obj_light_alloc_hook.install();
    obj_light_free_hook.install();

    // Fix invalid vertex offset in mesh lighting calculation
    write_mem<int8_t>(0x005042F0 + 2, sizeof(rf::Vector3));

    // Allow changing light scale
    AsmWriter{0x0050426F, 0x0050428D}.mov(asm_regs::ecx, AsmRegMem{&obj_light_scale});

    // Commands
    mesh_static_lighting_cmd.register_cmd();
    full_bright_entities_cmd.register_cmd();

    // Support full-bright entities.
    init_mesh_dynamic_light_data_patch.install();
}
