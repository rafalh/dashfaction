#include <shlwapi.h>
#include <algorithm>
#include <common/config/GameConfig.h>
#include <common/config/RegKey.h>
#include <common/version/version.h>

const char rf_key_name[] = R"(SOFTWARE\Volition\Red Faction)";
const char df_subkey_name[] = "Dash Faction";

const char GameConfig::default_rf_tracker[] = "rfgt.factionfiles.com";

#if VERSION_TYPE == VERSION_TYPE_DEV
unsigned GameConfig::min_fps_limit = 1u;
unsigned GameConfig::max_fps_limit = 10000u;
#else
unsigned GameConfig::min_fps_limit = 10u;
unsigned GameConfig::max_fps_limit = 240u;
#endif

const char fallback_executable_path[] = R"(C:\games\RedFaction\rf.exe)";

bool GameConfig::load() try
{
    bool result = visit_vars([](RegKey& reg_key, const char* name, auto& var) {
        auto value_temp = var.value();
        bool read_success = reg_key.read_value(name, &value_temp);
        var = value_temp;
        return read_success;
    }, false);

    if (game_executable_path.value().empty() && !detect_game_path()) {
        game_executable_path = fallback_executable_path;
    }

    if (update_rate == 0) {
        // Update Rate is set to "None" - this would prevent Multi menu from working - fix it
        update_rate = GameConfig::default_update_rate;
        result = false;
    }

    return result;
}
catch (...) {
    std::throw_with_nested(std::runtime_error("failed to load config"));
}

void GameConfig::save() try
{
    visit_vars([](RegKey& reg_key, const char* name, auto& var) {
        if (var.is_dirty()) {
            reg_key.write_value(name, var.value());
            var.set_dirty(false);
        }
        return true;
    }, true);
}
catch (...) {
    std::throw_with_nested(std::runtime_error("failed to save config"));
}

bool GameConfig::detect_game_path()
{
    std::string install_path;

    // Standard RF installer
    try
    {
        RegKey reg_key(HKEY_LOCAL_MACHINE, rf_key_name, KEY_READ);
        if (reg_key.read_value("InstallPath", &install_path))
        {
            game_executable_path = install_path + "\\RF.exe";
            return true;
        }
    }
    catch (...)
    {
        // ignore
    }

    // Steam
    try
    {
        BOOL Wow64Process = FALSE;
        IsWow64Process(GetCurrentProcess(), &Wow64Process);
        REGSAM reg_sam = KEY_READ;
        if (Wow64Process) {
            reg_sam |= KEY_WOW64_64KEY;
        }
        RegKey reg_key(HKEY_LOCAL_MACHINE, R"(SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\Steam App 20530)", reg_sam);
        if (reg_key.read_value("InstallLocation", &install_path))
        {
            game_executable_path = install_path + "\\RF.exe";
            return true;
        }
    }
    catch (...)
    {
        // ignore
    }

    // GOG
    try
    {
        RegKey reg_key(HKEY_LOCAL_MACHINE, "SOFTWARE\\Nordic Games\\Red Faction", KEY_READ);
        if (reg_key.read_value("INSTALL_DIR", &install_path))
        {
            game_executable_path = install_path + "\\RF.exe";
            return true;
        }
    }
    catch (...)
    {
        // ignore
    }

    char current_dir[MAX_PATH];
    GetCurrentDirectoryA(sizeof(current_dir), current_dir);
    std::string full_path = std::string(current_dir) + "\\RF.exe";
    if (PathFileExistsA(full_path.c_str()))
    {
        game_executable_path = full_path;
        return true;
    }

    return false;
}

