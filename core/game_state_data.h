#pragma once

// TODO: cleanup includdes
#include <vector>
#include <memory>
#include "../core/managers/sound_manager.h"
#include "../core/high_scores/high_scores.h"
#include "../entities/player.h"
#include "../entities/health_item.h"
#include "../entities/opponents/base_opponent.h"
#include "../entities/opponents/basic_opponent.h"
#include "../entities/opponents/aggressive_opponent.h"
#include "../entities/opponents/sniper_opponent.h"
#include "../plf/plf_colony.h" 

struct GameStateData {
    enum class State {
        MENU,
        PLAYING,
        GAME_OVER,
        HOW_TO_PLAY
    };

    State state = State::MENU;
    bool running = true;

    // high score
    struct HighScore {
        std::string name;
        int score;
    };
    std::vector<HighScore> highScores;
    static const int MAX_HIGH_SCORES = 10;

    // game state
    int maxWorldHealth = 10;
    int worldHealth;
    int playerScore;
    float cameraX;

    // entities
    std::unique_ptr<Player> player;
    plf::colony<Particle> particles;
    plf::colony<std::unique_ptr<BaseOpponent>> opponents;
    plf::colony<std::unique_ptr<HealthItem>> healthItems;

    // ui state (needed for menus)
    bool waitingForHighScore = false;
    int highScoreIndex = -1;
    std::string highScoreNameInput;

    std::vector<SDL_FPoint> landscape;
};

struct GameInput {
    bool moveLeft = false;
    bool moveRight = false;
    bool moveUp = false;
    bool moveDown = false;
    bool shoot = false;
    bool boost = false;
    bool quit = false;
    bool escape = false;
    bool enter = false;
    bool mouseClick = false;
    int mouseX = 0;
    int mouseY = 0;

    // for text input
    bool charInputEvent = false; // flag indicating a character input event occurred
    char inputChar = 0;
    bool backspacePressed = false;
};