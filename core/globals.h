#pragma once
#include <SDL3/SDL.h>

class Globals {
public:
    Globals() = default;

    SDL_Window* window = nullptr; // TODO: no longer needed in anything but platform
    SDL_Renderer* renderer = nullptr;
    
    int windowWidth = 1000;
    int windowHeight = 800;
};

// declare the global instance using 'extern'
extern Globals globals;