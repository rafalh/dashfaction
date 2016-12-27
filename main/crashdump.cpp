#include "crashdump.h"
#include <windows.h>
#include <stdio.h>
#include <psapi.h>
#include <stdint.h>

#define CRASHDUMP_USE_DMP 0
#define CRASHDUMP_USE_HEURISTIC_STACK_TRACE 1
#if CRASHDUMP_USE_DMP
#define CRASHDUMP_FILENAME "rfmod_crash.dmp"
#else
#define CRASHDUMP_FILENAME "rfmod_crash.txt"
#endif

#define CRASHDUMP_HEURISTIC_MAX_FUN_SIZE 4096

#ifdef _MSC_VER
#include <Dbghelp.h>
#else

typedef struct _MINIDUMP_EXCEPTION_INFORMATION
{
	DWORD ThreadId;
	PEXCEPTION_POINTERS ExceptionPointers;
	BOOL ClientPointers;
} MINIDUMP_EXCEPTION_INFORMATION, *PMINIDUMP_EXCEPTION_INFORMATION;

typedef enum _MINIDUMP_TYPE
{
	MiniDumpNormal                           = 0x00000000,
	MiniDumpWithDataSegs                     = 0x00000001,
	MiniDumpWithFullMemory                   = 0x00000002,
	MiniDumpWithHandleData                   = 0x00000004,
	MiniDumpFilterMemory                     = 0x00000008,
	MiniDumpScanMemory                       = 0x00000010,
	MiniDumpWithUnloadedModules              = 0x00000020,
	MiniDumpWithIndirectlyReferencedMemory   = 0x00000040,
	MiniDumpFilterModulePaths                = 0x00000080,
	MiniDumpWithProcessThreadData            = 0x00000100,
	MiniDumpWithPrivateReadWriteMemory       = 0x00000200,
	MiniDumpWithoutOptionalData              = 0x00000400,
	MiniDumpWithFullMemoryInfo               = 0x00000800,
	MiniDumpWithThreadInfo                   = 0x00001000,
	MiniDumpWithCodeSegs                     = 0x00002000,
	MiniDumpWithoutAuxiliaryState            = 0x00004000,
	MiniDumpWithFullAuxiliaryState           = 0x00008000,
	MiniDumpWithPrivateWriteCopyMemory       = 0x00010000,
	MiniDumpIgnoreInaccessibleMemory         = 0x00020000,
	MiniDumpWithTokenInformation             = 0x00040000
} MINIDUMP_TYPE;

typedef void *PMINIDUMP_USER_STREAM_INFORMATION;
typedef void *PMINIDUMP_CALLBACK_INFORMATION;

#endif /* !_MSC_VER */

typedef BOOL (WINAPI *PMINIDUMPWRITEDUMP)(HANDLE hProcess,
	DWORD ProcessId,
	HANDLE hFile,
	MINIDUMP_TYPE DumpType,
	PMINIDUMP_EXCEPTION_INFORMATION ExceptionParam,
	PMINIDUMP_USER_STREAM_INFORMATION UserStreamParam,
	PMINIDUMP_CALLBACK_INFORMATION CallbackParam
);

#if CRASHDUMP_USE_DMP
static HMODULE g_hDbgHelpLib = NULL;
static HANDLE g_hDumpFile = NULL;
static PMINIDUMPWRITEDUMP g_pMiniDumpWriteDump = NULL;
static MINIDUMP_EXCEPTION_INFORMATION g_MiniDumpInfo;
#else
static FILE *g_pFile = NULL;
static HMODULE Modules[256];
static DWORD cModules;
static char szBuffer[256];
#endif

#if CRASHDUMP_USE_HEURISTIC_STACK_TRACE == 0

static inline void WriteFrameBasedStackTrace(FILE *pFile, DWORD Ebp)
{
	PVOID *pFrame = (PVOID*)Ebp;
	while (!IsBadReadPtr(pFrame, 8))
	{
		fprintf(pFile, "%08lX\n", (DWORD)pFrame[1]);
		if (pFrame == (PVOID)pFrame[0])
		{
			fprintf(g_pFile, "...\n");
			break;
		}
		pFrame = (PVOID)pFrame[0];
	}
}

