// #include "core/game.h"

// // g++ -std=c++17 core/*.cpp entities/*.cpp entities/opponents/*.cpp main.cpp `pkg-config --cflags --libs sdl3` -lSDL3_image -lSDL3_ttf -o m

// int main(int argc, char* argv[]) {
//     Game game;
//     game.run();
//     return 0;
// }


#include "core/platform.h"
#include "core/game.h"

int main(int argc, char* argv[]) {
    Game sim;
    Platform platform;

    if (!platform.initialize()) {
        return -1;
    }

    sim.loadHighScores();
    platform.run(sim);
    sim.saveHighScores();

    platform.shutdown();
    return 0;
}