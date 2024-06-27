#pragma once

#include "Common.hpp"

enum LCDMode {
    Hblank,
    Vblank,
    AccessOam,
    AccessVram,
};

struct LCD {
    u8 Control = 0x91;
    u8 Status = 0x84;
    u8 ScrollY = 0;
    u8 ScrollX = 0;
    u8 LY = 0;
    u8 LYCompare = 0;
    u8 DMA = 0;
    u8 BGPalette = 0xFC;
    u8 ObjPalette0 = 0xFF;
    u8 ObjPalette1 = 0xFF;
    u8 WindowY = 0;
    u8 WindowX = 7;
};

class PPU {
public:
    PPU() : m_Framebuffer(m_FrameWidth * m_FrameHeight, 0) {}

    LCD &GetLCD() { return m_LCD; }

    const std::vector<u32> &GetFramebuffer() const { return m_Framebuffer; }
    usize GetCurrentFrame() const { return m_CurrentFrame; }

    void Tick(u8 cycles);

    void CheckForReset();

    void SetColors(u32 mainColor);

private:
    void LYUpdate(u8 newLY);
    void LYIncrement();
    void LYReset();

    void Maintain60FPS();

    LCDMode GetLCDMode() const;
    void SetLCDMode(LCDMode mode);

    bool InsideWindow(u8 x, u8 y);

    void WriteBGLine();
    void WriteSprites();

private:
    usize m_FrameWidth = 160;
    usize m_FrameHeight = 144;
    std::vector<u32> m_Framebuffer;

    u32 m_Colors[4];

    LCD m_LCD;
    bool m_LCDEnabled = true;

    usize m_CurrentFrame = 0;
    usize m_Counter = 0;
    u32 m_TimerStart = 0;
    u32 m_TimerEnd = 0;
};