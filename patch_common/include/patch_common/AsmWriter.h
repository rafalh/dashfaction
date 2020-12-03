#pragma once

#include <patch_common/AsmOpcodes.h>
#include <patch_common/MemUtils.h>
#include <patch_common/ShortTypes.h>
#include <cassert>
#include <optional>

class AsmReg
{
public:
    int reg_num;
    int size;

    constexpr explicit AsmReg(int reg_num, int size) :
        reg_num(reg_num), size(size)
    {}

    constexpr bool operator==(const AsmReg& other) const
    {
        return reg_num == other.reg_num && size == other.size;
    }
};

struct AsmReg32 : public AsmReg
{
    explicit constexpr AsmReg32(int num) :
        AsmReg(num, 4)
    {}

    std::pair<AsmReg32, int> operator+(int n) const
    {
        return {*this, n};
    }
};

inline std::pair<AsmReg32, int> operator+(std::pair<AsmReg32, int> p, int n)
{
    return {p.first, p.second + n};
}

inline std::pair<AsmReg32, int> operator-(std::pair<AsmReg32, int> p, int n)
{
    return {p.first, p.second - n};
}

struct AsmReg16 : public AsmReg
{
    explicit constexpr AsmReg16(int num) :
        AsmReg(num, 2)
    {}
};

struct AsmReg8 : public AsmReg
{
    explicit constexpr AsmReg8(int num) :
        AsmReg(num, 1)
    {}
};

struct AsmRegMem
{
    bool memory;
    std::optional<AsmReg> reg_opt;
    int32_t displacement;

    AsmRegMem(bool memory, std::optional<AsmReg> reg_opt, int32_t displacement = 0) :
        memory(memory), reg_opt(reg_opt), displacement(displacement)
    {}

    AsmRegMem(AsmReg reg) :
        memory(false), reg_opt({reg}), displacement(0)
    {}

    AsmRegMem(uint32_t addr) :
        memory(true), displacement(static_cast<int32_t>(addr))
    {}

    template<typename T>
    AsmRegMem(T* ptr) :
        memory(true), displacement(reinterpret_cast<int32_t>(ptr))
    {}
};

inline AsmRegMem operator*(AsmReg32 reg)
{
    return {true, {reg}, 0};
}

inline AsmRegMem operator*(std::pair<AsmReg32, int> p)
{
    return {true, {p.first}, p.second};
}

namespace asm_regs
{

namespace asm_reg_num
{

enum
{
    AX,
    CX,
    DX,
    BX,
    SP,
    BP,
    SI,
    DI,
};

}

constexpr AsmReg32 eax(asm_reg_num::AX), ecx(asm_reg_num::CX), edx(asm_reg_num::DX), ebx(asm_reg_num::BX);
constexpr AsmReg32 esp(asm_reg_num::SP), ebp(asm_reg_num::BP), esi(asm_reg_num::SI), edi(asm_reg_num::DI);
constexpr AsmReg16 ax(asm_reg_num::AX), cx(asm_reg_num::CX), dx(asm_reg_num::DX), bx(asm_reg_num::BX);
constexpr AsmReg16 sp(asm_reg_num::SP), bp(asm_reg_num::BP), si(asm_reg_num::SI), di(asm_reg_num::DI);
constexpr AsmReg8 al(asm_reg_num::AX), cl(asm_reg_num::CX), dl(asm_reg_num::DX), bl(asm_reg_num::BX);
constexpr AsmReg8 ah(asm_reg_num::SP), ch(asm_reg_num::BP), dh(asm_reg_num::SI), bh(asm_reg_num::DI);
} // namespace asm_regs

class AsmWriter
{
public:
    static constexpr uintptr_t unk_end_addr = UINTPTR_MAX;

    AsmWriter(uintptr_t begin_addr, uintptr_t end_addr = unk_end_addr) :
        m_addr(begin_addr), m_end_addr(end_addr)
    {}

    AsmWriter(void* begin_ptr, void* end_ptr = reinterpret_cast<void*>(unk_end_addr)) :
        m_addr(reinterpret_cast<uintptr_t>(begin_ptr)), m_end_addr(reinterpret_cast<uintptr_t>(end_ptr))
    {}

    ~AsmWriter()
    {
        if (m_end_addr != unk_end_addr) {
            assert(m_addr <= m_end_addr);
            while (m_addr < m_end_addr) nop();
        }
    }

    AsmWriter& xor_(const AsmReg32& dst_reg, const AsmRegMem& src_reg_mem)
    {
        write<u8>(0x33); // Opcode
        write_mod_rm(src_reg_mem, dst_reg);
        return *this;
    }

