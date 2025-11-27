#pragma once

#include <functional>
#include <optional>
#include <string>
#include <unordered_map>
#include <variant>
#include <cstdint>

void load_af_level_info_config(const std::string& level_filename);

enum class AlpineLevelInfoId {
    LightmapClampFloor,
    LightmapClampCeiling,
    IdealPlayerCount,
    AuthorContact,
    AuthorWebsite,
    Description,
    PlayerHeadlampColor,
    PlayerHeadlampIntensity,
    PlayerHeadlampRange,
    PlayerHeadlampRadius,
    ChatMap1,
    ChatMap2,
    ChatMap3,
    ChatMap4,
    ChatMap5,
    ChatMap6,
    ChatMap7,
    ChatMap8,
    ChatMap9,
    CraterTexturePpm,
    _last_variant
};

// Represents different configuration values.
using LevelInfoValue = std::variant<std::string, uint32_t, float, int, bool>;

struct LevelInfoMetadata {
    AlpineLevelInfoId id{};
    std::function<std::optional<LevelInfoValue>(const std::string&)> parse{};
};

struct AlpineLevelInfoConfig {
    std::string current_level{};
    std::unordered_map<AlpineLevelInfoId, LevelInfoValue> level_options{};
    // `mesh_replacements` is lower case.
    std::unordered_map<std::string, std::string> mesh_replacements{};

    void reset_for_level(const std::string& level) {
        current_level = level;
        level_options.clear();
        mesh_replacements.clear();
    }

    bool is_option_loaded(const AlpineLevelInfoId option_id) const {
        return level_options.find(option_id) != level_options.end();
    }
};

extern AlpineLevelInfoConfig g_af_level_info_config;
