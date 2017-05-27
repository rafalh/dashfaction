#pragma once

#include "MemUtils.h"
#include "AsmOpcodes.h"
#include <cassert>

class AsmReg
{
public:
    int regNum;
    int size;

    constexpr explicit AsmReg(int regNum, int size) : regNum(regNum), size(size) {}
    constexpr bool operator==(const AsmReg &other) const { return regNum == other.regNum && size == other.size; }
};

struct AsmReg32 : public AsmReg
{
    constexpr AsmReg32(int num) : AsmReg(num, 4) {}
};

struct AsmReg16 : public AsmReg
{
    constexpr AsmReg16(int num) : AsmReg(num, 2) {}
};

struct AsmReg8 : public AsmReg
{
    constexpr AsmReg8(int num) : AsmReg(num, 1) {}
};

namespace AsmRegs
{
    enum GenPurpRegNum
    {
        GPRN_AX, GPRN_CX, GPRN_DX, GPRN_BX,
        GPRN_SP, GPRN_BP, GPRN_SI, GPRN_DI,
    };

    constexpr AsmReg32 EAX(GPRN_AX), ECX(GPRN_CX), EDX(GPRN_DX), EBX(GPRN_BX);
    constexpr AsmReg32 ESP(GPRN_SP), EBP(GPRN_BP), ESI(GPRN_SI), EDI(GPRN_DI);
    constexpr AsmReg16 AX(GPRN_AX), CX(GPRN_CX), DX(GPRN_DX), BX(GPRN_BX);
    constexpr AsmReg16 SP(GPRN_SP), BP(GPRN_BP), SI(GPRN_SI), DI(GPRN_DI);
    constexpr AsmReg8 AL(GPRN_AX), CL(GPRN_CX), DL(GPRN_DX), BL(GPRN_BX);
    constexpr AsmReg8 AH(GPRN_SP), CH(GPRN_BP), DH(GPRN_SI), BH(GPRN_DI);
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

    AsmWritter &push(const AsmReg32 &reg)
    {
        WriteMemUInt8(m_addr++, 0x50 | reg.regNum); // Opcode
        return *this;
    }

    AsmWritter &push(const AsmReg16 &reg)
    {
        WriteMemUInt8(m_addr++, PR_OPERAND_SIZE_OVERRIDE); // Prefix
        WriteMemUInt8(m_addr++, 0x50 | reg.regNum);
        return *this;
    }

    AsmWritter &xor(const AsmReg32 &dstReg, const AsmReg32 &srcReg)
    {
        WriteMemUInt8(m_addr++, 0x33); // Opcode
        WriteMemUInt8(m_addr++, 0xC0 | (srcReg.regNum << 3) | dstReg.regNum); // ModR/M
        return *this;
    }

    AsmWritter &nop()
    {
        WriteMemUInt8(m_addr++, 0x90);
        return *this;
    }

    AsmWritter &jmpLong(uint32_t addr)
    {
        WriteMemUInt8(m_addr, 0xE9);
        WriteMemInt32(m_addr + 1, addr - (m_addr + 0x5));
        m_addr += 5;
        return *this;
    }

    AsmWritter &jmpLong(void *addr)
    {
        return jmpLong(reinterpret_cast<uintptr_t>(addr));
    }

    AsmWritter &jmpNear(uint32_t addr)
    {
        WriteMemUInt8(m_addr, 0xEB);
        WriteMemInt8(m_addr + 1, addr - (m_addr + 0x2));
        m_addr += 2;
        return *this;
    }

    AsmWritter &callLong(uint32_t addr)
    {
        WriteMemUInt8(m_addr, 0xE8);
        WriteMemInt32(m_addr + 1, addr - (m_addr + 0x5));
        m_addr += 5;
        return *this;
    }

    AsmWritter &callLong(void *addr)
    {
        WriteMemUInt8(m_addr, 0xE8);
        WriteMemInt32(m_addr + 1, reinterpret_cast<intptr_t>(addr) - (m_addr + 0x5));
        m_addr += 5;
        return *this;
    }

