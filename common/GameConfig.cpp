#include "GameConfig.h"
#include "RegKey.h"
#include <shlwapi.h>
#include <algorithm>

const char rf_key_name[] = "SOFTWARE\\Volition\\Red Faction";
const char df_subkey_name[] = "Dash Faction";

bool GameConfig::load() try
{
    bool result = true;

    RegKey reg_key(HKEY_CURRENT_USER, rf_key_name, KEY_READ);
    result &= reg_key.is_open();

    result &= reg_key.read_value("Resolution Width", &res_width);
    result &= reg_key.read_value("Resolution Height", &res_height);
    result &= reg_key.read_value("Resolution Bit Depth", &res_bpp);
    result &= reg_key.read_value("Resolution Backbuffer Format", &res_backbuffer_format);
    result &= reg_key.read_value("Selected Video Card", &selected_video_card);
    result &= reg_key.read_value("Vsync", &vsync);
    result &= reg_key.read_value("Fast Animations", &fast_anims);
    result &= reg_key.read_value("Geometry Cache Size", &geometry_cache_size);
    result &= reg_key.read_value("GameTracker", &tracker);
    result &= reg_key.read_value("EAX", &eax_sound);
    result &= reg_key.read_value("UpdateRate", &update_rate);
    result &= reg_key.read_value("ForcePort", &force_port);

    RegKey dash_faction_key(reg_key, df_subkey_name, KEY_READ);
    result &= reg_key.is_open();
    unsigned temp;
    if (dash_faction_key.read_value("Window Mode", &temp))
        wnd_mode = (WndMode)temp;
    result &= dash_faction_key.read_value("Disable LOD Models", &disable_lod_models);
    result &= dash_faction_key.read_value("Anisotropic Filtering", &anisotropic_filtering);
    result &= dash_faction_key.read_value("MSAA", &msaa);
    result &= dash_faction_key.read_value("FPS Counter", &fps_counter);
    result &= dash_faction_key.read_value("Max FPS", &max_fps);
    result &= dash_faction_key.read_value("High Scanner Resolution", &high_scanner_res);
    result &= dash_faction_key.read_value("High Monitor Resolution", &high_monitor_res);
    result &= dash_faction_key.read_value("True Color Textures", &true_color_textures);
    if (!dash_faction_key.read_value("Executable Path", &game_executable_path))
        detect_game_path();
    result &= dash_faction_key.read_value("Direct Input", &direct_input);
    result &= dash_faction_key.read_value("Fast Start", &fast_start);
    result &= dash_faction_key.read_value("Scoreboard Animations", &scoreboard_anim);
    result &= dash_faction_key.read_value("Level Sound Volume", &level_sound_volume);
    result &= dash_faction_key.read_value("Allow Overwriting Game Files", &allow_overwrite_game_files);
    result &= dash_faction_key.read_value("Version", &dash_faction_version);
    result &= dash_faction_key.read_value("Swap Assault Rifle Controls", &swap_assault_rifle_controls);
    result &= dash_faction_key.read_value("Glares", &glares);
    result &= dash_faction_key.read_value("Linear Pitch", &linear_pitch);
    result &= dash_faction_key.read_value("Show Enemy Bullets", &show_enemy_bullets);
    result &= dash_faction_key.read_value("Keep Launcher Open", &keep_launcher_open);
    result &= dash_faction_key.read_value("Skip Cutscene Control", &skip_cutscene_ctrl);
    result &= dash_faction_key.read_value("Screen Flash", &screen_flash);

#ifdef NDEBUG
    max_fps = std::clamp(max_fps, MIN_FPS_LIMIT, MAX_FPS_LIMIT);
#endif

    if (update_rate == 0) {
        // Update Rate is set to "None" - this would prevent Multi menu from working - fix it
        update_rate = DEFAULT_UPDATE_RATE;
        result = false;
    }

    return result;
}
catch (...) {
    std::throw_with_nested(std::runtime_error("failed to load config"));
}

void GameConfig::save() try
{
    RegKey reg_key(HKEY_CURRENT_USER, rf_key_name, KEY_WRITE, true);
    reg_key.write_value("Resolution Width", res_width);
    reg_key.write_value("Resolution Height", res_height);
    reg_key.write_value("Resolution Bit Depth", res_bpp);
    reg_key.write_value("Resolution Backbuffer Format", res_backbuffer_format);
    reg_key.write_value("Selected Video Card", selected_video_card);
    reg_key.write_value("Vsync", vsync);
    reg_key.write_value("Fast Animations", fast_anims);
    reg_key.write_value("Geometry Cache Size", geometry_cache_size);
    reg_key.write_value("GameTracker", tracker);
    reg_key.write_value("UpdateRate", update_rate);
    reg_key.write_value("EAX", eax_sound);
    reg_key.write_value("ForcePort", force_port);

    RegKey dash_faction_key(reg_key, df_subkey_name, KEY_WRITE, true);
    dash_faction_key.write_value("Window Mode", (unsigned)wnd_mode);
    dash_faction_key.write_value("Disable LOD Models", disable_lod_models);
    dash_faction_key.write_value("Anisotropic Filtering", anisotropic_filtering);
    dash_faction_key.write_value("MSAA", msaa);
    dash_faction_key.write_value("FPS Counter", fps_counter);
    dash_faction_key.write_value("Max FPS", max_fps);
    dash_faction_key.write_value("High Scanner Resolution", high_scanner_res);
    dash_faction_key.write_value("High Monitor Resolution", high_monitor_res);
    dash_faction_key.write_value("True Color Textures", true_color_textures);
    dash_faction_key.write_value("Executable Path", game_executable_path);
    dash_faction_key.write_value("Direct Input", direct_input);
    dash_faction_key.write_value("Fast Start", fast_start);
    dash_faction_key.write_value("Scoreboard Animations", scoreboard_anim);
    dash_faction_key.write_value("Level Sound Volume", level_sound_volume);
    dash_faction_key.write_value("Allow Overwriting Game Files", allow_overwrite_game_files);
    dash_faction_key.write_value("Version", dash_faction_version);
    dash_faction_key.write_value("Swap Assault Rifle Controls", swap_assault_rifle_controls);
    dash_faction_key.write_value("Glares", &glares);
    dash_faction_key.write_value("Linear Pitch", linear_pitch);
    dash_faction_key.write_value("Show Enemy Bullets", show_enemy_bullets);
    dash_faction_key.write_value("Keep Launcher Open", keep_launcher_open);
    dash_faction_key.write_value("Skip Cutscene Control", skip_cutscene_ctrl);
    dash_faction_key.write_value("Screen Flash", screen_flash);
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
    catch (std::exception)
    {
        // ignore
    }

    // Steam
    try
    {
        RegKey reg_key(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\Steam App 20530", KEY_READ);
        if (reg_key.read_value("InstallLocation", &install_path))
        {
            game_executable_path = install_path + "\\RF.exe";
            return true;
        }
    }
    catch (std::exception)
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
