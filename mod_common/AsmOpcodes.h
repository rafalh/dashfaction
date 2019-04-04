#pragma once

// Deprecated. Use AsmWritter.

// clang-format off
#define ASM_NOP           0x90
#define ASM_LONG_CALL_REL 0xE8
#define ASM_LONG_JMP_REL  0xE9
#define ASM_SHORT_JMP_REL 0xEB
#define ASM_RET           0xC3
#define ASM_PUSH_ESI      0x56
#define ASM_FADD          0xD8
#define ASM_JAE_SHORT     0x73
// clang-format on
