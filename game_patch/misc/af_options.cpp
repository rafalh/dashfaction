#include "af_options.h"
#include <algorithm>
#include <sstream>
#include <regex>
#include <xlog/xlog.h>
#include "../rf/file/file.h"
#include <common/utils/string-utils.h>

AlpineLevelInfoConfig g_af_level_info_config;

// trim leading and trailing whitespace
std::string trim(const std::string& str, bool remove_quotes = false)
{
    auto start = str.find_first_not_of(" \t\n\r");
    auto end = str.find_last_not_of(" \t\n\r");
    if (start == std::string::npos) {
        return "";
    }        

    std::string trimmed = str.substr(start, end - start + 1);

    // extract quoted value
    if (remove_quotes && trimmed.size() >= 2 && trimmed.front() == '"' && trimmed.back() == '"') {
        trimmed = trimmed.substr(1, trimmed.size() - 2);
    }

    return trimmed;
}

// Parsing functions for level settings
std::optional<LevelInfoValue> parse_float_level(const std::string& value)
{
    try { return std::stof(value); } catch (...) { return std::nullopt; } 
}
std::optional<LevelInfoValue> parse_int_level(const std::string& value)
{
    try { return std::stoi(value); } catch (...) { return std::nullopt; } 
}
std::optional<LevelInfoValue> parse_bool_level(const std::string& value)
{
    return value == "1" || value == "true" || value == "True";
}
std::optional<LevelInfoValue> parse_string_level(const std::string& value)
{
    return trim(value, true);
}

std::optional<LevelInfoValue> parse_color_level(const std::string& value)
{
    std::string trimmed_value = trim(value, true);

    try {
        // Check if it's a valid hex string (6 or 8 characters)
        if (!trimmed_value.empty() && trimmed_value.length() <= 8 &&
            std::all_of(trimmed_value.begin(), trimmed_value.end(), ::isxdigit)) {
            uint32_t color = std::stoul(trimmed_value, nullptr, 16); // Parse hex

            // If it's a 24-bit color (6 characters), add full alpha (0xFF)
            if (trimmed_value.length() == 6)
                color = (color << 8) | 0xFF;

            return color;
        }

        // Check for RGB formats using regex
        std::regex rgb_pattern(R"([\{\<]?\s*(\d+)\s*,\s*(\d+)\s*,\s*(\d+)\s*[\>\}]?)");
        std::smatch matches;

        if (std::regex_match(trimmed_value, matches, rgb_pattern)) {
            if (matches.size() == 4) { // First match is the full string, next 3 are R, G, B
                int r = std::stoi(matches[1].str());
                int g = std::stoi(matches[2].str());
                int b = std::stoi(matches[3].str());

                // Ensure values are within the valid range
                if (r >= 0 && r <= 255 && g >= 0 && g <= 255 && b >= 0 && b <= 255) {
                    uint32_t color = (r << 24) | (g << 16) | (b << 8) | 0xFF; // Add full alpha
                    return color;
                }
            }
        }
    }
    catch (...) {
        return std::nullopt; // Return null if parsing fails
    }

    return std::nullopt;
}

std::optional<std::pair<std::string, std::string>> parse_mesh_replacement(const std::string& value)
{
    std::string trimmed_value = trim(value, true);
    std::regex mesh_replacement_pattern("\\{\\s*\"([^\"]+)\"\\s*,\\s*\"([^\"]+)\"\\s*\\}");
    std::smatch matches;

    if (std::regex_match(trimmed_value, matches, mesh_replacement_pattern)) {
        if (matches.size() == 3) { // Full match and 2 filename captures
            return std::make_pair(trim(matches[1].str(), true), trim(matches[2].str(), true));
        }
    }
    return std::nullopt;
}

