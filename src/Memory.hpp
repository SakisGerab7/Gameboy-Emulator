#pragma once

#include "Common.hpp"

class Memory {
public:
    u8 Read(u16 addr) const;
    void Write(u16 addr, u8 val);

    u16 Read16(u16 addr) const;
    void Write16(u16 addr, u16 val);

private:
    u8 IORead(u16 addr) const;
    void IOWrite(u16 addr, u8 val);

    void DMATransfer(u8 val);

private:
    u8 m_Vram[0x2000] = {};
    u8 m_Wram[0x2000] = {};
    u8 m_Oam[0xA0] = {};
    u8 m_Hram[0x80] = {};
    u8 m_SerialData[2];
};