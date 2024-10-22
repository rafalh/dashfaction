#include <cassert>
#include <xlog/xlog.h>
#include <patch_common/FunHook.h>
#include <patch_common/CallHook.h>
#include <common/utils/list-utils.h>
#include "../rf/object.h"
#include "../rf/item.h"
#include "../rf/clutter.h"
#include "../rf/gr/gr.h"
#include "../rf/multi.h"
#include "../rf/crt.h"
#include "../os/console.h"
#include "../main/main.h"

void gr_light_use_static(bool use_static);

void obj_mesh_lighting_alloc_one(rf::Object *objp)
{
    // Note: ObjDeleteMesh frees mesh_lighting_data
    assert(objp->mesh_lighting_data == nullptr);
    auto size = rf::vmesh_calc_lighting_data_size(objp->vmesh);
    objp->mesh_lighting_data = rf::rf_malloc(size);
}

void obj_mesh_lighting_free_one(rf::Object *objp)
{
    if (objp->mesh_lighting_data) {
        rf::rf_free(objp->mesh_lighting_data);
        objp->mesh_lighting_data = nullptr;
    }
}

void obj_mesh_lighting_update_one(rf::Object *objp)
{
    gr_light_use_static(true);
    rf::vmesh_update_lighting_data(objp->vmesh, objp->room, objp->pos, objp->orient, objp->mesh_lighting_data);
    gr_light_use_static(false);
}

static bool obj_should_be_lit(rf::Object *objp)
{
    if (!g_game_config.mesh_static_lighting) {
        return false;
    }
    if (!objp->vmesh || rf::vmesh_get_type(objp->vmesh) != rf::MESH_TYPE_STATIC) {
        return false;
    }
    // Clutter object always use static lighting
    // Items does only in single-player. In multi-player they spins so static lighting make less sense
    return objp->type == rf::OT_CLUTTER || (objp->type == rf::OT_ITEM && !rf::is_multi);
}

void obj_mesh_lighting_maybe_update(rf::Object *objp)
{
    if (!objp->mesh_lighting_data && obj_should_be_lit(objp)) {
        obj_mesh_lighting_alloc_one(objp);
        obj_mesh_lighting_update_one(objp);
    }
}

FunHook<void()> obj_light_calculate_hook{
    0x0048B0E0,
    []() {
        xlog::trace("update_mesh_lighting_hook");
        // Init transform for lighting calculation
        rf::gr::view_matrix.make_identity();
        rf::gr::view_pos.zero();
        rf::gr::light_matrix.make_identity();
        rf::gr::light_base.zero();

        if (g_game_config.mesh_static_lighting) {
            // Enable static lights
            gr_light_use_static(true);
            // Calculate lighting for meshes now
            obj_light_calculate_hook.call_target();
            // Switch back to dynamic lights
            gr_light_use_static(false);
        }
        else {
            obj_light_calculate_hook.call_target();
        }
    },
};

FunHook<void()> obj_light_alloc_hook{
    0x0048B1D0,
    []() {
        for (auto& item: DoublyLinkedList{rf::item_list}) {
            if (item.vmesh && !(item.obj_flags & rf::OF_DELAYED_DELETE)
                && rf::vmesh_get_type(item.vmesh) == rf::MESH_TYPE_STATIC) {
                auto size = rf::vmesh_calc_lighting_data_size(item.vmesh);
                item.mesh_lighting_data = rf::rf_malloc(size);
            }
        }
        for (auto& clutter: DoublyLinkedList{rf::clutter_list}) {
            if (clutter.vmesh && !(clutter.obj_flags & rf::OF_DELAYED_DELETE)
                && rf::vmesh_get_type(clutter.vmesh) == rf::MESH_TYPE_STATIC) {
                auto size = rf::vmesh_calc_lighting_data_size(clutter.vmesh);
                clutter.mesh_lighting_data = rf::rf_malloc(size);
            }
        }
    },
};

FunHook<void()> obj_light_free_hook{
    0x0048B370,
    []() {
        for (auto& item: DoublyLinkedList{rf::item_list}) {
            rf::rf_free(item.mesh_lighting_data);
            item.mesh_lighting_data = nullptr;
        }
        for (auto& clutter: DoublyLinkedList{rf::clutter_list}) {
            rf::rf_free(clutter.mesh_lighting_data);
            clutter.mesh_lighting_data = nullptr;
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

CallHook<void(rf::Entity&)> entity_update_muzzle_flash_light_hook{
    0x0041E814,
    [](rf::Entity& ep) {
        if (g_game_config.muzzle_flash) {
            entity_update_muzzle_flash_light_hook.call_target(ep);
        }        
    },
};

ConsoleCommand2 muzzle_flash_cmd{
    "muzzle_flash",
    []() {
        g_game_config.muzzle_flash = !g_game_config.muzzle_flash;
        g_game_config.save();
        rf::console::print("Muzzle flash lights are {}", g_game_config.muzzle_flash ? "enabled" : "disabled");
    },
    "Toggle muzzle flash dynamic lights",
};

void obj_light_apply_patch()
{
    // Fix/improve items and clutters static lighting calculation: fix matrices being zero and use static lights
    obj_light_calculate_hook.install();
    obj_light_alloc_hook.install();
    obj_light_free_hook.install();

    // Fix invalid vertex offset in mesh lighting calculation
    write_mem<int8_t>(0x005042F0 + 2, sizeof(rf::Vector3));

    // Don't create muzzle flash lights
    entity_update_muzzle_flash_light_hook.install();

    // Commands
    mesh_static_lighting_cmd.register_cmd();
    muzzle_flash_cmd.register_cmd();
}
