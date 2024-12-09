#include <cassert>
#include <xlog/xlog.h>
#include <patch_common/FunHook.h>
#include <common/utils/list-utils.h>
#include "../rf/object.h"
#include "../rf/item.h"
#include "../rf/level.h"
#include "../rf/clutter.h"
#include "../rf/gr/gr.h"
#include "../rf/multi.h"
#include "../rf/crt.h"
#include "../os/console.h"
#include "../main/main.h"
#include "../multi/multi.h"

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

void recalc_mesh_static_lighting()
{
    rf::obj_light_free();
    rf::obj_light_alloc();
    rf::obj_light_calculate();
}

void evaluate_fullbright_meshes()
{
    if (!rf::LEVEL_LOADED)
        return;

    bool server_side_restrict_fb_mesh =
        rf::is_multi && !rf::is_server && get_df_server_info() && !get_df_server_info()->allow_fb_mesh;

    if (server_side_restrict_fb_mesh && g_game_config.try_mesh_fullbright) {
        rf::console::print("This server does not allow you to force fullbright meshes!");
        return;
    }

    auto [r, g, b] = g_game_config.try_mesh_fullbright
                         ? std::make_tuple(1.0f, 1.0f, 1.0f)
                         : std::make_tuple(static_cast<float>(rf::level.ambient_light.red) / 255.0f,
                                           static_cast<float>(rf::level.ambient_light.green) / 255.0f,
                                           static_cast<float>(rf::level.ambient_light.blue) / 255.0f);

    rf::gr::light_set_ambient(r, g, b);
    recalc_mesh_static_lighting();
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

ConsoleCommand2 fullbright_models_cmd{
    "mesh_fullbright",
    []() {
        g_game_config.try_mesh_fullbright = !g_game_config.try_mesh_fullbright;
        g_game_config.save();

        evaluate_fullbright_meshes();    
                
        rf::console::print("Fullbright meshes are {}", g_game_config.try_mesh_fullbright ?
			"enabled. In multiplayer, this will only apply if the server allows it." : "disabled.");
    },
    "Set all meshes to render fullbright. In multiplayer, this is only available if the server allows it.",
};

void obj_light_apply_patch()
{
    // Fix/improve items and clutters static lighting calculation: fix matrices being zero and use static lights
    obj_light_calculate_hook.install();
    obj_light_alloc_hook.install();
    obj_light_free_hook.install();

    // Fix invalid vertex offset in mesh lighting calculation
    write_mem<int8_t>(0x005042F0 + 2, sizeof(rf::Vector3));

    // Commands
    mesh_static_lighting_cmd.register_cmd();
    fullbright_models_cmd.register_cmd();
}