#else // CRASHDUMP_USE_HEURISTIC_STACK_TRACE

static void inline WriteHeuristicStackTrace(FILE *pFile, DWORD Eip, DWORD Esp)
{
	int i;
	BYTE *PrevCodePtr = (BYTE*)Eip;
	for (i = 0; i < 1024; ++i)
	{
		// Read next values from the stack
		DWORD *StackPtr = ((DWORD*)Esp) + i;
		if (IsBadReadPtr(StackPtr, sizeof(DWORD)))
			break;
		BYTE *CodePtr = (BYTE*)*StackPtr;

		// Check if this is address to code
		if (IsBadCodePtr((FARPROC)(CodePtr - 6)) || IsBadCodePtr((FARPROC)CodePtr))
			continue;

		if (CodePtr[-5] == 0xE8) // call rel32
		{
			BYTE *CallAddr = (BYTE*)((int)CodePtr + *(int*)(CodePtr - 4));
			//WARN("%p %p %u", CodePtr, CallAddr, abs(CallAddr - PrevCodePtr));
			if (abs(CallAddr - PrevCodePtr) > CRASHDUMP_HEURISTIC_MAX_FUN_SIZE) // check if call destinatio is near 
				continue;
			fprintf(pFile, "%08p\n", CodePtr - 5);
		}
		//else if (CodePtr[-6] == 0xFF && CodePtr[-5] == 0x15) // call ds:rel32
		//	fprintf(g_pFile, "%08p\n", CodePtr - 6);
		else
			continue;

		PrevCodePtr = CodePtr;
	}
}

#endif

static int ModuleHandleCompare(const void *phMod1, const void *phMod2)
{
	return *((HMODULE*)phMod1) - *((HMODULE*)phMod2);
}

