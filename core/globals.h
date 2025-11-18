#pragma once
#include <SDL3/SDL.h>

class Globals {
public:
    Globals() = default;

    SDL_Window* window = nullptr;
    SDL_Renderer* renderer = nullptr;
    
    int windowWidth = 1000;
    int windowHeight = 800;
};

// declare the global instance using 'extern'
extern Globals globals;