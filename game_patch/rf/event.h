#pragma once

#include "object.h"
#include "os/timestamp.h"

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

        virtual void initialize() = 0;
        virtual void turn_on() = 0;
        virtual void turn_off() = 0;
        virtual void process() = 0;

    };
    static_assert(sizeof(Event) == 0x2B8);

    static auto& event_lookup_from_uid = addr_as_ref<Event*(int uid)>(0x004B6820);
}
