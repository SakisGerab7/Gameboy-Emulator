#pragma once

#include "Common.hpp"
#include "CPU.hpp"
#include "PPU.hpp"
#include "Memory.hpp"
#include "Timer.hpp"
#include "UI.hpp"
#include "Cartrige.hpp"

class Gameboy {
public:
    Gameboy(int argc, char **argv);
    ~Gameboy() = default;

    static Gameboy &Get() { return *s_Gameboy; }

    void Quit() { m_Quit = true; }

    Cartrige &GetCartrige() { return m_Cartrige; }
    CPU      &GetCPU()      { return m_CPU;      }
    PPU      &GetPPU()      { return m_PPU;      }
    Timer    &GetTimer()    { return m_Timer;    }
    UI       &GetUI()       { return m_UI;       }
    Memory   &GetMemory()   { return m_Memory;   }

    void Run();

private:
    static Gameboy *s_Gameboy;

private:
    Memory m_Memory;
    CPU m_CPU;
    PPU m_PPU;
    Cartrige m_Cartrige;
    Timer m_Timer;
    UI m_UI;

    u64 m_Ticks = 0;
    bool m_Quit = false;

    std::stringstream m_DebugMessage;

    std::mutex m_Mtx;
    std::condition_variable m_Cond;
};