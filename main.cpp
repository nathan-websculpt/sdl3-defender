#include "core/game.h"

// g++ -std=c++17 core/*.cpp entities/*.cpp entities/opponents/*.cpp main.cpp `pkg-config --cflags --libs sdl3` -lSDL3_image -lSDL3_ttf -o m

int main(int argc, char* argv[]) {
    Game game;
    game.run();
    return 0;
}