template<typename T>
bool GameConfig::visit_vars(T&& visitor, bool is_save)
{
    bool result = true;

    RegKey red_faction_key(HKEY_CURRENT_USER, rf_key_name, is_save ? KEY_WRITE : KEY_READ, is_save);
    result &= red_faction_key.is_open();
    result &= visitor(red_faction_key, "Resolution Width", res_width);
    result &= visitor(red_faction_key, "Resolution Height", res_height);
    result &= visitor(red_faction_key, "Resolution Bit Depth", res_bpp);
    result &= visitor(red_faction_key, "Resolution Backbuffer Format", res_backbuffer_format);
    result &= visitor(red_faction_key, "Selected Video Card", selected_video_card);
    result &= visitor(red_faction_key, "Vsync", vsync);
    result &= visitor(red_faction_key, "Fast Animations", fast_anims);
    result &= visitor(red_faction_key, "Geometry Cache Size", geometry_cache_size);
    result &= visitor(red_faction_key, "GameTracker", tracker);
    result &= visitor(red_faction_key, "UpdateRate", update_rate);
    result &= visitor(red_faction_key, "EAX", eax_sound);
    result &= visitor(red_faction_key, "ForcePort", force_port);

    RegKey dash_faction_key(red_faction_key, df_subkey_name, is_save ? KEY_WRITE : KEY_READ, is_save);
    result &= dash_faction_key.is_open();
    result &= visitor(dash_faction_key, "Window Mode", wnd_mode);
    result &= visitor(dash_faction_key, "Disable LOD Models", disable_lod_models);
    result &= visitor(dash_faction_key, "Anisotropic Filtering", anisotropic_filtering);
    result &= visitor(dash_faction_key, "Nearest Texture Filtering", nearest_texture_filtering);
    result &= visitor(dash_faction_key, "MSAA", msaa);
    result &= visitor(dash_faction_key, "FPS Counter", fps_counter);
    result &= visitor(dash_faction_key, "Max FPS", max_fps);
    result &= visitor(dash_faction_key, "Server Max FPS", server_max_fps);
    result &= visitor(dash_faction_key, "High Scanner Resolution", high_scanner_res);
    result &= visitor(dash_faction_key, "High Monitor Resolution", high_monitor_res);
    result &= visitor(dash_faction_key, "True Color Textures", true_color_textures);
    result &= visitor(dash_faction_key, "Renderer", renderer);
    result &= visitor(dash_faction_key, "Horizontal FOV", horz_fov);
    result &= visitor(dash_faction_key, "Fpgun FOV Scale", fpgun_fov_scale);
    result &= visitor(dash_faction_key, "Executable Path", game_executable_path);
    result &= visitor(dash_faction_key, "Direct Input", direct_input);
    result &= visitor(dash_faction_key, "Fast Start", fast_start);
    result &= visitor(dash_faction_key, "Scoreboard Animations", scoreboard_anim);
    result &= visitor(dash_faction_key, "Level Sound Volume", level_sound_volume);
    result &= visitor(dash_faction_key, "Allow Overwriting Game Files", allow_overwrite_game_files);
    result &= visitor(dash_faction_key, "Version", dash_faction_version);
    result &= visitor(dash_faction_key, "Swap Assault Rifle Controls", swap_assault_rifle_controls);
    result &= visitor(dash_faction_key, "Swap Grenade Controls", swap_grenade_controls);
    result &= visitor(dash_faction_key, "Glares", glares);
    result &= visitor(dash_faction_key, "Gibs", gibs);
    result &= visitor(dash_faction_key, "Linear Pitch", linear_pitch);
    result &= visitor(dash_faction_key, "Show Enemy Bullets", show_enemy_bullets);
    result &= visitor(dash_faction_key, "Keep Launcher Open", keep_launcher_open);
    result &= visitor(dash_faction_key, "Skip Cutscene Control", skip_cutscene_ctrl);
    result &= visitor(dash_faction_key, "Damage Screen Flash", damage_screen_flash);
    result &= visitor(dash_faction_key, "Language", language);
    result &= visitor(dash_faction_key, "Reduced Speed In Background", reduced_speed_in_background);
    result &= visitor(dash_faction_key, "Big HUD", big_hud);
    result &= visitor(dash_faction_key, "Reticle Scale", reticle_scale);
    result &= visitor(dash_faction_key, "Mesh Static Lighting", mesh_static_lighting);
    result &= visitor(dash_faction_key, "Player Join Beep", player_join_beep);
    result &= visitor(dash_faction_key, "Autosave", autosave);

    return result;
}

template<>
bool is_valid_enum_value<GameConfig::WndMode>(int value)
{
    return value == GameConfig::FULLSCREEN
        || value == GameConfig::WINDOWED
        || value == GameConfig::STRETCHED;
}

template<>
bool is_valid_enum_value<GameConfig::Renderer>(int value)
{
    return value == static_cast<int>(GameConfig::Renderer::d3d8)
        || value == static_cast<int>(GameConfig::Renderer::d3d9)
        || value == static_cast<int>(GameConfig::Renderer::d3d11);
}
