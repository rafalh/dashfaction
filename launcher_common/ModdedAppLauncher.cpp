#include "ModdedAppLauncher.h"
#include "crc32.h"
#include "Win32Handle.h"
#include "Win32Error.h"
#include "Process_.h"
#include "Thread.h"
#include "DllInjector.h"
#include "InjectingProcessLauncher.h"
#include <GameConfig.h>
#include <Exception.h>
#include <windows.h>
#include <shlwapi.h>

// Needed by MinGW
#ifndef ERROR_ELEVATION_REQUIRED
#define ERROR_ELEVATION_REQUIRED 740
#endif

#define INIT_TIMEOUT 10000
#define RF_120_NA_CRC32 0xA7BF79E4
#define RED_120_NA_CRC32 0xBAF6C754

std::string ModdedAppLauncher::get_mod_dll_path()
{
    char buf[MAX_PATH];
    DWORD result = GetModuleFileNameA(nullptr, buf, std::size(buf));
    if (!result)
        THROW_WIN32_ERROR();
    if (result == std::size(buf))
        THROW_EXCEPTION("insufficient buffer for module file name");

    std::string dir = get_dir_from_path(buf);
    return dir + "\\" + m_mod_dll_name;
}

void ModdedAppLauncher::launch(const char* mod_name)
{
    std::string app_path = get_app_path();
    uint32_t checksum = file_crc32(app_path.c_str());

    // Error reporting for headless env
    if (checksum != m_expected_crc32) {
        if (checksum == 0)
            std::fprintf(stderr, "Invalid App Path %s\n", app_path.c_str());
        else
            std::fprintf(stderr, "Invalid App Checksum %x, expected %x\n", checksum, m_expected_crc32);
        throw IntegrityCheckFailedException(checksum);
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

    try {
        InjectingProcessLauncher proc_launcher{app_path.c_str(), work_dir.c_str(), cmd_line.c_str(), si, INIT_TIMEOUT};

        std::string mod_path = get_mod_dll_path();
        proc_launcher.inject_dll(mod_path.c_str(), "Init", INIT_TIMEOUT);

        proc_launcher.resume_main_thread();

        // Wait for child process in Wine No GUI mode
        if (no_gui)
            proc_launcher.wait(INFINITE);
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

GameLauncher::GameLauncher() : ModdedAppLauncher("DashFaction.dll", RF_120_NA_CRC32)
{
    if (!m_conf.load()) {
        // Failed to load config - save defaults
        m_conf.save();
    }
}

std::string GameLauncher::get_app_path()
{
    return m_conf.gameExecutablePath;
}

EditorLauncher::EditorLauncher() : ModdedAppLauncher("DashEditor.dll", RED_120_NA_CRC32)
{
    if (!m_conf.load()) {
        // Failed to load config - save defaults
        m_conf.save();
    }
}

std::string EditorLauncher::get_app_path()
{
    std::string workDir = get_dir_from_path(m_conf.gameExecutablePath);
    return workDir + "\\RED.exe";
}
