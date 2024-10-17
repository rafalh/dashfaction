#pragma once

#include "../math/vector.h"
#include "../math/matrix.h"

namespace rf
{
    struct Entity;
    struct Player;

    enum CameraMode
    {
        CAMERA_FIRST_PERSON  = 0x0,
        CAMERA_THIRD_PERSON  = 0x1,
        CAMERA_FREELOOK      = 0x2,
        CAMERA_DESCENT_FLY   = 0x3,
        CAMERA_DEAD_VIEW     = 0x4,
        CAMERA_CUTSCENE      = 0x5,
        CAMERA_FIXED_VIEW    = 0x6,
        CAMERA_ORBIT         = 0x7,
    };

    struct Camera
    {
        Entity *camera_entity;
        Player *player;
        CameraMode mode;
    };
    static_assert(sizeof(Camera) == 0xC);

    static auto& camera_enter_first_person = addr_as_ref<bool(Camera *camera)>(0x0040DDF0);
    static auto& camera_enter_freelook = addr_as_ref<bool(Camera *camera)>(0x0040DCF0);
    static auto& camera_enter_fixed = addr_as_ref<bool(Camera *camera)>(0x0040DF70);

    inline Vector3 camera_get_pos(Camera *camera)
    {
        Vector3 result;
        AddrCaller{0x0040D760}.c_call(&result, camera);
        return result;
    }

    inline Matrix3 camera_get_orient(Camera *camera)
    {
        Matrix3 result;
        AddrCaller{0x0040D780}.c_call(&result, camera);
        return result;
    }

    static auto& camera_get_mode = addr_as_ref<CameraMode(const rf::Camera&)>(0x0040D740);
}
