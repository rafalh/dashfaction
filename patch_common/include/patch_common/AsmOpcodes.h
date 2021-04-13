#pragma once

// Deprecated. Use AsmWriter.

#include <cstdint>

namespace asm_opcodes
{

// clang-format off
constexpr std::uint8_t jae_rel_short  = 0x73;
constexpr std::uint8_t jz_rel_short   = 0x74;
constexpr std::uint8_t nop            = 0x90;
constexpr std::uint8_t int3           = 0xCC;
constexpr std::uint8_t fadd           = 0xD8;
constexpr std::uint8_t call_rel_long  = 0xE8;
constexpr std::uint8_t jmp_rel_long   = 0xE9;
constexpr std::uint8_t jmp_rel_short  = 0xEB;
// clang-format on

}
