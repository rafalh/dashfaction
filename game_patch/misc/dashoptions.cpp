#include "dashoptions.h"
#include <patch_common/CallHook.h>
#include <patch_common/FunHook.h>
#include <patch_common/CodeInjection.h>
#include <patch_common/ShortTypes.h>
#include <patch_common/AsmWriter.h>
#include "../rf/file/file.h"
#include "../rf/gr/gr.h"
#include <algorithm>
#include <memory>
#include <sstream>
#include <xlog/xlog.h>

DashOptionsConfig g_dash_options_config;

void mark_option_loaded(DashOptionID id)
{
    g_dash_options_config.options_loaded[static_cast<size_t>(id)] = true;
}

namespace dashopt
{
std::unique_ptr<rf::File> dashoptions_file;

// trim leading and trailing whitespace
std::string trim(const std::string& str)
{
    auto start = std::find_if_not(str.begin(), str.end(), [](unsigned char ch) { return std::isspace(ch); });
    auto end = std::find_if_not(str.rbegin(), str.rend(), [](unsigned char ch) { return std::isspace(ch); }).base();

    if (start >= end) {
        return ""; // Return empty string if nothing remains after trimming
    }
    return std::string(start, end);
}

// for values provided in quotes, dump the quotes before storing the values
std::optional<std::string> extract_quoted_value(const std::string& value)
{
    std::string trimmed_value = trim(value);

    // check if trimmed value starts and ends with quotes, if so, extract the content inside them
    if (trimmed_value.size() >= 2 && trimmed_value.front() == '"' && trimmed_value.back() == '"') {
        std::string extracted_value =
            trimmed_value.substr(1, trimmed_value.size() - 2);
        return extracted_value;
    }

    // if not wrapped in quotes, assume valid
    xlog::warn("String value is not enclosed in quotes, accepting it anyway: '{}'", trimmed_value);
    return trimmed_value;
}

void disable_mod_playercfg_patch()
{
    if (g_dash_options_config.use_stock_game_players_config) {
        // set mod to not make its own players.cfg but instead use the stock game one
        AsmWriter(0x004A8F99).jmp(0x004A9010);
        AsmWriter(0x004A8DCC).jmp(0x004A8E53);
    }
}

CodeInjection fpgun_ar_ammo_digit_color_injection{
    0x004ABC03,
    [](auto& regs) {
        if (g_dash_options_config.is_option_loaded(DashOptionID::AssaultRifleAmmoColor)) {
            uint32_t color = g_dash_options_config.ar_ammo_color.value();
            rf::gr::set_color((color >> 24) & 0xFF, // r
                              (color >> 16) & 0xFF, // g
                              (color >> 8) & 0xFF,  // b 
                              color & 0xFF          // a
            );
        }         
        regs.eip = 0x004ABC08;
    }
};

void apply_dashoptions_patches()
{
    xlog::warn("Applying Dash Options patches");
    disable_mod_playercfg_patch();
    fpgun_ar_ammo_digit_color_injection.install();
}

// consolidated processing for options
template<typename T>
void set_option(DashOptionID option_id, std::optional<T>& option_field, const std::string& option_value)
{
    try {
        if constexpr (std::is_same_v<T, std::string>) {
            // strip quotes for strings
            if (auto quoted_value = extract_quoted_value(option_value)) {
                option_field = quoted_value.value();
                xlog::warn("Successfully extracted string: {}", quoted_value.value());
            }
            else {
                xlog::warn("Invalid string format: {}", option_value);
                return;
            }
        }
        else if constexpr (std::is_same_v<T, uint32_t>) { // handle color values
            option_field = static_cast<uint32_t>(
                std::stoul(extract_quoted_value(option_value).value_or(option_value), nullptr, 16));
        }
        else if constexpr (std::is_same_v<T, float>) {
            option_field = std::stof(option_value);
        }
        else if constexpr (std::is_same_v<T, int>) {
            option_field = std::stoi(option_value);
        }
        else if constexpr (std::is_same_v<T, bool>) { // handle bools with every reasonable capitalization
            option_field =
                (option_value == "1" || option_value == "true" || option_value == "True" || option_value == "TRUE");
        }
        // mark option as loaded
        mark_option_loaded(option_id);
        xlog::warn("Parsed value has been saved: {}", option_field.value());
    }
    catch (const std::exception& e) {
        xlog::warn("Failed to parse value for option: {}. Error: {}", option_value, e.what());
    }
}

// identify custom options and parse where found
void process_dashoption_line(const std::string& option_name, const std::string& option_value)
{
    xlog::warn("Found an option! Attempting to process {} with value {}", option_name, option_value);

    if (option_name == "$Scoreboard Logo") {
        set_option(DashOptionID::ScoreboardLogo, g_dash_options_config.scoreboard_logo, option_value);
    }
    if (option_name == "$Use Base Game Players Config") {
        set_option(DashOptionID::UseStockPlayersConfig, g_dash_options_config.use_stock_game_players_config, option_value);
    }
    if (option_name == "$Assault Rifle Ammo Counter Color") {
        set_option(DashOptionID::AssaultRifleAmmoColor, g_dash_options_config.ar_ammo_color, option_value);
    }
    else {
        xlog::warn("Ignoring unsupported option: {}", option_name);
    }
}

bool open_file(const std::string& file_path)
{
    dashoptions_file = std::make_unique<rf::File>();
    if (dashoptions_file->open(file_path.c_str()) != 0) {
        xlog::error("Failed to open {}", file_path);
        return false;
    }
    xlog::warn("Successfully opened {}", file_path);
    return true;
}

void close_file()
{
    if (dashoptions_file) {
        xlog::warn("Closing file.");
        dashoptions_file->close();
        dashoptions_file.reset();

        // after parsing is done, apply patches
        apply_dashoptions_patches();
    }
}

void parse()
{
    std::string line, full_line;
    char buffer[256];
    bool in_options_section = false; // track section, eventually this should be enum and support multiple sections

    xlog::warn("Start parsing dashoptions.tbl");

    while (int bytes_read = dashoptions_file->read(buffer, sizeof(buffer) - 1)) {
        if (bytes_read <= 0) {
            xlog::warn("End of file or read error in dashoptions.tbl.");
            break;
        }

        buffer[bytes_read] = '\0';
        std::istringstream file_stream(buffer);
        while (std::getline(file_stream, line)) {
            line = trim(line);
            xlog::warn("Parsing line: '{}'", line);

            if (line == "#General") {
                xlog::warn("Entering General section");
                in_options_section = true;
                continue;
            }
            else if (line == "#End") {
                xlog::warn("Exiting General section");
                in_options_section = false;
                break; // stop parsing after reaching the end
            }

            // do not parse anything outside the options section
            if (!in_options_section) {
                xlog::warn("Skipping line outside of General section");
                continue;
            }

            // Skip empty lines and comments
            if (line.empty() || line.find("//") == 0) {
                xlog::warn("Skipping line because it's empty or a comment");
                continue;
            }

            // Parse options
            auto delimiter_pos = line.find(':');
            if (delimiter_pos == std::string::npos) {
                xlog::warn("Skipping malformed line: {}", line);
                continue;
            }

            std::string option_name = trim(line.substr(0, delimiter_pos));
            std::string option_value = trim(line.substr(delimiter_pos + 1));

            process_dashoption_line(option_name, option_value);
        }
    }
}


void load_dashoptions_config()
{
    xlog::warn("Mod launched, attempting to load Dash Options configuration");

    if (!open_file("dashoptions.tbl")) {
        return;
    }

    parse();
    close_file();

    xlog::warn("Dash Options configuration loaded");
}
}
