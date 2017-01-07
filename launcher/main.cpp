#include "stdafx.h"
#include "version.h"
#include "crc32.h"
#include "sharedoptions.h"
#include "Exception.h"

#define HRESULT_CUST_BIT 0x20000000
#define FACILITY_MOD 0x09F
#define MAKE_MOD_ERROR(code) (0x80000000 | HRESULT_CUST_BIT | (FACILITY_MOD << 16) | ((code) & 0xFFFF))
#define GET_LAST_WIN32_ERROR() ((HRESULT)(GetLastError()) < 0 ? ((HRESULT)(GetLastError())) : ((HRESULT) (((GetLastError()) & 0x0000FFFF) | (FACILITY_WIN32 << 16) | 0x80000000)))
#define RF_120_NA_CRC32 0xA7BF79E4

#define THROW_EXCEPTION_WITH_WIN32_ERROR() THROW_EXCEPTION("win32 error %lu", GetLastError())

void InitProcess(HANDLE hProcess, const TCHAR *pszPath, SHARED_OPTIONS *pOptions)
{
    HANDLE hThread = NULL;
    PVOID pVirtBuf = NULL;
    DWORD dwExitCode, dwWaitResult;
    FARPROC pfnLoadLibrary, pfnInit;
    unsigned cbPath = strlen(pszPath) + 1;
    HMODULE hLib;
    
    pfnLoadLibrary = GetProcAddress(GetModuleHandle("kernel32.dll"), "LoadLibraryA");
    if (!pfnLoadLibrary)
        THROW_EXCEPTION_WITH_WIN32_ERROR();
    
    pVirtBuf = VirtualAllocEx(hProcess, NULL, cbPath, MEM_RESERVE|MEM_COMMIT, PAGE_EXECUTE_READWRITE);
    if (!pVirtBuf)
        THROW_EXCEPTION_WITH_WIN32_ERROR();
    
    /* For some reason WriteProcessMemory returns 0, but memory is written */
    WriteProcessMemory(hProcess, pVirtBuf, pszPath, cbPath, NULL);
    
    printf("Loading %s\n", pszPath);
    hThread = CreateRemoteThread(hProcess, NULL, 0, (LPTHREAD_START_ROUTINE)pfnLoadLibrary, pVirtBuf, 0, NULL);
    if (!hThread)
        THROW_EXCEPTION_WITH_WIN32_ERROR();
    
    dwWaitResult = WaitForSingleObject(hThread, 5000);
    if (dwWaitResult != WAIT_OBJECT_0)
    {
        if (dwWaitResult == WAIT_TIMEOUT)
            THROW_EXCEPTION("timeout");
        else
            THROW_EXCEPTION_WITH_WIN32_ERROR();
    }
    
    if (!GetExitCodeThread(hThread, &dwExitCode))
        THROW_EXCEPTION_WITH_WIN32_ERROR();
    
    if (!dwExitCode)
        THROW_EXCEPTION("remote LoadLibrary failed");
    
    hLib = LoadLibrary(pszPath);
    if (!hLib)
        THROW_EXCEPTION_WITH_WIN32_ERROR();

    pfnInit = GetProcAddress(hLib, "Init");
    FreeLibrary(hLib);
    if (!pfnInit)
        THROW_EXCEPTION_WITH_WIN32_ERROR();
    
    CloseHandle(hThread);

    pVirtBuf = VirtualAllocEx(hProcess, NULL, sizeof(*pOptions), MEM_RESERVE | MEM_COMMIT, PAGE_EXECUTE_READWRITE);
    if (!pVirtBuf)
        THROW_EXCEPTION_WITH_WIN32_ERROR();
    
    WriteProcessMemory(hProcess, pVirtBuf, pOptions, sizeof(*pOptions), NULL);

    hThread = CreateRemoteThread(hProcess, NULL, 0, (LPTHREAD_START_ROUTINE)((DWORD_PTR)pfnInit - (DWORD_PTR)hLib + dwExitCode), pVirtBuf, 0, NULL);
    if (!hThread)
        THROW_EXCEPTION_WITH_WIN32_ERROR();
    
    dwWaitResult = WaitForSingleObject(hThread, INFINITE);
    if (dwWaitResult != WAIT_OBJECT_0)
        THROW_EXCEPTION_WITH_WIN32_ERROR();
    
    if (!GetExitCodeThread(hThread, &dwExitCode))
        THROW_EXCEPTION_WITH_WIN32_ERROR();
    
    if (!dwExitCode)
        THROW_EXCEPTION("Init failed");

    VirtualFreeEx(hProcess, pVirtBuf, 0, MEM_RELEASE);
    CloseHandle(hThread);
}

