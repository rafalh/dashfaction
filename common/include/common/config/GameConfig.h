#pragma once

#include <string>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <common/config/CfgVar.h>

template<typename T>
struct EnumTrait;

struct GameConfig
{
    // Path
    CfgVar<std::string> game_executable_path{""};

    // Display
    CfgVar<unsigned> res_width{1024, [](auto val) { return std::max(val, 128u); }};
    CfgVar<unsigned> res_height{768, [](auto val) { return std::max(val, 96u); }};
    CfgVar<unsigned> res_bpp = 32;
    CfgVar<unsigned> res_backbuffer_format = 22U; // D3DFMT_X8R8G8B8
    CfgVar<unsigned> selected_video_card = 0;
    enum WndMode
    {
        FULLSCREEN,
        WINDOWED,
        STRETCHED,
    };

    CfgVar<WndMode> wnd_mode = FULLSCREEN;
    CfgVar<bool> vsync = true;
    CfgVar<unsigned> geometry_cache_size{32, [](auto val) { return std::clamp(val, 2u, 32u); }};

    static unsigned min_fps_limit;
    static unsigned max_fps_limit;
    CfgVar<unsigned> max_fps{60, [](auto val) { return std::clamp(val, min_fps_limit, max_fps_limit); }};
    CfgVar<unsigned> server_max_fps{60, [](auto val) { return std::clamp(val, min_fps_limit, max_fps_limit); }};

    enum class Renderer
    {
        // separate values for d3d8/d3d9?
        d3d8 = 0,
        d3d9 = 1,
        d3d11 = 2,
    };
    CfgVar<Renderer> renderer = Renderer::d3d9;

    // Graphics
    CfgVar<bool> fast_anims = false;
    CfgVar<bool> disable_lod_models = true;
    CfgVar<bool> anisotropic_filtering = true;
    CfgVar<bool> nearest_texture_filtering = false;
    CfgVar<unsigned> msaa = 0;


    CfgVar<bool> high_scanner_res = true;
    CfgVar<bool> high_monitor_res = true;
    CfgVar<bool> true_color_textures = true;
    CfgVar<bool> damage_screen_flash = true;
    CfgVar<bool> mesh_static_lighting = true;
    CfgVar<bool> glares = true;
    CfgVar<bool> show_enemy_bullets = true;
    CfgVar<bool> gibs = false;

    static constexpr float min_fov = 75.0f;
    static constexpr float max_fov = 160.0f;
    CfgVar<float> horz_fov{0.0f, [](float val) { return val == 0.0f ? 0.0f : std::clamp(val, min_fov, max_fov); }};
    CfgVar<float> fpgun_fov_scale{1.0f, [](auto val) { return std::clamp(val, 0.1f, 1.5f); }};

    // Audio
    CfgVar<float> level_sound_volume = 1.0f;
    CfgVar<bool> eax_sound = true;

    // Multiplayer
    static const char default_rf_tracker[];
    CfgVar<std::string> tracker{default_rf_tracker};

    static constexpr unsigned default_update_rate = 200000; // T1/LAN in stock launcher
    CfgVar<unsigned> update_rate = default_update_rate;

    CfgVar<unsigned> force_port{0, [](auto val) { return std::min<unsigned>(val, std::numeric_limits<uint16_t>::max()); }};

    // Input
    CfgVar<bool> direct_input = false;
    CfgVar<bool> linear_pitch = false;
    CfgVar<bool> swap_assault_rifle_controls = false;
    CfgVar<bool> swap_grenade_controls = false;
    CfgVar<int> skip_cutscene_ctrl = -1;

    // Interface
    CfgVar<bool> fps_counter = true;
    CfgVar<bool> big_hud = false;
    CfgVar<float> reticle_scale = 1.0f;
    CfgVar<int> language = -1;
    CfgVar<bool> scoreboard_anim = false;

    // Misc
    CfgVar<bool> fast_start = true;
    CfgVar<bool> allow_overwrite_game_files = false;
    CfgVar<bool> keep_launcher_open = false;
    CfgVar<bool> reduced_speed_in_background = false;
    CfgVar<bool> player_join_beep = false;
    CfgVar<bool> autosave = true;

    // Internal
    CfgVar<std::string> dash_faction_version{""};

    bool load();
    void save();
    bool detect_game_path();

private:
    template<typename T>
    bool visit_vars(T&& visitor, bool is_save);
};
