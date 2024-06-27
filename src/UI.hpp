#pragma once

#include "Common.hpp"

struct Joypad {
    bool Action   = false;
    bool Directon = false;
    bool A        = false;
    bool B        = false;
    bool Start    = false;
    bool Select   = false;
    bool Up       = false;
    bool Down     = false;
    bool Right    = false;
    bool Left     = false;
};

class UI {
public:
    UI();
    ~UI();

    Joypad &GetJoypad() { return m_Joypad; }

    void HandleEvents();
    void Update(const std::vector<u32> &framebuffer);

private:
    usize m_PixelSize = 5;
    usize m_Spacing = 0;
    usize m_FrameWidth = 160;
    usize m_FrameHeight = 144;
    usize m_WindowWidth;
    usize m_WindowHeight;
    
    SDL_Window *m_Window;
    SDL_Renderer *m_Renderer;
    SDL_Texture *m_Texture;
    SDL_Surface *m_Surface;
    SDL_GameController *m_Controler = nullptr;

    Joypad m_Joypad;
};