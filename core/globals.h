#pragma once
#include <SDL3/SDL.h>

class Globals {
public:
    Globals() = default;

    SDL_Window* window = nullptr;
    SDL_Renderer* renderer = nullptr;
    
    int initialWindowWidth = 800;
    int initialWindowHeight = 600;
    int windowWidth = 800;
    int windowHeight = 600;
};

// declare the global instance using 'extern'
extern Globals globals;





// #pragma once

// class Globals {
// public:
//     Globals() = default;
//     int getInitialWindowWidth() { return initialWindowWidth; }
//     int getInitialWindowHeight() { return initialWindowHeight; }

//     int getWindowWidth() { return windowWidth; }
//     void setWindowWidth(int w) { windowWidth = w; }

//     int getWindowHeight() { return windowHeight; }
//     void setWindowHeight(int h) { windowHeight = h; }
    
// private:
//     int initialWindowWidth = 800;
//     int initialWindowHeight = 600;
//     int windowWidth = 800;
//     int windowHeight = 600;
// }