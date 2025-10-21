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
    int worldHealth = 10;
    int playerHealth = 10;
    int playerScore = 0;
    float cameraX = 0.0f;
    float worldWidth;  // world width goes beyond window
    float worldHeight; // height depends on window resize

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

    void handleInput(const GameInput& input);
    void update(float deltaTime);
    const GameStateData& getState() const { return m_state; }
    GameStateData& getState() { return m_state; } 

    void startNewGame();
    void resetForMenu();
    void submitHighScore(const std::string& name);
    void loadHighScores();
    void saveHighScores();

private:
    GameStateData m_state;
    float m_opponentSpawnTimer = 0.0f;
    const float OPPONENT_SPAWN_INTERVAL = 2.0f;

    void spawnOpponent();
    void checkCollisions();
    void updateCamera();
    bool isHighScore(int score) const;
    int getHighScoreIndex(int score) const;
};














// enum class FontSize {
//     SMALL,
//     MEDIUM,
//     LARGE,
//     GRANDELOCO
// };

// enum class GameState {
//     MENU,
//     PLAYING,
//     GAME_OVER,
//     HOW_TO_PLAY
// };

// class Game {
// public:
//     Game();
//     ~Game();
//     void run();

// private:
//     SDL_Window* m_window;
//     SDL_Renderer* m_renderer;
//     int m_windowWidth = 0;
//     int m_windowHeight = 0;

//     std::unique_ptr<Player> m_player;
//     plf::colony<std::unique_ptr<BaseOpponent>> m_opponents;
//     plf::colony<Particle> m_particles;

//     GameState m_state;
//     bool m_running;
//     int m_worldHealth;
//     int m_playerHealth;
//     int m_playerScore;

//     float m_cameraX;
//     const float m_worldWidth; // world width goes beyond window
//     float m_worldHeight; // height depends on window resize

//     // high scores
//     struct HighScore {
//         std::string name;
//         int score;
//     };
//     std::vector<HighScore> m_highScores;
//     static const int MAX_HIGH_SCORES = 10;

//     void loadHighScores();
//     void saveHighScores();
//     bool isHighScore(int score) const;
//     int getHighScoreIndex(int score) const;
//     void addHighScore(const std::string& name, int score);
//     void handleGameOverScreen();
//     void handleHighScoreEntryScreen(int scoreIndex, int playerScore);
//     void renderGameOverScreen();
//     void renderHighScoreEntryScreen(int scoreIndex, int playerScore);
//     std::string getUserInput(int maxChars);
//     // END: high scores


//     void handleHowToPlayEvents();
//     void renderHowToPlayScreen();

//     void handleEvents();
//     void handleMenuEvents();
//     void update(float deltaTime);
//     // void render();
//     void renderMenu();
//     void spawnOpponent();
//     void checkCollisions();
//     void updateCamera();
//     void renderMinimap();
//     // void renderText(const char* text, int x, int y, const SDL_Color& color, FontSize size);
//     bool pointInRect(int x, int y, const SDL_FRect& rect);
//     void playGame();
// };