    AsmWriter& cmp(const AsmReg8& reg, int8_t imm)
    {
        if (reg == asm_regs::al) // al
            write<u8>(0x3C);
        else
            assert(false);
        write<i8>(imm);
        return *this;
    }

    AsmWriter& push(const AsmReg32& reg)
    {
        write<u8>(0x50 | reg.reg_num); // Opcode
        return *this;
    }

    AsmWriter& push(const AsmReg16& reg)
    {
        write<u8>(operand_size_override_prefix); // Prefix
        write<u8>(0x50 | reg.reg_num);
        return *this;
    }

    AsmWriter& pop(const AsmReg32& reg)
    {
        write<u8>(0x58 | reg.reg_num); // Opcode
        return *this;
    }

    AsmWriter& pop(const AsmReg16& reg)
    {
        write<u8>(operand_size_override_prefix); // Prefix
        write<u8>(0x58 | reg.reg_num);        // Opcode
        return *this;
    }

    AsmWriter& pusha()
    {
        write<u8>(0x60); // Opcode
        return *this;
    }

    AsmWriter& popa()
    {
        write<u8>(0x61); // Opcode
        return *this;
    }

    AsmWriter& push(int32_t imm)
    {
        if (abs(imm) < 128)
            return push(static_cast<i8>(imm));
        write<u8>(0x68); // Opcode
        write<i32>(static_cast<i32>(imm));
        return *this;
    }

    template<typename T>
    AsmWriter& push(const T* imm)
    {
        return push(reinterpret_cast<int32_t>(imm));
    }

    AsmWriter& push(int8_t imm)
    {
        write<u8>(0x6A); // Opcode
        write<i8>(static_cast<i8>(imm));
        return *this;
    }

    AsmWriter& add(const AsmRegMem& dst_rm, int32_t imm32)
    {
        if (abs(imm32) < 128)
            return add(dst_rm, static_cast<int8_t>(imm32));
        write<u8>(0x81); // Opcode
        write_mod_rm(dst_rm, 0);
        write<i32>(imm32);
        return *this;
    }

    AsmWriter& sub(const AsmRegMem& dst_rm, int32_t imm32)
    {
        if (abs(imm32) < 128)
            return sub(dst_rm, static_cast<int8_t>(imm32));
        write<u8>(0x81); // Opcode
        write_mod_rm(dst_rm, 5);
        write<i32>(imm32);
        return *this;
    }

    AsmWriter& add(const AsmRegMem& dst_rm, int8_t imm8)
    {
        write<u8>(0x83); // Opcode
        write_mod_rm(dst_rm, 0);
        write<i8>(imm8);
        return *this;
    }

    AsmWriter& sub(const AsmRegMem& dst_rm, int8_t imm8)
    {
        write<u8>(0x83); // Opcode
        write_mod_rm(dst_rm, 5);
        write<i8>(imm8);
        return *this;
    }

    AsmWriter& mov(const AsmRegMem& dst_rm, const AsmReg8& src_reg)
    {
        write<u8>(0x88); // Opcode
        write_mod_rm(dst_rm, src_reg);
        return *this;
    }

    AsmWriter& mov(const AsmRegMem& dst_rm, const AsmReg32& src_reg)
    {
        write<u8>(0x89); // Opcode
        write_mod_rm(dst_rm, src_reg);
        return *this;
    }

    AsmWriter& mov(const AsmReg32& dst_reg, const AsmReg32& src_reg)
    {
        // Fix ambiguous 'mov r, r' operation
        return mov(AsmRegMem(dst_reg), src_reg);
    }

    AsmWriter& mov(const AsmRegMem& dst_rm, const AsmReg16& src_reg)
    {
        write<u8>(operand_size_override_prefix); // Prefix
        write<u8>(0x89);                     // Opcode
        write_mod_rm(dst_rm, src_reg);
        return *this;
    }

    AsmWriter& mov(const AsmReg32& dst_reg, const AsmRegMem& src_rm)
    {
        write<u8>(0x8B); // Opcode
        write_mod_rm(src_rm, dst_reg);
        return *this;
    }

    AsmWriter& lea(const AsmReg32& dst_reg, const AsmRegMem& src_rm)
    {
        write<u8>(0x8D);
        write_mod_rm(src_rm, dst_reg);
        return *this;
    }

    AsmWriter& nop(int num = 1)
    {
        for (int i = 0; i < num; i++) {
            write<u8>(0x90);
        }
        return *this;
    }

    AsmWriter& pushf()
    {
        write<u8>(0x9C); // Opcode
        return *this;
    }

