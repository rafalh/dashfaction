#pragma once

#include "object.h"
#include "weapon.h"

namespace rf {
    struct DebrisCreateStruct
    {
        Vector3 pos;
        Matrix3 orient;
        Vector3 vel;
        Vector3 spin;
        int lifespan_ms = 1000;
        int material = 0;
        int explosion_index = -1;
        int debris_flags = 0;
        int obj_flags = 0;
        GRoom *room = nullptr;
        ImpactSoundSet *iss = nullptr;

        DebrisCreateStruct()
        {
            orient.make_identity();
        }
    };
    using Debris = Object;

    auto& debris_create = addr_as_ref<Debris *(int parent_handle, const char *vmesh_filename, float mass, DebrisCreateStruct *dcs, int mesh_num, float collision_radius)>(0x00412E70);
}
