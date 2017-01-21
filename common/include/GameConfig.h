#pragma once

#define DEFAULT_RF_TRACKER "rf.thqmultiplay.net"

struct GameConfig
{
    std::string gameExecutablePath;

    unsigned resWidth;
    unsigned resHeight;
    unsigned resBpp;
    enum WndMode { FULLSCREEN, WINDOWED, STRETCHED } wndMode;
    bool vsync;
    bool fastAnims;
    bool disableLodModels;
    bool anisotropicFiltering;
    unsigned msaa;
    unsigned geometryCacheSize;

    std::string tracker;
    bool directInput;
    bool eaxSound;

    GameConfig()
    {
        setDefaults();
    }

    void setDefaults();
    void load();
    void save();
    bool detectGamePath();
};
