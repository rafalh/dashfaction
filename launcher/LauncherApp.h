#pragma once

#include <wxx_wincore.h>
#include "resource.h"
#include <crash_handler_stub/WatchDogTimer.h>

class LauncherApp : public CWinApp
{
    WatchDogTimer m_watch_dog_timer;

public:
    virtual BOOL InitInstance() override;

    bool LaunchGame(HWND hwnd, const char* mod_name = nullptr);
    bool LaunchEditor(HWND hwnd, const char* mod_name = nullptr);

private:
    void MigrateConfig();
    int Message(HWND hwnd, const char *pszText, const char *pszTitle, int Flags);
};

inline LauncherApp* GetLauncherApp()
{
    return static_cast<LauncherApp*>(GetApp());
}
