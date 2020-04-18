#include "stdafx.h"
#include "LauncherApp.h"
#include <common/version.h>
#include <log/Logger.h>
#include <log/FileAppender.h>
#include <log/Win32Appender.h>
#include <log/ConsoleAppender.h>
#include <crash_handler_stub.h>

void InitLogging()
{
    auto app_data_dir = Win32xx::GetAppDataPath();
    auto df_data_dir = app_data_dir + "\\Dash Faction";
    CreateDirectoryA(df_data_dir, nullptr);
    auto log_file_path = df_data_dir + "\\DashFactionLauncher.log";
    auto& logger_config = logging::LoggerConfig::root();
    logger_config.add_appender(std::make_unique<logging::FileAppender>(log_file_path.c_str(), false));
    logger_config.add_appender(std::make_unique<logging::ConsoleAppender>());
    logger_config.add_appender(std::make_unique<logging::Win32Appender>());
    INFO("Dash Faction Launcher %s (%s %s)", VERSION_STR, __DATE__, __TIME__);
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