    AsmWritter &ret()
    {
        WriteMemUInt8(m_addr++, 0xC3);
        return *this;
    }

    AsmWritter &mov(const AsmReg32 &dstReg, const AsmReg32 &srcReg)
    {
        WriteMemUInt8(m_addr++, 0x89); // Opcode
        WriteMemUInt8(m_addr++, 0xC0 | (srcReg.regNum << 3) | dstReg.regNum); // ModR/M
        return *this;
    }

    AsmWritter &mov(const AsmReg16 &dstReg, const AsmReg16 &srcReg)
    {
        WriteMemUInt8(m_addr++, PR_OPERAND_SIZE_OVERRIDE); // Prefix
        WriteMemUInt8(m_addr++, 0x89); // Opcode
        WriteMemUInt8(m_addr++, 0xC0 | (srcReg.regNum << 3) | dstReg.regNum); // ModR/M
        return *this;
    }

    AsmWritter &mov(const AsmReg8 &dstReg, const AsmReg8 &srcReg)
    {
        WriteMemUInt8(m_addr++, 0x88); // Opcode
        WriteMemUInt8(m_addr++, 0xC0 | (srcReg.regNum << 3) | dstReg.regNum); // ModR/M
        return *this;
    }

    AsmWritter &mov(const AsmReg32 &dstReg, int32_t imm)
    {
        WriteMemUInt8(m_addr++, 0xB8 | dstReg.regNum); // Opcode
        WriteMemInt32(m_addr, imm);
        m_addr += 4;
        return *this;
    }

    AsmWritter &mov(const AsmReg16 &dstReg, int16_t imm)
    {
        WriteMemUInt8(m_addr++, PR_OPERAND_SIZE_OVERRIDE); // Prefix
        WriteMemUInt8(m_addr++, 0xB8 | dstReg.regNum); // Opcode
        WriteMemInt16(m_addr, imm);
        m_addr += 2;
        return *this;
    }

    AsmWritter &mov(const AsmReg8 &dstReg, int8_t imm)
    {
        WriteMemUInt8(m_addr++, 0xB0 | dstReg.regNum); // Opcode
        WriteMemInt8(m_addr++, imm);
        return *this;
    }

    AsmWritter &movEaxMemEsp(int8_t off)
    {
        WriteMemUInt8(m_addr++, 0x88);
        WriteMemUInt8(m_addr++, 0x4C);
        WriteMemUInt8(m_addr++, 0x24);
        WriteMemInt8(m_addr++, off);
        return *this;
    }

    AsmWritter &movEaxMemEsp32(int32_t off)
    {
        WriteMemUInt8(m_addr++, 0x8B);
        WriteMemUInt8(m_addr++, 0x84);
        WriteMemUInt8(m_addr++, 0x24);
        WriteMemInt32(m_addr++, off);
        return *this;
    }

    AsmWritter &addEsp(int8_t val)
    {
        WriteMemUInt8(m_addr++, 0x83);
        WriteMemUInt8(m_addr++, 0xC4);
        WriteMemInt8(m_addr++, val);
        return *this;
    }

    AsmWritter &leaEaxEsp(int8_t addVal)
    {
        WriteMemUInt8(m_addr++, 0x8D);
        WriteMemUInt8(m_addr++, 0x44);
        WriteMemUInt8(m_addr++, 0x24);
        WriteMemInt8(m_addr++, addVal);
        return *this;
    }

    AsmWritter &leaEdxEsp(int8_t addVal)
    {
        WriteMemUInt8(m_addr++, 0x8D);
        WriteMemUInt8(m_addr++, 0x54);
        WriteMemUInt8(m_addr++, 0x24);
        WriteMemInt8(m_addr++, addVal);
        return *this;
    }

    AsmWritter &cmp(const AsmReg8 &reg, int8_t imm)
    {
        if (reg == AsmRegs::AL) // al
            WriteMemUInt8(m_addr++, 0x3C);
        else
            assert(false);
        WriteMemInt8(m_addr++, imm);
        return *this;
    }

private:
    unsigned m_addr, m_endAddr;
};
