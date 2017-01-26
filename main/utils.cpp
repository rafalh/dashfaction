#include "stdafx.h"
#include "utils.h"
#include "rf.h"

#define DEBUG_LOG_FILENAME "logs/DashFaction.log"

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
        pFile = fopen(DEBUG_LOG_FILENAME, "w");
    else
        pFile = fopen(DEBUG_LOG_FILENAME, "a");
    
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
    rf::RfConsoleWrite(szBuf, NULL);
}

const char *stristr(const char *haystack, const char *needle)
{
    unsigned i, j;
    
    for (i = 0; haystack[i]; ++i)
    {
        for (j = 0; needle[j]; ++j)
            if (tolower(haystack[i + j]) != tolower(needle[j]))
                break;
        
        if (!needle[j])
            return (char*)(haystack+i);
    }
    return NULL;
}

#define DEFINE_HRESULT_ERROR(hr) { hr, #hr },

const static std::map<HRESULT, const char*> DX_ERRORS = {
    DEFINE_HRESULT_ERROR(D3D_OK)
    DEFINE_HRESULT_ERROR(D3DERR_WRONGTEXTUREFORMAT)
    DEFINE_HRESULT_ERROR(D3DERR_UNSUPPORTEDCOLOROPERATION)
    DEFINE_HRESULT_ERROR(D3DERR_UNSUPPORTEDCOLORARG)
    DEFINE_HRESULT_ERROR(D3DERR_UNSUPPORTEDALPHAOPERATION)
    DEFINE_HRESULT_ERROR(D3DERR_UNSUPPORTEDALPHAARG)
    DEFINE_HRESULT_ERROR(D3DERR_TOOMANYOPERATIONS)
    DEFINE_HRESULT_ERROR(D3DERR_CONFLICTINGTEXTUREFILTER)
    DEFINE_HRESULT_ERROR(D3DERR_UNSUPPORTEDFACTORVALUE)
    DEFINE_HRESULT_ERROR(D3DERR_CONFLICTINGRENDERSTATE)
    DEFINE_HRESULT_ERROR(D3DERR_UNSUPPORTEDTEXTUREFILTER)
    DEFINE_HRESULT_ERROR(D3DERR_CONFLICTINGTEXTUREPALETTE)
    DEFINE_HRESULT_ERROR(D3DERR_DRIVERINTERNALERROR)

    DEFINE_HRESULT_ERROR(D3DERR_NOTFOUND)
    DEFINE_HRESULT_ERROR(D3DERR_MOREDATA)
    DEFINE_HRESULT_ERROR(D3DERR_DEVICELOST)
    DEFINE_HRESULT_ERROR(D3DERR_DEVICENOTRESET)
    DEFINE_HRESULT_ERROR(D3DERR_NOTAVAILABLE)
    DEFINE_HRESULT_ERROR(D3DERR_OUTOFVIDEOMEMORY)
    DEFINE_HRESULT_ERROR(D3DERR_INVALIDDEVICE)
    DEFINE_HRESULT_ERROR(D3DERR_INVALIDCALL)
    DEFINE_HRESULT_ERROR(D3DERR_DRIVERINVALIDCALL)
};

const char *getDxErrorStr(HRESULT hr)
{
    auto it = DX_ERRORS.find(hr);
    if (it != DX_ERRORS.end())
        return it->second;
    return nullptr;
}