    AsmWriter& popf()
    {
        write<u8>(0x9D); // Opcode
        return *this;
    }

    AsmWriter& call_long(uint32_t addr)
    {
        auto jmp_addr = m_addr;
        write<u8>(0xE8);
        write<i32>(addr - (jmp_addr + 0x5));
        return *this;
    }

    AsmWriter& call(uint32_t addr)
    {
        return call_long(addr);
    }

    template<typename T>
    AsmWriter& call(T* addr)
    {
        return call_long(reinterpret_cast<uint32_t>(addr));
    }

    AsmWriter& jmp_long(uint32_t addr)
    {
        auto jmp_addr = m_addr;
        write<u8>(0xE9);
        write<i32>(addr - (jmp_addr + 0x5));
        return *this;
    }

    AsmWriter& jmp_short(uint32_t addr)
    {
        auto jmp_addr = m_addr;
        write<u8>(0xEB);
        write<i8>(addr - (jmp_addr + 0x2));
        return *this;
    }

    AsmWriter& jmp(uint32_t addr)
    {
        if (std::abs(static_cast<int>(addr - (m_addr + 0x2))) < 127)
            return jmp_short(addr);
        else
            return jmp_long(addr);
    }

    template<typename T>
    AsmWriter& jmp(T* addr)
    {
        return jmp(reinterpret_cast<uintptr_t>(addr));
    }

    AsmWriter& mov(const AsmReg8& dst_reg, int8_t imm)
    {
        write<u8>(0xB0 | dst_reg.reg_num); // Opcode
        write<i8>(imm);
        return *this;
    }

    AsmWriter& mov(const AsmReg32& dst_reg, int32_t imm)
    {
        write<u8>(0xB8 | dst_reg.reg_num); // Opcode
        write<i32>(imm);
        return *this;
    }

    template<typename T>
    AsmWriter& mov(const AsmReg32& dst_reg, T* imm)
    {
        return mov(dst_reg, reinterpret_cast<int32_t>(imm));
    }

    AsmWriter& mov(const AsmReg16& dst_reg, int16_t imm)
    {
        write<u8>(operand_size_override_prefix); // Prefix
        write<u8>(0xB8 | dst_reg.reg_num);     // Opcode
        write<i16>(imm);
        return *this;
    }

    AsmWriter& ret(uint16_t imm)
    {
        write<u8>(0xC2);
        write<u16>(imm);
        return *this;
    }

    AsmWriter& ret()
    {
        write<u8>(0xC3);
        return *this;
    }

    template<typename T>
    AsmWriter& fstp(const AsmRegMem& dst_rm);

private:
    uintptr_t m_addr, m_end_addr;

    static constexpr u8 operand_size_override_prefix = 0x66;

    void write_mod_rm(const AsmRegMem& rm, uint8_t reg_field)
    {
        uint8_t mod_field = 0;
        if (!rm.memory)
            mod_field = 3;
        else if (rm.displacement == 0 || !rm.reg_opt)
            mod_field = 0;
        else if (abs(rm.displacement) < 128)
            mod_field = 1;
        else
            mod_field = 2;

        uint8_t rm_field = rm.reg_opt ? rm.reg_opt.value().reg_num : 5;
        uint8_t mod_reg_rm_byte = (mod_field << 6) | (reg_field << 3) | rm_field;
        write<u8>(mod_reg_rm_byte); // 0xD

        // TODO: full SIB support?
        if (rm_field == 4 && mod_field != 3) {
            uint8_t scale_field = 0;
            uint8_t index_field = 4; // no index
            uint8_t base_field = 4;  // esp
            uint8_t sib_byte = (scale_field << 6) | (index_field << 3) | base_field;
            write<u8>(sib_byte);
        }

        if (mod_field == 1)
            write<i8>(rm.displacement);
        else if (mod_field == 2 || !rm.reg_opt) {
            write<i32>(rm.displacement);
        }
    }

    void write_mod_rm(const AsmRegMem& rm, const AsmReg& reg)
    {
        write_mod_rm(rm, reg.reg_num);
    }

    template<typename T>
    void write(typename TypeIdentity<T>::type value)
    {
        write_mem<T>(m_addr, value);
        m_addr += sizeof(value);
    }

    template<typename T>
    static bool can_imm_fit_in_one_byte(T imm)
    {
        static_assert(sizeof(T) <= 4);
        return abs(static_cast<int32_t>(imm)) < 128;
    }
};

template<>
inline AsmWriter& AsmWriter::fstp<double>(const AsmRegMem& dst_rm)
{
    write<u8>(0xDD);
    write_mod_rm(dst_rm, 3);
    return *this;
}
