#ifndef UTILS_H_INCLUDED
#define UTILS_H_INCLUDED

#include <windows.h>
#include <stdint.h>

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

void DbgPrint(char *format, ...);

#ifdef NDEBUG
#define DEBUG_LEVEL 2
#else
#define DEBUG_LEVEL 3
#endif

#if DEBUG_LEVEL >= 1
#define ERR(...) DbgPrint(__VA_ARGS__)
#else
#define ERR(...) do {} while (false)
#endif

#if DEBUG_LEVEL >= 2
#define WARN(...) DbgPrint(__VA_ARGS__)
#else
#define WARN(...) do {} while (false)
#endif

#if DEBUG_LEVEL >= 3
#define INFO(...) DbgPrint(__VA_ARGS__)
#else
#define INFO(...) do {} while (false)
#endif

#if DEBUG_LEVEL >= 4
#define TRACE(...) DbgPrint(__VA_ARGS__)
#else
#define TRACE(...) do {} while (false)
#endif


char *stristr(const char *haystack, const char *needle);

#define COUNTOF(a) (sizeof(a)/sizeof((a)[0]))

#define ASM_NOP           0x90
#define ASM_LONG_CALL_REL 0xE8
#define ASM_LONG_JMP_REL  0xE9
#define ASM_SHORT_JMP_REL 0xEB
#define ASM_RET           0xC3
#define ASM_MOV_ECX       0xB9
#define ASM_MOV_ESI_PTR   0x358B
#define ASM_MOV_EAX     0xB8
#define ASM_PUSH_EBX      0x53
#define ASM_PUSH_EDI      0x57
#define ASM_ADD_ESP_BYTE  0xC483

#endif // UTILS_H_INCLUDED
