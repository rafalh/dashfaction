#pragma once

#include <array>
#include <optional>
#include <string>
#include <type_traits>

enum class DashOptionID
{
    ScoreboardLogo,
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
