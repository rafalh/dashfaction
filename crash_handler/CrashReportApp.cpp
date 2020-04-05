#include "CrashReportApp.h"
#include "MainDlg.h"
#include "ProgressDlg.h"
#include "CommandLineInfo.h"
#include "MiniDumpHelper.h"
#include "TextDumpHelper.h"
#include "ZipHelper.h"
#include "config.h"
#include "res/resource.h"
#include <common/Exception.h>
#include <common/ErrorUtils.h>
#include <common/version.h>
#include <common/HttpRequest.h>
#include <windows.h>
#include <fstream>

BOOL CrashReportApp::InitInstance() try
{
    CommandLineInfo cmd_line_info;
    if (!cmd_line_info.Parse())
        return FALSE;

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

    // Since the dialog has been closed, return FALSE so that we exit the
    //  application, rather than start the application's message pump.
    return FALSE;
}
catch (const std::exception& e) {
    std::string msg = "Fatal error: ";
    msg += generate_message_for_exception(e);
    Message(nullptr, msg.c_str(), nullptr, MB_ICONERROR | MB_OK | MB_SETFOREGROUND | MB_TASKMODAL);
    return FALSE;
}

void ArchiveReport(const char* crash_dump_filename, const char* exc_info_filename) try
{
    CreateDirectoryA(CRASHHANDLER_TARGET_DIR, nullptr);
    ZipHelper zip(CRASHHANDLER_TARGET_DIR "/" CRASHHANDLER_TARGET_NAME);
    zip.add_file(crash_dump_filename, "CrashDump.dmp");
    zip.add_file(exc_info_filename, "exception.txt");
    zip.add_file(CRASHHANDLER_LOG_PATH, "AppLog.log");
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
    char cur_dir[MAX_PATH];
    GetCurrentDirectoryA(std::size(cur_dir), cur_dir);
    return std::string(cur_dir) + "\\" + CRASHHANDLER_TARGET_DIR "\\" CRASHHANDLER_TARGET_NAME;
}

void CrashReportApp::PrepareReport(const CommandLineInfo& cmd_line_info) try
{
    auto exception_ptrs = cmd_line_info.GetExceptionPtrs();
    auto process_handle = cmd_line_info.GetProcessHandle();
    auto thread_id = cmd_line_info.GetThreadId();
    auto event = cmd_line_info.GetEvent();

    auto crash_dump_filename = GetTempFileNameInTempDir("DF_Dump");
    auto exc_info_filename = GetTempFileNameInTempDir("DF_ExcInfo");

    MiniDumpHelper dump_helper;
    dump_helper.add_known_module(L"ntdll");
    dump_helper.add_known_module(L"DashFaction");
    dump_helper.add_known_module(L"RF");
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

void CrashReportApp::SendReport() try
{
    auto file_path = GetArchivedReportFilePath();
    std::ifstream file(file_path, std::ios_base::in | std::ios_base::binary);
    if (!file)
        THROW_EXCEPTION("cannot open " CRASHHANDLER_TARGET_NAME);

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
        size_t num = static_cast<size_t>(file.gcount());
        req.write(buf, num);
    }
    req.send();
}
catch (...) {
    std::throw_with_nested(std::runtime_error("failed to send crash report"));
}

int CrashReportApp::Message(HWND hwnd, const char *text, const char *title, int flags)
{
    if (GetSystemMetrics(SM_CMONITORS) > 0)
        return MessageBoxA(hwnd, text, title, flags);
    else
    {
        auto prefix = "";
        if (title)
            prefix = title;
        else if (flags & MB_ICONERROR)
            prefix = "Error: ";
        else if (flags & MB_ICONWARNING)
            prefix = "Warning: ";

        fprintf(stderr, "%s%s", prefix, text);
        return -1;
    }
}
