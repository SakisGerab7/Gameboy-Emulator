#pragma once

#include "Common.hpp"

class CPU {
public:
    enum Interrupt {
        Vblank  = 0b00000001,
        LcdStat = 0b00000010,
        Timer   = 0b00000100,
        Serial  = 0b00001000,
        Joypad  = 0b00010000,
    };

public:
    u8 Step();

    u8 GetIF() const { return m_IF; }
    u8 GetIE() const { return m_IE; }

    void SetIF(u8 val) { m_IF = val; }
    void SetIE(u8 val) { m_IE = val; }

    void RequestInterrupt(Interrupt in) { m_IF |= in; }

private:
    u8 GetCycles(u8 opcode);
    bool HandleInterrupts();
    void PrintInstruction(u8 opcode);

    void Execute(u8 opcode);

    void ExecuteBlock0(u8 opcode);
    void ExecuteBlock1(u8 opcode);
    void ExecuteBlock2(u8 opcode);
    void ExecuteBlock3(u8 opcode);

    void ExecuteCB();

    enum FlagBit { C = 4, H = 5, N = 6, Z = 7 };

    u8 GetFlagC() const { return BIT(m_Reg.F, FlagBit::C); }
    u8 GetFlagH() const { return BIT(m_Reg.F, FlagBit::H); }
    u8 GetFlagN() const { return BIT(m_Reg.F, FlagBit::N); }
    u8 GetFlagZ() const { return BIT(m_Reg.F, FlagBit::Z); }

    void SetFlagC(u8 val) { SET_BIT(m_Reg.F, FlagBit::C, val); }
    void SetFlagH(u8 val) { SET_BIT(m_Reg.F, FlagBit::H, val); }
    void SetFlagN(u8 val) { SET_BIT(m_Reg.F, FlagBit::N, val); }
    void SetFlagZ(u8 val) { SET_BIT(m_Reg.F, FlagBit::Z, val); }

    u8 ReadMem(u16 addr) const;
    u16 ReadMem16(u16 addr) const;

    void WriteMem(u16 addr, u8 val);
    void WriteMem16(u16 addr, u16 val);

    u8 GetR8(u8 idx) const;
    void SetR8(u8 idx, u8 val);

    u16 GetR16(u8 idx) const;
    void SetR16(u8 idx, u16 val);

    u8 GetR16Mem(u8 idx);
    void SetR16Mem(u8 idx, u8 val);

    u8 GetImm8();
    u16 GetImm16();

    bool CheckCondition(u8 cond) const;

    void Inc(u8 operand);
    void Dec(u8 operand);

    void Add(u8 val);
    void Adc(u8 val);
    void Sub(u8 val);
    void Sbc(u8 val);
    void And(u8 val);
    void Or(u8 val);
    void Xor(u8 val);
    void Cp(u8 val);

    void Daa();

    void Push(u8 src);
    void Pop(u8 dest);

    void Rst(u8 opcode);
    void Call();
    void Ret();

    void Rlc(u8 operand);
    void Rl(u8 operand);
    void Rrc(u8 operand);
    void Rr(u8 operand);
    void Sla(u8 operand);
    void Sra(u8 operand);
    void Swap(u8 operand);
    void Srl(u8 operand);

    void Bit(u8 operand, u8 bit);
    void Res(u8 operand, u8 bit);
    void Set(u8 operand, u8 bit);

    u16 AddSPImm8();

private:
    static const u8 s_CyclesNormal[0x100];
    static const u8 s_CyclesJumped[0x100];
    static const u8 s_CyclesCB[0x100];
    static const char *s_OpcodeNames[0x100];

private:
    struct Registers {
        union {
            struct {
                u16 AF;
                u16 BC;
                u16 DE;
                u16 HL;
            };
            struct {
                u8 F, A;
                u8 C, B;
                u8 E, D;
                u8 L, H;
            };
        };
        u16 SP;
        u16 PC;

        Registers()
            : AF(0x01B0),
              BC(0x0013),
              DE(0x00D8),
              HL(0x014D),
              SP(0xFFFE),
              PC(0x0100)
            {}
    };

    Registers m_Reg;

    bool m_IME = false;
    u8 m_IF = 0;
    u8 m_IE = 0;

    bool m_Halted = false;
    bool m_Jumped = false;
    bool m_IsCB = false;
};