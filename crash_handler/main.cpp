#include "CrashReportApp.h"
#include <common/error/error-utils.h>
#include <format>

int APIENTRY WinMain(HINSTANCE, HINSTANCE, LPSTR, int) try
{
    // Start Win32++
    CrashReportApp app;

    // Run the application
    return app.Run(); // NOLINT(clang-analyzer-core.StackAddressEscape)
}
// catch all unhandled CException types
catch (const Win32xx::CException &e)
{
    // Display the exception and quit
    std::string msg = std::format("{}\nerror {}: {}", e.GetText(), e.GetError(), e.GetErrorString());
    MessageBox(nullptr, msg.c_str(), nullptr, MB_ICONERROR | MB_OK);
    return -1;
}
// catch all unhandled std::exception types
catch (const std::exception& e) {
    std::string msg = std::format("Fatal error: {}", generate_message_for_exception(e));
    MessageBox(nullptr, msg.c_str(), nullptr, MB_ICONERROR | MB_OK);
    return -1;
}
