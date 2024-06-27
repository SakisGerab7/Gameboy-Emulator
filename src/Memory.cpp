#include "Memory.hpp"
#include "Gameboy.hpp"
#include "Log.hpp"

u8 Memory::Read(u16 addr) const {
    Cartrige &cart = Gameboy::Get().GetCartrige();

    switch (addr) {
        case 0x0000 ... 0x7FFF: return cart.Read(addr);
        case 0x8000 ... 0x9FFF: return m_Vram[addr - 0x8000];
        case 0xA000 ... 0xBFFF: return cart.Read(addr);
        case 0xC000 ... 0xDFFF: return m_Wram[addr - 0xC000];
        case 0xE000 ... 0xFDFF: Log::Error("Reserved - Echo RAM. Can't Read (addr 0x%04X)\n", addr); return 0;
        case 0xFE00 ... 0xFE9F: return m_Oam[addr - 0xFE00];
        case 0xFEA0 ... 0xFEFF: Log::Error("Reserved - Unusable. Can't Read (addr 0x%04X)\n", addr); return 0;
        case 0xFF00 ... 0xFF7F: return IORead(addr);
        case 0xFF80 ... 0xFFFE: return m_Hram[addr - 0xFF80];
        case 0xFFFF: return Gameboy::Get().GetCPU().GetIE();
        default: return 0;
    }

    return 0;
}

void Memory::Write(u16 addr, u8 val) {
    Cartrige &cart = Gameboy::Get().GetCartrige();

    switch (addr) {
        case 0x0000 ... 0x7FFF: cart.Write(addr, val); break;
        case 0x8000 ... 0x9FFF: m_Vram[addr - 0x8000] = val; break;
        case 0xA000 ... 0xBFFF: cart.Write(addr, val); break;
        case 0xC000 ... 0xDFFF: m_Wram[addr - 0xC000] = val; break;
        case 0xE000 ... 0xFDFF: Log::Error("Reserved - Echo RAM. Can't Write (addr 0x%04X)\n", addr); break;
        case 0xFE00 ... 0xFE9F: m_Oam[addr - 0xFE00] = val; break;
        case 0xFEA0 ... 0xFEFF: Log::Error("Reserved - Unusable. Can't Write (addr 0x%04X)\n", addr); break;
        case 0xFF00 ... 0xFF7F: IOWrite(addr, val); break;
        case 0xFF80 ... 0xFFFE: m_Hram[addr - 0xFF80] = val; break;
        case 0xFFFF: Gameboy::Get().GetCPU().SetIE(val); break;
        default: break;
    }
}

u16 Memory::Read16(u16 addr) const {
    return static_cast<u16>(Read(addr + 1) << 8) | static_cast<u16>(Read(addr));
}

void Memory::Write16(u16 addr, u16 val) {
    Write(addr, static_cast<u8>(val));
    Write(addr + 1, static_cast<u8>(val >> 8));
}

u8 Memory::IORead(u16 addr) const {
    Joypad &joypad = Gameboy::Get().GetUI().GetJoypad();
    Timer &timer = Gameboy::Get().GetTimer();
    LCD &lcd = Gameboy::Get().GetPPU().GetLCD();

    switch (addr) {
        // joypad
        case 0xFF00: {
            u8 val = 0xFF;
            if (joypad.Action) {
                if (joypad.A)      SET_BIT(val, 0, 0);
                if (joypad.B)      SET_BIT(val, 1, 0);
                if (joypad.Select) SET_BIT(val, 2, 0);
                if (joypad.Start)  SET_BIT(val, 3, 0);
            } else if (joypad.Directon) {
                if (joypad.Right) SET_BIT(val, 0, 0);
                if (joypad.Left)  SET_BIT(val, 1, 0);
                if (joypad.Up)    SET_BIT(val, 2, 0);
                if (joypad.Down)  SET_BIT(val, 3, 0);
            }

            return val;
        }
        // serial data
        case 0xFF01: return m_SerialData[0];
        case 0xFF02: return m_SerialData[1];
        // timer
        case 0xFF04: return timer.GetDIV();
        case 0xFF05: return timer.GetTIMA();
        case 0xFF06: return timer.GetTMA();
        case 0xFF07: return timer.GetTAC();
        // interputs fired
        case 0xFF0F: return Gameboy::Get().GetCPU().GetIF();
        // lcd
        case 0xFF40: return lcd.Control;
        case 0xFF41: return lcd.Status;
        case 0xFF42: return lcd.ScrollY;
        case 0xFF43: return lcd.ScrollX;
        case 0xFF44: return lcd.LY;
        case 0xFF45: return lcd.LYCompare;
        case 0xFF46: return lcd.DMA;
        case 0xFF47: return lcd.BGPalette;
        case 0xFF48: return lcd.ObjPalette0;
        case 0xFF49: return lcd.ObjPalette1;
        case 0xFF4A: return lcd.WindowY;
        case 0xFF4B: return lcd.WindowX;
        default: return 0;
    }

    return 0;
}

void Memory::IOWrite(u16 addr, u8 val) {
    Joypad &joypad = Gameboy::Get().GetUI().GetJoypad();
    Timer &timer = Gameboy::Get().GetTimer();
    PPU &ppu = Gameboy::Get().GetPPU();
    LCD &lcd = ppu.GetLCD();

    switch (addr) {
        // joypad
        case 0xFF00: {
            joypad.Action   = (BIT(val, 5) == 0);
            joypad.Directon = (BIT(val, 4) == 0);
        } break;
        // serial data
        case 0xFF01: m_SerialData[0] = val; break;
        case 0xFF02: m_SerialData[1] = val; break;
        // timer 
        case 0xFF04: timer.SetDIV(0);    break;
        case 0xFF05: timer.SetTIMA(val); break;
        case 0xFF06: timer.SetTMA(val);  break;
        case 0xFF07: timer.SetTAC(val);  break;
        // interupts fired
        case 0xFF0F: Gameboy::Get().GetCPU().SetIF(val); break;
        // lcd
        case 0xFF40: {
            lcd.Control = val;
            ppu.CheckForReset();
        } break;
        case 0xFF41: lcd.Status    = (lcd.Status & 0b11) | val; break;
        case 0xFF42: lcd.ScrollY   = val; break;
        case 0xFF43: lcd.ScrollX   = val; break;
        case 0xFF44: lcd.LY        = val; break;
        case 0xFF45: lcd.LYCompare = val; break;
        case 0xFF46: {
            lcd.DMA = val;
            DMATransfer(val);
            break;
        }
        case 0xFF47: lcd.BGPalette   = val; break;
        case 0xFF48: lcd.ObjPalette0 = val; break;
        case 0xFF49: lcd.ObjPalette1 = val; break;
        case 0xFF4A: lcd.WindowY     = val; break;
        case 0xFF4B: lcd.WindowX     = val; break;
        default: break;
    }
}

void Memory::DMATransfer(u8 val) {
    u16 src_start = val << 8;
    u16 dest_start = 0xFE00;

    for (u16 i = 0; i < 0xA0; i++) {
        u16 src = src_start + i;
        u16 dest = dest_start + i;
        Write(dest, Read(src));
    }
}