// Metadata map for level options
const std::unordered_map<std::string, LevelInfoMetadata> level_info_metadata = {
    {"$Lightmap Clamp Floor", {AlpineLevelInfoId::LightmapClampFloor, parse_color_level}},
    {"$Lightmap Clamp Ceiling", {AlpineLevelInfoId::LightmapClampCeiling, parse_color_level}},
    {"$Ideal Player Count", {AlpineLevelInfoId::IdealPlayerCount, parse_int_level}},
    {"$Author Contact", {AlpineLevelInfoId::AuthorContact, parse_string_level}},
    {"$Author Website", {AlpineLevelInfoId::AuthorWebsite, parse_string_level}},
    {"$Description", {AlpineLevelInfoId::Description, parse_string_level}},
    {"$Player Headlamp Color", {AlpineLevelInfoId::PlayerHeadlampColor, parse_color_level}},
    {"$Player Headlamp Intensity", {AlpineLevelInfoId::PlayerHeadlampIntensity, parse_float_level}},
    {"$Player Headlamp Range", {AlpineLevelInfoId::PlayerHeadlampRange, parse_float_level}},
    {"$Player Headlamp Radius", {AlpineLevelInfoId::PlayerHeadlampRadius, parse_float_level}},
    {"$Chat Menu 1", {AlpineLevelInfoId::ChatMap1, parse_string_level}},
    {"$Chat Menu 2", {AlpineLevelInfoId::ChatMap2, parse_string_level}},
    {"$Chat Menu 3", {AlpineLevelInfoId::ChatMap3, parse_string_level}},
    {"$Chat Menu 4", {AlpineLevelInfoId::ChatMap4, parse_string_level}},
    {"$Chat Menu 5", {AlpineLevelInfoId::ChatMap5, parse_string_level}},
    {"$Chat Menu 6", {AlpineLevelInfoId::ChatMap6, parse_string_level}},
    {"$Chat Menu 7", {AlpineLevelInfoId::ChatMap7, parse_string_level}},
    {"$Chat Menu 8", {AlpineLevelInfoId::ChatMap8, parse_string_level}},
    {"$Chat Menu 9", {AlpineLevelInfoId::ChatMap9, parse_string_level}},
    {"$Crater Texture PPM", {AlpineLevelInfoId::CraterTexturePpm, parse_float_level}}
};

// Load level info from filename_info.tbl
void load_af_level_info_config(const std::string& level_filename)
{
    std::string base_filename = level_filename.substr(0, level_filename.size() - 4);
    std::string info_filename = base_filename + "_info.tbl";
    auto level_info_file = std::make_unique<rf::File>();

    if (level_info_file->open(info_filename.c_str()) != 0) {
        xlog::debug("Could not open {}", info_filename);
        return;
    }

    xlog::debug("Successfully opened {}", info_filename);

    // Read entire file content into a single string buffer
    std::string file_content;
    std::string buffer(2048, '\0');
    int bytes_read;

    while ((bytes_read = level_info_file->read(&buffer[0], buffer.size() - 1)) > 0) {
        buffer.resize(bytes_read);
        file_content += buffer;
        buffer.resize(2048, '\0');
    }

    level_info_file->close();

    // Process file content line-by-line
    std::istringstream file_stream(file_content);
    std::string line;
    bool found_start = false;

    // Search for #Start marker
    while (std::getline(file_stream, line)) {
        line = trim(line, false);
        if (line == "#Start") {
            found_start = true;
            break;
        }
    }

    if (!found_start) {
        xlog::warn("No #Start marker found in {}", info_filename);
        return;
    }

    // Process options until #End marker is found
    bool found_end = false;
    while (std::getline(file_stream, line)) {
        line = trim(line, false);

        if (line == "#End") {
            found_end = true;
            break;
        }
        if (line.empty() || line.starts_with("//")) {
            continue;
        }

        auto delimiter_pos = line.find(':');
        if (delimiter_pos == std::string::npos || line[0] != '$') {
            continue;
        }

        std::string option_name = trim(line.substr(0, delimiter_pos), false);
        std::string option_value = trim(line.substr(delimiter_pos + 1), false);

        // handle mesh replacements
        if (option_name == "$Mesh Replacement") {
            auto parsed_value = parse_mesh_replacement(option_value);
            if (parsed_value) {
                g_af_level_info_config.mesh_replacements[level_filename][string_to_lower(parsed_value->first)]= parsed_value->second;

                xlog::debug("Mesh Replacement Added: {} -> {} in {}", parsed_value->first, parsed_value->second, level_filename);
            }
            else {
                xlog::warn("Invalid mesh replacement format in {}", info_filename);
            }

            continue;
        }

        // handle other level info options
        auto meta_it = level_info_metadata.find(option_name);
        if (meta_it != level_info_metadata.end()) {
            const auto& metadata = meta_it->second;
            auto parsed_value = metadata.parse(option_value);
            if (parsed_value) {
                g_af_level_info_config.level_options[level_filename][metadata.id] = *parsed_value;
                xlog::debug("Parsed and applied {} for {}: {}", option_name, level_filename, option_value);
            }
        }
        else {
            xlog::warn("Unknown option {} in {}", option_name, info_filename);
        }
    }

    if (!found_end) {
        xlog::warn("No #End marker found in {}", info_filename);
    }
}
