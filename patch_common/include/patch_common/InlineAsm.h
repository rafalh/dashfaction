#pragma once

// clang-format off

#ifdef __GNUC__

#define ASM_STR_INTERNAL(...) #__VA_ARGS__
#define ASM_FUNC(name, ...) extern "C" int name(); \
    __asm__ ( \
    ".text\r\n" \
    "_" #name ":\r\n" \
    ".intel_syntax noprefix\r\n" \
    ASM_STR_INTERNAL(__VA_ARGS__) "\r\n" \
    ".att_syntax\r\n");

#define ASM_I ;
#define ASM_SYM(sym) _ ## sym
#define ASM_LABEL(label) .L ## label

#else // __GNUC__

#define ASM_FUNC(name, ...) __declspec(naked) void name() { __asm { \
    __asm __VA_ARGS__ \
    } }
#define ASM_I __asm
#define ASM_SYM(sym) sym
#define ASM_LABEL(label) label

#endif // __GNUC__
