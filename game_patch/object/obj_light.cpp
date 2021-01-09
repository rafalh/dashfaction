#include <cassert>
#include <xlog/xlog.h>
#include <patch_common/FunHook.h>
#include <common/utils/list-utils.h>
#include "../rf/object.h"
#include "../rf/item.h"
#include "../rf/clutter.h"
#include "../os/console.h"
#include "../main/main.h"

void gr_light_use_static(bool use_static);

void obj_mesh_lighting_alloc_one(rf::Object *objp)
{
    // Note: ObjDeleteMesh frees mesh_lighting_data
    assert(objp->mesh_lighting_data == nullptr);
    auto size = rf::vmesh_calc_lighting_data_size(objp->vmesh);
    objp->mesh_lighting_data = rf::Malloc(size);
}

void obj_mesh_lighting_update_one(rf::Object *objp)
{
    gr_light_use_static(true);
    rf::vmesh_update_lighting_data(objp->vmesh, objp->room, objp->pos, objp->orient, objp->mesh_lighting_data);
    gr_light_use_static(false);
}

FunHook<void()> obj_mesh_lighting_update_hook{
    0x0048B0E0,
    []() {
        xlog::trace("update_mesh_lighting_hook");
        // Init transform for lighting calculation
        auto& gr_view_matrix = addr_as_ref<rf::Matrix3>(0x018186C8);
        auto& gr_view_pos = addr_as_ref<rf::Vector3>(0x01818690);
        auto& light_matrix = addr_as_ref<rf::Matrix3>(0x01818A38);
        auto& light_base = addr_as_ref<rf::Vector3>(0x01818A28);
        gr_view_matrix.make_identity();
        gr_view_pos.zero();
        light_matrix.make_identity();
        light_base.zero();

        if (g_game_config.mesh_static_lighting) {
            // Enable static lights
            gr_light_use_static(true);
            // Calculate lighting for meshes now
            obj_mesh_lighting_update_hook.call_target();
            // Switch back to dynamic lights
            gr_light_use_static(false);
        }
        else {
            obj_mesh_lighting_update_hook.call_target();
        }
    },
};

FunHook<void()> obj_mesh_lighting_alloc_hook{
    0x0048B1D0,
    []() {
        for (auto& item: DoublyLinkedList{rf::item_list}) {
            if (item.vmesh && !(item.obj_flags & rf::OF_DELAYED_DELETE)
                && rf::vmesh_get_type(item.vmesh) == rf::MESH_TYPE_STATIC) {
                auto size = rf::vmesh_calc_lighting_data_size(item.vmesh);
                item.mesh_lighting_data = rf::Malloc(size);
            }
        }
        for (auto& clutter: DoublyLinkedList{rf::clutter_list}) {
            if (clutter.vmesh && !(clutter.obj_flags & rf::OF_DELAYED_DELETE)
                && rf::vmesh_get_type(clutter.vmesh) == rf::MESH_TYPE_STATIC) {
                auto size = rf::vmesh_calc_lighting_data_size(clutter.vmesh);
                clutter.mesh_lighting_data = rf::Malloc(size);
            }
        }
    },
};

FunHook<void()> obj_mesh_lighting_free_hook{
    0x0048B370,
    []() {
        for (auto& item: DoublyLinkedList{rf::item_list}) {
            rf::Free(item.mesh_lighting_data);
            item.mesh_lighting_data = nullptr;
        }
        for (auto& clutter: DoublyLinkedList{rf::clutter_list}) {
            rf::Free(clutter.mesh_lighting_data);
            clutter.mesh_lighting_data = nullptr;
        }
    },
};

FunHook<void(rf::Object*)> obj_delete_mesh_hook{
    0x00489FC0,
    [](rf::Object* objp) {
        obj_delete_mesh_hook.call_target(objp);
        if (objp->mesh_lighting_data) {
            rf::Free(objp->mesh_lighting_data);
            objp->mesh_lighting_data = nullptr;
        }
    },
};

void recalc_mesh_static_lighting()
{
    AddrCaller{0x0048B370}.c_call(); // CleanupMeshLighting
    AddrCaller{0x0048B1D0}.c_call(); // AllocBuffersForMeshLighting
    AddrCaller{0x0048B0E0}.c_call(); // UpdateMeshLighting
}

ConsoleCommand2 mesh_static_lighting_cmd{
    "mesh_static_lighting",
    []() {
        g_game_config.mesh_static_lighting = !g_game_config.mesh_static_lighting;
        g_game_config.save();
        recalc_mesh_static_lighting();
        rf::console_printf("Mesh static lighting is %s", g_game_config.mesh_static_lighting ? "enabled" : "disabled");
    },
    "Toggle mesh static lighting calculation",
};

void obj_light_apply_patch()
{
    // Fix/improve items and clutters static lighting calculation: fix matrices being zero and use static lights
    obj_mesh_lighting_update_hook.install();
    obj_mesh_lighting_alloc_hook.install();
    obj_mesh_lighting_free_hook.install();
    obj_delete_mesh_hook.install();

    // Fix invalid vertex offset in mesh lighting calculation
    write_mem<int8_t>(0x005042F0 + 2, sizeof(rf::Vector3));

    // Commands
    mesh_static_lighting_cmd.register_cmd();
}
