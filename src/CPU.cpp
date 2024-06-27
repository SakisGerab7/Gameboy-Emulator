#include "CPU.hpp"
#include "Gameboy.hpp"
#include "Log.hpp"

u8 CPU::Step() {
    u8 opcode = ReadMem(m_Reg.PC);

    if (HandleInterrupts()) return 12;

    if (m_Halted) return 4;

    // PrintInstruction(opcode);

    m_Reg.PC++;
    m_Jumped = false;
    m_IsCB = false;

    Execute(opcode);

    return GetCycles(opcode);
}

u8 CPU::GetCycles(u8 opcode) {
    if (m_IsCB) {
        return 4 * s_CyclesCB[opcode];
    } else if (m_Jumped) {
        return 4 * s_CyclesJumped[opcode];
    } else {
        return 4 * s_CyclesNormal[opcode];
    }
}

bool CPU::HandleInterrupts() {
    if (!m_IME) return false;

    u8 enabledInterrupts = (m_IE & m_IF & 0x1F);
    if (enabledInterrupts == 0) return false;

    if (m_Halted) {
        m_Halted = false;
    }

    static const u16 intAddrs[] = { 0x40, 0x48, 0x50, 0x58, 0x60 };
    for (u8 i = 0; i < 5; i++) {
        if (BIT(enabledInterrupts, i)) {
            m_Reg.SP -= 2;
            WriteMem16(m_Reg.SP, m_Reg.PC);
            m_Reg.PC = intAddrs[i];

            SET_BIT(m_IF, i, 0);
            m_Halted = false;
            m_IME = false;
            return true;
        }
    }

    return true;
}

void CPU::PrintInstruction(u8 opcode) {
    Log::Info("[0x%04X] ", m_Reg.PC);
    Log::Info("%6s (0x%02X, 0x%04X) ", s_OpcodeNames[opcode], opcode, ReadMem16(m_Reg.PC + 1));
    Log::Info("AF = 0x%04X, ", m_Reg.AF);
    Log::Info("BC = 0x%04X, ", m_Reg.BC);
    Log::Info("DE = 0x%04X, ", m_Reg.DE);
    Log::Info("HL = 0x%04X, ", m_Reg.HL);
    Log::Info("SP = 0x%04X\n", m_Reg.SP);
}

void CPU::Execute(u8 opcode) {
    u8 block = (opcode & 0b11000000) >> 6;
    switch (block) {
        case 0: ExecuteBlock0(opcode); break;
        case 1: ExecuteBlock1(opcode); break;
        case 2: ExecuteBlock2(opcode); break;
        case 3: ExecuteBlock3(opcode); break;
        default: break;
    }
}

void CPU::ExecuteBlock0(u8 opcode) {
    if (opcode == 0x00 || opcode == 0x10) return; // NOP, STOP

    u8 row = (opcode & 0b00110000) >> 4;
    u8 col = (opcode & 0b00001111);
    switch (col) {
        case 0b0001: {
            u8 dest = row;
            SetR16(dest, GetImm16());
            return;
        }
        case 0b0010: {
            u8 dest = row;
            SetR16Mem(dest, m_Reg.A);
            return;
        }
        case 0b1010: {
            u8 src = row;
            m_Reg.A = GetR16Mem(src);
            return;
        }
        case 0b1000: {
            if (row == 0b00) {
                WriteMem(GetImm16(), m_Reg.SP);
                return;
            }
            break;
        }
        case 0b0011: {
            u8 operand = row;
            SetR16(operand, GetR16(operand) + 1);
            return;
        }
        case 0b1011: {
            u8 operand = row;
            SetR16(operand, GetR16(operand) - 1);
            return;
        }
        case 0b1001: {
            u8 operand = row;
            u16 val = GetR16(operand);
            u32 res = m_Reg.HL + val;
            SetFlagN(false);
            SetFlagH((m_Reg.HL & 0xFFF) + (val & 0xFFF) > 0xFFF);
            SetFlagC(res > 0xFFFF);
            m_Reg.HL = res;
            return;
        }
    }

    row = (opcode & 0b00111000) >> 3;
    col = (opcode & 0b00000111);
    switch (col) {
        case 0b100: {
            u8 operand = row;
            Inc(operand);
            return;
        }
        case 0b101: {
            u8 operand = row;
            Dec(operand);
            return;
        }
        case 0b110: {
            u8 dest = row;
            SetR8(dest, GetImm8());
            return;
        }
        case 0b111: {
            switch (row) {
                case 0b000: { // RLCA
                    Rlc(7);
                    SetFlagZ(false);
                    return;
                }
                case 0b001: { // RRCA
                    Rrc(7);
                    SetFlagZ(false);
                    return;
                }
                case 0b010: { // RLA
                    Rl(7);
                    SetFlagZ(false);
                    return;
                }
                case 0b011: { // RRA
                    Rr(7);
                    SetFlagZ(false);
                    return;
                }
                case 0b100: { // DAA
                    Daa();
                    return;
                }
                case 0b101: { // CPL
                    m_Reg.A = ~m_Reg.A;
                    SetFlagN(true);
                    SetFlagH(true);
                    return;
                }
                case 0b110: { // SCF
                    SetFlagN(false);
                    SetFlagH(false);
                    SetFlagC(true);
                    return;
                }
                case 0b111: { // CCF
                    SetFlagN(false);
                    SetFlagH(false);
                    SetFlagC(!GetFlagC());
                    return;
                }
            }
        }
        case 0b000: { // JR
            u8 cond = row;
            if (cond == 0b011 || CheckCondition(cond & 0b011)) {
                m_Reg.PC += static_cast<i8>(ReadMem(m_Reg.PC)) + 1;
                m_Jumped = true;
            } else {
                m_Reg.PC++;
            }
            return;
        }
    }
}

