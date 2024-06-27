#include "Timer.hpp"
#include "Gameboy.hpp"

const u16 Timer::s_Dividers[] = { 1024, 16, 64, 256 };
    
void Timer::Tick(u8 cycles) {
    u8 clockSelect = m_TAC & 0b11;
    u8 timerEnable = BIT(m_TAC, 2);

    u16 divider = s_Dividers[clockSelect];
    bool timerUpdate = (m_DIV & divider) && (!((m_DIV + cycles) & divider));

    m_DIV += cycles;

    if (timerUpdate && timerEnable) {
        u16 temp = m_TIMA + cycles;
        m_TIMA = temp;
        if (temp > 0xFF) {
            m_TIMA = m_TMA;
            Gameboy::Get().GetCPU().RequestInterrupt(CPU::Interrupt::Timer);
        }
    }
}