#include "GameConfig.h"
#include "RegKey.h"
#include <shlwapi.h>
#include <algorithm>

const char rf_key_name[] = "SOFTWARE\\Volition\\Red Faction";
const char df_subkey_name[] = "Dash Faction";

bool GameConfig::load()
{
    RegKey reg_key(HKEY_CURRENT_USER, rf_key_name, KEY_READ);

    reg_key.read_value("Resolution Width", &resWidth);
    reg_key.read_value("Resolution Height", &resHeight);
    reg_key.read_value("Resolution Bit Depth", &resBpp);
    reg_key.read_value("Resolution Backbuffer Format", &resBackbufferFormat);
    reg_key.read_value("Selected Video Card", &selectedVideoCard);
    reg_key.read_value("Vsync", &vsync);
    reg_key.read_value("Fast Animations", &fastAnims);
    reg_key.read_value("Geometry Cache Size", &geometryCacheSize);
    reg_key.read_value("GameTracker", &tracker);
    reg_key.read_value("EAX", &eaxSound);
    reg_key.read_value("UpdateRate", &updateRate);
    reg_key.read_value("ForcePort", &forcePort);

    RegKey dash_faction_key(reg_key, df_subkey_name, KEY_READ);
    unsigned temp;
    if (dash_faction_key.read_value("Window Mode", &temp))
        wndMode = (WndMode)temp;
    dash_faction_key.read_value("Disable LOD Models", &disableLodModels);
    dash_faction_key.read_value("Anisotropic Filtering", &anisotropicFiltering);
    dash_faction_key.read_value("MSAA", &msaa);
    dash_faction_key.read_value("FPS Counter", &fpsCounter);
    dash_faction_key.read_value("Max FPS", &maxFps);
    dash_faction_key.read_value("High Scanner Resolution", &highScannerRes);
    dash_faction_key.read_value("High Monitor Resolution", &highMonitorRes);
    dash_faction_key.read_value("True Color Textures", &trueColorTextures);
    if (!dash_faction_key.read_value("Executable Path", &gameExecutablePath))
        detectGamePath();
    dash_faction_key.read_value("Direct Input", &directInput);
    dash_faction_key.read_value("Fast Start", &fastStart);
    dash_faction_key.read_value("Scoreboard Animations", &scoreboardAnim);
    dash_faction_key.read_value("Level Sound Volume", reinterpret_cast<unsigned*>(&levelSoundVolume));
    dash_faction_key.read_value("Allow Overwriting Game Files", &allowOverwriteGameFiles);
    dash_faction_key.read_value("Version", &dashFactionVersion);
    dash_faction_key.read_value("Swap Assault Rifle Controls", &swapAssaultRifleControls);
    dash_faction_key.read_value("Glares", &glares);
    dash_faction_key.read_value("Linear Pitch", &linearPitch);
    dash_faction_key.read_value("Show Enemy Bullets", &showEnemyBullets);
    dash_faction_key.read_value("Keep Launcher Open", &keepLauncherOpen);

#ifdef NDEBUG
    maxFps = std::clamp(maxFps, MIN_FPS_LIMIT, MAX_FPS_LIMIT);
#endif

    return reg_key.is_open();
}

void GameConfig::save()
{
    RegKey reg_key(HKEY_CURRENT_USER, rf_key_name, KEY_WRITE, true);
    reg_key.write_value("Resolution Width", resWidth);
    reg_key.write_value("Resolution Height", resHeight);
    reg_key.write_value("Resolution Bit Depth", resBpp);
    reg_key.write_value("Resolution Backbuffer Format", resBackbufferFormat);
    reg_key.write_value("Selected Video Card", selectedVideoCard);
    reg_key.write_value("Vsync", vsync);
    reg_key.write_value("Fast Animations", fastAnims);
    reg_key.write_value("Geometry Cache Size", geometryCacheSize);
    reg_key.write_value("GameTracker", tracker);
    reg_key.write_value("UpdateRate", updateRate);
    reg_key.write_value("EAX", eaxSound);
    reg_key.write_value("ForcePort", forcePort);

    RegKey dash_faction_key(reg_key, df_subkey_name, KEY_WRITE, true);
    dash_faction_key.write_value("Window Mode", (unsigned)wndMode);
    dash_faction_key.write_value("Disable LOD Models", disableLodModels);
    dash_faction_key.write_value("Anisotropic Filtering", anisotropicFiltering);
    dash_faction_key.write_value("MSAA", msaa);
    dash_faction_key.write_value("FPS Counter", fpsCounter);
    dash_faction_key.write_value("Max FPS", maxFps);
    dash_faction_key.write_value("High Scanner Resolution", highScannerRes);
    dash_faction_key.write_value("High Monitor Resolution", highMonitorRes);
    dash_faction_key.write_value("True Color Textures", trueColorTextures);
    dash_faction_key.write_value("Executable Path", gameExecutablePath);
    dash_faction_key.write_value("Direct Input", directInput);
    dash_faction_key.write_value("Fast Start", fastStart);
    dash_faction_key.write_value("Scoreboard Animations", scoreboardAnim);
    dash_faction_key.write_value("Level Sound Volume", *reinterpret_cast<unsigned*>(&levelSoundVolume));
    dash_faction_key.write_value("Allow Overwriting Game Files", allowOverwriteGameFiles);
    dash_faction_key.write_value("Version", dashFactionVersion);
    dash_faction_key.write_value("Swap Assault Rifle Controls", swapAssaultRifleControls);
    dash_faction_key.write_value("Glares", &glares);
    dash_faction_key.write_value("Linear Pitch", linearPitch);
    dash_faction_key.write_value("Show Enemy Bullets", showEnemyBullets);
    dash_faction_key.write_value("Keep Launcher Open", keepLauncherOpen);
}

bool GameConfig::detectGamePath()
{
    std::string install_path;

    // Standard RF installer
    try
    {
        RegKey reg_key(HKEY_LOCAL_MACHINE, rf_key_name, KEY_READ);
        if (reg_key.read_value("InstallPath", &install_path))
        {
            gameExecutablePath = install_path + "\\RF.exe";
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
            gameExecutablePath = install_path + "\\RF.exe";
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
        gameExecutablePath = full_path;
        return true;
    }

    return false;
}
