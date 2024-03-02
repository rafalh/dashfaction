#pragma once

#include "object.h"

namespace rf
{
    struct GSolid;

    struct MoverBrush : Object
    {
        MoverBrush *next;
        MoverBrush *prev;
        GSolid *geometry;
    };

    static auto& mover_brush_list = addr_as_ref<MoverBrush>(0x0064E6E0);
}

