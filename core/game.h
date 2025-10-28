#pragma once
#include <vector>
#include <memory>
#include "../entities/player.h"
#include "../entities/opponents/base_opponent.h"
#include "../entities/opponents/basic_opponent.h"
#include "../entities/opponents/aggressive_opponent.h"
#include "../entities/opponents/sniper_opponent.h"
#include "../plf/plf_colony.h" 

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
    float worldWidth;  // world width goes beyond window
    float worldHeight; // height depends on window size
    float screenWidth; 
    float screenHeight;

    // entities
    std::unique_ptr<Player> player;
    plf::colony<std::unique_ptr<BaseOpponent>> opponents;
    plf::colony<Particle> particles;

    // ui state (needed for menus)
    bool waitingForHighScore = false;
    int highScoreIndex = -1;
    std::string highScoreNameInput;
};

class Game {
public:
    Game();
    ~Game() = default;

    void startNewGame();
    void update(float deltaTime);
    void handleInput(const GameInput& input, float deltaTime);
    const GameStateData& getState() const { return m_state; }
    GameStateData& getState() { return m_state; } 

    void submitHighScore(const std::string& name);
    void loadHighScores();
    void saveHighScores();

private:
    GameStateData m_state;
    float m_opponentSpawnTimer;
    const float OPPONENT_SPAWN_INTERVAL = 2.0f;
    bool m_prevShootState = false;

    void updateCamera();
    void checkCollisions();
    void spawnOpponent();

    bool isHighScore(int score) const;
    int getHighScoreIndex(int score) const;

    // helpers
    bool rectsIntersect(const SDL_FRect& a, const SDL_FRect& b) const;
    bool isOutOfWorld(const SDL_FRect& r, float mx = 100.0f, float my = 100.0f) const;
    void updateAndPruneProjectiles(plf::colony<Projectile>& proj, float deltaTime);
    void updateAndPruneParticles(float deltaTime);

};