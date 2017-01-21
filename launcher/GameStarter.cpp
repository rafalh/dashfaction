#include "stdafx.h"
#include "GameStarter.h"
#include "GameConfig.h"
#include "crc32.h"

#define DLL_FILENAME "DashFaction.dll"

#include "Exception.h"

#define THROW_EXCEPTION_WITH_WIN32_ERROR() THROW_EXCEPTION("win32 error %lu", GetLastError())
#define INIT_TIMEOUT 10000
#define RF_120_NA_CRC32 0xA7BF79E4

void ModdedAppLauncher::injectDLL(HANDLE hProcess, const TCHAR *pszPath)
{
    // Note: code from CobraMods cannot be used here because it uses EnumProcessModules API but it doesnt work on
    // suspended process

    unsigned cbPath = strlen(pszPath) + 1;

    FARPROC pfnLoadLibrary = GetProcAddress(GetModuleHandle("kernel32.dll"), "LoadLibraryA");
    if (!pfnLoadLibrary)
        THROW_EXCEPTION_WITH_WIN32_ERROR();

    PVOID pVirtBuf = VirtualAllocEx(hProcess, NULL, cbPath, MEM_RESERVE | MEM_COMMIT, PAGE_EXECUTE_READWRITE);
    if (!pVirtBuf)
        THROW_EXCEPTION_WITH_WIN32_ERROR();

    /* For some reason WriteProcessMemory returns 0, but memory is written */
    WriteProcessMemory(hProcess, pVirtBuf, pszPath, cbPath, NULL);

    printf("Loading %s\n", pszPath);
    HANDLE hThread = CreateRemoteThread(hProcess, NULL, 0, (LPTHREAD_START_ROUTINE)pfnLoadLibrary, pVirtBuf, 0, NULL);
    if (!hThread)
        THROW_EXCEPTION_WITH_WIN32_ERROR();

    DWORD dwWaitResult = WaitForSingleObject(hThread, INIT_TIMEOUT);
    if (dwWaitResult != WAIT_OBJECT_0)
    {
        if (dwWaitResult == WAIT_TIMEOUT)
            THROW_EXCEPTION("timeout");
        else
            THROW_EXCEPTION_WITH_WIN32_ERROR();
    }

    DWORD dwExitCode;
    if (!GetExitCodeThread(hThread, &dwExitCode))
        THROW_EXCEPTION_WITH_WIN32_ERROR();

    if (!dwExitCode)
        THROW_EXCEPTION("remote LoadLibrary failed");

    HMODULE hLib = LoadLibrary(pszPath);
    if (!hLib)
        THROW_EXCEPTION_WITH_WIN32_ERROR();

    FARPROC pfnInit = GetProcAddress(hLib, "Init");
    FreeLibrary(hLib);
    if (!pfnInit)
        THROW_EXCEPTION_WITH_WIN32_ERROR();

    CloseHandle(hThread);

    hThread = CreateRemoteThread(hProcess, NULL, 0, (LPTHREAD_START_ROUTINE)((DWORD_PTR)pfnInit - (DWORD_PTR)hLib + dwExitCode), nullptr, 0, NULL);
    if (!hThread)
        THROW_EXCEPTION_WITH_WIN32_ERROR();

    dwWaitResult = WaitForSingleObject(hThread, INIT_TIMEOUT);
    if (dwWaitResult != WAIT_OBJECT_0)
        THROW_EXCEPTION_WITH_WIN32_ERROR();

    if (!GetExitCodeThread(hThread, &dwExitCode))
        THROW_EXCEPTION_WITH_WIN32_ERROR();

    if (!dwExitCode)
        THROW_EXCEPTION("Init failed");

    VirtualFreeEx(hProcess, pVirtBuf, 0, MEM_RELEASE);
    CloseHandle(hThread);
}

std::string ModdedAppLauncher::getModDllPath()
{
    char buf[MAX_PATH];
    if (!GetModuleFileNameA(0, buf, sizeof(buf)))
        THROW_EXCEPTION_WITH_WIN32_ERROR();
    char *ptr = strrchr(buf, '\\');
    if (!ptr)
        THROW_EXCEPTION("Invalid path");
    sprintf(ptr, "\\%s", m_modDllName.c_str());
    return buf;
}

void ModdedAppLauncher::launch()
{
    GameConfig conf;
    conf.load();

    uint32_t checksum = GetFileCRC32(conf.gameExecutablePath.c_str());
    if (checksum != RF_120_NA_CRC32)
        throw IntegrityCheckFailedException(checksum);

    std::string appPath = getAppPath();
    size_t pos = appPath.find_last_of("\\/");
    std::string workDir = appPath.substr(0, pos);

    STARTUPINFO si;
    PROCESS_INFORMATION pi;

    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);

    BOOL result = CreateProcess(appPath.c_str(), GetCommandLine(),
        NULL, NULL, FALSE, CREATE_SUSPENDED, NULL, workDir.c_str(), &si, &pi);
    if (!result)
    {
        if (GetLastError() == ERROR_ELEVATION_REQUIRED)
            throw PrivilegeElevationRequiredException();

        THROW_EXCEPTION_WITH_WIN32_ERROR();
    }

    std::string modPath = getModDllPath();
    injectDLL(pi.hProcess, modPath.c_str());

    ResumeThread(pi.hThread);
    CloseHandle(pi.hThread);
}

GameStarter::GameStarter() :
    ModdedAppLauncher("DashFaction.dll", RF_120_NA_CRC32)
{
    m_conf.load();
}

std::string GameStarter::getAppPath()
{
    return m_conf.gameExecutablePath;
}
