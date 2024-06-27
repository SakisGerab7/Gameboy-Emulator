#include "PPU.hpp"
#include "Gameboy.hpp"

void LoadPallete(u8 palette, u8 *colors) {
    colors[0] = (palette & 0b00000011) >> 0;
    colors[1] = (palette & 0b00001100) >> 2;
    colors[2] = (palette & 0b00110000) >> 4;
    colors[3] = (palette & 0b11000000) >> 6;
}
    
void PPU::Tick(u8 cycles) {
    if (!m_LCDEnabled) return;

    u8 controlBGEnabled = BIT(m_LCD.Control, 0);
    u8 controlObjEnabled = BIT(m_LCD.Control, 1);
    u8 controlLCDEnabled = BIT(m_LCD.Control, 7);

    m_Counter += cycles;

    switch (GetLCDMode()) {
        case LCDMode::AccessOam: {
            if (m_Counter >= 80) {
                m_Counter %= 80;
                SetLCDMode(LCDMode::AccessVram);
            }
            break;
        }
        case LCDMode::AccessVram: {
            if (m_Counter >= 172) {
                if (controlLCDEnabled && controlBGEnabled) {
                    WriteBGLine();
                }

                if (controlLCDEnabled && controlObjEnabled) {
                    WriteSprites();
                }

                m_Counter %= 172;
                SetLCDMode(LCDMode::Hblank);
            }
            break;
        }
        case LCDMode::Hblank: {
            if (m_Counter >= 204) {
                m_Counter %= 204;

                if (m_LCD.LY >= m_FrameHeight - 1) {
                    SetLCDMode(LCDMode::Vblank);
                    m_CurrentFrame++;

                    Gameboy::Get().GetCPU().RequestInterrupt(CPU::Interrupt::Vblank);
                    Maintain60FPS();
                } else {
                    LYIncrement();
                    SetLCDMode(LCDMode::AccessOam);
                }
            }
            break;
        }
        case LCDMode::Vblank: {
            if (m_Counter >= 456) {
                m_Counter %= 456;
                LYIncrement();

                if (m_LCD.LY > 153) {
                    LYReset();
                    SetLCDMode(LCDMode::AccessOam);
                }
            }
            break;
        }
        default: break;
    }
}

void PPU::CheckForReset() {
    if (!m_LCDEnabled && BIT(m_LCD.Control, 7)) {
        m_LCDEnabled = true;
    }

    if (m_LCDEnabled && !BIT(m_LCD.Control, 7)) {
        m_LCD.Status = (m_LCD.Status & ~0b11) | LCDMode::Hblank;
        m_LCDEnabled = false;
        m_Counter = 0;
        LYReset();
    }
}

void PPU::LYUpdate(u8 newLY) {
    m_LCD.LY = newLY;
    if (m_LCD.LY == m_LCD.LYCompare) {
        SET_BIT(m_LCD.Status, 2, 1);

        u8 statusLyc = BIT(m_LCD.Status, 6);
        if (statusLyc) {
            Gameboy::Get().GetCPU().RequestInterrupt(CPU::Interrupt::LcdStat);
        }
    } else {
        SET_BIT(m_LCD.Status, 2, 0);
    }
}

void PPU::LYIncrement() {
    LYUpdate(m_LCD.LY + 1);
}

void PPU::LYReset() {
    LYUpdate(0);
}

void PPU::Maintain60FPS() {
    static u8 totalFrames = 0;
    static u64 startTime = 0;
    
    m_TimerEnd = SDL_GetTicks();
    u32 dt = m_TimerEnd - m_TimerStart;

    const u32 totalFrameTime = 1000 / 60;
    if (dt < totalFrameTime) {
        SDL_Delay(totalFrameTime - dt);
    }

    if (m_TimerEnd - startTime >= 1000) {
        // LOG_INFO("FPS: %d\n", total_frames);
        startTime = m_TimerEnd;
        totalFrames = 0;
    }

    totalFrames++;
    m_TimerStart = SDL_GetTicks();
}

LCDMode PPU::GetLCDMode() const {
    return static_cast<LCDMode>(m_LCD.Status & 0b11);
}

void PPU::SetLCDMode(LCDMode mode) {
    m_LCD.Status = (m_LCD.Status & ~0b11) | mode;

    u8 statusOam = BIT(m_LCD.Status, 5);
    u8 statusVblank = BIT(m_LCD.Status, 4);
    u8 statusHblank =  BIT(m_LCD.Status, 3);

    if ((statusOam && mode == LCDMode::AccessOam) ||
        (statusVblank && mode == LCDMode::Vblank) ||
        (statusHblank && mode == LCDMode::Hblank)
    ) {
        Gameboy::Get().GetCPU().RequestInterrupt(CPU::Interrupt::LcdStat);
    }
}

