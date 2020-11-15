#pragma once

#include "object.h"

namespace rf
{
    struct Event : Object
    {
        int event_type;
        float delay_seconds;
        Timestamp delay_timestamp;
        VArray<int> links;
        int triggered_by_handle;
        int trigger_handle;
        int event_flags;
        bool delayed_msg;

        virtual void Initialize() = 0;
        virtual void TurnOn() = 0;
        virtual void TurnOff() = 0;
        virtual void Process() = 0;

    };
    static_assert(sizeof(Event) == 0x2B8);

    static auto& EventLookupFromUid = AddrAsRef<Event*(int uid)>(0x004B6820);
}
