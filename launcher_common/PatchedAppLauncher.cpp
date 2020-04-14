#include "PatchedAppLauncher.h"
#include "sha1.h"
#include "Win32Handle.h"
#include "Process.h"
#include "Thread.h"
#include "DllInjector.h"
#include "InjectingProcessLauncher.h"
#include <common/GameConfig.h>
#include <common/Exception.h>
#include <common/Win32Error.h>
#include <log/Logger.h>
#include <windows.h>
#include <shlwapi.h>
#include <fstream>

// Needed by MinGW
#ifndef ERROR_ELEVATION_REQUIRED
#define ERROR_ELEVATION_REQUIRED 740
#endif

#define INIT_TIMEOUT 10000
#define RF_120_NA_SHA1  "f94f2e3f565d18f75ab6066e77f73a62a593fe03"
#define RF_120_NA_4GB_SHA1  "4140f7619b6427c170542c66178b47bd48795a99"
#define RED_120_NA_SHA1 "b4f421bfa9343362d7cc565e9f7ab8c6cc36f3a2"
#define TABLES_VPP_SHA1 "ded5e1b5932f47044ba760699d5931fda6bdc8ba"

std::string PatchedAppLauncher::get_patch_dll_path()
{
    char buf[MAX_PATH];
    DWORD result = GetModuleFileNameA(nullptr, buf, std::size(buf));
    if (!result)
        THROW_WIN32_ERROR();
    if (result == std::size(buf))
        THROW_EXCEPTION("insufficient buffer for module file name");

    std::string dir = get_dir_from_path(buf);
    return dir + "\\" + m_patch_dll_name;
}

void PatchedAppLauncher::launch(const char* mod_name)
{
    std::string app_path = get_app_path();
    INFO("Verifying %s SHA1", app_path.c_str());
    std::ifstream file(app_path.c_str(), std::fstream::in | std::fstream::binary);
    if (!file.is_open()) {
        throw FileNotFoundException(app_path);
    }
    SHA1 sha1;
    sha1.update(file);
    auto hash = sha1.final();

    // Error reporting for headless env
    if (!check_app_hash(hash)) {
        throw FileHashVerificationException(app_path, hash);
    }

    std::string work_dir = get_dir_from_path(app_path);

    std::string tables_vpp_path = work_dir + "\\tables.vpp";
    if (!PathFileExistsA(tables_vpp_path.c_str())) {
        throw LauncherError(
            "Game directory validation has failed.\nPlease make sure game executable specified in options is located "
            "inside a valid Red Faction installation root directory.");
    }

    STARTUPINFO si;
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);

    bool no_gui = GetSystemMetrics(SM_CMONITORS) == 0;
    if (no_gui) {
        // Redirect std handles - fixes nohup logging
        si.dwFlags |= STARTF_USESTDHANDLES;
        si.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
        si.hStdOutput = GetStdHandle(STD_OUTPUT_HANDLE);
        si.hStdError = GetStdHandle(STD_ERROR_HANDLE);
    }

    std::string cmd_line = GetCommandLineA();
    if (mod_name && mod_name[0]) {
        cmd_line += " -mod ";
        cmd_line += mod_name;
    }

    INFO("Starting the process: %s", app_path.c_str());
    try {
        InjectingProcessLauncher proc_launcher{app_path.c_str(), work_dir.c_str(), cmd_line.c_str(), si, INIT_TIMEOUT};

        std::string mod_path = get_patch_dll_path();
        INFO("Injecting %s", mod_path.c_str());
        proc_launcher.inject_dll(mod_path.c_str(), "Init", INIT_TIMEOUT);

        INFO("Resuming app main thread");
        proc_launcher.resume_main_thread();
        INFO("Process launched successfully");

        // Wait for child process in Wine No GUI mode
        if (no_gui) {
            INFO("Waiting for app to close");
            proc_launcher.wait(INFINITE);
        }
    }
    catch (const Win32Error& e)
    {
        if (e.error() == ERROR_ELEVATION_REQUIRED)
            throw PrivilegeElevationRequiredException();
        throw;
    }
    catch (const ProcessTerminatedError&)
    {
        throw LauncherError(
            "Game process has terminated before injection!\n"
            "Check your Red Faction installation.");
    }
}

void PatchedAppLauncher::check_installation()
{
    std::string app_path = get_app_path();
    std::string root_dir = get_dir_from_path(app_path);

    auto tables_vpp_path = root_dir + "\\tables.vpp";
    std::ifstream file(tables_vpp_path.c_str(), std::fstream::in | std::fstream::binary);
    if (!file.is_open()) {
        throw FileNotFoundException("tables.vpp");
    }

    SHA1 sha1;
    sha1.update(file);
    auto checksum = sha1.final();

    if (checksum != TABLES_VPP_SHA1) {
        throw FileHashVerificationException("tables.vpp", checksum);
    }
}

GameLauncher::GameLauncher() : PatchedAppLauncher("DashFaction.dll")
{
    INFO("Checking if config exists");
    if (!m_conf.load()) {
        // Failed to load config - save defaults
        INFO("Saving default config");
        m_conf.save();
    }
}

std::string GameLauncher::get_app_path()
{
    return m_conf.game_executable_path;
}

bool GameLauncher::check_app_hash(const std::string& sha1)
{
    return sha1 == RF_120_NA_SHA1 || sha1 == RF_120_NA_4GB_SHA1;
}


EditorLauncher::EditorLauncher() : PatchedAppLauncher("DashEditor.dll")
{
    if (!m_conf.load()) {
        // Failed to load config - save defaults
        m_conf.save();
    }
}

std::string EditorLauncher::get_app_path()
{
    std::string workDir = get_dir_from_path(m_conf.game_executable_path);
    return workDir + "\\RED.exe";
}

bool EditorLauncher::check_app_hash(const std::string& sha1)
{
    return sha1 == RED_120_NA_SHA1;
}
