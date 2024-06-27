#include "Gameboy.hpp"
#include "Log.hpp"

Gameboy *Gameboy::s_Gameboy = nullptr;

Gameboy::Gameboy(int argc, char **argv)
    : m_Cartrige(std::string(argv[1]))
{
    if (argc == 1 || argc > 3) {
        Log::Error("Wrong number of arguments!\n");
        return;
    }

    if (argc == 3) {
        std::string mainColor(argv[2]);
        if (mainColor.size() == 2 && mainColor[0] == '-') {
            switch (mainColor[1]) {
                case 'r': m_PPU.SetColors(0xFF0000); break;
                case 'g': m_PPU.SetColors(0x00FF00); break;
                case 'b': m_PPU.SetColors(0x0000FF); break;
                case 'y': m_PPU.SetColors(0xFFFF00); break;
                case 'c': m_PPU.SetColors(0x00FFFF); break;
                case 'm': m_PPU.SetColors(0xFF00FF); break;
                default: {
                    Log::Error("Wrong format of 2nd argument!\n(Must be -[Color] where [Color] is one of 'r', 'g', 'b', 'y', 'c', 'm')\n");
                    break;
                }
            }
        } else {
            Log::Error("Wrong format of 2nd argument!\n(Must be -[Color] where [Color] is one of 'r', 'g', 'b', 'y', 'c', 'm')\n");
        }
    } else {
        m_PPU.SetColors(0xFFFFFF);
    }

    s_Gameboy = this;
}

void Gameboy::Run() {
    std::future<void> cpuThread = std::async(std::launch::async, [this] {
        while (!m_Quit) {
            std::unique_lock<std::mutex> lock(m_Mtx);

            u8 cycles = m_CPU.Step();
            m_Ticks += cycles;

            m_PPU.Tick(cycles);
            m_Timer.Tick(cycles);

            // Debug
            if (m_Memory.Read(0xFF02) == 0x81) {
                char c = m_Memory.Read(0xFF01);
                m_DebugMessage << c;
                m_Memory.Write(0xFF02, 0);
            }

            m_Cond.notify_one();
        }
    });

    usize prevFrame = 0;
    while (true) {
        std::unique_lock<std::mutex> lock(m_Mtx);

        m_UI.HandleEvents();
        if (m_Quit) break;

        m_Cond.wait(lock, [this, prevFrame] { return prevFrame == m_PPU.GetCurrentFrame(); });

        m_UI.Update(m_PPU.GetFramebuffer());

        prevFrame++;
    }
}

int main(int argc, char **argv) {
    Gameboy(argc, argv).Run();
}