#pragma once

#include <string>

inline const char DEFAULT_RF_TRACKER[] = "rfgt.factionfiles.com";
inline const char DEFAULT_EXECUTABLE_PATH[] = "C:\\games\\RedFaction\\rf.exe";
constexpr unsigned MIN_FPS_LIMIT = 10u;
constexpr unsigned MAX_FPS_LIMIT = 240u;
constexpr unsigned DEFAULT_UPDATE_RATE = 200000; // T1/LAN in stock launcher

struct GameConfig
{
    std::string game_executable_path = DEFAULT_EXECUTABLE_PATH;

    // Graphics
    unsigned res_width = 1024;
    unsigned res_height = 768;
    unsigned res_bpp = 32;
    unsigned res_backbuffer_format = 22U; // D3DFMT_X8R8G8B8
    unsigned selected_video_card = 0;
    enum WndMode
    {
        FULLSCREEN,
        WINDOWED,
        STRETCHED
    };
    WndMode wnd_mode = FULLSCREEN;
    bool vsync = true;
    bool fast_anims = false;
    bool disable_lod_models = true;
    bool anisotropic_filtering = true;
    unsigned msaa = 0;
    unsigned geometry_cache_size = 32;
    unsigned max_fps = 60;
    bool fps_counter = true;
    bool high_scanner_res = true;
    bool high_monitor_res = true;
    bool true_color_textures = true;
    bool screen_flash = true;

    // Multiplayer
    std::string tracker = DEFAULT_RF_TRACKER;
    unsigned update_rate = DEFAULT_UPDATE_RATE;
    unsigned force_port = 0;

    // Misc
    bool direct_input = false;
    bool eax_sound = true;
    bool fast_start = true;
    bool allow_overwrite_game_files = false;
    bool scoreboard_anim = false;
    bool keep_launcher_open = false;
    int skip_cutscene_ctrl = -1;
    int language = -1;
    bool reduced_speed_in_background = false;

    // Advanced
    float level_sound_volume = 1.0f;
    std::string dash_faction_version;
    bool swap_assault_rifle_controls = false;
    bool glares = true;
    bool show_enemy_bullets = true;
    bool linear_pitch = false;

    bool load();
    void save();
    bool detect_game_path();
};
