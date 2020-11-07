#include "exports.h"
#include <common/version.h>
#include <common/BuildConfig.h>
#include <xlog/xlog.h>
#include <patch_common/MemUtils.h>
#include <patch_common/CallHook.h>
#include <patch_common/FunHook.h>
#include <patch_common/AsmWriter.h>
#include <patch_common/CodeInjection.h>
#include <cstddef>
#include <cstring>
#include <crash_handler_stub.h>
#include <string_view>
#include "resources.h"
#include <sstream>
#include <common/ErrorUtils.h>
#include <launcher_common/PatchedAppLauncher.h>

#include <xlog/ConsoleAppender.h>
#include <xlog/FileAppender.h>
#include <xlog/Win32Appender.h>

HINSTANCE g_module;

void InitLogging()
{
    CreateDirectoryA("logs", nullptr);
    xlog::LoggerConfig::get()
        .add_appender<xlog::FileAppender>("logs/LauncherGuest.log", false)
        .add_appender<xlog::ConsoleAppender>()
        .add_appender<xlog::Win32Appender>();
    xlog::info("DashFaction launcher guest log started.");
}

int Message(HWND hwnd, const char *text, const char *title, int flags)
{
    //WatchDogTimer::ScopedPause wdt_pause{m_watch_dog_timer};
    xlog::info("%s: %s", title ? title : "Error", text);
    bool no_gui = GetSystemMetrics(SM_CMONITORS) == 0;
    if (no_gui) {
        std::fprintf(stderr, "%s: %s", title ? title : "Error", text);
        return -1;
    }
    else {
        return MessageBoxA(hwnd, text, title, flags);
    }
}

bool LaunchGame()
{
    GameLauncher launcher;
    launcher.set_this_module(g_module);

    HWND hwnd = nullptr;
    try {
        xlog::info("Checking installation");
        launcher.check_installation();
        xlog::info("Installation is okay");
    }
    catch (FileNotFoundException &e) {
        std::stringstream ss;
        std::string download_url;

        ss << "Game directory validation has failed! File is missing:\n" << e.get_file_name() << "\n"
            << "Please make sure game executable specified in options is located inside a valid Red Faction installation "
            << "root directory.";
        std::string str = ss.str();
        Message(hwnd, str.c_str(), nullptr, MB_OK | MB_ICONWARNING);
        return false;
    }
    catch (FileHashVerificationException &e) {
        std::stringstream ss;
        std::string download_url;

        ss << "Game directory validation has failed! File " << e.get_file_name() << " has unrecognized hash sum.\n\n"
            << "SHA1:\n" << e.get_sha1();
        if (e.get_file_name() == "tables.vpp") {
            ss << "\n\nIt can prevent multiplayer functionality or entire game from working properly.\n"
                << "If your game has not been updated to 1.20 please do it first. If this warning still shows up "
                << "replace your tables.vpp file with original 1.20 NA " << e.get_file_name() << " available on FactionFiles.com.\n"
                << "Do you want to open download page?";
            std::string str = ss.str();
            download_url = "https://www.factionfiles.com/ff.php?action=file&id=517871";
            int result = Message(hwnd, str.c_str(), nullptr, MB_YESNOCANCEL | MB_ICONWARNING);
            if (result == IDYES) {
                ShellExecuteA(hwnd, "open", download_url.c_str(), nullptr, nullptr, SW_SHOW);
                return false;
            }
            else if (result == IDCANCEL) {
                return false;
            }
        }
        else {
            std::string str = ss.str();
            if (Message(hwnd, str.c_str(), nullptr, MB_OKCANCEL | MB_ICONWARNING) == IDCANCEL) {
                return false;
            }
        }
    }

    try {
        xlog::info("Launching the game...");
        launcher.launch();
        xlog::info("Game launched!");
        return true;
    }
    catch (PrivilegeElevationRequiredException&){
        Message(hwnd,
            "Privilege elevation is required. Please change RF.exe file properties and disable all "
            "compatibility settings (Run as administrator, Compatibility mode for Windows XX, etc.) or run "
            "Dash Faction launcher as administrator.",
            nullptr, MB_OK | MB_ICONERROR);
    }
    catch (FileNotFoundException&) {
        Message(hwnd, "Game executable has not been found. Please set a proper path in Options.",
                nullptr, MB_OK | MB_ICONERROR);
    }
    catch (FileHashVerificationException &e) {
        std::stringstream ss;
        ss << "Unsupported game executable has been detected!\n\n"
            << "SHA1:\n" << e.get_sha1() << "\n\n"
            << "Dash Faction supports only unmodified Red Faction 1.20 NA executable.\n"
            << "If your game has not been updated to 1.20 please do it first. If the error still shows up "
            << "replace your RF.exe file with original 1.20 NA RF.exe available on FactionFiles.com.\n"
            << "Click OK to open download page.";
        std::string str = ss.str();
        if (Message(hwnd, str.c_str(), nullptr, MB_OKCANCEL | MB_ICONERROR) == IDOK)
            ShellExecuteA(hwnd, "open", "https://www.factionfiles.com/ff.php?action=file&id=517545", NULL, NULL, SW_SHOW);
    }
    catch (std::exception &e) {
        std::string msg = generate_message_for_exception(e);
        Message(hwnd, msg.c_str(), nullptr, MB_ICONERROR | MB_OK);
    }
    return false;
}

CodeInjection injection{
    0x004A566E,
    []() {
        MessageBoxA(0,"Hello world (from RedFaction.exe)",0,0);
        LaunchGame();
        ExitProcess(0);
    },
};

extern "C" DWORD DF_DLL_EXPORT Init([[maybe_unused]] void* unused)
{
    InitLogging();
    CrashHandlerStubInstall(g_module);

    injection.Install();

    return 1; // success
}

extern "C" void subhook_unk_opcode_handler(uint8_t* opcode)
{
    xlog::error("SubHook unknown opcode 0x%X at 0x%p", *opcode, opcode);
}

BOOL WINAPI DllMain(HINSTANCE instance, [[maybe_unused]] DWORD reason, [[maybe_unused]] LPVOID reserved)
{
    g_module = (HMODULE)instance;
    DisableThreadLibraryCalls(instance);
    return TRUE;
}
