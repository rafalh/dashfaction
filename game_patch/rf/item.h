#pragma once

#include "object.h"

namespace rf
{
    struct ItemObj
    {
        Object _super;
        ItemObj *next;
        ItemObj *prev;
        int field_294;
        int item_cls_id;
        String field_29c;
        int field_2a4;
        int field_2a8;
        int field_2ac;
        char field_2b0[12];
        int field_2bc;
        int field_2c0;
    };
    static_assert(sizeof(ItemObj) == 0x2C4);
}
