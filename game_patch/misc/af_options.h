#pragma once

#include <functional>
#include <optional>
#include <string>
#include <unordered_map>
#include <variant>

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
    std::unordered_map<std::string, std::unordered_map<AlpineLevelInfoId, LevelInfoValue>> level_options{};
    // `mesh_replacements` is lowercase.
    std::unordered_map<std::string, std::unordered_map<std::string, std::string>> mesh_replacements{};

    bool is_option_loaded(const std::string level_filename, const AlpineLevelInfoId option_id) const {
        const auto level = level_options.find(level_filename);
        if (level != level_options.end()) {
            return level->second.find(option_id) != level->second.end();
        }
        return false;
    }
};

extern AlpineLevelInfoConfig g_af_level_info_config;
