#pragma once

#include "MemUtils.h"
#include "AsmOpcodes.h"
#include "ShortTypes.h"
#include <cassert>
#include <optional>

class AsmReg
{
public:
    int regNum;
    int size;

    constexpr explicit AsmReg(int regNum, int size) :
        regNum(regNum), size(size) {}

    constexpr bool operator==(const AsmReg &other) const {
        return regNum == other.regNum && size == other.size;
    }
};

struct AsmReg32 : public AsmReg
{
    constexpr AsmReg32(int num) : AsmReg(num, 4) {}

    std::pair<AsmReg32, int> operator+(int n) const {
        return {*this, n};
    }
};

struct AsmReg16 : public AsmReg
{
    constexpr AsmReg16(int num) : AsmReg(num, 2) {}
};

struct AsmReg8 : public AsmReg
{
    constexpr AsmReg8(int num) : AsmReg(num, 1) {}
};

struct AsmMem
{
    std::optional<AsmReg32> regOpt;
    int32_t displacement;

    AsmMem(AsmReg32 reg, int32_t displacement = 0) :
        regOpt(reg), displacement(displacement)
    {}

    AsmMem(int32_t displacement) :
        displacement(displacement)
    {}

    AsmMem(std::pair<AsmReg32, int> pair) :
        regOpt(pair.first), displacement(pair.second)
    {}
};

struct AsmRegMem
{
    bool memory;
    std::optional<AsmReg> regOpt;
    int32_t displacement;

    AsmRegMem(bool memory, std::optional<AsmReg> regOpt, int32_t displacement = 0) :
        memory(memory), regOpt(regOpt), displacement(displacement)
    {}

    AsmRegMem(AsmReg reg) :
        memory(false), regOpt({reg}), displacement(0)
    {}

    AsmRegMem(AsmMem mem) :
        memory(true), regOpt(mem.regOpt), displacement(mem.displacement)
    {}
};

namespace asm_regs
{
    enum GenPurpRegNum
    {
        GPRN_AX, GPRN_CX, GPRN_DX, GPRN_BX,
        GPRN_SP, GPRN_BP, GPRN_SI, GPRN_DI,
    };

    constexpr AsmReg32 eax(GPRN_AX), ecx(GPRN_CX), edx(GPRN_DX), ebx(GPRN_BX);
    constexpr AsmReg32 esp(GPRN_SP), ebp(GPRN_BP), esi(GPRN_SI), edi(GPRN_DI);
    constexpr AsmReg16 ax(GPRN_AX), cx(GPRN_CX), dx(GPRN_DX), bx(GPRN_BX);
    constexpr AsmReg16 sp(GPRN_SP), bp(GPRN_BP), si(GPRN_SI), di(GPRN_DI);
    constexpr AsmReg8 al(GPRN_AX), cl(GPRN_CX), dl(GPRN_DX), bl(GPRN_BX);
    constexpr AsmReg8 ah(GPRN_SP), ch(GPRN_BP), dh(GPRN_SI), bh(GPRN_DI);
}

class AsmWritter
{
private:
    enum AsmPrefix
    {
        PR_OPERAND_SIZE_OVERRIDE = 0x66,
    };

public:
    static const unsigned UNK_END_ADDR = 0xFFFFFFFF;

    AsmWritter(unsigned beginAddr, unsigned endAddr = UNK_END_ADDR) :
        m_addr(beginAddr), m_endAddr(endAddr) {}

    ~AsmWritter()
    {
        if (m_endAddr != UNK_END_ADDR)
        {
            assert(m_addr <= m_endAddr);
            while (m_addr < m_endAddr)
                nop();
        }
    }

    AsmWritter &xor_(const AsmReg32 &dst_reg, const AsmRegMem &src_reg_mem)
    {
        WriteMem<u8>(m_addr++, 0x33); // Opcode
        writeModRm(src_reg_mem, dst_reg);
        return *this;
    }

    AsmWritter &cmp(const AsmReg8 &reg, int8_t imm)
    {
        if (reg == asm_regs::al) // al
            WriteMem<u8>(m_addr++, 0x3C);
        else
            assert(false);
        WriteMem<i8>(m_addr++, imm);
        return *this;
    }

    AsmWritter &push(const AsmReg32 &reg)
    {
        WriteMem<u8>(m_addr++, 0x50 | reg.regNum); // Opcode
        return *this;
    }

