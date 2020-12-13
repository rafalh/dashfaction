#include "PatchedAppLauncher.h"
#include "sha1.h"
#include "Win32Handle.h"
#include "Process.h"
#include "Thread.h"
#include "DllInjector.h"
#include "InjectingProcessLauncher.h"
#include <common/config/GameConfig.h>
#include <common/error/Exception.h>
#include <common/error/Win32Error.h>
#include <xlog/xlog.h>
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

inline bool is_no_gui_mode()
{
    return GetSystemMetrics(SM_CMONITORS) == 0;
}

std::string PatchedAppLauncher::get_patch_dll_path()
{
    std::string buf;
    buf.resize(MAX_PATH);
    DWORD result = GetModuleFileNameA(nullptr, buf.data(), buf.size());
    if (!result)
        THROW_WIN32_ERROR();
    if (result == buf.size())
        THROW_EXCEPTION("insufficient buffer for module file name");
    buf.resize(result);

    // Get GetFinalPathNameByHandleA function address dynamically in order to support Windows XP
    using GetFinalPathNameByHandleA_Type = decltype(GetFinalPathNameByHandleA);
    auto kernel32_module = GetModuleHandleA("kernel32");
    auto GetFinalPathNameByHandleA_ptr = reinterpret_cast<GetFinalPathNameByHandleA_Type*>(reinterpret_cast<void(*)()>(
        GetProcAddress(kernel32_module, "GetFinalPathNameByHandleA")));
    // Make sure path is pointing to an actual module and not a symlink
    if (GetFinalPathNameByHandleA_ptr) {
        auto file_handle = CreateFileA(buf.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
        if (file_handle != INVALID_HANDLE_VALUE) {
            buf.resize(MAX_PATH);

            result = GetFinalPathNameByHandleA(file_handle, buf.data(), buf.size(), FILE_NAME_NORMALIZED);
            if (result > buf.size()) {
                buf.resize(result);
                result = GetFinalPathNameByHandleA(file_handle, buf.data(), buf.size(), FILE_NAME_NORMALIZED);
            }
            if (result == 0 || result > buf.size()) {
                THROW_WIN32_ERROR();
            }
            // Windows Vista returns number of characters including the terminating null character
            if (buf[result - 1] == '\0') {
                --result;
            }
            buf.resize(result);
            CloseHandle(file_handle);
        }
    }

    std::string dir = get_dir_from_path(buf);
    xlog::info("Determined Dash Faction directory: %s", dir.c_str());
    return dir + "\\" + m_patch_dll_name;
}

std::string PatchedAppLauncher::get_app_path()
{
    if (m_forced_app_exe_path) {
        return m_forced_app_exe_path.value();
    }
    else {
        return get_default_app_path();
    }
}

void PatchedAppLauncher::launch()
{
    std::string app_path = get_app_path();
    std::string work_dir = get_dir_from_path(app_path);
    verify_before_launch();

    STARTUPINFO si;
    setup_startup_info(si);

    std::string cmd_line = build_cmd_line(app_path);

    xlog::info("Starting the process: %s", app_path.c_str());
    try {
        InjectingProcessLauncher proc_launcher{app_path.c_str(), work_dir.c_str(), cmd_line.c_str(), si, INIT_TIMEOUT};

        std::string patch_dll_path = get_patch_dll_path();
        xlog::info("Injecting %s", patch_dll_path.c_str());
        proc_launcher.inject_dll(patch_dll_path.c_str(), "Init", INIT_TIMEOUT);

        xlog::info("Resuming app main thread");
        proc_launcher.resume_main_thread();
        xlog::info("Process launched successfully");

        // Wait for child process in Wine No GUI mode
        if (is_no_gui_mode()) {
            xlog::info("Waiting for app to close");
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

void PatchedAppLauncher::verify_before_launch()
{
    std::string app_path = get_app_path();
    xlog::info("Verifying %s SHA1", app_path.c_str());
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
    xlog::info("SHA1 is valid");

    std::string work_dir = get_dir_from_path(app_path);

    std::string tables_vpp_path = work_dir + "\\tables.vpp";
    if (!PathFileExistsA(tables_vpp_path.c_str())) {
        throw LauncherError(
            "Game directory validation has failed.\nPlease make sure game executable specified in options is located "
            "inside a valid Red Faction installation root directory.");
    }
}

void PatchedAppLauncher::setup_startup_info(_STARTUPINFOA& startup_info)
{
    ZeroMemory(&startup_info, sizeof(startup_info));
    startup_info.cb = sizeof(startup_info);

    if (is_no_gui_mode()) {
        // Redirect std handles - fixes nohup logging
        startup_info.dwFlags |= STARTF_USESTDHANDLES;
        startup_info.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
        startup_info.hStdOutput = GetStdHandle(STD_OUTPUT_HANDLE);
        startup_info.hStdError = GetStdHandle(STD_ERROR_HANDLE);
    }
}

static std::string escape_cmd_line_arg(const std::string& arg)
{
    std::string result;
    bool enclose_in_quotes = arg.find(' ') != std::string::npos;
    result.reserve(arg.size());
    if (enclose_in_quotes) {
        result += '"';
    }
    for (char c : arg) {
        if (c == '\\' || c == '"') {
            result += '\\';
        }
        result += c;
    }
    if (enclose_in_quotes) {
        result += '"';
    }
    return result;
}

std::string PatchedAppLauncher::build_cmd_line(const std::string& app_path)
{
    std::vector<std::string> all_args;
    all_args.push_back(app_path);
    if (!m_mod_name.empty()) {
        all_args.push_back("-mod");
        all_args.push_back(m_mod_name);
    }
    all_args.insert(all_args.end(), m_args.begin(), m_args.end());

    std::string cmd_line;
    for (const auto& arg : all_args) {
        if (!cmd_line.empty()) {
            cmd_line += ' ';
        }
        cmd_line += escape_cmd_line_arg(arg);
    }
    return cmd_line;
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
    xlog::info("Checking if config exists");
    if (!m_conf.load()) {
        // Failed to load config - save defaults
        xlog::info("Saving default config");
        m_conf.save();
    }
}

std::string GameLauncher::get_default_app_path()
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

std::string EditorLauncher::get_default_app_path()
{
    std::string workDir = get_dir_from_path(m_conf.game_executable_path);
    return workDir + "\\RED.exe";
}

bool EditorLauncher::check_app_hash(const std::string& sha1)
{
    return sha1 == RED_120_NA_SHA1;
}
