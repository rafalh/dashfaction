#include "CrashReportApp.h"
#include <common/ErrorUtils.h>

int APIENTRY WinMain(HINSTANCE, HINSTANCE, LPSTR, int) try
{
    // Start Win32++
    CrashReportApp app;

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
catch (const Win32xx::CException &e)
{
    // Display the exception and quit
    std::ostringstream ss;
    ss << e.GetText() << "\nerror " << e.GetError() << ": " << e.GetErrorString();
    auto msg = ss.str();
    MessageBox(nullptr, msg.c_str(), nullptr, MB_ICONERROR | MB_OK);

    return -1;
}