HRESULT GetRfPath(char *pszPath, DWORD cbPath)
{
    HKEY hKey;
    LONG iError;
    DWORD dwType;
    
    /* Open RF registry key */
    iError = RegOpenKeyEx(HKEY_LOCAL_MACHINE, "SOFTWARE\\Volition\\Red Faction", 0, KEY_READ, &hKey);
    if (iError != ERROR_SUCCESS)
    {
        printf("RegOpenKeyEx failed: %lu\n", GetLastError());
        return HRESULT_FROM_WIN32(iError);
    }
    
    /* Read install path and close key */
    iError = RegQueryValueEx(hKey, "InstallPath", NULL, &dwType, (LPBYTE)pszPath, &cbPath);
    RegCloseKey(hKey);
    if (iError != ERROR_SUCCESS)
        return HRESULT_FROM_WIN32(iError);
    
    if (dwType != REG_SZ)
        return MAKE_MOD_ERROR(0x04);
    
    /* Is path NULL terminated? */
    if (cbPath == 0 || pszPath[cbPath - 1] != 0)
        return MAKE_MOD_ERROR(0x05);
    
    return S_OK;
}

bool IsWindowedModeEnabled()
{
    HKEY hKey;
    LONG iError;
    DWORD dwType, dwWindowMode, dwSize = sizeof(dwWindowMode);

    /* Open RF registry key */
    iError = RegOpenKeyEx(HKEY_CURRENT_USER, "SOFTWARE\\Volition\\Red Faction", 0, KEY_READ, &hKey);
    if (iError != ERROR_SUCCESS)
    {
        printf("RegOpenKeyEx failed: %lu\n", GetLastError());
        return FALSE;
    }

    /* Read Pure Faction window mode setting */
    iError = RegQueryValueEx(hKey, "windowMode", NULL, &dwType, (LPBYTE)&dwWindowMode, &dwSize);
    RegCloseKey(hKey);
    if (iError != ERROR_SUCCESS)
        return FALSE;

    if (dwType != REG_DWORD)
        return FALSE;

    return dwWindowMode != 3;
}

