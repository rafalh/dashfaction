#include "utils.h"
#include "rf.h"
#include <stdio.h>

void WriteMem(PVOID pAddr, PVOID pValue, unsigned cbValue)
{
    DWORD dwOldProtect;
    
    VirtualProtect(pAddr, cbValue, PAGE_EXECUTE_READWRITE, &dwOldProtect);
    memcpy(pAddr, pValue, cbValue);
    VirtualProtect(pAddr, cbValue, dwOldProtect, NULL);
}

void WriteMemUInt32(PVOID pAddr, uint32_t uValue)
{
    WriteMem(pAddr, &uValue, sizeof(uValue));
}

void WriteMemInt32(PVOID pAddr, int32_t uValue)
{
    WriteMem(pAddr, &uValue, sizeof(uValue));
}

void WriteMemUInt16(PVOID pAddr, uint16_t uValue)
{
    WriteMem(pAddr, &uValue, sizeof(uValue));
}

void WriteMemInt16(PVOID pAddr, int16_t uValue)
{
    WriteMem(pAddr, &uValue, sizeof(uValue));
}

void WriteMemUInt8Repeat(PVOID pAddr, uint8_t uValue, unsigned cRepeat)
{
    while (cRepeat > 0)
    {
        WriteMem(pAddr, &uValue, sizeof(uValue));
        pAddr = (PVOID)(((ULONG_PTR)pAddr) + sizeof(uValue));
        --cRepeat;
    }
}

void WriteMemUInt8(PVOID pAddr, uint8_t uValue)
{
    WriteMem(pAddr, &uValue, sizeof(uValue));
}

void WriteMemInt8(PVOID pAddr, int8_t uValue)
{
    WriteMem(pAddr, &uValue, sizeof(uValue));
}

void WriteMemFloat(PVOID pAddr, float fValue)
{
    WriteMem(pAddr, &fValue, sizeof(fValue));
}

void WriteMemPtr(PVOID pAddr, const void *Ptr)
{
    WriteMem(pAddr, (PVOID)&Ptr, sizeof(Ptr));
}

void WriteMemStr(PVOID pAddr, const char *pStr)
{
    WriteMem(pAddr, (PVOID)pStr, strlen(pStr) + 1);
}

void DbgPrint(char *pszFormat, ...)
{
    va_list pArgList;
    FILE *pFile;
    char szBuf[256];
    static int bFirstFileDebug = 1;
    
    if(bFirstFileDebug)
        pFile = fopen("rfmod_dbg.txt", "w");
    else
        pFile = fopen("rfmod_dbg.txt", "a");
    
    if(pFile)
    {
        bFirstFileDebug = 0;
        va_start(pArgList, pszFormat);
        vfprintf(pFile, pszFormat, pArgList);
        va_end(pArgList);
        fputs("\n", pFile);
        fclose(pFile);
    }

    va_start(pArgList, pszFormat);
    vsnprintf(szBuf, sizeof(szBuf), pszFormat, pArgList);
    va_end(pArgList);
    RfConsoleWrite(szBuf, NULL);
}

char *stristr(const char *haystack, const char *needle)
{
    unsigned i, j;
    
    for(i = 0; haystack[i]; ++i)
    {
        for(j = 0; needle[j]; ++j)
            if(tolower(haystack[i + j]) != tolower(needle[j]))
                break;
        
        if(!needle[j])
            return (char*)(haystack+i);
    }
    return NULL;
}