void CPU::ExecuteBlock1(u8 opcode) {
    if (opcode == 0x76) {
        m_Halted = true;
        return;
    }

    u8 src  = (opcode & 0b00000111);
    u8 dest = (opcode & 0b00111000) >> 3;
    SetR8(dest, GetR8(src));
}

void CPU::ExecuteBlock2(u8 opcode) {
    u8 operand = (opcode & 0b00000111);
    u8 func    = (opcode & 0b00111000) >> 3;
    switch (func) {
        case 0: Add(GetR8(operand)); break;
        case 1: Adc(GetR8(operand)); break;
        case 2: Sub(GetR8(operand)); break;
        case 3: Sbc(GetR8(operand)); break;
        case 4: And(GetR8(operand)); break;
        case 5: Xor(GetR8(operand)); break;
        case 6:  Or(GetR8(operand)); break;
        case 7:  Cp(GetR8(operand)); break;
    }
}

void CPU::ExecuteBlock3(u8 opcode) {
    switch (opcode) {
        case 0xF3: { // DI
            m_IME = false;
            return;
        }
        case 0xFB: { // EI
            m_IME = true;
            return;
        }
        case 0xCB: { // PREFIX CB
            ExecuteCB();
            return;
        }
        case 0xE0: {
            WriteMem(0xFF00 | GetImm8(), m_Reg.A);
            return;
        }
        case 0xE2: {
            WriteMem(0xFF00 | m_Reg.C, m_Reg.A);
            return;
        }
        case 0xEA: {
            WriteMem(GetImm16(), m_Reg.A);
            return;
        }
        case 0xF0: {
            m_Reg.A = ReadMem(0xFF00 | GetImm8());
            return;
        }
        case 0xF2: {
            m_Reg.A = ReadMem(0xFF00 | m_Reg.C);
            return;
        }
        case 0xFA: {
            m_Reg.A = ReadMem(GetImm16());
            return;
        }
        case 0xE8: {
            m_Reg.SP = AddSPImm8();
            return;
        }
        case 0xF8: {
            m_Reg.HL = AddSPImm8();
            return;
        }
        case 0xF9: {
            m_Reg.SP = m_Reg.HL;
            return;
        }
        case 0xC9: {
            Ret();
            return;
        }
        case 0xD9: {
            Ret();
            m_IME = true;
            return;
        }
        case 0xC3: {
            m_Reg.PC = ReadMem16(m_Reg.PC);
            m_Jumped = true;
            return;
        }
        case 0xE9: {
            m_Reg.PC = m_Reg.HL;
            m_Jumped = true;
            return;
        }
        case 0xCD: {
            Call();
            return;
        }
    }

    u8 row = (opcode & 0b00110000) >> 4;
    u8 col = (opcode & 0b00001111);
    switch (col) {
        case 0b0001: {
            u8 dest = row;
            Pop(dest);
            return;
        }
        case 0b0101: {
            u8 src = row;
            Push(src);
            return;
        }
    }

    row = (opcode & 0b00111000) >> 3;
    col = (opcode & 0b00000111);
    switch (col) {
        case 0b110: {
            u8 func = row;
            switch (func) {
                case 0: Add(GetImm8()); return;
                case 1: Adc(GetImm8()); return;
                case 2: Sub(GetImm8()); return;
                case 3: Sbc(GetImm8()); return;
                case 4: And(GetImm8()); return;
                case 5: Xor(GetImm8()); return;
                case 6:  Or(GetImm8()); return;
                case 7:  Cp(GetImm8()); return;
            }
        }
        case 0b111: { // RST
            Rst(opcode);
            return;
        }
    }

    row = (opcode & 0b00011000) >> 3;
    col = (opcode & 0b00100111);
    switch (col) {
        case 0b0000: {
            u8 cond = row;
            if (CheckCondition(cond)) {
                Ret();
            }
            return;
        }
        case 0b0010: {
            u8 cond = row;
            if (CheckCondition(cond)) {
                m_Reg.PC = ReadMem16(m_Reg.PC);
                m_Jumped = true;
            } else {
                m_Reg.PC += 2;
            }
            return;
        }
        case 0b0100: {
            u8 cond = row;
            if (CheckCondition(cond)) {
                Call();
            } else {
                m_Reg.PC += 2;
            }
            return;
        }
    }
}

