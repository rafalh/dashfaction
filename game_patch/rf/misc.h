#pragma once

#include <patch_common/MemUtils.h>
#include "os/string.h"

namespace rf
{
    /* Other */

    static auto& default_player_weapon = addr_as_ref<String>(0x007C7600);

    static auto& get_file_checksum = addr_as_ref<unsigned(const char* filename)>(0x00436630);
    static auto& fl_rand_range = addr_as_ref<float(float min, float max)>(0x00504E40);
}