static LONG WINAPI CrashDumpExceptionFilter(PEXCEPTION_POINTERS pExceptionPointers)
{
#if CRASHDUMP_USE_DMP
	if(g_hDbgHelpLib)
	{
		if(g_pMiniDumpWriteDump)
		{
			g_hDumpFile = CreateFile(TEXT(CRASHDUMP_FILENAME),
									 GENERIC_READ | GENERIC_WRITE,
                                     FILE_SHARE_WRITE | FILE_SHARE_READ,
                                     NULL,
									 CREATE_ALWAYS,
									 FILE_ATTRIBUTE_NORMAL,
									 NULL);
            
			if(INVALID_HANDLE_VALUE != g_hDumpFile)
			{
				g_MiniDumpInfo.ThreadId = GetCurrentThreadId();
				g_MiniDumpInfo.ExceptionPointers = pExceptionPointers;
				g_MiniDumpInfo.ClientPointers = TRUE;
				
				g_pMiniDumpWriteDump(GetCurrentProcess(),
						             GetCurrentProcessId(),
						             g_hDumpFile,
						             MiniDumpNormal,//MiniDumpWithDataSegs,//MiniDumpWithFullMemory|MiniDumpWithHandleData),
						             &g_MiniDumpInfo,
						             NULL,
						             NULL);
						
				CloseHandle(g_hDumpFile);
				
				MessageBox(NULL, TEXT("Application has crashed! Crashdump has been saved in " CRASHDUMP_FILENAME ". Please send this file to program author. His E-Mail can be found in About window."), NULL, MB_ICONERROR|MB_OK);
			} else
                MessageBox(NULL, TEXT("Error! CreateFile failed."), 0, MB_ICONERROR|MB_OK);
		} else
            MessageBox(NULL, TEXT("Error! GetProcAddress failed."), 0, MB_ICONERROR|MB_OK);
	} else
        MessageBox(NULL, TEXT("Error! LoadLibrary failed."), 0, MB_ICONERROR|MB_OK);
#else // CRASHDUMP_USE_DMP
    g_pFile = fopen(CRASHDUMP_FILENAME, "w");
    if (g_pFile)
    {
        unsigned i;
        
        fprintf(g_pFile, "ExceptionCode: %08lX\n", pExceptionPointers->ExceptionRecord->ExceptionCode);
        fprintf(g_pFile, "ExceptionFlags: %08lX\n", pExceptionPointers->ExceptionRecord->ExceptionFlags);
        fprintf(g_pFile, "ExceptionAddress: %p\n", pExceptionPointers->ExceptionRecord->ExceptionAddress);
        
        fprintf(g_pFile, "ExceptionInformation:");
        for (i = 0; i < pExceptionPointers->ExceptionRecord->NumberParameters; ++i)
            fprintf(g_pFile, " %08lX", pExceptionPointers->ExceptionRecord->ExceptionInformation[i]);
        fprintf(g_pFile, "\n");
        
#ifdef _X86_
        fprintf(g_pFile, "\nRegisters:\n");
        fprintf(g_pFile, "EIP: %08lX\t", pExceptionPointers->ContextRecord->Eip);
        fprintf(g_pFile, "EBP: %08lX\t", pExceptionPointers->ContextRecord->Ebp);
        fprintf(g_pFile, "ESP: %08lX\n", pExceptionPointers->ContextRecord->Esp);
        fprintf(g_pFile, "EAX: %08lX\t", pExceptionPointers->ContextRecord->Eax);
        fprintf(g_pFile, "EBX: %08lX\t", pExceptionPointers->ContextRecord->Ebx);
        fprintf(g_pFile, "ECX: %08lX\t", pExceptionPointers->ContextRecord->Ecx);
        fprintf(g_pFile, "EDX: %08lX\n", pExceptionPointers->ContextRecord->Edx);
        fprintf(g_pFile, "EDI: %08lX\t", pExceptionPointers->ContextRecord->Edi);
        fprintf(g_pFile, "ESI: %08lX\n", pExceptionPointers->ContextRecord->Esi);
        
		// Note: Red Faction has 'omit frame pointer' optimization enabled
		fprintf(g_pFile, "\nStacktrace:\n%08lX\n", pExceptionPointers->ContextRecord->Eip);
#if CRASHDUMP_USE_HEURISTIC_STACK_TRACE
		WriteHeuristicStackTrace(g_pFile, pExceptionPointers->ContextRecord->Eip, pExceptionPointers->ContextRecord->Esp);
#else
		WriteFrameBasedStackTrace(g_pFile, pExceptionPointers->ContextRecord->Ebp);
#endif

		fprintf(g_pFile, "\nStack:\n");
		for (i = 0; i < 1024; ++i)
		{
			DWORD *Ptr = ((DWORD*)pExceptionPointers->ContextRecord->Esp) + i;
			if (IsBadReadPtr(Ptr, sizeof(DWORD)))
				break;
			fprintf(g_pFile, "%08lX%s", *Ptr, i % 8 == 7 ? "\n" : " ");
		}
        
        if (EnumProcessModules(GetCurrentProcess(), Modules, sizeof(Modules), &cModules))
        {
            if (cModules > sizeof(Modules))
                cModules = sizeof(Modules);
            cModules /= sizeof(HMODULE);
			qsort(Modules, cModules, sizeof(HMODULE), ModuleHandleCompare);
            
            fprintf(g_pFile, "\nModules:\n");
            for (i = 0; i < cModules; ++i)
            {
                if (!GetModuleFileName(Modules[i], szBuffer, sizeof(szBuffer)))
                    szBuffer[0] = 0;
                fprintf(g_pFile, "%p\t%s\n", Modules[i], szBuffer);
            }
        }
        
#endif // _X86_
        fclose(g_pFile);
        MessageBox(NULL, TEXT("Application has crashed! Crashdump has been saved in " CRASHDUMP_FILENAME ". Please send this file to the program author."), NULL, MB_ICONERROR|MB_OK);
    }
#endif // CRASHDUMP_USE_DMP

	return EXCEPTION_EXECUTE_HANDLER;
}

void InitCrashDumps(void)
{
#if CRASHDUMP_USE_DMP
    g_hDbgHelpLib = LoadLibrary(TEXT("Dbghelp.dll"));
    if(g_hDbgHelpLib)
        g_pMiniDumpWriteDump = (PMINIDUMPWRITEDUMP)GetProcAddress(g_hDbgHelpLib, "MiniDumpWriteDump");
#endif
    SetUnhandledExceptionFilter(&CrashDumpExceptionFilter);
}

void UninitCrashDumps(void)
{
#if CRASHDUMP_USE_DMP
    FreeLibrary(g_hDbgHelpLib);
#endif
}