void CPU::ExecuteCB() {
    m_IsCB = true;

    u8 opcode = GetImm8();

    u8 block = (opcode & 0b11000000) >> 6;
    u8 row = (opcode & 0b00111000) >> 3;
    u8 col = (opcode & 0b00000111);

    switch (block) {
        case 0: {
            u8 operand = col;
            switch (row) {
                case 0: Rlc(operand);  return;
                case 1: Rrc(operand);  return;
                case 2: Rl(operand);   return;
                case 3: Rr(operand);   return;
                case 4: Sla(operand);  return;
                case 5: Sra(operand);  return;
                case 6: Swap(operand); return;
                case 7: Srl(operand);  return;
            }
        }
        case 1: {
            u8 bit = row;
            u8 operand = col;
            Bit(operand, bit);
            return;
        }
        case 2: {
            u8 bit = row;
            u8 operand = col;
            Res(operand, bit);
            return;
        }
        case 3: {
            u8 bit = row;
            u8 operand = col;
            Set(operand, bit);
            return;
        }
    }
}

u8 CPU::ReadMem(u16 addr) const {
    return Gameboy::Get().GetMemory().Read(addr);
}

u16 CPU::ReadMem16(u16 addr) const {
    return Gameboy::Get().GetMemory().Read16(addr);
}

void CPU::WriteMem(u16 addr, u8 val) {
    Gameboy::Get().GetMemory().Write(addr, val);
}

void CPU::WriteMem16(u16 addr, u16 val) {
    Gameboy::Get().GetMemory().Write16(addr, val);
}

u8 CPU::GetR8(u8 idx) const {
    switch (idx) {
        case 0: return m_Reg.B;
        case 1: return m_Reg.C;
        case 2: return m_Reg.D;
        case 3: return m_Reg.E;
        case 4: return m_Reg.H;
        case 5: return m_Reg.L;
        case 6: return ReadMem(m_Reg.HL);
        case 7: return m_Reg.A;
    }

    return 0;
}

void CPU::SetR8(u8 idx, u8 val) {
    switch (idx) {
        case 0: m_Reg.B = val; break;
        case 1: m_Reg.C = val; break;
        case 2: m_Reg.D = val; break;
        case 3: m_Reg.E = val; break;
        case 4: m_Reg.H = val; break;
        case 5: m_Reg.L = val; break;
        case 6: WriteMem(m_Reg.HL, val); break;
        case 7: m_Reg.A = val; break;
    }
}

u16 CPU::GetR16(u8 idx) const {
    switch (idx) {
        case 0: return m_Reg.BC;
        case 1: return m_Reg.DE;
        case 2: return m_Reg.HL;
        case 3: return m_Reg.SP;
    }

    return 0;
}

void CPU::SetR16(u8 idx, u16 val) {
    switch (idx) {
        case 0: m_Reg.BC = val; break;
        case 1: m_Reg.DE = val; break;
        case 2: m_Reg.HL = val; break;
        case 3: m_Reg.SP = val; break;
    }
}

u8 CPU::GetR16Mem(u8 idx) {
    u8 val;
    switch (idx) {
        case 0: val = ReadMem(m_Reg.BC); break;
        case 1: val = ReadMem(m_Reg.DE); break;
        case 2: val = ReadMem(m_Reg.HL); m_Reg.HL++; break;
        case 3: val = ReadMem(m_Reg.HL); m_Reg.HL--; break;
    }

    return val;
}

void CPU::SetR16Mem(u8 idx, u8 val) {
    switch (idx) {
        case 0: WriteMem(m_Reg.BC, val); break;
        case 1: WriteMem(m_Reg.DE, val); break;
        case 2: WriteMem(m_Reg.HL, val); m_Reg.HL++; break;
        case 3: WriteMem(m_Reg.HL, val); m_Reg.HL--; break;
    }
}

u8 CPU::GetImm8() {
    u8 val = ReadMem(m_Reg.PC);
    m_Reg.PC++;
    return val;
}

u16 CPU::GetImm16() {
    u16 val = ReadMem16(m_Reg.PC);
    m_Reg.PC += 2;
    return val;
}

bool CPU::CheckCondition(u8 cond) const {
    switch (cond) {
        case 0: return !GetFlagZ();
        case 1: return GetFlagZ();
        case 2: return !GetFlagC();
        case 3: return GetFlagC();
    }

    return false;
}

void CPU::Inc(u8 operand) {
    u8 val = GetR8(operand);

    SetFlagZ(((val + 1) & 0xFF) == 0);
    SetFlagN(false);
    SetFlagH((val & 0xF) + 1 > 0xF);

    SetR8(operand, val + 1);
}

void CPU::Dec(u8 operand) {
    u8 val = GetR8(operand);

    SetFlagZ(((val - 1) & 0xFF) == 0);
    SetFlagN(true);
    SetFlagH(((val - 1) & 0xF) == 0xF);
    
    SetR8(operand, val - 1);
}

void CPU::Add(u8 val) {
    u16 res = m_Reg.A + val;

    SetFlagZ((res & 0xFF) == 0);
    SetFlagN(false);
    SetFlagH((m_Reg.A & 0xF) + (val & 0xF) > 0xF);
    SetFlagC(res > 0xFF);

    m_Reg.A = res;
}

