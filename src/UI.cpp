#include "UI.hpp"
#include "Gameboy.hpp"

UI::UI()
    : m_WindowWidth(m_FrameWidth * (m_PixelSize + m_Spacing)),
      m_WindowHeight(m_FrameHeight * (m_PixelSize + m_Spacing))
{
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_GAMECONTROLLER);

    m_Window = SDL_CreateWindow("Gameboy Emulator", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, m_WindowWidth, m_WindowHeight, 0);
    m_Renderer = SDL_CreateRenderer(m_Window, -1, 0);
    m_Surface = SDL_CreateRGBSurface(0, m_WindowWidth, m_WindowHeight, 32, 0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);
    m_Texture = SDL_CreateTexture(m_Renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, m_WindowWidth, m_WindowHeight);

    if (SDL_NumJoysticks() == 1) {
        m_Controler = SDL_GameControllerOpen(0);
    }
}

UI::~UI() {
    if (m_Controler) {
        SDL_GameControllerClose(m_Controler);
    }

    SDL_DestroyTexture(m_Texture);
    SDL_FreeSurface(m_Surface);
    SDL_DestroyRenderer(m_Renderer);
    SDL_DestroyWindow(m_Window);
    SDL_Quit();
}

void UI::HandleEvents() {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_QUIT || (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE)) {
            Gameboy::Get().Quit();
        }

        if (event.type == SDL_KEYDOWN) {
            switch (event.key.keysym.sym) {
                case SDLK_w: m_Joypad.Up     = true; break;
                case SDLK_a: m_Joypad.Left   = true; break;
                case SDLK_s: m_Joypad.Down   = true; break;
                case SDLK_d: m_Joypad.Right  = true; break;
                case SDLK_p: m_Joypad.A      = true; break;
                case SDLK_l: m_Joypad.B      = true; break;
                case SDLK_b: m_Joypad.Select = true; break;
                case SDLK_n: m_Joypad.Start  = true; break;
            }
        }

        if (event.type == SDL_KEYUP) {
            switch (event.key.keysym.sym) {
                case SDLK_w: m_Joypad.Up     = false; break;
                case SDLK_a: m_Joypad.Left   = false; break;
                case SDLK_s: m_Joypad.Down   = false; break;
                case SDLK_d: m_Joypad.Right  = false; break;
                case SDLK_p: m_Joypad.A      = false; break;
                case SDLK_l: m_Joypad.B      = false; break;
                case SDLK_b: m_Joypad.Select = false; break;
                case SDLK_n: m_Joypad.Start  = false; break;
            }
        }

        if (m_Controler && event.type == SDL_CONTROLLERBUTTONDOWN) {
            switch (event.cbutton.button) {
                case SDL_CONTROLLER_BUTTON_DPAD_UP:    m_Joypad.Up     = true; break;
                case SDL_CONTROLLER_BUTTON_DPAD_LEFT:  m_Joypad.Left   = true; break;
                case SDL_CONTROLLER_BUTTON_DPAD_DOWN:  m_Joypad.Down   = true; break;
                case SDL_CONTROLLER_BUTTON_DPAD_RIGHT: m_Joypad.Right  = true; break;
                case SDL_CONTROLLER_BUTTON_A:          m_Joypad.A      = true; break;
                case SDL_CONTROLLER_BUTTON_B:          m_Joypad.B      = true; break;
                case SDL_CONTROLLER_BUTTON_BACK:       m_Joypad.Select = true; break;
                case SDL_CONTROLLER_BUTTON_START:      m_Joypad.Start  = true; break;
            }
        }

        if (m_Controler && event.type == SDL_CONTROLLERBUTTONUP) {
            switch (event.cbutton.button) {
                case SDL_CONTROLLER_BUTTON_DPAD_UP:    m_Joypad.Up     = false; break;
                case SDL_CONTROLLER_BUTTON_DPAD_LEFT:  m_Joypad.Left   = false; break;
                case SDL_CONTROLLER_BUTTON_DPAD_DOWN:  m_Joypad.Down   = false; break;
                case SDL_CONTROLLER_BUTTON_DPAD_RIGHT: m_Joypad.Right  = false; break;
                case SDL_CONTROLLER_BUTTON_A:          m_Joypad.A      = false; break;
                case SDL_CONTROLLER_BUTTON_B:          m_Joypad.B      = false; break;
                case SDL_CONTROLLER_BUTTON_BACK:       m_Joypad.Select = false; break;
                case SDL_CONTROLLER_BUTTON_START:      m_Joypad.Start  = false; break;
            }
        }
    }
}

void UI::Update(const std::vector<u32> &framebuffer) {
    for (usize y = 0; y < m_FrameHeight; y++) {
        for (usize x = 0; x < m_FrameWidth; x++) {
            SDL_Rect rect;
            rect.x = x * (m_PixelSize + m_Spacing);
            rect.y = y * (m_PixelSize + m_Spacing);
            rect.w = m_PixelSize;
            rect.h = m_PixelSize;
            SDL_FillRect(m_Surface, &rect, framebuffer[m_FrameWidth * y + x]);
        }
    }

    SDL_UpdateTexture(m_Texture, nullptr, m_Surface->pixels, m_Surface->pitch);
    SDL_RenderClear(m_Renderer);
    SDL_RenderCopy(m_Renderer, m_Texture, nullptr, nullptr);
    SDL_RenderPresent(m_Renderer);
}