#pragma once

#include <wxx_wincore.h>
#include "resource.h"
#include "LauncherCommandLineInfo.h"
#include <crash_handler_stub/WatchDogTimer.h>

class LauncherApp : public CWinApp
{
    WatchDogTimer m_watch_dog_timer;
    LauncherCommandLineInfo m_cmd_line_info;

public:
    virtual BOOL InitInstance() override;

    bool LaunchGame(HWND hwnd, const char* mod_name = nullptr);
    bool LaunchEditor(HWND hwnd, const char* mod_name = nullptr);

private:
    void MigrateConfig();
    int Message(HWND hwnd, const char *text, const char *title, int flags);
};

inline LauncherApp* GetLauncherApp()
{
    return static_cast<LauncherApp*>(GetApp());
}
