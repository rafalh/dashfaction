#include "stdafx.h"
#include <Windows.h>
#include <stdexcept>
#include <Psapi.h>
#include <string>
#include "Process.h"

#ifdef __GNUC__
template<typename T>
constexpr PBYTE PtrFromRva(T base, int rva)
{
    return reinterpret_cast<PBYTE>(base) + rva;
}
#else
# define PtrFromRva( base, rva ) ( ( ( PBYTE ) base ) + rva )
#endif

Process::Process(int pid)
{
    m_hProcess = OpenProcess(PROCESS_CREATE_THREAD|PROCESS_VM_OPERATION|PROCESS_VM_WRITE|PROCESS_VM_READ|SYNCHRONIZE|PROCESS_QUERY_INFORMATION, FALSE, pid);
    if (!m_hProcess)
        throw std::runtime_error(std::string("OpenProcess failed: error ") + std::to_string(GetLastError()));
}

Process::~Process()
{
    CloseHandle(m_hProcess);
}

void *Process::ExecFunInternal(const char *ModuleName, const char *FunName, void *Arg, unsigned Timeout)
{
    HMODULE hMod = FindModule(ModuleName);
    if (!hMod)
        throw std::runtime_error(std::string("Module not found: ") + ModuleName);
    
    void *pProc = RemoteGetProcAddress(hMod, FunName);
    if (!pProc)
        throw std::runtime_error(std::string("Function not found: ") + FunName);
    
    HANDLE hThread = CreateRemoteThread(m_hProcess, NULL, 0, (LPTHREAD_START_ROUTINE)pProc, Arg, 0, NULL);
    if (!hThread)
        throw std::runtime_error(std::string("CreateRemoteThread failed: error ") + std::to_string(GetLastError()));
    
    if (WaitForSingleObject(hThread, Timeout) != WAIT_OBJECT_0)
        throw std::runtime_error(std::string("Timed out when running remote function: ") + FunName);
    
    DWORD dwExitCode;
    if (!GetExitCodeThread(hThread, &dwExitCode))
        throw std::runtime_error(std::string("GetExitCodeThread failed: error ") + std::to_string(GetLastError()));
    
    CloseHandle(hThread);
    return reinterpret_cast<void*>(dwExitCode);
}

void *Process::SaveData(const void *pData, size_t Size)
{
	PBYTE pMem = (PBYTE)VirtualAllocEx(m_hProcess, 0, Size, MEM_COMMIT, PAGE_READWRITE);
	if (!pMem)
		throw std::runtime_error("VirtualAllocEx failed");

	if (!WriteProcessMemory(m_hProcess, pMem, pData, Size, 0))
		throw std::runtime_error("WriteProcessMemory failed");

	return pMem;
}

void Process::FreeMem(void *pMem)
{
	VirtualFreeEx(m_hProcess, pMem, 0, MEM_RELEASE);
}

HMODULE Process::FindModule(const char *ModuleName)
{
	DWORD cBytes;
    if (!EnumProcessModules(m_hProcess, nullptr, 0, &cBytes))
        throw std::runtime_error(std::string("EnumProcessModules failed: error1 ") + std::to_string(GetLastError()));
	
    HMODULE *hModules = (HMODULE*)malloc(cBytes);
    if (!EnumProcessModules(m_hProcess, hModules, cBytes, &cBytes))
	{
		free(hModules);
		 throw std::runtime_error(std::string("EnumProcessModules failed: error2 ") + std::to_string(GetLastError()));
	}
    
	char buf[MAX_PATH];
	int cModules = cBytes / sizeof(HMODULE), i;
    for (i = 0; i < cModules; ++i)
        if (GetModuleBaseName(m_hProcess, hModules[i], buf, MAX_PATH) && !_stricmp(ModuleName, buf))
            break;
    
	HMODULE hRet = nullptr;
	if (i >= cModules)
		SetLastError(ERROR_MOD_NOT_FOUND);
	else
		hRet = hModules[i];
    free(hModules);
    
    return hRet;
}

void Process::ReadMem(void *Dest, const void *Src, size_t Len)
{
    if (!ReadProcessMemory(m_hProcess, Src, Dest, Len, nullptr))
		 throw std::runtime_error(std::string("ReadProcessMemory failed: error ") + std::to_string(GetLastError()));
}

void *Process::RemoteGetProcAddress(HMODULE hModule, const char *lpProcName)
{
    if (!hModule)
		 throw std::invalid_argument("hModule cannot be null");
	
	IMAGE_DOS_HEADER idh;
    if (!ReadProcessMemory(m_hProcess, hModule, &idh, sizeof(idh), nullptr))
		 throw std::runtime_error(std::string("ReadProcessMemory failed: error ") + std::to_string(GetLastError()));
    
    if (idh.e_magic != IMAGE_DOS_SIGNATURE)
	{
		SetLastError(ERROR_BAD_EXE_FORMAT);
		throw std::runtime_error("Invalid DOS header signature");
	}
	
	IMAGE_NT_HEADERS inth;
    if (!ReadProcessMemory(m_hProcess, PtrFromRva(hModule, idh.e_lfanew), &inth, sizeof(inth), nullptr))
		throw std::runtime_error(std::string("ReadProcessMemory failed: error ") + std::to_string(GetLastError()));
    
    if (inth.Signature != IMAGE_NT_SIGNATURE)
	{
		SetLastError(ERROR_BAD_EXE_FORMAT);
		throw std::runtime_error("Invalid NT header signature");
	}
	
    PIMAGE_EXPORT_DIRECTORY pIed = (PIMAGE_EXPORT_DIRECTORY)malloc(inth.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].Size);
    if (!pIed)
    	throw std::bad_alloc();
    
    if (!ReadProcessMemory(m_hProcess, PtrFromRva(hModule, inth.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress), pIed, inth.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].Size, NULL))
	{
		free(pIed);
		throw std::runtime_error(std::string("ReadProcessMemory failed: error ") + std::to_string(GetLastError()));
	}
	
    PVOID base = (PIMAGE_EXPORT_DIRECTORY*)(((PBYTE)pIed)-(inth.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress));
    DWORD *lpNames = (DWORD*)PtrFromRva(base, pIed->AddressOfNames);
    unsigned short *lpOrdinals = (unsigned short*)PtrFromRva(base, pIed->AddressOfNameOrdinals);
    DWORD *lpAddresses = (DWORD*)PtrFromRva(base, pIed->AddressOfFunctions);
    
    // Iterate over import descriptors/DLLs.
    for (unsigned i = 0; i < (pIed->NumberOfNames); ++i)
    {
        if (!strcmp((PSTR)PtrFromRva(base, lpNames[i]), lpProcName))
        {
			base = (PVOID)(DWORD_PTR)PtrFromRva(hModule, lpAddresses[lpOrdinals[i]]);
            free(pIed);
            return base;
        }
    }
    
    free(pIed);
    SetLastError(ERROR_MOD_NOT_FOUND);
    return nullptr;
}
