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

    auto& mover_brush_list = AddrAsRef<MoverBrush>(0x0064E6E0);
}

