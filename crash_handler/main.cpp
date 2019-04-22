#include "MiniDumpHelper.h"
#include "ZipHelper.h"
#include <Exception.h>
#include <version.h>
#include <HttpRequest.h>
#include <windows.h>
#include <fstream>

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

bool PrepareArchive(const char* CrashDumpFilename)
{
    CreateDirectoryA(CRASHHANDLER_TARGET_DIR, NULL);
    ZipHelper zip(CRASHHANDLER_TARGET_DIR "/" CRASHHANDLER_TARGET_NAME);
    zip.addFile(CrashDumpFilename, "CrashDump.dmp");
    zip.addFile(CRASHHANDLER_LOG_PATH, "AppLog.log");
    return true;
}

void SendArchive()
{
    auto file_path = CRASHHANDLER_TARGET_DIR "/" CRASHHANDLER_TARGET_NAME;
    std::ifstream file(file_path, std::ios_base::in | std::ios_base::binary);
    if (!file)
        THROW_EXCEPTION("cannot open " CRASHHANDLER_TARGET_NAME);

    file.seekg(0, std::ios_base::end);
    size_t size = file.tellg();
    file.seekg(0, std::ios_base::beg);

    HttpSession session{CRASHHANDLER_WEBSVC_AGENT};
    HttpRequest req(CRASHHANDLER_WEBSVC_URL, "POST", session);
    req.set_content_type("application/octet-stream");
    req.begin_body(size);
    char buf[2048];
    while (!file.eof()) {
        size_t num = file.readsome(buf, sizeof(buf));
        req.write(buf, num);
    }
    req.send();
}

int GetTempFileNameInTempDir(const char* Prefix, char Result[MAX_PATH])
{
    char TempDir[MAX_PATH];
    DWORD dwRetVal = GetTempPathA(std::size(TempDir), TempDir);
    if (dwRetVal == 0 || dwRetVal > std::size(TempDir))
        return -1;

    UINT RetVal = GetTempFileNameA(TempDir, Prefix, 0, Result);
    if (RetVal == 0)
        return -1;

    return 0;
}

#if 1
int CALLBACK WinMain([[maybe_unused]] HINSTANCE hInstance, [[maybe_unused]] HINSTANCE hPrevInstance,
                     [[maybe_unused]] LPSTR lpCmdLine, [[maybe_unused]] int nCmdShow) try {
    auto argc = __argc;
    auto argv = __argv;
#else
int main(int argc, const char* argv[]) try {
#endif

    if (argc < 5)
        return -1;

    EXCEPTION_POINTERS* ExceptionPtrs = (EXCEPTION_POINTERS*)strtoull(argv[1], nullptr, 0);
    HANDLE hProcess = (HANDLE)strtoull(argv[2], nullptr, 0);
    DWORD dwThreadId = (DWORD)strtoull(argv[3], nullptr, 0);
    HANDLE hEvent = (HANDLE)strtoull(argv[4], nullptr, 0);

    char CrashDumpFilename[MAX_PATH];
    if (GetTempFileNameInTempDir("DF_Dump", CrashDumpFilename) != 0)
        return -1;

    MiniDumpHelper dumpHelper;
    dumpHelper.addKnownModule(L"ntdll");
    dumpHelper.addKnownModule(L"DashFaction");
    dumpHelper.addKnownModule(L"RF");
    dumpHelper.setInfoLevel(CRASHHANDLER_DMP_LEVEL);
    dumpHelper.writeDump(CrashDumpFilename, ExceptionPtrs, hProcess, dwThreadId);

    SetEvent(hEvent);

    CloseHandle(hProcess);
    CloseHandle(hEvent);

    PrepareArchive(CrashDumpFilename);
    DeleteFileA(CrashDumpFilename);

    if (GetSystemMetrics(SM_CMONITORS) == 0)
        printf("%s\n", CRASHHANDLER_MSG);
    else {
#if CRASHHANDLER_WEBSVC_ENABLED
        if (MessageBox(NULL, TEXT(CRASHHANDLER_MSG), NULL, MB_ICONERROR | MB_YESNO | MB_SETFOREGROUND | MB_TASKMODAL) ==
            IDYES) {
            SendArchive();
            MessageBox(NULL, TEXT("Crash report has been sent. Thank you!"), NULL,
                       MB_ICONINFORMATION | MB_OK | MB_SETFOREGROUND | MB_TASKMODAL);
        }
#else  // CRASHHANDLER_WEBSVC_ENABLED
        MessageBox(NULL, TEXT(CRASHHANDLER_MSG), NULL, MB_ICONERROR | MB_OK | MB_SETFOREGROUND | MB_TASKMODAL);
#endif // CRASHHANDLER_WEBSVC_ENABLED
    }

    return 0;
}
catch (const std::exception& e) {
    MessageBoxA(NULL, e.what(), NULL, MB_ICONERROR | MB_OK | MB_SETFOREGROUND | MB_TASKMODAL);
    return -1;
}
