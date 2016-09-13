#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <shlwapi.h>
#include "version.h"

#define HRESULT_CUST_BIT 0x20000000
#define FACILITY_MOD 0x09F
#define MAKE_MOD_ERROR(code) (0x80000000 | HRESULT_CUST_BIT | (FACILITY_MOD << 16) | ((code) & 0xFFFF))
#define GET_LAST_WIN32_ERROR() ((HRESULT)(GetLastError()) < 0 ? ((HRESULT)(GetLastError())) : ((HRESULT) (((GetLastError()) & 0x0000FFFF) | (FACILITY_WIN32 << 16) | 0x80000000)))

HRESULT InitProcess(HANDLE hProcess, const TCHAR *pszPath, DWORD dwRfThreadId)
{
    HANDLE hThread = NULL;
    PVOID pVirtBuf = NULL;
    DWORD dwExitCode, dwWaitResult;
    FARPROC pfnLoadLibrary, pfnInit;
    unsigned cbPath = strlen(pszPath) + 1;
    HRESULT hr = S_OK;
    HMODULE hLib;
    
    pfnLoadLibrary = GetProcAddress(GetModuleHandle("kernel32.dll"), "LoadLibraryA");
    if (!pfnLoadLibrary)
        return GET_LAST_WIN32_ERROR();
    
    pVirtBuf = VirtualAllocEx(hProcess, NULL, cbPath, MEM_RESERVE|MEM_COMMIT, PAGE_EXECUTE_READWRITE);
    if (!pVirtBuf)
        return GET_LAST_WIN32_ERROR();
    
    /* For some reason WriteProcessMemory returns 0, but memory is written */
    WriteProcessMemory(hProcess, pVirtBuf, pszPath, cbPath, NULL);
    
    printf("Loading %s\n", pszPath);
    hThread = CreateRemoteThread(hProcess, NULL, 0, (LPTHREAD_START_ROUTINE)pfnLoadLibrary, pVirtBuf, 0, NULL);
    if (!hThread)
    {
        hr = GET_LAST_WIN32_ERROR();
        goto cleanup;
    }
    
    dwWaitResult = WaitForSingleObject(hThread, 5000);
    if (dwWaitResult != WAIT_OBJECT_0)
    {
        if(dwWaitResult == WAIT_TIMEOUT)
            hr = MAKE_MOD_ERROR(0x01);
        else
            hr = GET_LAST_WIN32_ERROR();
        goto cleanup;
    }
    
    if (!GetExitCodeThread(hThread, &dwExitCode))
    {
        hr = GET_LAST_WIN32_ERROR();
        goto cleanup;
    }
    
    if (!dwExitCode)
    {
        hr = MAKE_MOD_ERROR(0x02);
        goto cleanup;
    }
    
    hLib = LoadLibrary(pszPath);
    if (!hLib)
    {
        hr = GET_LAST_WIN32_ERROR();
        goto cleanup;
    }
    
    pfnInit = GetProcAddress(hLib, "Init");
    FreeLibrary(hLib);
    if (!pfnInit)
    {
        hr = GET_LAST_WIN32_ERROR();
        goto cleanup;
    }
    
    CloseHandle(hThread);
    hThread = CreateRemoteThread(hProcess, NULL, 0, (LPTHREAD_START_ROUTINE)((DWORD_PTR)pfnInit - (DWORD_PTR)hLib + dwExitCode), (PVOID)dwRfThreadId, 0, NULL);
    if (!hThread)
    {
        hr = GET_LAST_WIN32_ERROR();
        goto cleanup;
    }
    
    if (WaitForSingleObject(hThread, INFINITE) != WAIT_OBJECT_0)
    {
        hr = GET_LAST_WIN32_ERROR();
        goto cleanup;
    }
    
    if (!GetExitCodeThread(hThread, &dwExitCode))
    {
        hr = GET_LAST_WIN32_ERROR();
        goto cleanup;
    }
    
    if (!dwExitCode)
    {
        hr = MAKE_MOD_ERROR(0x03);
        goto cleanup;
    }

cleanup:
    if (pVirtBuf)
        VirtualFreeEx(hProcess, pVirtBuf, 0, MEM_RELEASE);
    CloseHandle(hThread);
    return hr;
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
    iError = RegQueryValueEx(hKey, "InstallPath", NULL, &dwType, (PVOID)pszPath, &cbPath);
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
	iError = RegQueryValueEx(hKey, "InstallLocation", NULL, &dwType, (PVOID)pszPath, &cbPath);
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

int main()
{
    STARTUPINFO si;
    PROCESS_INFORMATION pi;
    char szBuf[MAX_PATH], szBuf2[256];
    char szRfPath[MAX_PATH];
    unsigned i;
    HRESULT hr;
    
    printf("Starting " PRODUCT_NAME " " VER_STR "!\n");
    
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
    if(!PathFileExists(szBuf))
    {
        sprintf(szBuf, "Error %lX! Cannot find Red Faction path. Reinstall is needed.", hr);
        MessageBox(NULL, szBuf, NULL, MB_OK|MB_ICONERROR);
        return (int)hr;
    }
    
    ZeroMemory(&si, sizeof(si));
    printf("Starting %s...\n", szBuf);
    if(!CreateProcess(szBuf, NULL, NULL, NULL, FALSE, CREATE_SUSPENDED, NULL, szRfPath, &si, &pi))
    {
        hr = GET_LAST_WIN32_ERROR();
        sprintf(szBuf2, "Error %lX! Failed to start: %s", hr, szBuf);
        MessageBox(NULL, szBuf2, NULL, MB_OK|MB_ICONERROR);
        return (int)hr;
    }
    
    i = GetCurrentDirectory(sizeof(szBuf)/sizeof(szBuf[0]), szBuf);
    if(!i)
    {
        hr = GET_LAST_WIN32_ERROR();
        sprintf(szBuf, "Error %lX! Failed to get current directory", hr);
        MessageBox(NULL, szBuf, NULL, MB_OK|MB_ICONERROR);
        TerminateProcess(pi.hProcess, 0);
        return (int)hr;
    }
    
    sprintf(szBuf + i, "\\DashFaction.dll");
    hr = InitProcess(pi.hProcess, szBuf, pi.dwThreadId);
    if(FAILED(hr))
    {
        sprintf(szBuf, "Error %lX! Failed to init process.", hr);
        MessageBox(NULL, szBuf, NULL, MB_OK|MB_ICONERROR);
        TerminateProcess(pi.hProcess, 0);
    }
    
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    
    return hr;
}
