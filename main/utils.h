#pragma once

#include <windows.h>
#include <stdint.h>
#include <log/Logger.h>

void WriteMem(PVOID pAddr, PVOID pValue, unsigned cbValue);
void WriteMemUInt32(PVOID pAddr, uint32_t uValue);
void WriteMemInt32(PVOID pAddr, int32_t uValue);
void WriteMemUInt16(PVOID pAddr, uint16_t uValue);
void WriteMemInt16(PVOID pAddr, int16_t uValue);
void WriteMemUInt8Repeat(PVOID pAddr, uint8_t uValue, unsigned cRepeat);
void WriteMemUInt8(PVOID pAddr, uint8_t uValue);
void WriteMemInt8(PVOID pAddr, int8_t uValue);
void WriteMemFloat(PVOID pAddr, float fValue);
void WriteMemPtr(PVOID pAddr, const void *Ptr);
void WriteMemStr(PVOID pAddr, const char *pStr);

const char *stristr(const char *haystack, const char *needle);

const char *getDxErrorStr(HRESULT hr);
std::string getOsVersion();
std::string getRealOsVersion();
bool IsUserAdmin();
const char *GetProcessElevationType();

#define COUNTOF(a) (sizeof(a)/sizeof((a)[0]))

#define NAKED __declspec(naked)

#define ASSERT(x) assert(x)

#define ASM_NOP           0x90
#define ASM_LONG_CALL_REL 0xE8
#define ASM_LONG_JMP_REL  0xE9
#define ASM_SHORT_JMP_REL 0xEB
#define ASM_RET           0xC3
#define ASM_MOV_ECX       0xB9
#define ASM_MOV_ESI_PTR   0x358B
#define ASM_MOV_EAX       0xB8
#define ASM_PUSH_EBX      0x53
#define ASM_PUSH_ESI      0x56
#define ASM_PUSH_EDI      0x57
#define ASM_ADD_ESP_BYTE  0xC483
#define ASM_FADD          0xD8
