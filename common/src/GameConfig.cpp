#include "stdafx.h"
#include "GameConfig.h"
#include "RegKey.h"

const char RF_KEY_NAME[] = "SOFTWARE\\Volition\\Red Faction";
const char DF_SUBKEY_NAME[] = "Dash Faction";

void GameConfig::setDefaults()
{
    gameExecutablePath = "C:\\games\\RedFaction\\rf.exe";
    resWidth = 1024;
    resHeight = 768;
    resBpp = 32;
    wndMode = FULLSCREEN;
    vsync = true;
    fastAnims = false;
    disableLodModels = true;
    anisotropicFiltering = true;
    msaa = 4;
    geometryCacheSize = 32;

    tracker = DEFAULT_RF_TRACKER;
    directInput = false;
    eaxSound = true;
}

void GameConfig::load()
{
    setDefaults(); // FIXME

    RegKey regKey(HKEY_CURRENT_USER, RF_KEY_NAME, KEY_READ|KEY_CREATE_SUB_KEY);

    regKey.readValue("Resolution Width", &resWidth);
    regKey.readValue("Resolution Height", &resHeight);
    regKey.readValue("Resolution Bit Depth", &resBpp);
    regKey.readValue("Vsync", &vsync);
    regKey.readValue("Fast Animations", &fastAnims);
    regKey.readValue("Geometry Cache Size", &geometryCacheSize);
    regKey.readValue("GameTracker", &tracker);
    regKey.readValue("EAX", &eaxSound);

    RegKey dashFactionKey(regKey, DF_SUBKEY_NAME, KEY_READ);
    unsigned temp;
    if (dashFactionKey.readValue("Window Mode", &temp))
        wndMode = (WndMode)temp;
    dashFactionKey.readValue("Disable LOD Models", &disableLodModels);
    dashFactionKey.readValue("Anisotropic Filtering", &anisotropicFiltering);
    dashFactionKey.readValue("MSAA", &msaa);
    if (!dashFactionKey.readValue("Executable Path", &gameExecutablePath))
        detectGamePath();
}

void GameConfig::save()
{
    RegKey regKey(HKEY_CURRENT_USER, RF_KEY_NAME, KEY_WRITE);
    regKey.writeValue("Resolution Width", resWidth);
    regKey.writeValue("Resolution Height", resHeight);
    regKey.writeValue("Resolution Bit Depth", resBpp);
    regKey.writeValue("Resolution Backbuffer Format", (unsigned)(resBpp == 16 ? D3DFMT_R5G6B5 : D3DFMT_X8R8G8B8));
    regKey.writeValue("Vsync", vsync);
    regKey.writeValue("Fast Animations", fastAnims);
    regKey.writeValue("Geometry Cache Size", geometryCacheSize);
    regKey.writeValue("GameTracker", tracker);
    regKey.writeValue("EAX", eaxSound);

    RegKey dashFactionKey(regKey, DF_SUBKEY_NAME, KEY_WRITE);
    dashFactionKey.writeValue("Window Mode", (unsigned)wndMode);
    dashFactionKey.writeValue("Disable LOD Models", disableLodModels);
    dashFactionKey.writeValue("Anisotropic Filtering", anisotropicFiltering);
    dashFactionKey.writeValue("MSAA", msaa);
    dashFactionKey.writeValue("Executable Path", gameExecutablePath);
}

bool GameConfig::detectGamePath()
{
    std::string installPath;

    // Standard RF installer
    {
        RegKey regKey(HKEY_LOCAL_MACHINE, RF_KEY_NAME, KEY_READ);
        if (regKey.readValue("InstallPath", &installPath))
        {
            gameExecutablePath = installPath + "\\RF.exe";
            return true;
        }
    }
    
    // Steam
    {
        RegKey regKey(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\Steam App 20530", KEY_READ);
        if (regKey.readValue("InstallLocation", &installPath))
        {
            gameExecutablePath = installPath + "\\RF.exe";
            return true;
        }
    }
    
    return false;
}
