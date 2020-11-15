#pragma once

#include "common.h"

namespace rf {
    struct GlareInfo;
    struct GFace;
    struct MoverBrush;
    struct Player;

    struct Glare : Object
    {
        char enabled;
        char in_view;
        int last_covering_objh;
        MoverBrush *last_covering_mover;
        GFace *last_covering_face;
        float current_alpha[2];
        float current_radius[2];
        GlareInfo *info;
        int info_index;
        int flags;
        struct Glare *next;
        struct Glare *prev;
        Vector3 field_2C0;
        Player *player;
        bool use_prop_points;
        Vector3 corona_point1;
        Vector3 corona_point2;
    };
    static_assert(sizeof(Glare) == 0x2EC);

    static auto& GlareEntityCollisionTest = AddrAsRef<bool(Object *obj, Glare *glare, Vector3 *eye_pos)>(0x00415280);
}
