#include "sdl_frontend.h"
#include <SDL.h>
#include <SDL_render.h>
#include <SDL_video.h>
#include <iostream>

bool SDLFrontend::init(){
    if (SDL_Init(SDL_INIT_VIDEO) < 0){
        std::cerr << "SDL_Init failed: " << SDL_GetError() << "\n";
        return false;
    }

    // two 256x192 screens stacked vertically
    window = SDL_CreateWindow("NDS Emulator", 
                              SDL_WINDOWPOS_CENTERED, 
                              SDL_WINDOWPOS_CENTERED, 
                              256 * 2, 384 * 2, 
                              SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
    if (!window) {
        std::cerr << "Window creation failed...\n";
        return false;
    }

    renderer = SDL_CreateRenderer(window, -1,
                    SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!renderer){
        std::cerr << "Renderer creation failed...\n";
        return false; 
    }

    SDL_RenderSetLogicalSize(renderer, 256, 384);

    // BGR555 = SDL_PIXELFORMAT_BGR555
    // Each pixel is 2 bytes, 256 pixels wide
    texTop = SDL_CreateTexture(renderer,
        SDL_PIXELFORMAT_BGR555, SDL_TEXTUREACCESS_STREAMING, 256, 192);
    texBot = SDL_CreateTexture(renderer,
        SDL_PIXELFORMAT_BGR555, SDL_TEXTUREACCESS_STREAMING, 256, 192);

    if (!texTop || !texBot) {
        std::cerr << "Texture creation failed\n"; return false;
    }
    return true;
}

void SDLFrontend::shutdown(){
    if (texTop)     SDL_DestroyTexture(texTop);
    if (texBot)     SDL_DestroyTexture(texBot);
    if (renderer)   SDL_DestroyRenderer(renderer);
    if (window)     SDL_DestroyWindow(window);
    SDL_Quit(); 
}

bool SDLFrontend::pollEvents(uint16_t& keyinput) {
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
        if (e.type == SDL_QUIT) return false;
    }

    // KEYINPUT is active low, bit clear means button pressed
    keyinput = 0x3FF; // all released
    const uint8_t* keys = SDL_GetKeyboardState(nullptr);

    // DS button mapping
    if (keys[SDL_SCANCODE_X])      keyinput &= ~(1 << 0);  // A
    if (keys[SDL_SCANCODE_Z])      keyinput &= ~(1 << 1);  // B
    if (keys[SDL_SCANCODE_S])      keyinput &= ~(1 << 2);  // Select
    if (keys[SDL_SCANCODE_RETURN]) keyinput &= ~(1 << 3);  // Start
    if (keys[SDL_SCANCODE_RIGHT])  keyinput &= ~(1 << 4);  // Right
    if (keys[SDL_SCANCODE_LEFT])   keyinput &= ~(1 << 5);  // Left
    if (keys[SDL_SCANCODE_UP])     keyinput &= ~(1 << 6);  // Up
    if (keys[SDL_SCANCODE_DOWN])   keyinput &= ~(1 << 7);  // Down
    if (keys[SDL_SCANCODE_A])      keyinput &= ~(1 << 8);  // R
    if (keys[SDL_SCANCODE_Q])      keyinput &= ~(1 << 9);  // L

    return true;
}

void SDLFrontend::presentFrame(const uint16_t* top, const uint16_t* bot) {
    // Upload pixel data — pitch is bytes per row = 256 pixels * 2 bytes
    SDL_UpdateTexture(texTop, nullptr, top, 256 * 2);
    SDL_UpdateTexture(texBot, nullptr, bot, 256 * 2);

    SDL_RenderClear(renderer);

    // Top screen occupies the top half, bottom screen the bottom half
    SDL_Rect topRect = {0,   0, 256, 192};
    SDL_Rect botRect = {0, 192, 256, 192};
    SDL_RenderCopy(renderer, texTop, nullptr, &topRect);
    SDL_RenderCopy(renderer, texBot, nullptr, &botRect);

    SDL_RenderPresent(renderer);
}
