#pragma once

#include "object.h"

namespace rf
{
    struct EventObj : Object
    {
        int event_type;
        float delay;
        Timer delay_timer;
        DynamicArray<int> links;
        int activated_by_entity_handle;
        int activated_by_trigger_handle;
        int field_2B0;
        int is_on_state;

        virtual void UnkVirtFun() = 0;
        virtual void HandleOnMsg() = 0;
        virtual void HandleOffMsg() = 0;
        virtual void Update() = 0;

    };
    static_assert(sizeof(EventObj) == 0x2B8);

    static auto& EventGetByUid = AddrAsRef<EventObj*(int uid)>(0x004B6820);
}
