#include "core/platform.h"
#include "core/game.h"
#include <filesystem>
#include <iostream>

int main(int argc, char* argv[]) {
    try {
        // change working directory to the executable's directory
        std::filesystem::path exePath = std::filesystem::absolute(argv[0]);
        std::filesystem::path exeDir  = exePath.parent_path();
        std::filesystem::current_path(exeDir);

        std::cout << "Working directory set to: " 
                  << std::filesystem::current_path().string() << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Failed to set working directory: " << e.what() << std::endl;
    }

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