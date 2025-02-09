#pragma once

#include <xlog/xlog.h>
#include "../rf/file/file.h"

constexpr int dash_level_props_chunk_id = 0xDA58FA00;

struct DashLevelProps
{
    // backward compatible defaults
    bool lightmaps_full_depth = false;

    static DashLevelProps& instance()
    {
        static DashLevelProps instance;
        return instance;
    }

    void deserialize(rf::File& file)
    {
        lightmaps_full_depth = file.read<std::uint8_t>();
        xlog::debug("lightmaps_full_depth {}", lightmaps_full_depth);
    }
};
