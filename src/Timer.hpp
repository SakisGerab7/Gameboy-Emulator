#pragma once

#include "Common.hpp"

class Timer {
public:
    void Tick(u8 cycles);

    u16 GetDIV() const { return m_DIV >> 8; }
    u8 GetTIMA() const { return m_TIMA; }
    u8 GetTMA() const { return m_TMA; }
    u8 GetTAC() const { return m_TAC; }

    void SetDIV(u16 val) { m_DIV = val; }
    void SetTIMA(u8 val) { m_TIMA = val; }
    void SetTMA(u8 val) { m_TMA = val; }
    void SetTAC(u8 val) { m_TAC = val; }

private:
    static const u16 s_Dividers[];

private:
    u16 m_DIV = 0;
    u8 m_TIMA = 0;
    u8 m_TMA = 0;
    u8 m_TAC = 0;
};
