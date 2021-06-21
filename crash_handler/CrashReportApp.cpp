#include "CrashReportApp.h"
#include "MainDlg.h"
#include "ProgressDlg.h"
#include "CommandLineInfo.h"
#include "MiniDumpHelper.h"
#include "TextDumpHelper.h"
#include "ZipHelper.h"
#include "config.h"
#include "res/resource.h"
#include <common/version/version.h>
#include <common/HttpRequest.h>
#include <windows.h>
#include <fstream>

int CrashReportApp::Run()
{
    CommandLineInfo cmd_line_info;
    if (!cmd_line_info.Parse())
        return 1;

    // InitCommonControlsEx() is required on Windows XP if an application
    // manifest specifies use of ComCtl32.dll version 6 or later to enable
    // visual styles. Otherwise, any window creation will fail.
    Win32xx::LoadCommonControls();

    // Create the report
    ProgressDlg prepare_report_dlg("Preparing crash report... Please wait.", [&, this]() { PrepareReport(cmd_line_info); });
    prepare_report_dlg.DoModal();
    prepare_report_dlg.ThrowIfError();

    // Show report details and ask for permission to send it
    MainDlg details_dlg;
    if (details_dlg.DoModal() == IDOK) {

        auto additional_info = details_dlg.GetAdditionalInfo();
        if (additional_info) {
            auto archive_path = GetArchivedReportFilePath();
            ZipHelper zip{archive_path.c_str(), false};
            zip.add_file("additional_info.txt", additional_info.value());
            zip.close();
        }

        // Send the report
        ProgressDlg send_report_dlg("Sending crash report... Please wait.", [this]() { SendReport(); });
        send_report_dlg.DoModal();
        send_report_dlg.ThrowIfError();
        Message(nullptr, "Crash report has been sent. Thank you!", nullptr,
                MB_ICONINFORMATION | MB_OK | MB_SETFOREGROUND | MB_TASKMODAL);
    }

    return 0;
}

void CrashReportApp::ArchiveReport(const char* crash_dump_filename, const char* exc_info_filename) try
{
    std::string output_dir = m_config.output_dir;
    CreateDirectoryA(output_dir.c_str(), nullptr);

    std::string archive_path_name = output_dir + "\\" + m_config.app_name + "-crash.zip";
    ZipHelper zip(archive_path_name.c_str());
    zip.add_file(crash_dump_filename, "minidump.dmp");
    zip.add_file(exc_info_filename, "exception.txt");
    const char* log_file_name = "app.log";
    const char* last_slash_ptr = std::max(
        std::strrchr(m_config.log_file, '\\'),
        std::strrchr(m_config.log_file, '/')
    );
    if (last_slash_ptr) {
        log_file_name = last_slash_ptr + 1;
    }
    try {
        zip.add_file(m_config.log_file, log_file_name);
    } catch (...) {
        // log file may not exist yet - ignore exception
    }
    zip.close();
}
catch (...) {
    std::throw_with_nested(std::runtime_error("failed to archive crash report"));
}

std::string GetTempFileNameInTempDir(const char* prefix)
{
    char temp_dir[MAX_PATH];
    DWORD ret_val_dw = GetTempPathA(std::size(temp_dir), temp_dir);
    if (ret_val_dw == 0 || ret_val_dw > std::size(temp_dir))
        THROW_WIN32_ERROR();

    char result[MAX_PATH];
    UINT ret_val = GetTempFileNameA(temp_dir, prefix, 0, result);
    if (ret_val == 0)
        THROW_WIN32_ERROR();
    return result;
}

std::string CrashReportApp::GetArchivedReportFilePath() const
{
    std::string output_dir = m_config.output_dir;
    std::string archive_path_name = output_dir + "\\" + m_config.app_name + "-crash.zip";
    return archive_path_name;
}

void CrashReportApp::PrepareReport(const CommandLineInfo& cmd_line_info) try
{
    auto* exception_ptrs = cmd_line_info.GetExceptionPtrs();
    HANDLE process_handle = cmd_line_info.GetProcessHandle();
    auto thread_id = cmd_line_info.GetThreadId();
    HANDLE event = cmd_line_info.GetEvent();
    auto* config_ptr = cmd_line_info.GetCrashHandlerConfigPtr();

    ReadProcessMemory(process_handle, config_ptr, &m_config, sizeof(CrashHandlerConfig), nullptr);

    auto crash_dump_filename = GetTempFileNameInTempDir("DF_Dump");
    auto exc_info_filename = GetTempFileNameInTempDir("DF_ExcInfo");

    MiniDumpHelper dump_helper;

    for (int i = 0; i < m_config.num_known_modules; ++i) {
        wchar_t known_module[256];
        mbstowcs(known_module, m_config.known_modules[i], std::size(known_module));
        dump_helper.add_known_module(known_module);
    }
    dump_helper.set_info_level(CRASHHANDLER_DMP_LEVEL);
    dump_helper.write_dump(crash_dump_filename.c_str(), exception_ptrs, process_handle, thread_id);

    TextDumpHelper text_dump_hlp{process_handle};
    text_dump_hlp.write_dump(exc_info_filename.c_str(), exception_ptrs, process_handle);

    SetEvent(event);

    CloseHandle(process_handle);
    CloseHandle(event);

    ArchiveReport(crash_dump_filename.c_str(), exc_info_filename.c_str());

    // Remove temp files
    DeleteFileA(crash_dump_filename.c_str());
    DeleteFileA(exc_info_filename.c_str());
}
catch (...) {
    std::throw_with_nested(std::runtime_error("failed to prepare crash report"));
}

void CrashReportApp::SendReport() const try
{
    auto file_path = GetArchivedReportFilePath();
    std::ifstream file(file_path, std::ios_base::in | std::ios_base::binary);
    if (!file)
        throw std::runtime_error("cannot open crash report archive");

    file.seekg(0, std::ios_base::end);
    size_t size = static_cast<size_t>(file.tellg());
    file.seekg(0, std::ios_base::beg);

    HttpSession session{CRASHHANDLER_WEBSVC_AGENT};
    HttpRequest req{CRASHHANDLER_WEBSVC_URL, "POST", session};
    req.set_content_type("application/octet-stream");
    req.begin_body(size);
    char buf[4096];
    while (!file.eof()) {
        file.read(buf, sizeof(buf));
        auto num = static_cast<size_t>(file.gcount());
        req.write(buf, num);
    }
    req.send();
}
catch (...) {
    std::throw_with_nested(std::runtime_error("failed to send crash report"));
}

int CrashReportApp::Message(HWND hwnd, const char *text, const char *title, int flags)
{
    if (GetSystemMetrics(SM_CMONITORS) > 0) {
        return MessageBoxA(hwnd, text, title, flags);
    }
    const char* prefix = "";
    if (title)
        prefix = title;
    else if (flags & MB_ICONERROR)
        prefix = "Error: ";
    else if (flags & MB_ICONWARNING)
        prefix = "Warning: ";

    std::fprintf(stderr, "%s%s", prefix, text);
    return -1;
}
