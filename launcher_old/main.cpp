#include <launcher_common/PatchedAppLauncher.h>
#include <launcher_common/UpdateChecker.h>
#include <sstream>
#include <common/version.h>
#include <common/Win32Error.h>

bool launch_game(HWND hwnd)
{
    GameLauncher launcher;
    try {
        launcher.launch();
        return true;
    }
    catch (const PrivilegeElevationRequiredException&) {
        MessageBoxA(hwnd,
                    "Privilege elevation is required. Please change RF.exe file properties and disable all "
                    "compatibility settings (Run as administrator, Compatibility mode for Windows XX, etc.) or run "
                    "Dash Faction launcher as administrator.",
                    nullptr, MB_OK | MB_ICONERROR);
        return false;
    }
    catch (const FileNotFoundException&) {
        MessageBoxA(hwnd, "Game executable has not been found. Please set a proper path in Options.", NULL,
                    MB_OK | MB_ICONERROR);
        return false;
    }
    catch (const FileHashVerificationException& e) {
        std::stringstream ss;
        ss << "Unsupported game executable has been detected (SHA1: " << e.get_sha1() << ").\n"
            << "Dash Faction supports only unmodified Red Faction 1.20 NA executable.\n"
            << "If your game has not been updated to 1.20 please do it first. If the error still shows up "
            << "replace your RF.exe file with original 1.20 NA RF.exe available on FactionFiles.com (you MUST "
            << "unpack it inside the game root directory).\n"
            << "Click OK to open download page.";
        std::string str = ss.str();
        if (MessageBoxA(hwnd, str.c_str(), nullptr, MB_OKCANCEL | MB_ICONERROR) == IDOK) {
            ShellExecuteA(hwnd, "open", "https://www.factionfiles.com/ff.php?action=file&id=517545", NULL, NULL, SW_SHOW);
        }

        return false;
    }
    catch (const std::exception& e) {
        auto msg = generate_message_for_exception(e);
        MessageBoxA(hwnd, msg.c_str(), nullptr, MB_ICONERROR | MB_OK);
        return false;
    }
    return true;
}

#ifndef DEBUG

static bool check_for_update()
{
    UpdateChecker update_checker;

    auto chk_result = update_checker.check();
    if (!chk_result) {
        std::printf("No new version is available.\n");
        return false;
    }

    std::printf("%s\n", chk_result.message.c_str());
    int result = MessageBox(nullptr, chk_result.message.c_str(), "DashFaction update is available!", MB_OKCANCEL | MB_ICONEXCLAMATION);
    if (result == IDOK) {
        std::printf("url %s\n", chk_result.url.c_str());
        ShellExecute(nullptr, "open", chk_result.url.c_str(), nullptr, nullptr, SW_SHOW);
        return true;
    }

    return false;
}

#endif // DEBUG

int main() try {
    std::printf("Starting " PRODUCT_NAME_VERSION "!\n");

#ifndef DEBUG
    std::printf("Checking for update...\n");
    try {
        if (check_for_update())
            return 0;
    }
    catch (const std::exception& e) {
        std::fprintf(stderr, "Failed to check for update: %s\n", e.what());
    }
#endif

    printf("Starting game process...\n");
    if (!launch_game(nullptr))
    {
        std::fprintf(stderr, "Failed to launch!\n");
        return -1;
    }

    return 0;
}
catch (const std::exception& e) {
    std::string msg = generate_message_for_exception(e);
    std::fprintf(stderr, "Fatal error: %s\n", msg.c_str());
}
