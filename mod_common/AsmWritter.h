#pragma once

#include "MemUtils.h"
#include "AsmOpcodes.h"
#include <cassert>

#define ASM_PUSH_EAX 0x50
#define ASM_PUSH_EBP 0x55

enum AsmReg : int
{
    EAX, EBX, ECX, EDX, EDI, ESI, ESP,
};

class AsmWritter
{
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

    AsmWritter &nop()
    {
        WriteMemUInt8(m_addr++, ASM_NOP);
        return *this;
    }

    AsmWritter &jmpLong(uint32_t addr)
    {
        WriteMemUInt8(m_addr, ASM_LONG_JMP_REL);
        WriteMemInt32(m_addr + 1, addr - (m_addr + 0x5));
        m_addr += 5;
        return *this;
    }

    AsmWritter &jmpNear(uint32_t addr)
    {
        WriteMemUInt8(m_addr, ASM_SHORT_JMP_REL);
        WriteMemInt8(m_addr + 1, addr - (m_addr + 0x2));
        m_addr += 2;
        return *this;
    }

    AsmWritter &callLong(uint32_t addr)
    {
        WriteMemUInt8(m_addr, ASM_LONG_CALL_REL);
        WriteMemInt32(m_addr + 1, addr - (m_addr + 0x5));
        m_addr += 5;
        return *this;
    }

    AsmWritter &callLong(void *addr)
    {
        WriteMemUInt8(m_addr, ASM_LONG_CALL_REL);
        WriteMemInt32(m_addr + 1, reinterpret_cast<intptr_t>(addr) - (m_addr + 0x5));
        m_addr += 5;
        return *this;
    }

    AsmWritter &pushEax()
    {
        WriteMemUInt8(m_addr++, ASM_PUSH_EAX);
        return *this;
    }

    AsmWritter &pushEbx()
    {
        WriteMemUInt8(m_addr++, ASM_PUSH_EBX);
        return *this;
    }

    AsmWritter &pushEdx()
    {
        WriteMemUInt8(m_addr++, 0x52);
        return *this;
    }

    AsmWritter &pushEbp()
    {
        WriteMemUInt8(m_addr++, ASM_PUSH_EBP);
        return *this;
    }

    AsmWritter &pushEdi()
    {
        WriteMemUInt8(m_addr++, ASM_PUSH_EDI);
        return *this;
    }

    AsmWritter &pushEsi()
    {
        WriteMemUInt8(m_addr++, ASM_PUSH_ESI);
        return *this;
    }

    AsmWritter &pushEsi(int8_t val)
    {
        WriteMemUInt8(m_addr++, ASM_PUSH_BYTE);
        WriteMemInt8(m_addr++, val);
        return *this;
    }

    AsmWritter &mov(AsmReg dst, AsmReg src)
    {
        if (dst == AsmReg::EDI && src == AsmReg::EAX)
        {
            WriteMemUInt8(m_addr++, 0x89);
            WriteMemUInt8(m_addr++, 0xC7);
        }
        else if (dst == AsmReg::EBX && src == AsmReg::EAX)
        {
            WriteMemUInt8(m_addr++, 0x89);
            WriteMemUInt8(m_addr++, 0xC3);
        }
        else
            assert(false);
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

    AsmWritter &cmpRegImm8(AsmReg reg, int8_t imm)
    {
        if (reg == AsmReg::EAX) // al
            WriteMemUInt8(m_addr++, 0x3C);
        else
            assert(false);
        WriteMemInt8(m_addr++, imm);
        return *this;
    }

    

private:
    unsigned m_addr, m_endAddr;
};