HRESULT GetRfSteamPath(char *pszPath, DWORD cbPath)
{
    HKEY hKey;
    LONG iError;
    DWORD dwType;

    /* Open RF registry key */
    iError = RegOpenKeyEx(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\Steam App 20530", 0, KEY_READ, &hKey);
    if (iError != ERROR_SUCCESS)
        return HRESULT_FROM_WIN32(iError);

    /* Read install path and close key */
    iError = RegQueryValueEx(hKey, "InstallLocation", NULL, &dwType, (LPBYTE)pszPath, &cbPath);
    RegCloseKey(hKey);
    if (iError != ERROR_SUCCESS)
        return HRESULT_FROM_WIN32(iError);

    if (dwType != REG_SZ)
        return MAKE_MOD_ERROR(0x04);

    /* Is path NULL terminated? */
    if (cbPath == 0 || pszPath[cbPath - 1] != 0)
        return MAKE_MOD_ERROR(0x05);

    return S_OK;
}

uint32_t GetFileCRC32(const char *path)
{
    char buf[1024];
    FILE *pFile = fopen(path, "rb");
    if (!pFile) return 0;
    uint32_t hash = 0;
    while (1)
    {
        size_t len = fread(buf, 1, sizeof(buf), pFile);
        if (!len) break;
        hash = crc32(hash, buf, len);
    }
    fclose(pFile);
    return hash;
}

int main(int argc, const char *argv[]) try
{
    STARTUPINFO si;
    PROCESS_INFORMATION pi;
    char szBuf[MAX_PATH], szBuf2[256];
    char szRfPath[MAX_PATH];
    unsigned i;
    HRESULT hr;
    SHARED_OPTIONS Options;
    
    printf("Starting " PRODUCT_NAME_VERSION "!\n");

    //Options.bMultiSampling = FALSE;
    Options.bMultiSampling = TRUE;
    Options.bWindowed = IsWindowedModeEnabled();
    printf("Windowed mode: %d\n", Options.bWindowed);

    for (i = 1; i < (unsigned) argc; ++i)
    {
        if (!strcmp(argv[i], "-msaa"))
            Options.bMultiSampling = TRUE;
        else if (!strcmp(argv[i], "-no-msaa"))
            Options.bMultiSampling = FALSE;
        else if (!strcmp(argv[i], "-windowed"))
            Options.bWindowed = TRUE;
        else if (!strcmp(argv[i], "-h"))
            printf("Supported options:\n"
                "\t-msaa  Enable experimental Multisample Anti-Aliasing support");
    }

    hr = GetRfPath(szRfPath, sizeof(szRfPath));
    if (FAILED(hr) && FAILED(GetRfSteamPath(szRfPath, sizeof(szRfPath))))
    {
        printf("Warning %lX! Failed to read RF install path from registry.\n", hr);

        /* Use default path */
        if (PathFileExists("C:\\games\\RedFaction\\rf.exe"))
            strcpy(szRfPath, "C:\\games\\RedFaction");
        else
        {
            /* Fallback to current directory */
            GetCurrentDirectory(sizeof(szRfPath), szRfPath);
        }
    }
    
    /* Start RF process */
    sprintf(szBuf, "%s\\rf.exe", szRfPath);
    if (!PathFileExists(szBuf))
    {
        sprintf(szBuf, "Error %lX! Cannot find Red Faction path. Reinstall is needed.", hr);
        MessageBox(NULL, szBuf, NULL, MB_OK|MB_ICONERROR);
        return (int)hr;
    }
    
    uint32_t hash = GetFileCRC32(szBuf);
    printf("CRC32: %X\n", hash);
    if (hash != RF_120_NA_CRC32)
    {
        sprintf(szBuf, "Error! Unsupported version of Red Faction executable has been detected (crc32 0x%X). Only version 1.20 North America is supported.", hash);
        MessageBox(NULL, szBuf, NULL, MB_OK | MB_ICONERROR);
        return -1;
    }
    
    ZeroMemory(&si, sizeof(si));
    printf("Starting %s...\n", szBuf);
    if (!CreateProcess(szBuf, GetCommandLine(), NULL, NULL, FALSE, CREATE_SUSPENDED, NULL, szRfPath, &si, &pi))
    {
        if (GetLastError() == ERROR_ELEVATION_REQUIRED)
        {
            MessageBox(NULL,
                "Privilege elevation is required. Please change RF.exe file properties and disable all "
                "compatibility settings (Run as administrator, Compatibility mode for Windows XX, etc.) or run "
                "Dash Faction launcher as administrator.",
                NULL, MB_OK | MB_ICONERROR);
            return -1;
        }
        
        hr = GET_LAST_WIN32_ERROR();
        sprintf(szBuf2, "Error %lX! Failed to start: %s", hr, szBuf);
        MessageBox(NULL, szBuf2, NULL, MB_OK|MB_ICONERROR);
        return (int)hr;
    }
    
    i = GetCurrentDirectory(sizeof(szBuf)/sizeof(szBuf[0]), szBuf);
    if (!i)
    {
        hr = GET_LAST_WIN32_ERROR();
        sprintf(szBuf, "Error %lX! Failed to get current directory", hr);
        MessageBox(NULL, szBuf, NULL, MB_OK|MB_ICONERROR);
        TerminateProcess(pi.hProcess, 0);
        return (int)hr;
    }
    
    sprintf(szBuf + i, "\\DashFaction.dll");
    try
    {
        InitProcess(pi.hProcess, szBuf, &Options);
    }
    catch (const std::exception &e)
    {
        sprintf(szBuf, "Failed to initialize game process! Error: %s", e.what());
        MessageBox(NULL, szBuf, NULL, MB_OK | MB_ICONERROR);
        TerminateProcess(pi.hProcess, 0);
        return -1;
    }
    
#if 0
    printf("Press ENTER to resume game launch...");
    getchar();
#endif

    ResumeThread(pi.hThread);

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    
    return hr;
}
catch (const std::exception &e)
{
    fprintf(stderr, "Fatal error: %s\n", e.what());
}