    AsmWritter &push(const AsmReg16 &reg)
    {
        WriteMem<u8>(m_addr++, PR_OPERAND_SIZE_OVERRIDE); // Prefix
        WriteMem<u8>(m_addr++, 0x50 | reg.regNum);
        return *this;
    }

    AsmWritter &pop(const AsmReg32 &reg)
    {
        WriteMem<u8>(m_addr++, 0x58 | reg.regNum); // Opcode
        return *this;
    }

    AsmWritter &pop(const AsmReg16 &reg)
    {
        WriteMem<u8>(m_addr++, PR_OPERAND_SIZE_OVERRIDE); // Prefix
        WriteMem<u8>(m_addr++, 0x58 | reg.regNum); // Opcode
        return *this;
    }

    AsmWritter &pusha()
    {
        WriteMem<u8>(m_addr++, 0x60); // Opcode
        return *this;
    }

    AsmWritter &popa()
    {
        WriteMem<u8>(m_addr++, 0x61); // Opcode
        return *this;
    }

    AsmWritter &push(int32_t Val)
    {
        WriteMem<u8>(m_addr++, 0x68); // Opcode
        WriteMem<i32>(m_addr, Val);
        m_addr += 4;
        return *this;
    }

    AsmWritter &add(const AsmRegMem &dstRM, int32_t imm32)
    {
        if (abs(imm32) < 128)
            return add(dstRM, static_cast<int8_t>(imm32));
        WriteMem<u8>(m_addr++, 0x81); // Opcode
        writeModRm(dstRM, 0);
        WriteMem<i32>(m_addr, imm32);
        m_addr += 4;
        return *this;
    }

    AsmWritter &sub(const AsmRegMem &dstRM, int32_t imm32)
    {
        if (abs(imm32) < 128)
            return sub(dstRM, static_cast<int8_t>(imm32));
        WriteMem<u8>(m_addr++, 0x81); // Opcode
        writeModRm(dstRM, 5);
        WriteMem<i32>(m_addr, imm32);
        m_addr += 4;
        return *this;
    }

    AsmWritter &add(const AsmRegMem &dstRM, int8_t imm8)
    {
        WriteMem<u8>(m_addr++, 0x83); // Opcode
        writeModRm(dstRM, 0);
        WriteMem<i8>(m_addr++, imm8);
        return *this;
    }

    AsmWritter &sub(const AsmRegMem &dstRM, int8_t imm8)
    {
        WriteMem<u8>(m_addr++, 0x83); // Opcode
        writeModRm(dstRM, 5);
        WriteMem<i8>(m_addr++, imm8);
        return *this;
    }

    AsmWritter &mov(const AsmRegMem &dstRM, const AsmReg8 &srcReg)
    {
        WriteMem<u8>(m_addr++, 0x88); // Opcode
        writeModRm(dstRM, srcReg);
        return *this;
    }

    AsmWritter &mov(const AsmRegMem &dstRM, const AsmReg32 &srcReg)
    {
        WriteMem<u8>(m_addr++, 0x89); // Opcode
        writeModRm(dstRM, srcReg);
        return *this;
    }

    AsmWritter &mov(const AsmReg32 &dstReg, const AsmReg32 &srcReg)
    {
        // Fix ambiguous 'mov r, r' operation
        return mov(AsmRegMem(dstReg), srcReg);
    }

    AsmWritter &mov(const AsmRegMem &dstRM, const AsmReg16 &srcReg)
    {
        WriteMem<u8>(m_addr++, PR_OPERAND_SIZE_OVERRIDE); // Prefix
        WriteMem<u8>(m_addr++, 0x89); // Opcode
        writeModRm(dstRM, srcReg);
        return *this;
    }

    AsmWritter &mov(const AsmReg32 &dstReg, const AsmRegMem &srcRM)
    {
        WriteMem<u8>(m_addr++, 0x8B); // Opcode
        writeModRm(srcRM, dstReg);
        return *this;
    }

    AsmWritter &lea(const AsmReg32 &dstReg, const AsmRegMem &srcRM)
    {
        WriteMem<u8>(m_addr++, 0x8D);
        writeModRm(srcRM, dstReg);
        return *this;
    }

    AsmWritter &nop(int num = 1)
    {
        for (int i = 0; i < num; i++) {
            WriteMem<u8>(m_addr++, 0x90);
        }
        return *this;
    }

