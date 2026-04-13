#ifndef __SDL_FRONTEND_H
#define __SDL_FRONTEND_H

#pragma once
#include <SDL2/SDL.h>
#include <cstdint>

class SDLFrontend {
private:
    SDL_Window*   window   = nullptr;
    SDL_Renderer* renderer = nullptr;
    SDL_Texture*  texTop   = nullptr;
    SDL_Texture*  texBot   = nullptr;

public:
    bool init();
    void shutdown();

    // returns false if window closes
    bool pollEvents(uint16_t& keyinput);

    // upload framebuffers and display them
    void presentFrame(const uint16_t* top, const uint16_t* bot);
};

#endif
