#pragma once

#include "../math/vector.h"
#include "../math/plane.h"

namespace rf
{
    struct GSolid;
}

namespace rf::gr
{
    enum LightType
    {
        LT_NONE = 0x0,
        LT_DIRECTIONAL = 0x1,
        LT_POINT = 0x2,
        LT_SPOT = 0x3,
        LT_TUBE = 0x4,
    };

    enum LightShadowcastCondition
    {
        SHADOWCAST_NEVER = 0x0,
        SHADOWCAST_EDITOR = 0x1,
        SHADOWCAST_RUNTIME = 0x2,
    };

    struct Light
    {
        struct GrLight *next;
        struct GrLight *prev;
        LightType type;
        Vector3 vec;
        Vector3 vec2;
        Vector3 spotlight_dir;
        float spotlight_fov1;
        float spotlight_fov2;
        float spotlight_atten;
        float rad_2;
        float r;
        float g;
        float b;
        bool on;
        bool dynamic;
        bool use_squared_fov_falloff;
        LightShadowcastCondition shadow_condition;
        int attenuation_algorithm;
        int field_58;
        Vector3 local_vec;
        Vector3 local_vec2;
        Vector3 rotated_spotlight_dir;
        float rad2_squared;
        float spotlight_fov1_dot;
        float spotlight_fov2_dot;
        float spotlight_max_radius;
        int room;
        Plane spotlight_frustrum[6];
        int spotlight_nverts[6];
    };
    static_assert(sizeof(Light) == 0x10C);

    static auto& light_filter_set_solid = addr_as_ref<int(GSolid *s, bool include_dynamic, bool include_static)>(0x004D9DD0);
    static auto& light_filter_reset = addr_as_ref<void()>(0x004D9FA0);
    static auto& light_get_ambient = addr_as_ref<void(float *r, float *g, float *b)>(0x004D8D10);

    static auto& num_relevant_lights = addr_as_ref<int>(0x00C9687C);
    static auto& relevant_lights = addr_as_ref<Light*[1100]>(0x00C4D588);
}
