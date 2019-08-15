#include "MiniDumpHelper.h"
#include "ZipHelper.h"
#include <common/Exception.h>
#include <common/version.h>
#include <common/HttpRequest.h>
#include <windows.h>
#include <fstream>
#include <common/ErrorUtils.h>

// Config
#define CRASHHANDLER_LOG_PATH "logs/DashFaction.log"
#define CRASHHANDLER_DMP_FILENAME "logs/DashFaction.dmp"
#define CRASHHANDLER_TARGET_DIR "logs"
#define CRASHHANDLER_TARGET_NAME "DashFaction-crash.zip"
//#define CRASHHANDLER_MSG "Game has crashed!\nTo help resolve the problem please send " CRASHHANDLER_TARGET_NAME " file
//from logs subdirectory in RedFaction directory to " PRODUCT_NAME " author."
#define CRASHHANDLER_MSG                                                                                     \
    "Game has crashed!\nReport has been generated in " CRASHHANDLER_TARGET_DIR "\\" CRASHHANDLER_TARGET_NAME \
    ".\nDo you want to send it to " PRODUCT_NAME " author to help resolve the problem?"
// Information Level: 0 smallest - 2 - biggest
#ifdef NDEBUG
#define CRASHHANDLER_DMP_LEVEL 0
#else
#define CRASHHANDLER_DMP_LEVEL 1
#endif

#define CRASHHANDLER_WEBSVC_ENABLED 1
#define CRASHHANDLER_WEBSVC_URL "https://ravin.tk/api/rf/dashfaction/crashreport.php"
#define CRASHHANDLER_WEBSVC_AGENT "DashFaction"

bool PrepareArchive(const char* crash_dump_filename)
{
    CreateDirectoryA(CRASHHANDLER_TARGET_DIR, NULL);
    ZipHelper zip(CRASHHANDLER_TARGET_DIR "/" CRASHHANDLER_TARGET_NAME);
    zip.add_file(crash_dump_filename, "CrashDump.dmp");
    zip.add_file(CRASHHANDLER_LOG_PATH, "AppLog.log");
    return true;
}

void SendArchive() try
{
    auto file_path = CRASHHANDLER_TARGET_DIR "/" CRASHHANDLER_TARGET_NAME;
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
    std::throw_with_nested(std::runtime_error("failed to send crashdump"));
}

int GetTempFileNameInTempDir(const char* prefix, char result[MAX_PATH])
{
    char temp_dir[MAX_PATH];
    DWORD ret_val_dw = GetTempPathA(std::size(temp_dir), temp_dir);
    if (ret_val_dw == 0 || ret_val_dw > std::size(temp_dir))
        return -1;

    UINT ret_val = GetTempFileNameA(temp_dir, prefix, 0, result);
    if (ret_val == 0)
        return -1;

    return 0;
}

#if 1
int CALLBACK WinMain([[maybe_unused]] HINSTANCE instance, [[maybe_unused]] HINSTANCE prev_instance,
                     [[maybe_unused]] LPSTR cmd_line, [[maybe_unused]] int cmd_show) try {
    auto argc = __argc;
    auto argv = __argv;
#else
int main(int argc, const char* argv[]) try {
#endif

    if (argc < 5)
        return -1;

    EXCEPTION_POINTERS* exception_ptrs = reinterpret_cast<EXCEPTION_POINTERS*>(std::strtoull(argv[1], nullptr, 0));
    HANDLE process_handle = reinterpret_cast<HANDLE>(std::strtoull(argv[2], nullptr, 0));
    DWORD thread_id = static_cast<DWORD>(std::strtoull(argv[3], nullptr, 0));
    HANDLE event = reinterpret_cast<HANDLE>(std::strtoull(argv[4], nullptr, 0));

    char crash_dump_filename[MAX_PATH];
    if (GetTempFileNameInTempDir("DF_Dump", crash_dump_filename) != 0)
        return -1;

    MiniDumpHelper dump_helper;
    dump_helper.add_known_module(L"ntdll");
    dump_helper.add_known_module(L"DashFaction");
    dump_helper.add_known_module(L"RF");
    dump_helper.set_info_level(CRASHHANDLER_DMP_LEVEL);
    dump_helper.write_dump(crash_dump_filename, exception_ptrs, process_handle, thread_id);

    SetEvent(event);

    CloseHandle(process_handle);
    CloseHandle(event);

    PrepareArchive(crash_dump_filename);
    DeleteFileA(crash_dump_filename);

    if (GetSystemMetrics(SM_CMONITORS) == 0)
        printf("%s\n", CRASHHANDLER_MSG);
    else {
#if CRASHHANDLER_WEBSVC_ENABLED
        if (MessageBox(nullptr, TEXT(CRASHHANDLER_MSG), nullptr, MB_ICONERROR | MB_YESNO | MB_SETFOREGROUND | MB_TASKMODAL) ==
            IDYES) {
            SendArchive();
            MessageBox(nullptr, TEXT("Crash report has been sent. Thank you!"), nullptr,
                       MB_ICONINFORMATION | MB_OK | MB_SETFOREGROUND | MB_TASKMODAL);
        }
#else  // CRASHHANDLER_WEBSVC_ENABLED
        MessageBox(nullptr, TEXT(CRASHHANDLER_MSG), nullptr, MB_ICONERROR | MB_OK | MB_SETFOREGROUND | MB_TASKMODAL);
#endif // CRASHHANDLER_WEBSVC_ENABLED
    }

    return 0;
}
catch (const std::exception& e) {
    std::string msg = generate_message_for_exception(e);
    MessageBoxA(nullptr, msg.c_str(), nullptr, MB_ICONERROR | MB_OK | MB_SETFOREGROUND | MB_TASKMODAL);
    return -1;
}
