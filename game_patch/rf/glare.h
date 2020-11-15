#pragma once

#include "common.h"

namespace rf {
    struct GlareInfo;
    struct GFace;
    struct MoverBrush;
    struct Player;

    struct Glare : Object
    {
        bool enabled;
        bool visible;
        int last_covering_objh;
        MoverBrush *last_covering_mover_brush;
        GFace *last_covering_face;
        float last_rendered_intensity[2];
        float last_rendered_radius[2];
        GlareInfo *info;
        int info_index;
        int flags;
        Glare *next;
        Glare *prev;
        Vector3 field_2C0;
        Player *parent_player;
        bool is_rod;
        Vector3 rod_pos1;
        Vector3 rod_pos2;
    };
    static_assert(sizeof(Glare) == 0x2EC);

    static auto& GlareEntityCollisionTest = AddrAsRef<bool(Object *obj, Glare *glare, Vector3 *eye_pos)>(0x00415280);
}