    AsmWritter &pushf()
    {
        WriteMem<u8>(m_addr++, 0x9C); // Opcode
        return *this;
    }

    AsmWritter &popf()
    {
        WriteMem<u8>(m_addr++, 0x9D); // Opcode
        return *this;
    }

    AsmWritter &callLong(uint32_t addr)
    {
        WriteMem<u8>(m_addr, 0xE8);
        WriteMem<i32>(m_addr + 1, addr - (m_addr + 0x5));
        m_addr += 5;
        return *this;
    }

    AsmWritter &call(uint32_t addr)
    {
        callLong(addr);
    }

    template<typename T>
    AsmWritter &call(T *addr)
    {
        return callLong(reinterpret_cast<uint32_t>(addr));
    }

    AsmWritter &jmpLong(uint32_t addr)
    {
        WriteMem<u8>(m_addr, 0xE9);
        WriteMem<i32>(m_addr + 1, addr - (m_addr + 0x5));
        m_addr += 5;
        return *this;
    }

    AsmWritter &jmpShort(uint32_t addr)
    {
        WriteMem<u8>(m_addr, 0xEB);
        WriteMem<i8>(m_addr + 1, addr - (m_addr + 0x2));
        m_addr += 2;
        return *this;
    }

    AsmWritter &jmp(uint32_t addr)
    {
        if (std::abs(static_cast<int>(addr - (m_addr + 0x2))) < 127)
            return jmpShort(addr);
        else
            return jmpLong(addr);
    }

    template<typename T>
    AsmWritter &jmp(T *addr)
    {
        return jmp(reinterpret_cast<uintptr_t>(addr));
    }

    AsmWritter &mov(const AsmReg8 &dstReg, int8_t imm)
    {
        WriteMem<u8>(m_addr++, 0xB0 | dstReg.regNum); // Opcode
        WriteMem<i8>(m_addr++, imm);
        return *this;
    }

    AsmWritter &mov(const AsmReg32 &dstReg, int32_t imm)
    {
        WriteMem<u8>(m_addr++, 0xB8 | dstReg.regNum); // Opcode
        WriteMem<i32>(m_addr, imm);
        m_addr += 4;
        return *this;
    }

    AsmWritter &mov(const AsmReg16 &dstReg, int16_t imm)
    {
        WriteMem<u8>(m_addr++, PR_OPERAND_SIZE_OVERRIDE); // Prefix
        WriteMem<u8>(m_addr++, 0xB8 | dstReg.regNum); // Opcode
        WriteMem<i16>(m_addr, imm);
        m_addr += 2;
        return *this;
    }

    AsmWritter &ret()
    {
        WriteMem<u8>(m_addr++, 0xC3);
        return *this;
    }

    template<typename T>
    AsmWritter &fstp(const AsmRegMem &regMem);

private:
    unsigned m_addr, m_endAddr;

    void writeModRm(const AsmRegMem &regMem, uint8_t regField)
    {
        uint8_t modField = 0;
        if (!regMem.memory)
            modField = 3;
        else if (regMem.displacement == 0 || !regMem.regOpt)
            modField = 0;
        else if (abs(regMem.displacement) < 128)
            modField = 1;
        else
            modField = 2;

        uint8_t rmField = regMem.regOpt ? regMem.regOpt.value().regNum : 5;
        uint8_t modRegRmByte = (modField << 6) | (regField << 3) | rmField;
        WriteMem<u8>(m_addr++, modRegRmByte);

        // TODO: full SIB support?
        if (rmField == 4 && modField != 3)
        {
            uint8_t scaleField = 0;
            uint8_t indexField = 4; // no index
            uint8_t baseField = 4; // esp
            uint8_t sibByte = (scaleField << 6) | (indexField << 3) | baseField;
            WriteMem<u8>(m_addr++, sibByte);
        }

        if (modField == 1)
            WriteMem<i8>(m_addr++, regMem.displacement);
        else if (modField == 2 || !regMem.regOpt)
            WriteMem<i32>(m_addr++, regMem.displacement);
    }

    void writeModRm(const AsmRegMem &regMem, const AsmReg &reg)
    {
        writeModRm(regMem, reg.regNum);
    }
};

template<>
inline AsmWritter &AsmWritter::fstp<double>(const AsmRegMem &regMem)
{
    WriteMem<i8>(m_addr++, 0xDD);
    writeModRm(regMem, 3);
    return *this;
}
