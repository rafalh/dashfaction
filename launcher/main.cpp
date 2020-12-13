#include "LauncherApp.h"
#include <common/version/version.h>
#include <common/error/error-utils.h>
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

int APIENTRY WinMain(HINSTANCE, HINSTANCE, LPSTR, int) try
{
    InitLogging();
    CrashHandlerStubInstall(nullptr);

    // Start Win32++
    LauncherApp app;

    // Run the application
    return app.Run();
}
// catch all unhandled std::exception types
catch (const std::exception& e) {
    std::string msg = "Fatal error: ";
    msg += generate_message_for_exception(e);
    MessageBox(nullptr, msg.c_str(), nullptr, MB_ICONERROR | MB_OK);
    return -1;
}
// catch all unhandled CException types
catch (const Win32xx::CException &e) {
    // Display the exception and quit
    std::ostringstream ss;
    ss << e.GetText() << "\nerror " << e.GetError() << ": " << e.GetErrorString();
    auto msg = ss.str();
    MessageBox(nullptr, msg.c_str(), nullptr, MB_ICONERROR | MB_OK);

    return -1;
}
