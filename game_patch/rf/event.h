#pragma once

#include "object.h"

namespace rf
{
    struct Event : Object
    {
        int event_type;
        float delay;
        Timestamp delay_timestamp;
        VArray<int> links;
        int activated_by_entity_handle;
        int activated_by_trigger_handle;
        int field_2B0;
        int is_on_state;

        virtual void UnkVirtFun() = 0;
        virtual void TurnOn() = 0;
        virtual void TurnOff() = 0;
        virtual void Process() = 0;

    };
    static_assert(sizeof(Event) == 0x2B8);

    static auto& EventLookupFromUid = AddrAsRef<Event*(int uid)>(0x004B6820);
}
