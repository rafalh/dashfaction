#pragma once

#ifdef __GNUC__

#define _ASM_STR_INTERNAL(...) #__VA_ARGS__
#define ASM_FUNC(name, ...) extern "C" int name(void); \
    __asm__ ( \
    "_" #name ":\r\n" \
    ".intel_syntax noprefix\r\n" \
    _ASM_STR_INTERNAL(__VA_ARGS__) "\r\n" \
    ".att_syntax\r\n");

#define ASM_I ;
#define ASM_SYM(sym) _ ## sym

#else // __GNUC__

#define ASM_FUNC(name, ...) NAKED void name(void) { __asm { \
    __asm __VA_ARGS__ \
    } }
#define ASM_I __asm
#define ASM_SYM(sym) sym

#endif // __GNUC__
