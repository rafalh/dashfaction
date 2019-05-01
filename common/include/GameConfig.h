#pragma once

#include <string>
#include <windows.h>

// Only include D3D header if one has not been included before (fix for afx.h including d3d9 and DF using d3d8)
#ifndef DIRECT3D_VERSION
#include <d3d8.h>
#endif

#define DEFAULT_RF_TRACKER "rfgt.factionfiles.com"
#define DEFAULT_EXECUTABLE_PATH "C:\\games\\RedFaction\\rf.exe"
#define MIN_FPS_LIMIT 10u
#define MAX_FPS_LIMIT 240u

struct GameConfig
{
    std::string gameExecutablePath = DEFAULT_EXECUTABLE_PATH;

    // Graphics
    unsigned resWidth = 1024;
    unsigned resHeight = 768;
    unsigned resBpp = 32;
    unsigned resBackbufferFormat = D3DFMT_X8R8G8B8;
    unsigned selectedVideoCard = 0;
    enum WndMode
    {
        FULLSCREEN,
        WINDOWED,
        STRETCHED
    };
    WndMode wndMode = FULLSCREEN;
    bool vsync = true;
    bool fastAnims = false;
    bool disableLodModels = true;
    bool anisotropicFiltering = true;
    unsigned msaa = 0;
    unsigned geometryCacheSize = 32;
    unsigned maxFps = 60;
    bool fpsCounter = true;
    bool highScannerRes = true;
    bool trueColorTextures = true;

    // Multiplayer
    std::string tracker = DEFAULT_RF_TRACKER;
    unsigned updateRate = 2600;
    unsigned forcePort = 0;

    // Misc
    bool directInput = false;
    bool eaxSound = true;
    bool fastStart = true;
    bool allowOverwriteGameFiles = false;
    bool scoreboardAnim = false;
    bool keepLauncherOpen = false;

    // Advanced
    float levelSoundVolume = 1.0f;
    std::string dashFactionVersion;
    bool swapAssaultRifleControls = false;
    bool glares = true;
    bool showEnemyBullets = true;
    bool linearPitch = false;

    bool load();
    void save();
    bool detectGamePath();
};
