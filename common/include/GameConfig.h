#pragma once

#define DEFAULT_RF_TRACKER "rfgt.factionfiles.com"
#define DEFAULT_EXECUTABLE_PATH "C:\\games\\RedFaction\\rf.exe"
#define MIN_FPS_LIMIT 10
#define MAX_FPS_LIMIT 240

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
    bool glares = true;
    bool showEnemyBullets = true;
    bool linearPitch = false;

    std::string tracker = DEFAULT_RF_TRACKER;
    unsigned updateRate = 2600;
    unsigned forcePort = 0;
    bool directInput = false;
    bool eaxSound = true;
    bool fastStart = true;
    bool allowOverwriteGameFiles = false;
    bool scoreboardAnim = false;
    float levelSoundVolume = 1.0f;
    std::string dashFactionVersion;
    bool swapAssaultRifleControls = false;

    bool load();
    void save();
    bool detectGamePath();
};