void PPU::SetColors(u32 mainColor) {
    m_Colors[0] = mainColor;
    m_Colors[1] = mainColor & 0xFFAAAAAA;
    m_Colors[2] = mainColor & 0xFF555555;
    m_Colors[3] = 0;
}

bool PPU::InsideWindow(u8 x, u8 y) {
    u8 winEnabled = BIT(m_LCD.Control, 5);
    return winEnabled && (y >= m_LCD.WindowY) && (x >= m_LCD.WindowX - 7);
}

void PPU::WriteBGLine() {
    u8 colors[4];
    LoadPallete(m_LCD.BGPalette, colors);

    u8 y = m_LCD.LY;

    u8 controlBGMapArea = BIT(m_LCD.Control, 3);
    u8 controlBGDataArea = BIT(m_LCD.Control, 4);
    u8 controlWinMapArea = BIT(m_LCD.Control, 6);

    u16 tileDataBase = controlBGDataArea ? 0x8000 : 0x9000;
    u16 tileMapBase = controlBGMapArea ? 0x9C00 : 0x9800;

    for (u8 x = 0; x < m_FrameWidth; x++) {
        u8 bgMapX = (x + m_LCD.ScrollX) % 256;
        u8 bgMapY = (y + m_LCD.ScrollY) % 256;

        if (InsideWindow(x, y)) {
            tileMapBase = controlWinMapArea ? 0x9C00 : 0x9800;
            bgMapX = x - m_LCD.WindowX + 7;
            bgMapY = y - m_LCD.WindowY;
        }

        u8 tileX = bgMapX / 8;
        u8 tileY = bgMapY / 8;
        u8 tilePixelX = bgMapX % 8;
        u8 tilePixelY = bgMapY % 8;

        Memory &memory = Gameboy::Get().GetMemory();

        u16 tileIdx = 32 * static_cast<u16>(tileY) + static_cast<u16>(tileX);
        u8 tile = memory.Read(tileMapBase + tileIdx);

        i16 tileOffset = controlBGDataArea ? 16 * tile : 16 * static_cast<i8>(tile);
        u16 pixelOffset = 2 * tilePixelY;

        u8 b1 = memory.Read(tileDataBase + tileOffset + pixelOffset);
        u8 b2 = memory.Read(tileDataBase + tileOffset + pixelOffset + 1);

        u8 colorIdx = (BIT(b2, 7 - tilePixelX) << 1) | BIT(b1, 7 - tilePixelX);
        u32 color = m_Colors[colors[colorIdx]];
        m_Framebuffer[m_FrameWidth * y + x] = color;
    }
}

void PPU::WriteSprites() {
    u8 controlObjSize = BIT(m_LCD.Control, 2);
    u8 mult = controlObjSize ? 2 : 1;

    u8 y = m_LCD.LY;

    for (u8 i = 0; i < 40; i++) {
        u16 spriteAddr = 0xFE00 + 4 * i;

        Memory &memory = Gameboy::Get().GetMemory();

        u8 spriteYpos = memory.Read(spriteAddr);
        u8 spriteXpos = memory.Read(spriteAddr + 1);
        u8 spriteIdx = memory.Read(spriteAddr + 2);
        u8 spriteFlags = memory.Read(spriteAddr + 3);

        u8 pallete = BIT(spriteFlags, 4) ? m_LCD.ObjPalette1 : m_LCD.ObjPalette0;
        u8 colors[4];
        LoadPallete(pallete, colors);

        u16 tileAddr = 0x8000 + 16 * spriteIdx;

        if (static_cast<i16>(y) >= static_cast<i16>(spriteYpos - 16) &&
            static_cast<i16>(y) < static_cast<i16>(spriteYpos - 16 + 8 * mult))
        {
            u8 yFlipped = BIT(spriteFlags, 6) ? 8 * mult - 1 - (y - spriteYpos + 16) : (y - spriteYpos + 16); 

            u8 b1 = memory.Read(tileAddr + 2 * yFlipped);
            u8 b2 = memory.Read(tileAddr + 2 * yFlipped + 1);

            for (u8 x = 0; x < 8; x++) {
                u8 xFlipped = BIT(spriteFlags, 5) ? 7 - x : x;
                u8 colorIdx = (BIT(b2, 7 - xFlipped) << 1) | BIT(b1, 7 - xFlipped);
                if (colorIdx == 0) continue;

                u32 color = m_Colors[colors[colorIdx]];

                u8 finalXpos = spriteXpos + x - 8;

                if (static_cast<i16>(finalXpos) < 0 || finalXpos >= m_FrameWidth) continue;

                usize finalIdx = m_FrameWidth * static_cast<usize>(y) + static_cast<usize>(finalXpos);
                if (BIT(spriteFlags, 7) && m_Framebuffer[finalIdx] != colors[colors[0]]) continue;
                m_Framebuffer[finalIdx] = color;
            }
        }
    }
}