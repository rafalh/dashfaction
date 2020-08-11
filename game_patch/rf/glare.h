#pragma once

#include "common.h"

namespace rf {
    struct EffectsTbl;
    struct RflGeometry;
    struct RflFace;
    struct MoverObj;
    struct Player;

    struct GlareObj : Object
    {
        char enabled;
        char in_view;
        int last_covering_objh;
        MoverObj *last_covering_mover;
        RflFace *last_covering_face;
        float current_alpha[2];
        float current_radius[2];
        EffectsTbl *effect;
        int effect_id;
        int flags;
        struct GlareObj *next;
        struct GlareObj *prev;
        Vector3 field_2C0;
        Player *player;
        bool use_prop_points;
        Vector3 corona_point1;
        Vector3 corona_point2;
    };
    static_assert(sizeof(GlareObj) == 0x2EC);

    static auto& GlareEntityCollisionTest = AddrAsRef<bool(Object *obj, GlareObj *glare, Vector3 *eye_pos)>(0x00415280);
}
