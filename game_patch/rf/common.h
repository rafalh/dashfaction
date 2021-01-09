#pragma once

#include <cmath>
#include <cstdarg>
#include <patch_common/MemUtils.h>
#include <common/utils/string-utils.h>
#include "math/vector.h"
#include "math/matrix.h"
#include "math/plane.h"
#include "os/string.h"
#include "os/array.h"
#include "os/timestamp.h"

#ifndef __GNUC__
#define ALIGN(n) __declspec(align(n))
#else
#define ALIGN(n) __attribute__((aligned(n)))
#endif

namespace rf
{
    using uint = unsigned int;
    using ushort = unsigned short;
    using ubyte = unsigned char;

    /* Utils */

    struct Color
    {
        uint8_t red;
        uint8_t green;
        uint8_t blue;
        uint8_t alpha;

        constexpr Color(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255) :
            red(r), green(g), blue(b), alpha(a) {}

        void set(uint8_t r, uint8_t g, uint8_t b, uint8_t a)
        {
            this->red = r;
            this->green = g;
            this->blue = b;
            this->alpha = a;
        }
    };

    struct BaseVertex
    {
        Vector3 world_pos; // world or clip space
        float sx; // screen space
        float sy;
        float sw;
        ubyte codes;
        ubyte flags;
    };

    struct Vertex : BaseVertex
    {
        float u1;
        float v1;
        float u2;
        float v2;
        ubyte r;
        ubyte g;
        ubyte b;
        ubyte a;
    };
    static_assert(sizeof(Vertex) == 0x30);

    // RF stdlib functions are not compatible with GCC

    static auto& rf_free = addr_as_ref<void(void *mem)>(0x00573C71);
    static auto& rf_malloc = addr_as_ref<void*(int size)>(0x00573B37);

    // Timer
    static auto& timer_get = addr_as_ref<int(int frequency)>(0x00504AB0);

    static auto& current_fps = addr_as_ref<float>(0x005A4018);
    static auto& frametime = addr_as_ref<float>(0x005A4014);
    static auto& frametime_min = addr_as_ref<float>(0x005A4024);
}
