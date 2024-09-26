#pragma once

#include <patch_common/MemUtils.h>
#include "os/string.h"

namespace rf
{
    /* Other */

    static auto& default_player_weapon = addr_as_ref<String>(0x007C7600);

    static auto& get_file_checksum = addr_as_ref<unsigned(const char* filename)>(0x00436630);
    static auto& geomod_shape_init = addr_as_ref<void()>(0x004374C0);
    static auto& geomod_shape_create = addr_as_ref<int(const char*)>(0x00437500);
    static auto& geomod_shape_shutdown = addr_as_ref<void()>(0x00437460);
    }
