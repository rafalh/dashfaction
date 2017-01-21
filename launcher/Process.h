#pragma once

#include <Windows.h>

class Process
{
    public:
        Process(int pid);
        Process(HANDLE hProcess) : m_hProcess(hProcess) {}
        ~Process();
        
        template<typename T = void*, typename T2>
        T ExecFun(const char *ModuleName, const char *FunName, T2 Arg = 0, unsigned Timeout = 3000)
        {
            return reinterpret_cast<T>(ExecFunInternal(ModuleName, FunName, reinterpret_cast<void*>(Arg), Timeout));
        }
        
		void *SaveData(const void *pData, size_t Len);

		void *SaveString(const char *Value)
		{
			return SaveData(Value, strlen(Value) + 1);
		}

		void FreeMem(void *pMem);
        HMODULE FindModule(const char *ModuleName);
		void ReadMem(void *Dest, const void *Src, size_t Len);
    
    private:
        HANDLE m_hProcess;
        
        void *ExecFunInternal(const char *ModuleName, const char *FunName, void *Arg, unsigned Timeout);
        void *RemoteGetProcAddress(HMODULE hModule, const char *lpProcName);
};
