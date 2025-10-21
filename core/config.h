#pragma once
#include <string>

enum class FontSize {
    SMALL,
    MEDIUM,
    LARGE,
    GRANDELOCO
};

namespace Config {
    namespace Textures {
        const std::string PLAYER = "assets/defender.png";
        const std::string BASIC_OPPONENT = "assets/bang.png";
        const std::string AGGRESSIVE_OPPONENT = "assets/aggressive_opponent.png";
        const std::string SNIPER_OPPONENT = "assets/sniper_opponent.png";
    } 

    namespace Fonts {
        const std::string DEFAULT_FONT_FILE = "assets/Audiowide-Regular.ttf";
    }

    namespace Game {
        const int WORLD_WIDTH = 6400;
        const int WORLD_HEIGHT = 600;
        const std::string HIGH_SCORES_PATH = "resources/highscores.txt";
    }

}