void CPU::Adc(u8 val) {
    u16 c = GetFlagC();
    u16 res = m_Reg.A + val + c;

    SetFlagZ((res & 0xFF) == 0);
    SetFlagN(false);
    SetFlagH((m_Reg.A & 0xF) + (val & 0xF) + c > 0xF);
    SetFlagC(res > 0xFF);

    m_Reg.A = res;
}

void CPU::Sub(u8 val) {
    i16 res = m_Reg.A - val;

    SetFlagZ((res & 0xFF) == 0);
    SetFlagN(true);
    SetFlagH((m_Reg.A & 0xF) - (val & 0xF) < 0);
    SetFlagC(res < 0);

    m_Reg.A = res;
}

void CPU::Sbc(u8 val) {
    u16 c = GetFlagC();
    i16 res = m_Reg.A - val - c;

    SetFlagZ((res & 0xFF) == 0);
    SetFlagN(true);
    SetFlagH((m_Reg.A & 0xF) - (val & 0xF) - c < 0);
    SetFlagC(res < 0);

    m_Reg.A = res;
}

void CPU::And(u8 val) {
    u16 res = m_Reg.A & val;

    SetFlagZ((res & 0xFF) == 0);
    SetFlagN(false);
    SetFlagH(true);
    SetFlagC(false);

    m_Reg.A = res;
}

void CPU::Or(u8 val) {
    u16 res = m_Reg.A | val;

    SetFlagZ((res & 0xFF) == 0);
    SetFlagN(false);
    SetFlagH(false);
    SetFlagC(false);

    m_Reg.A = res;
}

void CPU::Xor(u8 val) {
    u16 res = m_Reg.A ^ val;

    SetFlagZ((res & 0xFF) == 0);
    SetFlagN(false);
    SetFlagH(false);
    SetFlagC(false);

    m_Reg.A = res;
}

void CPU::Push(u8 src) {
    m_Reg.SP -= 2;
    switch (src) {
        case 0: WriteMem16(m_Reg.SP, m_Reg.BC); break;
        case 1: WriteMem16(m_Reg.SP, m_Reg.DE); break;
        case 2: WriteMem16(m_Reg.SP, m_Reg.HL); break;
        case 3: WriteMem16(m_Reg.SP, m_Reg.AF); break;
    }
}

void CPU::Pop(u8 dest) {
    switch (dest) {
        case 0: m_Reg.BC = ReadMem16(m_Reg.SP); break;
        case 1: m_Reg.DE = ReadMem16(m_Reg.SP); break;
        case 2: m_Reg.HL = ReadMem16(m_Reg.SP); break;
        case 3: m_Reg.AF = ReadMem16(m_Reg.SP) & 0xFFF0; break;
    }
    m_Reg.SP += 2;
}

void CPU::Cp(u8 val) {
    i16 res = m_Reg.A - val;

    SetFlagZ((res & 0xFF) == 0);
    SetFlagN(true);
    SetFlagH((m_Reg.A & 0xF) - (val & 0xF) < 0);
    SetFlagC(res < 0);
}

void CPU::Rst(u8 opcode) {
    m_Reg.SP -= 2;
    WriteMem16(m_Reg.SP, m_Reg.PC);
    m_Reg.PC = opcode & 0x38;
    m_Jumped = true;
}

void CPU::Call() {
    m_Reg.SP -= 2;
    WriteMem16(m_Reg.SP, m_Reg.PC + 2);
    m_Reg.PC = ReadMem16(m_Reg.PC);
    m_Jumped = true;
}

void CPU::Ret() {
    m_Reg.PC = ReadMem16(m_Reg.SP);
    m_Reg.SP += 2;
    m_Jumped = true;
}

u16 CPU::AddSPImm8() {
    i8 val = GetImm8();
    i32 res = m_Reg.SP + val;

    SetFlagZ(false);
    SetFlagN(false);
    if (val > 0) {
        SetFlagH((m_Reg.SP & 0xF) + (val & 0xF) > 0xF);
        SetFlagC((m_Reg.SP & 0xFF) + val > 0xFF);
    } else {
        SetFlagH((res & 0xF) < (m_Reg.SP & 0xF));
        SetFlagC((res & 0xFF) < (m_Reg.SP & 0xFF));
    }

    return res;
}

void CPU::Rlc(u8 operand) {
    u8 val = GetR8(operand);
    u8 c = (val >> 7) & 1;
    u8 res = (val << 1) | c;
    SetR8(operand, res);

    SetFlagZ(res == 0);
    SetFlagN(false);
    SetFlagH(false);
    SetFlagC(c);
}

void CPU::Rrc(u8 operand) {
    u8 val = GetR8(operand);
    u8 c = val & 1;
    u8 res = (val >> 1) | (c << 7);
    SetR8(operand, res);

    SetFlagZ(res == 0);
    SetFlagN(false);
    SetFlagH(false);
    SetFlagC(c);
}

void CPU::Rl(u8 operand) {
    u8 val = GetR8(operand);
    u8 c = (val >> 7) & 1;
    u8 res = (val << 1) | GetFlagC();
    SetR8(operand, res);

    SetFlagZ(res == 0);
    SetFlagN(false);
    SetFlagH(false);
    SetFlagC(c);
}

