#pragma once

#define DEFAULT_RF_TRACKER "rf.thqmultiplay.net"
#define DEFAULT_EXECUTABLE_PATH "C:\\games\\RedFaction\\rf.exe"
#define MIN_FPS_LIMIT 10
#define MAX_FPS_LIMIT 150

struct GameConfig
{
    std::string gameExecutablePath = DEFAULT_EXECUTABLE_PATH;

    unsigned resWidth = 1024;
    unsigned resHeight = 768;
    unsigned resBpp = 32;
    unsigned resBackbufferFormat = D3DFMT_X8R8G8B8;
    unsigned selectedVideoCard = 0;
    enum WndMode { FULLSCREEN, WINDOWED, STRETCHED } wndMode = FULLSCREEN;
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

    std::string tracker = DEFAULT_RF_TRACKER;
    unsigned updateRate = 2600;
    bool directInput = false;
    bool eaxSound = true;
    bool fastStart = true;
    bool allowOverwriteGameFiles = false;

    bool load();
    void save();
    bool detectGamePath();
};
