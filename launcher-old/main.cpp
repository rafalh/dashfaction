#include <version.h>
#include <UpdateChecker.h>
#include <ModdedAppLauncher.h>
#include <sstream>

bool LaunchGame(HWND hwnd)
{
    GameLauncher launcher;
    try
    {
        launcher.launch();
        return true;
    }
    catch (PrivilegeElevationRequiredException&)
    {
        MessageBoxA(hwnd,
            "Privilege elevation is required. Please change RF.exe file properties and disable all "
            "compatibility settings (Run as administrator, Compatibility mode for Windows XX, etc.) or run "
            "Dash Faction launcher as administrator.",
            NULL, MB_OK | MB_ICONERROR);
        return false;
    }
    catch (IntegrityCheckFailedException &e)
    {
        if (e.getCrc32() == 0)
        {
            MessageBoxA(hwnd, "Game executable has not been found. Please set a proper path in Options.",
                NULL, MB_OK | MB_ICONERROR);
        }
        else
        {
            std::stringstream ss;
            ss << "Unsupported game executable has been detected (CRC32 = 0x" << std::hex << e.getCrc32() << "). "
                << "Dash Faction supports only unmodified Red Faction 1.20 NA executable.\n"
                << "If your game has not been updated to 1.20 please do it first. If the error still shows up "
                << "replace your RF.exe file with original 1.20 NA RF.exe available on FactionFiles.com.\n"
                << "Click OK to open download page.";
            std::string str = ss.str();
            if (MessageBoxA(hwnd, str.c_str(), NULL, MB_OKCANCEL | MB_ICONERROR) == IDOK)
                ShellExecuteA(hwnd, "open", "https://www.factionfiles.com/ff.php?action=file&id=517545", NULL, NULL, SW_SHOW);
        }
        return false;
    }
    catch (std::exception &e)
    {
        MessageBoxA(hwnd, e.what(), nullptr, MB_ICONERROR | MB_OK);
        return false;
    }
    return true;
}

#ifndef DEBUG

static bool CheckForUpdate()
{
    UpdateChecker update_checker;

    if (!update_checker.check()) {
        printf("No new version is available.\n");
        return false;
    }

    const char* msg_str = update_checker.getMessage().c_str();
    printf("%s\n", msg_str);
    int result =
        MessageBox(nullptr, msg_str, "DashFaction update is available!", MB_OKCANCEL | MB_ICONEXCLAMATION);
    if (result == IDOK) {
        const char* url = update_checker.getUrl().c_str();
        printf("url %s\n", url);
        ShellExecute(nullptr, "open", url, nullptr, nullptr, SW_SHOW);
        return true;
    }

    return false;
}

#endif // DEBUG

int main() try
{
    printf("Starting " PRODUCT_NAME_VERSION "!\n");

#ifndef DEBUG
    printf("Checking for update...\n");
    try {
        if (CheckForUpdate())
            return 0;
    }
    catch (std::exception& e) {
        fprintf(stderr, "Failed to check for update: %s\n", e.what());
    }
#endif

    printf("Starting game process...\n");
    if (!LaunchGame(nullptr))
        return -1;

    return 0;
}
catch (const std::exception& e) {
    fprintf(stderr, "Fatal error: %s\n", e.what());
}