void CPU::Rr(u8 operand) {
    u8 val = GetR8(operand);
    u8 c = val & 1;
    u8 res = (val >> 1) | (GetFlagC() << 7);
    SetR8(operand, res);

    SetFlagZ(res == 0);
    SetFlagN(false);
    SetFlagH(false);
    SetFlagC(c);
}

void CPU::Sla(u8 operand) {
    u8 val = GetR8(operand);
    u8 c = (val >> 7) & 1;
    u8 res = val << 1;
    SetR8(operand, res);

    SetFlagZ(res == 0);
    SetFlagN(false);
    SetFlagH(false);
    SetFlagC(c);
}

void CPU::Sra(u8 operand) {
    u8 val = GetR8(operand);
    u8 c = val & 1;
    u8 res = static_cast<i8>(val) >> 1;
    SetR8(operand, res);

    SetFlagZ(res == 0);
    SetFlagN(false);
    SetFlagH(false);
    SetFlagC(c);
}

void CPU::Swap(u8 operand) {
    u8 val = GetR8(operand);
    u8 res = ((val & 0xF0) >> 4) | ((val & 0xF) << 4);
    SetR8(operand, res);

    SetFlagZ(res == 0);
    SetFlagN(false);
    SetFlagH(false);
    SetFlagC(false);
}

void CPU::Srl(u8 operand) {
    u8 val = GetR8(operand);
    u8 c = val & 1;
    u8 res = val >> 1;
    SetR8(operand, res);

    SetFlagZ(res == 0);
    SetFlagN(false);
    SetFlagH(false);
    SetFlagC(c);
}

void CPU::Bit(u8 operand, u8 bit) {
    u8 val = GetR8(operand);
    SetFlagZ(!BIT(val, bit));
    SetFlagN(false);
    SetFlagH(true);
}

void CPU::Res(u8 operand, u8 bit) {
    u8 val = GetR8(operand);
    SET_BIT(val, bit, 0);
    SetR8(operand, val);
}


void CPU::Set(u8 operand, u8 bit) {
    u8 val = GetR8(operand);
    SET_BIT(val, bit, 1);
    SetR8(operand, val);
}

void CPU::Daa() {
    u8 off = 0;
    bool n = GetFlagN();
    bool h = GetFlagH();
    bool c = GetFlagC();
    if (h || (!n && ((m_Reg.A & 0x0F) > 9))) {
        off |= 0x06;
    }

    if (c || (!n && m_Reg.A > 0x99)) {
        off |= 0x60;
    }

    m_Reg.A += n ? -off : off;
    SetFlagZ(m_Reg.A == 0);
    SetFlagH(0);
    SetFlagC((off & 0x60) != 0);
}

const u8 CPU::s_CyclesNormal[0x100] = {
    1, 3, 2, 2, 1, 1, 2, 1, 5, 2, 2, 2, 1, 1, 2, 1,
    1, 3, 2, 2, 1, 1, 2, 1, 3, 2, 2, 2, 1, 1, 2, 1,
    2, 3, 2, 2, 1, 1, 2, 1, 2, 2, 2, 2, 1, 1, 2, 1,
    2, 3, 2, 2, 3, 3, 3, 1, 2, 2, 2, 2, 1, 1, 2, 1,
    1, 1, 1, 1, 1, 1, 2, 1, 1, 1, 1, 1, 1, 1, 2, 1,
    1, 1, 1, 1, 1, 1, 2, 1, 1, 1, 1, 1, 1, 1, 2, 1,
    1, 1, 1, 1, 1, 1, 2, 1, 1, 1, 1, 1, 1, 1, 2, 1,
    2, 2, 2, 2, 2, 2, 1, 2, 1, 1, 1, 1, 1, 1, 2, 1,
    1, 1, 1, 1, 1, 1, 2, 1, 1, 1, 1, 1, 1, 1, 2, 1,
    1, 1, 1, 1, 1, 1, 2, 1, 1, 1, 1, 1, 1, 1, 2, 1,
    1, 1, 1, 1, 1, 1, 2, 1, 1, 1, 1, 1, 1, 1, 2, 1,
    1, 1, 1, 1, 1, 1, 2, 1, 1, 1, 1, 1, 1, 1, 2, 1,
    2, 3, 3, 4, 3, 4, 2, 4, 2, 4, 3, 0, 3, 6, 2, 4,
    2, 3, 3, 0, 3, 4, 2, 4, 2, 4, 3, 0, 3, 0, 2, 4,
    3, 3, 2, 0, 0, 4, 2, 4, 4, 1, 4, 0, 0, 0, 2, 4,
    3, 3, 2, 1, 0, 4, 2, 4, 3, 2, 4, 1, 0, 0, 2, 4
};

