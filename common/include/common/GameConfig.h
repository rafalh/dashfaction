#pragma once

#include <string>
#include <common/CfgVar.h>

inline const char DEFAULT_RF_TRACKER[] = "rfgt.factionfiles.com";
inline const char DEFAULT_EXECUTABLE_PATH[] = "C:\\games\\RedFaction\\rf.exe";
constexpr unsigned MIN_FPS_LIMIT = 10u;
constexpr unsigned MAX_FPS_LIMIT = 240u;
constexpr unsigned DEFAULT_UPDATE_RATE = 200000; // T1/LAN in stock launcher

template<typename T>
struct EnumTrait;

struct GameConfig
{
    CfgVar<std::string> game_executable_path{""};

    // Graphics
    CfgVar<unsigned> res_width = 1024;
    CfgVar<unsigned> res_height = 768;
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
    CfgVar<bool> fast_anims = false;
    CfgVar<bool> disable_lod_models = true;
    CfgVar<bool> anisotropic_filtering = true;
    CfgVar<bool> nearest_texture_filtering = false;
    CfgVar<unsigned> msaa = 0;
    CfgVar<unsigned> geometry_cache_size = 32;
    CfgVar<unsigned> max_fps = 60;
    CfgVar<bool> fps_counter = true;
    CfgVar<bool> high_scanner_res = true;
    CfgVar<bool> high_monitor_res = true;
    CfgVar<bool> true_color_textures = true;
    CfgVar<bool> damage_screen_flash = true;
    CfgVar<bool> mesh_static_lighting = true;

    // Multiplayer
    CfgVar<std::string> tracker{DEFAULT_RF_TRACKER};
    CfgVar<unsigned> update_rate = DEFAULT_UPDATE_RATE;
    CfgVar<unsigned> force_port = 0;

    // Misc
    CfgVar<bool> direct_input = false;
    CfgVar<bool> eax_sound = true;
    CfgVar<bool> openal_soft_sound = false;
    CfgVar<bool> fast_start = true;
    CfgVar<bool> allow_overwrite_game_files = false;
    CfgVar<bool> scoreboard_anim = false;
    CfgVar<bool> keep_launcher_open = false;
    CfgVar<int> skip_cutscene_ctrl = -1;
    CfgVar<int> language = -1;
    CfgVar<bool> reduced_speed_in_background = false;
    CfgVar<bool> big_hud = false;
    CfgVar<float> reticle_scale = 1.0f;

    // Advanced
    CfgVar<float> level_sound_volume = 1.0f;
    CfgVar<std::string> dash_faction_version{""};
    CfgVar<bool> swap_assault_rifle_controls = false;
    CfgVar<bool> glares = true;
    CfgVar<bool> show_enemy_bullets = true;
    CfgVar<bool> linear_pitch = false;

    bool load();
    void save();
    bool detect_game_path();

private:
    template<typename T>
    bool visit_vars(T&& visitor, bool is_save);
};
