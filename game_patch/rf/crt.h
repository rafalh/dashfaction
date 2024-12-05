#pragma once

#include <patch_common/MemUtils.h>

namespace rf
{
    // RF stdlib functions are not compatible with GCC
    static auto& rf_free = addr_as_ref<void(void* mem)>(0x00573C71);
    static auto& rf_malloc = addr_as_ref<void*(int size)>(0x00573B37);
    static auto& operator_new = addr_as_ref<void*(unsigned size)>(0x00573619);
    static auto& operator_delete = addr_as_ref<void(void*)>(0x0057360E);
}