const u8 CPU::s_CyclesJumped[0x100] = {
    1, 3, 2, 2, 1, 1, 2, 1, 5, 2, 2, 2, 1, 1, 2, 1,
    1, 3, 2, 2, 1, 1, 2, 1, 3, 2, 2, 2, 1, 1, 2, 1,
    3, 3, 2, 2, 1, 1, 2, 1, 3, 2, 2, 2, 1, 1, 2, 1,
    3, 3, 2, 2, 3, 3, 3, 1, 3, 2, 2, 2, 1, 1, 2, 1,
    1, 1, 1, 1, 1, 1, 2, 1, 1, 1, 1, 1, 1, 1, 2, 1,
    1, 1, 1, 1, 1, 1, 2, 1, 1, 1, 1, 1, 1, 1, 2, 1,
    1, 1, 1, 1, 1, 1, 2, 1, 1, 1, 1, 1, 1, 1, 2, 1,
    2, 2, 2, 2, 2, 2, 1, 2, 1, 1, 1, 1, 1, 1, 2, 1,
    1, 1, 1, 1, 1, 1, 2, 1, 1, 1, 1, 1, 1, 1, 2, 1,
    1, 1, 1, 1, 1, 1, 2, 1, 1, 1, 1, 1, 1, 1, 2, 1,
    1, 1, 1, 1, 1, 1, 2, 1, 1, 1, 1, 1, 1, 1, 2, 1,
    1, 1, 1, 1, 1, 1, 2, 1, 1, 1, 1, 1, 1, 1, 2, 1,
    5, 3, 4, 4, 6, 4, 2, 4, 5, 4, 4, 0, 6, 6, 2, 4,
    5, 3, 4, 0, 6, 4, 2, 4, 5, 4, 4, 0, 6, 0, 2, 4,
    3, 3, 2, 0, 0, 4, 2, 4, 4, 1, 4, 0, 0, 0, 2, 4,
    3, 3, 2, 1, 0, 4, 2, 4, 3, 2, 4, 1, 0, 0, 2, 4
};

const u8 CPU::s_CyclesCB[0x100] = {
    2, 2, 2, 2, 2, 2, 4, 2, 2, 2, 2, 2, 2, 2, 4, 2,
    2, 2, 2, 2, 2, 2, 4, 2, 2, 2, 2, 2, 2, 2, 4, 2,
    2, 2, 2, 2, 2, 2, 4, 2, 2, 2, 2, 2, 2, 2, 4, 2,
    2, 2, 2, 2, 2, 2, 4, 2, 2, 2, 2, 2, 2, 2, 4, 2,
    2, 2, 2, 2, 2, 2, 3, 2, 2, 2, 2, 2, 2, 2, 3, 2,
    2, 2, 2, 2, 2, 2, 3, 2, 2, 2, 2, 2, 2, 2, 3, 2,
    2, 2, 2, 2, 2, 2, 3, 2, 2, 2, 2, 2, 2, 2, 3, 2,
    2, 2, 2, 2, 2, 2, 3, 2, 2, 2, 2, 2, 2, 2, 3, 2,
    2, 2, 2, 2, 2, 2, 4, 2, 2, 2, 2, 2, 2, 2, 4, 2,
    2, 2, 2, 2, 2, 2, 4, 2, 2, 2, 2, 2, 2, 2, 4, 2,
    2, 2, 2, 2, 2, 2, 4, 2, 2, 2, 2, 2, 2, 2, 4, 2,
    2, 2, 2, 2, 2, 2, 4, 2, 2, 2, 2, 2, 2, 2, 4, 2,
    2, 2, 2, 2, 2, 2, 4, 2, 2, 2, 2, 2, 2, 2, 4, 2,
    2, 2, 2, 2, 2, 2, 4, 2, 2, 2, 2, 2, 2, 2, 4, 2,
    2, 2, 2, 2, 2, 2, 4, 2, 2, 2, 2, 2, 2, 2, 4, 2,
    2, 2, 2, 2, 2, 2, 4, 2, 2, 2, 2, 2, 2, 2, 4, 2
};

