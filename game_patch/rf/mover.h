#pragma once

#include "object.h"

namespace rf
{
    struct MoverObj : Object
    {
        struct MoverObj *next;
        struct MoverObj *prev;
        RflGeometry *geometry;
    };

    auto& mover_obj_list = AddrAsRef<rf::MoverObj>(0x0064E6E0);
}

