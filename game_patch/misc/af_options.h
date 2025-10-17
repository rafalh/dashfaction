#pragma once

#include <functional>
#include <optional>
#include <string>
#include <unordered_map>
#include <variant>

// ======= Globals and utility =======
void load_level_info_config(const std::string& level_filename);

// ======= Alpine level info ======= 
enum class AlpineLevelInfoId
{
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
    _optioncount       // dummy for total count
};

// Variant type to represent different configuration values
using LevelInfoValue = std::variant<std::string, uint32_t, float, int, bool>;

struct LevelInfoMetadata
{
    AlpineLevelInfoId id;
    std::function<std::optional<LevelInfoValue>(const std::string&)> parse_function;
};

// Main configuration structure for level info
struct AlpineLevelInfoConfig
{
    // maps of level options and mesh replacements
    std::unordered_map<std::string, std::unordered_map<AlpineLevelInfoId, LevelInfoValue>> level_options;
    std::unordered_map<std::string, std::unordered_map<std::string, std::string>> mesh_replacements; // stored lowercase

    // Check if an option exists for a given level
    bool is_option_loaded(const std::string& level, AlpineLevelInfoId option_id) const
    {
        auto level_it = level_options.find(level);
        if (level_it != level_options.end()) {
            return level_it->second.find(option_id) != level_it->second.end();
        }
        return false; // no options loaded for this level
    }
};

extern AlpineLevelInfoConfig g_af_level_info_config;