const char *CPU::s_OpcodeNames[0x100] = {
	// 0x0X
	[0x00] = "NOP        ",
	[0x01] = "LD BC,d16  ",
	[0x02] = "LD (BC),A  ",
	[0x03] = "INC BC     ",
	[0x04] = "INC B      ",
	[0x05] = "DEC B      ",
	[0x06] = "LD B,d8    ",
	[0x07] = "RLCA       ",
	[0x08] = "LD (a16),SP",
	[0x09] = "ADD HL,BC  ",
	[0x0A] = "LD A,(BC)  ",
	[0x0B] = "DEC BC     ",
	[0x0C] = "INC C      ",
	[0x0D] = "DEC C      ",
	[0x0E] = "LD C,d8    ",
	[0x0F] = "RRCA       ",

	// 0x1X
	[0x10] = "STOP 0     ",
	[0x11] = "LD DE,d16  ",
	[0x12] = "LD (DE),A  ",
	[0x13] = "INC DE     ",
	[0x14] = "INC D      ",
	[0x15] = "DEC D      ",
	[0x16] = "LD D,d8    ",
	[0x17] = "RLA        ",
	[0x18] = "JR r8      ",
	[0x19] = "ADD HL,DE  ",
	[0x1A] = "LD A,(DE)  ",
	[0x1B] = "DEC DE     ",
	[0x1C] = "INC E      ",
	[0x1D] = "DEC E      ",
	[0x1E] = "LD E,d8    ",
	[0x1F] = "RRA        ",

	// 0x2X
	[0x20] = "JR NZ,r8   ",
	[0x21] = "LD HL,d16  ",
	[0x22] = "LD (HL+),A ",
	[0x23] = "INC HL     ",
	[0x24] = "INC H      ",
	[0x25] = "DEC H      ",
	[0x26] = "LD H,d8    ",
	[0x27] = "DAA        ",
	[0x28] = "JR Z,r8    ",
	[0x29] = "ADD HL,HL  ",
	[0x2A] = "LD A,(HL+) ",
	[0x2B] = "DEC HL     ",
	[0x2C] = "INC L      ",
	[0x2D] = "DEC L      ",
	[0x2E] = "LD L,d8    ",
	[0x2F] = "CPL        ",

	// 0x3X
	[0x30] = "JR NC,r8   ",
	[0x31] = "LD SP,d16  ",
	[0x32] = "LD (HL-),A ",
	[0x33] = "INC SP     ",
	[0x34] = "INC (HL)   ",
	[0x35] = "DEC (HL)   ",
	[0x36] = "LD (HL),d8 ",
	[0x37] = "SCF        ",
	[0x38] = "JR C,r8    ",
	[0x39] = "ADD HL,SP  ",
	[0x3A] = "LD A,(HL-) ",
	[0x3B] = "DEC SP     ",
	[0x3C] = "INC A      ",
	[0x3D] = "DEC A      ",
	[0x3E] = "LD A,d8    ",
	[0x3F] = "CCF        ",

	// 0x4X
	[0x40] = "LD B,B     ",
	[0x41] = "LD B,C     ",
	[0x42] = "LD B,D     ",
	[0x43] = "LD B,E     ",
	[0x44] = "LD B,H     ",
	[0x45] = "LD B,L     ",
	[0x46] = "LD B,(HL)  ",
	[0x47] = "LD B,A     ",
	[0x48] = "LD C,B     ",
	[0x49] = "LD C,C     ",
	[0x4A] = "LD C,D     ",
	[0x4B] = "LD C,E     ",
	[0x4C] = "LD C,H     ",
	[0x4D] = "LD C,L     ",
	[0x4E] = "LD C,(HL)  ",
	[0x4F] = "LD C,A     ",

	// 0x5X
	[0x50] = "LD D,B     ",
	[0x51] = "LD D,C     ",
	[0x52] = "LD D,D     ",
	[0x53] = "LD D,E     ",
	[0x54] = "LD D,H     ",
	[0x55] = "LD D,L     ",
	[0x56] = "LD D,(HL)  ",
	[0x57] = "LD D,A     ",
	[0x58] = "LD E,B     ",
	[0x59] = "LD E,C     ",
	[0x5A] = "LD E,D     ",
	[0x5B] = "LD E,E     ",
	[0x5C] = "LD E,H     ",
	[0x5D] = "LD E,L     ",
	[0x5E] = "LD E,(HL)  ",
	[0x5F] = "LD E,A     ",

	// 0x6X
	[0x60] = "LD H,B     ",
	[0x61] = "LD H,C     ",
	[0x62] = "LD H,D     ",
	[0x63] = "LD H,E     ",
	[0x64] = "LD H,H     ",
	[0x65] = "LD H,L     ",
	[0x66] = "LD H,(HL)  ",
	[0x67] = "LD H,A     ",
	[0x68] = "LD L,B     ",
	[0x69] = "LD L,C     ",
	[0x6A] = "LD L,D     ",
	[0x6B] = "LD L,E     ",
	[0x6C] = "LD L,H     ",
	[0x6D] = "LD L,L     ",
	[0x6E] = "LD L,(HL)  ",
	[0x6F] = "LD L,A     ",

	// 0x7X
	[0x70] = "LD (HL),B  ",
	[0x71] = "LD (HL),C  ",
	[0x72] = "LD (HL),D  ",
	[0x73] = "LD (HL),E  ",
	[0x74] = "LD (HL),H  ",
	[0x75] = "LD (HL),L  ",
	[0x76] = "HALT       ",
	[0x77] = "LD (HL),A  ",
	[0x78] = "LD A,B     ",
	[0x79] = "LD A,C     ",
	[0x7A] = "LD A,D     ",
	[0x7B] = "LD A,E     ",
	[0x7C] = "LD A,H     ",
	[0x7D] = "LD A,L     ",
	[0x7E] = "LD A,(HL)  ",
	[0x7F] = "LD A,A     ",

	// 0x8X
	[0x80] = "ADD A,B    ",
	[0x81] = "ADD A,C    ",
	[0x82] = "ADD A,D    ",
	[0x83] = "ADD A,E    ",
	[0x84] = "ADD A,H    ",
	[0x85] = "ADD A,L    ",
	[0x86] = "ADD A,(HL) ",
	[0x87] = "ADD A,A    ",
	[0x88] = "ADC A,B    ",
	[0x89] = "ADC A,C    ",
	[0x8A] = "ADC A,D    ",
	[0x8B] = "ADC A,E    ",
	[0x8C] = "ADC A,H    ",
	[0x8D] = "ADC A,L    ",
	[0x8E] = "ADC A,(HL) ",
	[0x8F] = "ADC A,A    ",

	// 0x9X
	[0x90] = "SUB B      ",
	[0x91] = "SUB C      ",
	[0x92] = "SUB D      ",
	[0x93] = "SUB E      ",
	[0x94] = "SUB H      ",
	[0x95] = "SUB L      ",
	[0x96] = "SUB (HL)   ",
	[0x97] = "SUB A      ",
	[0x98] = "SBC A,B    ",
	[0x99] = "SBC A,C    ",
	[0x9A] = "SBC A,D    ",
	[0x9B] = "SBC A,E    ",
	[0x9C] = "SBC A,H    ",
	[0x9D] = "SBC A,L    ",
	[0x9E] = "SBC A,(HL) ",
	[0x9F] = "SBC A,A    ",

	// 0xAX
	[0xA0] = "AND B      ",
	[0xA1] = "AND C      ",
	[0xA2] = "AND D      ",
	[0xA3] = "AND E      ",
	[0xA4] = "AND H      ",
	[0xA5] = "AND L      ",
	[0xA6] = "AND (HL)   ",
	[0xA7] = "AND A      ",
	[0xA8] = "XOR B      ",
	[0xA9] = "XOR C      ",
	[0xAA] = "XOR D      ",
	[0xAB] = "XOR E      ",
	[0xAC] = "XOR H      ",
	[0xAD] = "XOR L      ",
	[0xAE] = "XOR (HL)   ",
	[0xAF] = "XOR A      ",

	// 0xBX
	[0xB0] = "OR B       ",
	[0xB1] = "OR C       ",
	[0xB2] = "OR D       ",
	[0xB3] = "OR E       ",
	[0xB4] = "OR H       ",
	[0xB5] = "OR L       ",
	[0xB6] = "OR (HL)    ",
	[0xB7] = "OR A       ",
	[0xB8] = "CP B       ",
	[0xB9] = "CP C       ",
	[0xBA] = "CP D       ",
	[0xBB] = "CP E       ",
	[0xBC] = "CP H       ",
	[0xBD] = "CP L       ",
	[0xBE] = "CP (HL)    ",
	[0xBF] = "CP A       ",

	// 0xCX
	[0xC0] = "RET NZ     ",
	[0xC1] = "POP BC     ",
	[0xC2] = "JP NZ,a16  ",
	[0xC3] = "JP a16     ",
	[0xC4] = "CALL NZ,a16",
	[0xC5] = "PUSH BC    ",
	[0xC6] = "ADD A,d8   ",
	[0xC7] = "RST 00H    ",
	[0xC8] = "RET Z      ",
	[0xC9] = "RET        ",
	[0xCA] = "JP Z,a16   ",
	[0xCB] = "PREFIX CB  ",
	[0xCC] = "CALL Z,a16 ",
	[0xCD] = "CALL a16   ",
	[0xCE] = "ADC A,d8   ",
	[0xCF] = "RST 08H    ",

	// 0xDX
	[0xD0] = "RET NC     ",
	[0xD1] = "POP DE     ",
	[0xD2] = "JP NC,a16  ",
	[0xD3] = "           ",
	[0xD4] = "CALL NC,a16",
	[0xD5] = "PUSH DE    ",
	[0xD6] = "SUB d8     ",
	[0xD7] = "RST 10H    ",
	[0xD8] = "RET C      ",
	[0xD9] = "RETI       ",
	[0xDA] = "JP C,a16   ",
	[0xDB] = "           ",
	[0xDC] = "CALL C,a16 ",
	[0xDD] = "           ",
	[0xDE] = "SBC A,d8   ",
	[0xDF] = "RST 18H    ",

	// 0xEX
	[0xE0] = "LDH (a8),A ",
	[0xE1] = "POP HL     ",
	[0xE2] = "LD (C),A   ",
	[0xE3] = "           ",
	[0xE4] = "           ",
	[0xE5] = "PUSH HL    ",
	[0xE6] = "AND d8     ",
	[0xE7] = "RST 20H    ",
	[0xE8] = "ADD SP,r8  ",
	[0xE9] = "JP (HL)    ",
	[0xEA] = "LD (a16),A ",
	[0xEB] = "           ",
	[0xEC] = "           ",
	[0xED] = "           ",
	[0xEE] = "XOR d8     ",
	[0xEF] = "RST 28H    ",

	// 0xFX
	[0xF0] = "LDH A,(a8) ",
	[0xF1] = "POP AF     ",
	[0xF2] = "LD A,(C)   ",
	[0xF3] = "DI         ",
	[0xF4] = "           ",
	[0xF5] = "PUSH AF    ",
	[0xF6] = "OR d8      ",
	[0xF7] = "RST 30H    ",
	[0xF8] = "LD HL,SP+r8",
	[0xF9] = "LD SP,HL   ",
	[0xFA] = "LD A,(a16) ",
	[0xFB] = "EI         ",
	[0xFC] = "           ",
	[0xFD] = "           ",
	[0xFE] = "CP d8      ",
	[0xFF] = "RST 38H    ",
};