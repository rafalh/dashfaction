#include "stdafx.h"
#include "LauncherApp.h"
#include <common/version.h>
#include <xlog/xlog.h>
#include <xlog/FileAppender.h>
#include <xlog/Win32Appender.h>
#include <xlog/ConsoleAppender.h>
#include <crash_handler_stub.h>

void InitLogging()
{
    auto app_data_dir = Win32xx::GetAppDataPath();
    auto df_data_dir = app_data_dir + "\\Dash Faction";
    CreateDirectoryA(df_data_dir, nullptr);
    auto log_file_path = df_data_dir + "\\DashFactionLauncher.log";
    xlog::LoggerConfig::get()
        .add_appender<xlog::FileAppender>(log_file_path.c_str(), false)
        .add_appender<xlog::ConsoleAppender>()
        .add_appender<xlog::Win32Appender>();
    xlog::info("Dash Faction Launcher %s (%s %s)", VERSION_STR, __DATE__, __TIME__);
}

int APIENTRY WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
    InitLogging();
    CrashHandlerStubInstall(nullptr);

    try
    {
        // Start Win32++
        LauncherApp app;

        // Run the application
        return app.Run();
    }

    // catch all unhandled CException types
    catch (const CException &e)
    {
        // Display the exception and quit
        MessageBox(NULL, e.GetText(), AtoT(e.what()), MB_ICONERROR);

        return -1;
    }
}
