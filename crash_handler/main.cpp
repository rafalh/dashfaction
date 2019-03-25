#include <windows.h>
#include "version.h"
#include "ZipHelper.h"
#include "MiniDumpHelper.h"
#include "HttpRequest.h"
#include "Exception.h"

// Config
#define CRASHHANDLER_LOG_PATH "logs/DashFaction.log"
#define CRASHHANDLER_DMP_FILENAME "logs/DashFaction.dmp"
#define CRASHHANDLER_TARGET_DIR "logs"
#define CRASHHANDLER_TARGET_NAME "DashFaction-crash.zip"
//#define CRASHHANDLER_MSG "Game has crashed!\nTo help resolve the problem please send " CRASHHANDLER_TARGET_NAME " file from logs subdirectory in RedFaction directory to " PRODUCT_NAME " author."
#define CRASHHANDLER_MSG "Game has crashed!\nReport has been generated in " CRASHHANDLER_TARGET_DIR "\\" CRASHHANDLER_TARGET_NAME ".\nDo you want to send it to " PRODUCT_NAME " author to help resolve the problem?"
// Information Level: 0 smallest - 2 - biggest
#ifdef NDEBUG
#define CRASHHANDLER_DMP_LEVEL 0
#else
#define CRASHHANDLER_DMP_LEVEL 1
#endif

#define CRASHHANDLER_WEBSVC_ENABLED 1
#define CRASHHANDLER_WEBSVC_HOST "ravin.tk"
#define CRASHHANDLER_WEBSVC_PATH "/api/rf/dashfaction/crashreport.php"
#define CRASHHANDLER_WEBSVC_AGENT "DashFaction"

bool PrepareArchive(const char *CrashDumpFilename)
{
    CreateDirectoryA(CRASHHANDLER_TARGET_DIR, NULL);
    ZipHelper zip(CRASHHANDLER_TARGET_DIR "/" CRASHHANDLER_TARGET_NAME);
    zip.addFile(CrashDumpFilename, "CrashDump.dmp");
    zip.addFile(CRASHHANDLER_LOG_PATH, "AppLog.log");
    return true;
}

void SendArchive()
{
    HttpRequestInfo info;
    info.method = "POST";
    info.host = CRASHHANDLER_WEBSVC_HOST;
    info.path = CRASHHANDLER_WEBSVC_PATH;
    info.agent = CRASHHANDLER_WEBSVC_AGENT;

    FILE *file = fopen(CRASHHANDLER_TARGET_DIR "/" CRASHHANDLER_TARGET_NAME, "rb");
    if (!file)
        THROW_EXCEPTION("cannot open " CRASHHANDLER_TARGET_NAME);

    fseek(file, 0, SEEK_END);
    size_t size = ftell(file);
    fseek(file, 0, SEEK_SET);

    HttpRequest req(info);
    req.addHeaders("Content-Type: application/octet-stream\r\n");
    req.begin(size);
    char buf[2048];
    while (!feof(file))
    {
        size_t num = fread(buf, 1, sizeof(buf), file);
        req.write(buf, num);
    }
    fclose(file);
    req.end();
}

int GetTempFileNameInTempDir(const char *Prefix, char Result[MAX_PATH])
{
    char TempDir[MAX_PATH];
    DWORD dwRetVal = GetTempPathA(_countof(TempDir), TempDir);
    if (dwRetVal == 0 || dwRetVal > _countof(TempDir))
        return -1;

    UINT uRetVal = GetTempFileNameA(TempDir, Prefix, 0, Result);
    if (uRetVal == 0)
        return -1;

    return 0;
}

#if 1
int CALLBACK WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) try
{
    // Unused parameters
    (void)hInstance;
    (void)hPrevInstance;
    (void)lpCmdLine;
    (void)nCmdShow;

    auto argc = __argc;
    auto argv = __argv;
#else
int main(int argc, const char *argv[]) try
{
#endif

    if (argc < 5)
        return -1;

    EXCEPTION_POINTERS *pExceptionPtrs = (EXCEPTION_POINTERS*)strtoull(argv[1], nullptr, 0);
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
    dumpHelper.writeDump(CrashDumpFilename, pExceptionPtrs, hProcess, dwThreadId);

    SetEvent(hEvent);

    CloseHandle(hProcess);
    CloseHandle(hEvent);

    PrepareArchive(CrashDumpFilename);
    DeleteFileA(CrashDumpFilename);

    if (GetSystemMetrics(SM_CMONITORS) == 0)
        printf("%s\n", CRASHHANDLER_MSG);
    else
    {
#if CRASHHANDLER_WEBSVC_ENABLED
        if (MessageBox(NULL, TEXT(CRASHHANDLER_MSG), NULL, MB_ICONERROR | MB_YESNO | MB_SETFOREGROUND | MB_TASKMODAL) == IDYES)
        {
            SendArchive();
            MessageBox(NULL, TEXT("Crash report has been sent. Thank you!"), NULL, MB_ICONINFORMATION | MB_OK | MB_SETFOREGROUND | MB_TASKMODAL);
        }
#else // CRASHHANDLER_WEBSVC_ENABLED
        MessageBox(NULL, TEXT(CRASHHANDLER_MSG), NULL, MB_ICONERROR | MB_OK | MB_SETFOREGROUND | MB_TASKMODAL);
#endif // CRASHHANDLER_WEBSVC_ENABLED
    }

    return 0;
}
catch (std::exception &e)
{
    MessageBoxA(NULL, e.what(), NULL, MB_ICONERROR | MB_OK | MB_SETFOREGROUND | MB_TASKMODAL);
    return -1;
}
