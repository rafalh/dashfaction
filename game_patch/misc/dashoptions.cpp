#include "dashoptions.h"
#include <patch_common/CallHook.h>
#include <patch_common/FunHook.h>
#include <patch_common/CodeInjection.h>
#include <patch_common/ShortTypes.h>
#include <patch_common/AsmWriter.h>
#include "../rf/file/file.h"
#include "../rf/gr/gr.h"
#include "../rf/sound/sound.h"
#include "../rf/geometry.h"
#include "../rf/misc.h"
#include <algorithm>
#include <memory>
#include <sstream>
#include <string>
#include <stdexcept>
#include <windows.h>
#include <shellapi.h>
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

void open_url(const std::string& url)
{
    try {
        if (url.empty()) {
            xlog::error("URL is empty");
            return;
        }
        xlog::info("Opening URL: {}", url);
        HINSTANCE result = ShellExecuteA(nullptr, "open", url.c_str(), nullptr, nullptr, SW_SHOW);
        if (reinterpret_cast<int>(result) <= 32) {
            xlog::error("Failed to open URL. Error code: {}", reinterpret_cast<int>(result));
        }
    }
    catch (const std::exception& ex) {
        xlog::error("Exception occurred while trying to open URL: {}", ex.what());
    }
}

CodeInjection fpgun_ar_ammo_digit_color_injection{
    0x004ABC03,
    [](auto& regs) {
        uint32_t color = g_dash_options_config.ar_ammo_color.value();
        rf::gr::set_color((color >> 24) & 0xFF, // r
                            (color >> 16) & 0xFF, // g
                            (color >> 8) & 0xFF,  // b 
                            color & 0xFF);        // a              
        regs.eip = 0x004ABC08;
    }
};

// consolidated logic for handling geo mesh changes
int handle_geomod_shape_create(const char* filename, const std::optional<std::string>& config_value,
                               CallHook<int(const char*)>& hook)
{
    std::string original_filename{filename};
    std::string modded_filename = config_value.value_or(original_filename);
    return hook.call_target(modded_filename.c_str());
}

// Set default geo mesh
CallHook<int(const char*)> default_geomod_shape_create_hook{
    0x004374CF, [](const char* filename) -> int {
        return handle_geomod_shape_create(filename,
            g_dash_options_config.geomodmesh_default, default_geomod_shape_create_hook);
    }
};

// Set driller double geo mesh
CallHook<int(const char*)> driller_double_geomod_shape_create_hook{
    0x004374D9, [](const char* filename) -> int {
        return handle_geomod_shape_create(filename,
            g_dash_options_config.geomodmesh_driller_double, driller_double_geomod_shape_create_hook);
    }
};

// Set driller single geo mesh
CallHook<int(const char*)> driller_single_geomod_shape_create_hook{
    0x004374E3, [](const char* filename) -> int {
        return handle_geomod_shape_create(filename,
            g_dash_options_config.geomodmesh_driller_single, driller_single_geomod_shape_create_hook);
    }
};

// Set apc geo mesh
CallHook<int(const char*)> apc_geomod_shape_create_hook{
    0x004374ED, [](const char* filename) -> int {
        return handle_geomod_shape_create(filename,
            g_dash_options_config.geomodmesh_apc, apc_geomod_shape_create_hook);
    }
};

