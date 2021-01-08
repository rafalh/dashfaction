#pragma once

namespace rf
{
    struct Object;
}

void object_do_patch();
void obj_mesh_lighting_alloc_one(rf::Object *objp);
void obj_mesh_lighting_update_one(rf::Object *objp);

constexpr size_t old_obj_limit = 1024;
constexpr size_t obj_limit = 65536;
