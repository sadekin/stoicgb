#pragma once

#include "common.hpp"
#include <SDL.h>
#include <SDL_ttf.h>
#include "Bus.hpp"
#include "PPU.hpp"
#include "Joypad.hpp"

class GB;

class UI {
public:
    UI(Bus* b, PPU* p, GB* gb, Joypad* j);
    ~UI();

public:
    void handleEvents();
    void update();

private:
    void updateDebugWindow();
    void displayTile(SDL_Surface *surface, uint16_t tileIdx, int x, int y);

private:
    // Main window
    SDL_Window*   gbWindow   = nullptr;
    SDL_Renderer* gbRenderer = nullptr;
    SDL_Texture*  gbTexture  = nullptr;
    SDL_Surface*  gbScreen   = nullptr;

private:
    // For debugging
    SDL_Window*   debugWindow   = nullptr;
    SDL_Renderer* debugRenderer = nullptr;
    SDL_Texture*  debugTexture  = nullptr;
    SDL_Surface*  debugScreen   = nullptr;

private:
    // Devices connected to the UI
    GB* gameBoy;
    Bus*     bus;
    PPU*     ppu;
    Joypad*  joypad;
};