void apply_geomod_mesh_patch()
{
    // array of geomod mesh options
    std::array<std::pair<DashOptionID, void (*)()>, 4> geomod_mesh_hooks = {
        {{DashOptionID::GeomodMesh_Default, [] { default_geomod_shape_create_hook.install(); }},
         {DashOptionID::GeomodMesh_DrillerDouble, [] { driller_double_geomod_shape_create_hook.install(); }},
         {DashOptionID::GeomodMesh_DrillerSingle, [] { driller_single_geomod_shape_create_hook.install(); }},
         {DashOptionID::GeomodMesh_APC, [] { apc_geomod_shape_create_hook.install(); }}
        }};

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

// consolidated logic for handling geomod smoke emitter overrides
int handle_geomod_emitter_change(const char* emitter_name,
    const std::optional<std::string>& new_emitter_name_opt, CallHook<int(const char*)>& hook) {
    std::string original_emitter_name{emitter_name};
    std::string new_emitter_name = new_emitter_name_opt.value_or(original_emitter_name);
    return hook.call_target(new_emitter_name.c_str());
}

// Override default geomod smoke emitter
CallHook<int(const char*)> default_geomod_emitter_get_index_hook{
    0x00437150, [](const char* emitter_name) -> int {
        return handle_geomod_emitter_change(emitter_name,
            g_dash_options_config.geomodemitter_default, default_geomod_emitter_get_index_hook);
    }
};

// Override driller geomod smoke emitter
CallHook<int(const char*)> driller_geomod_emitter_get_index_hook{
    0x0043715F, [](const char* emitter_name) -> int {
        return handle_geomod_emitter_change(emitter_name,
            g_dash_options_config.geomodemitter_driller, driller_geomod_emitter_get_index_hook);
    }
};

// geomod crater texture PPM broken currently and disabled, need to fix
CallHook<void(rf::GSolid*, float)> geomod_set_autotexture_ppm_hook{
    0x00466BD4,
    [](rf::GSolid* this_ptr, float ppm) {
        xlog::info("Original PPM: {}", ppm);

        float custom_ppm = 24.0f;
        xlog::info("Modified PPM: {}", custom_ppm);

        geomod_set_autotexture_ppm_hook.call_target(this_ptr, custom_ppm);
    }
};

// replace ice geomod region texture
CallHook<int(const char*, int, bool)> ice_geo_crater_bm_load_hook {
    {
        0x004673B5, // chunks
        0x00466BEF, // crater
    },
    [](const char* filename, int path_id, bool generate_mipmaps) -> int {
        std::string original_filename{filename};
        std::string new_filename = g_dash_options_config.geomodtexture_ice.value_or(original_filename);
        return ice_geo_crater_bm_load_hook.call_target(new_filename.c_str(), path_id, generate_mipmaps);
    }
};

// Consolidated logic for handling level filename overrides
inline void handle_level_name_change(const char* level_name, 
    const std::optional<std::string>& new_level_name_opt, CallHook<void(const char*)>& hook) {
    std::string new_level_name = new_level_name_opt.value_or(level_name);
    hook.call_target(new_level_name.c_str());
}

// Override first level filename for new game menu
CallHook<void(const char*)> first_load_level_hook{
    0x00443B15, [](const char* level_name) {
        handle_level_name_change(level_name, g_dash_options_config.first_level_filename, first_load_level_hook);
    }
};

// Override training level filename for new game menu
CallHook<void(const char*)> training_load_level_hook{
    0x00443A85, [](const char* level_name) {
        handle_level_name_change(level_name, g_dash_options_config.training_level_filename, training_load_level_hook);
    }
};

// Implement demo_extras_summoner_trailer_click using FunHook
FunHook<void(int, int)> extras_summoner_trailer_click_hook{
    0x0043EC80, [](int x, int y) {
        xlog::warn("Summoner trailer button clicked");
        int action = g_dash_options_config.sumtrailer_button_action.value_or(0);
        switch (action) {
        case 1:
            // open URL
            if (g_dash_options_config.sumtrailer_button_url) {
                const std::string& url = *g_dash_options_config.sumtrailer_button_url;
                open_url(url);
            }
            break;
        case 2:
            // disable button
            break;
        default:
            // play bink video, is case 0 but also default
            std::string trailer_path = g_dash_options_config.sumtrailer_button_bik_filename.value_or("sumtrailer.bik");
            xlog::warn("Playing BIK file: {}", trailer_path);
            rf::snd_pause(true);
            rf::bink_play(trailer_path.c_str());
            rf::snd_pause(false);
            break;
        }
    }
};

void handle_summoner_trailer_button()
{
    if (int action = g_dash_options_config.sumtrailer_button_action.value_or(-1); action != -1) {
        xlog::warn("Action ID: {}", g_dash_options_config.sumtrailer_button_action.value_or(-1));
        if (action == 3) {
            // action 3 means remove the button
            AsmWriter(0x0043EE14).nop(5);
        }
        else {
            // otherwise, install the hook
            extras_summoner_trailer_click_hook.install();
        }
    }
}

void apply_dashoptions_patches()
{
    xlog::warn("Applying Dash Options patches");
    // avoid unnecessary hooks by hooking only if corresponding options are specified

    // apply UseStockPlayersConfig
    if (g_dash_options_config.use_stock_game_players_config.value_or(false)) {
        // set mod to not make its own players.cfg but instead use the stock game one
        AsmWriter(0x004A8F99).jmp(0x004A9010);
        AsmWriter(0x004A8DCC).jmp(0x004A8E53);
    }

    // apply AR ammo counter coloring
    if (g_dash_options_config.is_option_loaded(DashOptionID::AssaultRifleAmmoColor)) {
        fpgun_ar_ammo_digit_color_injection.install();        
    }         

    // whether should apply is determined in helper function
    apply_geomod_mesh_patch();

    // apply default geomod smoke emitter
    if (g_dash_options_config.is_option_loaded(DashOptionID::GeomodEmitter_Default)) {
        default_geomod_emitter_get_index_hook.install();
    }

    // apply driller geomod smoke emitter
    if (g_dash_options_config.is_option_loaded(DashOptionID::GeomodEmitter_Driller)) {
        driller_geomod_emitter_get_index_hook.install();
    }    

    // broken currently, need to fix
    //geomod_set_autotexture_ppm_hook.install();

    if (g_dash_options_config.is_option_loaded(DashOptionID::GeomodTexture_Ice)) {
        ice_geo_crater_bm_load_hook.install();
    }

    // first level filename from new game menu
    if (g_dash_options_config.is_option_loaded(DashOptionID::FirstLevelFilename)) {
        first_load_level_hook.install();
    }

    // training level filename from new game menu
    if (g_dash_options_config.is_option_loaded(DashOptionID::TrainingLevelFilename)) {
        training_load_level_hook.install();
    }

    // disable multiplayer button
    if (g_dash_options_config.disable_multiplayer_button.value_or(false)) {
        AsmWriter(0x0044391F).nop(5); // multi
    }

    // disable singleplayer buttons
    if (g_dash_options_config.disable_singleplayer_buttons.value_or(false)) {
        AsmWriter(0x00443906).nop(5); // save
        AsmWriter(0x004438ED).nop(5); // load
        AsmWriter(0x004438D4).nop(5); // new game
    }

    // customize behaviour of Summoner Trailer button
    if (g_dash_options_config.is_option_loaded(DashOptionID::SumTrailerButtonAction)) {
        handle_summoner_trailer_button();
    }
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

    //core options
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
    else if (option_name == "$Default Geomod Smoke Emitter") {
        set_option(DashOptionID::GeomodEmitter_Default,
            g_dash_options_config.geomodemitter_default, option_value);
    }
    else if (option_name == "$Driller Geomod Smoke Emitter") {
        set_option(DashOptionID::GeomodEmitter_Driller,
            g_dash_options_config.geomodemitter_driller, option_value);
    }
    else if (option_name == "$Ice Geomod Texture") {
        set_option(DashOptionID::GeomodTexture_Ice,
            g_dash_options_config.geomodtexture_ice, option_value);
    }
    else if (option_name == "$First Level Filename") {
        set_option(DashOptionID::FirstLevelFilename,
            g_dash_options_config.first_level_filename, option_value);
    }
    else if (option_name == "$Training Level Filename") {
        set_option(DashOptionID::TrainingLevelFilename,
            g_dash_options_config.training_level_filename, option_value);
    }
    else if (option_name == "$Disable Multiplayer Button") {
        set_option(DashOptionID::DisableMultiplayerButton,
            g_dash_options_config.disable_multiplayer_button, option_value);
    }
    else if (option_name == "$Disable Singleplayer Buttons") {
        set_option(DashOptionID::DisableSingleplayerButtons,
            g_dash_options_config.disable_singleplayer_buttons, option_value);
    }
    else if (option_name == "$Use Base Game Players Config") {
        set_option(DashOptionID::UseStockPlayersConfig,
            g_dash_options_config.use_stock_game_players_config, option_value);
    }
    else if (option_name == "$Assault Rifle Ammo Counter Color") {
        set_option(DashOptionID::AssaultRifleAmmoColor,
            g_dash_options_config.ar_ammo_color, option_value);
    }
    else if (option_name == "$Summoner Trailer Button Action") {
        set_option(DashOptionID::SumTrailerButtonAction,
            g_dash_options_config.sumtrailer_button_action, option_value);
        // 0 = play_bik (default), 1 = open_url, 2 = disable, 3 = remove
    }

    //extended options
    else if (option_name == "+Summoner Trailer Button URL" &&
        g_dash_options_config.is_option_loaded(DashOptionID::SumTrailerButtonAction)) {
        set_option(DashOptionID::SumTrailerButtonURL,
            g_dash_options_config.sumtrailer_button_url, option_value);
    }
    else if (option_name == "+Summoner Trailer Button Bink Filename" &&
        g_dash_options_config.is_option_loaded(DashOptionID::SumTrailerButtonAction)) {
        set_option(DashOptionID::SumTrailerButtonBikFile,
            g_dash_options_config.sumtrailer_button_bik_filename, option_value);
    }

    //unknown option
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

            // valid option lines start with $ or + and contain a delimiter :
            if ((line[0] != '$' && line[0] != '+') || line.find(':') == std::string::npos) {
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
