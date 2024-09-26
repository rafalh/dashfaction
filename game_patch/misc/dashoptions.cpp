#include "dashoptions.h"
#include <patch_common/CallHook.h>
#include <patch_common/FunHook.h>
#include <patch_common/CodeInjection.h>
#include <patch_common/ShortTypes.h>
#include <patch_common/AsmWriter.h>
#include "../rf/file/file.h"
#include "../rf/gr/gr.h"
#include "../rf/misc.h"
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

// set default geo mesh
CallHook<int(const char*)> default_geomod_shape_create_hook{
    0x004374CF,
    [](const char* filename) -> int {
        std::string original_filename{filename};
        // set value, default to the original one if its somehow missing
        std::string modded_filename = g_dash_options_config.geomodmesh_default.value_or(original_filename);
        return default_geomod_shape_create_hook.call_target(modded_filename.c_str());
    }
};

// set driller double geo mesh
CallHook<int(const char*)> driller_double_geomod_shape_create_hook{
    0x004374D9, [](const char* filename) -> int {
        std::string original_filename{filename};
        // set value, default to the original one if its somehow missing
        std::string modded_filename = g_dash_options_config.geomodmesh_driller_double.value_or(original_filename);
        return driller_double_geomod_shape_create_hook.call_target(modded_filename.c_str());
    }
};

// set driller single geo mesh
CallHook<int(const char*)> driller_single_geomod_shape_create_hook{
    0x004374E3, [](const char* filename) -> int {
        std::string original_filename{filename};
        // set value, default to the original one if its somehow missing
        std::string modded_filename = g_dash_options_config.geomodmesh_driller_single.value_or(original_filename);
        return driller_single_geomod_shape_create_hook.call_target(modded_filename.c_str());
    }
};

// set apc geo mesh
CallHook<int(const char*)> apc_geomod_shape_create_hook{
    0x004374ED, [](const char* filename) -> int {
        std::string original_filename{filename};
        // set value, default to the original one if its somehow missing
        std::string modded_filename = g_dash_options_config.geomodmesh_apc.value_or(original_filename);
        return apc_geomod_shape_create_hook.call_target(modded_filename.c_str());
    }
};

void apply_geomod_mesh_patch()
{
    // array of geomod mesh options
    std::array<std::pair<DashOptionID, void (*)()>, 4> geomod_mesh_hooks = {
        {{DashOptionID::GeomodMesh_Default, [] { default_geomod_shape_create_hook.install(); }},
         {DashOptionID::GeomodMesh_DrillerDouble, [] { driller_double_geomod_shape_create_hook.install(); }},
         {DashOptionID::GeomodMesh_DrillerSingle, [] { driller_single_geomod_shape_create_hook.install(); }},
         {DashOptionID::GeomodMesh_APC, [] { apc_geomod_shape_create_hook.install(); }}}};

    bool any_option_loaded = false;

    // install only the hooks for the ones that were set
    for (const auto& [option_id, install_fn] : geomod_mesh_hooks) {
        if (g_dash_options_config.is_option_loaded(option_id)) {
            install_fn();
            any_option_loaded = true;
        }
    }

    // if any one was set, apply the necessary patches
    if (any_option_loaded) {
        AsmWriter(0x00437543).call(0x004ECED0); // Replace the call to load v3d instead of embedded
        rf::geomod_shape_init();                // Reinitialize geomod shapes
    }
}

void apply_dashoptions_patches()
{
    xlog::warn("Applying Dash Options patches");
    disable_mod_playercfg_patch();
    fpgun_ar_ammo_digit_color_injection.install();
    apply_geomod_mesh_patch();
    // dont init geo meshes at the start, thought was needed but not
    //AsmWriter(0x004B229F).nop(5);
}

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
        set_option(DashOptionID::ScoreboardLogo,
            g_dash_options_config.scoreboard_logo, option_value);
    }
    else if (option_name == "$Default Geomod Mesh") {
        set_option(DashOptionID::GeomodMesh_Default,
            g_dash_options_config.geomodmesh_default, option_value);
    }
    else if (option_name == "$Driller Double Geomod Mesh") {
        set_option(DashOptionID::GeomodMesh_DrillerDouble,
            g_dash_options_config.geomodmesh_driller_double, option_value);
    }
    else if (option_name == "$Driller Single Geomod Mesh") {
        set_option(DashOptionID::GeomodMesh_DrillerSingle,
            g_dash_options_config.geomodmesh_driller_single, option_value);
    }
    else if (option_name == "$APC Geomod Mesh") {
        set_option(DashOptionID::GeomodMesh_APC,
            g_dash_options_config.geomodmesh_apc, option_value);
    }
    else if (option_name == "$Use Base Game Players Config") {
        set_option(DashOptionID::UseStockPlayersConfig,
            g_dash_options_config.use_stock_game_players_config, option_value);
    }
    else if (option_name == "$Assault Rifle Ammo Counter Color") {
        set_option(DashOptionID::AssaultRifleAmmoColor,
            g_dash_options_config.ar_ammo_color, option_value);
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
    std::string line;
    bool in_options_section = false; // track section, eventually this should be enum and support multiple sections

    xlog::warn("Start parsing dashoptions.tbl");

    while (true) {
        std::string buffer(2048, '\0'); // handle lines up to 2048 bytes, should be plenty
        int bytes_read = dashoptions_file->read(&buffer[0], buffer.size() - 1);

        if (bytes_read <= 0) {
            xlog::warn("End of file or read error in dashoptions.tbl.");
            break;
        }

        buffer.resize(bytes_read); // trim unused space if fewer bytes were read
        std::istringstream file_stream(buffer);

        while (std::getline(file_stream, line)) {
            line = trim(line);
            xlog::warn("Parsing line: '{}'", line);

            // could be expanded to support multiple sections
            if (line == "#General") {
                xlog::warn("Entering General section");
                in_options_section = true;
                continue;
            }
            else if (line == "#End") {
                xlog::warn("Exiting General section");
                in_options_section = false;
                break; // stop, reached the end of section
            }

            // skip anything outside the options section
            if (!in_options_section) {
                xlog::warn("Skipping line outside of General section");
                continue;
            }

            // skip empty lines and comments
            if (line.empty() || line.find("//") == 0) {
                xlog::warn("Skipping empty or comment line");
                continue;
            }

            // valid option lines start with $ and contain delimiter :
            if (line[0] != '$' || line.find(':') == std::string::npos) {
                xlog::warn("Skipping malformed line: '{}'", line);
                continue;
            }

            // parse valid options lines
            auto delimiter_pos = line.find(':');
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
