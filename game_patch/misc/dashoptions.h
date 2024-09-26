#pragma once

#include <array>
#include <optional>
#include <string>
#include <type_traits>

enum class DashOptionID
{
    ScoreboardLogo,
    GeomodMesh_Default,
    GeomodMesh_DrillerDouble,
    GeomodMesh_DrillerSingle,
    GeomodMesh_APC,
    GeomodEmitter_Default,
    GeomodEmitter_Driller,
    GeomodTexture_Ice,
    FirstLevelFilename,
    TrainingLevelFilename,
    DisableMultiplayerButton,
    DisableSingleplayerButtons,
    UseStockPlayersConfig,
    AssaultRifleAmmoColor,
    _optioncount // dummy option for determining total num of options
};

// convert enum to its underlying type
constexpr std::size_t to_index(DashOptionID option_id)
{
    return static_cast<std::size_t>(option_id);
}

// total number of options in DashOptionID
constexpr std::size_t option_count = to_index(DashOptionID::_optioncount) + 1;

struct DashOptionsConfig
{
    //std::optional<float> float_something; // template for float
    //std::optional<int> int_something; // template for int
    std::optional<std::string> scoreboard_logo;
    std::optional<std::string> geomodmesh_default;
    std::optional<std::string> geomodmesh_driller_double;
    std::optional<std::string> geomodmesh_driller_single;
    std::optional<std::string> geomodmesh_apc;
    std::optional<std::string> geomodemitter_default;
    std::optional<std::string> geomodemitter_driller;
    std::optional<std::string> geomodtexture_ice;
    std::optional<std::string> first_level_filename;
    std::optional<std::string> training_level_filename;
    std::optional<bool> disable_multiplayer_button;
    std::optional<bool> disable_singleplayer_buttons;
    std::optional<bool> use_stock_game_players_config;
    std::optional<uint32_t> ar_ammo_color;

    // track options that are loaded
    std::array<bool, option_count> options_loaded = {};

    // check if specific option is loaded
    bool is_option_loaded(DashOptionID option_id) const
    {
        return options_loaded[to_index(option_id)];
    }
};

extern DashOptionsConfig g_dash_options_config;

namespace dashopt
{
void load_dashoptions_config();
}
