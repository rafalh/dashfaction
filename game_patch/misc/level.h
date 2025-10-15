#pragma once

#include <xlog/xlog.h>
#include "../rf/file/file.h"

constexpr int alpine_props_chunk_id = 0x0AFBA5ED;
constexpr int dash_level_props_chunk_id = 0xDA58FA00;

struct AlpineLevelProps {
    // Backward-compatible defaults.
    bool legacy_cyclic_timers = true;

    static AlpineLevelProps& instance() {
        static AlpineLevelProps instance{};
        return instance;
    }

    void reset() {
        legacy_cyclic_timers = true;
    }

    void deserialize(rf::File& file) {
        const std::int32_t version = file.read<std::int32_t>();
        // Forward-compatible deserialization.
        if (version >= 1) {
            legacy_cyclic_timers = file.read<std::uint8_t>();
            xlog::debug("legacy_cyclic_timers {}", legacy_cyclic_timers);
        }
    }
};

struct DashLevelProps {
    // Backward-compatible defaults.
    bool lightmaps_full_depth = false;

    static DashLevelProps& instance() {
        static DashLevelProps instance{};
        return instance;
    }

    void reset(int version) {
        lightmaps_full_depth = version >= 300;
    }

    void deserialize(rf::File& file) {
        const std::int32_t version = file.read<std::int32_t>();
        if (version == 1) {
            lightmaps_full_depth = file.read<std::uint8_t>();
            xlog::debug("lightmaps_full_depth {}", lightmaps_full_depth);
        }
    }
};

extern bool g_level_has_unsupported_event_classes;
