#pragma once

#include "object.h"

namespace rf
{
    struct MoverBrush : Object
    {
        struct MoverBrush *next;
        struct MoverBrush *prev;
        GSolid *geometry;
    };

    auto& mover_brush_list = AddrAsRef<rf::MoverBrush>(0x0064E6E0);
}

