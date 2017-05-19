#include "stdafx.h"
#include "GameConfig.h"
#include "RegKey.h"

const char RF_KEY_NAME[] = "SOFTWARE\\Volition\\Red Faction";
const char DF_SUBKEY_NAME[] = "Dash Faction";

bool GameConfig::load()
{
    RegKey regKey(HKEY_CURRENT_USER, RF_KEY_NAME, KEY_READ);

    regKey.readValue("Resolution Width", &resWidth);
    regKey.readValue("Resolution Height", &resHeight);
    regKey.readValue("Resolution Bit Depth", &resBpp);
    regKey.readValue("Resolution Backbuffer Format", &resBackbufferFormat);
    regKey.readValue("Selected Video Card", &selectedVideoCard);
    regKey.readValue("Vsync", &vsync);
    regKey.readValue("Fast Animations", &fastAnims);
    regKey.readValue("Geometry Cache Size", &geometryCacheSize);
    regKey.readValue("GameTracker", &tracker);
    regKey.readValue("EAX", &eaxSound);
    regKey.readValue("UpdateRate", &updateRate);
    regKey.readValue("ForcePort", &forcePort);

    RegKey dashFactionKey(regKey, DF_SUBKEY_NAME, KEY_READ);
    unsigned temp;
    if (dashFactionKey.readValue("Window Mode", &temp))
        wndMode = (WndMode)temp;
    dashFactionKey.readValue("Disable LOD Models", &disableLodModels);
    dashFactionKey.readValue("Anisotropic Filtering", &anisotropicFiltering);
    dashFactionKey.readValue("MSAA", &msaa);
    dashFactionKey.readValue("FPS Counter", &fpsCounter);
    dashFactionKey.readValue("Max FPS", &maxFps);
    dashFactionKey.readValue("High Scanner Resolution", &highScannerRes);
    dashFactionKey.readValue("True Color Textures", &trueColorTextures);
    if (!dashFactionKey.readValue("Executable Path", &gameExecutablePath))
        detectGamePath();
    dashFactionKey.readValue("Direct Input", &directInput);
    dashFactionKey.readValue("Fast Start", &fastStart);
    dashFactionKey.readValue("Scoreboard Animations", &scoreboardAnim);
    dashFactionKey.readValue("Level Sound Volume", reinterpret_cast<unsigned*>(&levelSoundVolume));
    dashFactionKey.readValue("Allow Overwriting Game Files", &allowOverwriteGameFiles);
    dashFactionKey.readValue("Version", &dashFactionVersion);
    dashFactionKey.readValue("Swap Assault Rifle Controls", &swapAssaultRifleControls);

#ifdef NDEBUG
    if (maxFps > MAX_FPS_LIMIT)
        maxFps = MAX_FPS_LIMIT;
    else if (maxFps < MIN_FPS_LIMIT)
        maxFps = MIN_FPS_LIMIT;
#endif

    return regKey.isOpen();
}

void GameConfig::save()
{
    RegKey regKey(HKEY_CURRENT_USER, RF_KEY_NAME, KEY_WRITE, true);
    regKey.writeValue("Resolution Width", resWidth);
    regKey.writeValue("Resolution Height", resHeight);
    regKey.writeValue("Resolution Bit Depth", resBpp);
    regKey.writeValue("Resolution Backbuffer Format", resBackbufferFormat);
    regKey.writeValue("Selected Video Card", selectedVideoCard);
    regKey.writeValue("Vsync", vsync);
    regKey.writeValue("Fast Animations", fastAnims);
    regKey.writeValue("Geometry Cache Size", geometryCacheSize);
    regKey.writeValue("GameTracker", tracker);
    regKey.writeValue("UpdateRate", updateRate);
    regKey.writeValue("EAX", eaxSound);
    regKey.writeValue("ForcePort", forcePort);

    RegKey dashFactionKey(regKey, DF_SUBKEY_NAME, KEY_WRITE, true);
    dashFactionKey.writeValue("Window Mode", (unsigned)wndMode);
    dashFactionKey.writeValue("Disable LOD Models", disableLodModels);
    dashFactionKey.writeValue("Anisotropic Filtering", anisotropicFiltering);
    dashFactionKey.writeValue("MSAA", msaa);
    dashFactionKey.writeValue("FPS Counter", fpsCounter);
    dashFactionKey.writeValue("Max FPS", maxFps);
    dashFactionKey.writeValue("High Scanner Resolution", highScannerRes);
    dashFactionKey.writeValue("True Color Textures", trueColorTextures);
    dashFactionKey.writeValue("Executable Path", gameExecutablePath);
    dashFactionKey.writeValue("Direct Input", directInput);
    dashFactionKey.writeValue("Fast Start", fastStart);
    dashFactionKey.writeValue("Scoreboard Animations", scoreboardAnim);
    dashFactionKey.writeValue("Level Sound Volume", *reinterpret_cast<unsigned*>(&levelSoundVolume));
    dashFactionKey.writeValue("Allow Overwriting Game Files", allowOverwriteGameFiles);
    dashFactionKey.writeValue("Version", dashFactionVersion);
    dashFactionKey.writeValue("Swap Assault Rifle Controls", swapAssaultRifleControls);
}

bool GameConfig::detectGamePath()
{
    std::string installPath;

    // Standard RF installer
    try
    {
        RegKey regKey(HKEY_LOCAL_MACHINE, RF_KEY_NAME, KEY_READ);
        if (regKey.readValue("InstallPath", &installPath))
        {
            gameExecutablePath = installPath + "\\RF.exe";
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
        RegKey regKey(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\Steam App 20530", KEY_READ);
        if (regKey.readValue("InstallLocation", &installPath))
        {
            gameExecutablePath = installPath + "\\RF.exe";
            return true;
        }
    }
    catch (std::exception)
    {
        // ignore
    }
    
    return false;
}
