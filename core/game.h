#pragma once
#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include "../entities/player.h"
#include "../entities/opponents/base_opponent.h"
#include "../entities/opponents/basic_opponent.h"
#include "../entities/opponents/aggressive_opponent.h"
#include "../entities/opponents/sniper_opponent.h"
#include <vector>
#include <memory>
#include "texture_manager.h"
#include "font_manager.h"
#include "../plf/plf_colony.h" 


enum class FontSize {
    SMALL,
    MEDIUM,
    LARGE,
    GRANDELOCO
};

enum class GameState {
    MENU,
    PLAYING,
    GAME_OVER,
    HOW_TO_PLAY
};

class Game {
public:
    Game();
    ~Game();
    void run();

private:
    SDL_Window* m_window;
    SDL_Renderer* m_renderer;
    int m_windowWidth = 0;
    int m_windowHeight = 0;

    std::unique_ptr<Player> m_player;
    plf::colony<std::unique_ptr<BaseOpponent>> m_opponents;
    plf::colony<Particle> m_particles;

    GameState m_state;
    bool m_running;
    int m_worldHealth;
    int m_playerHealth;
    int m_playerScore;

    float m_cameraX;
    const float m_worldWidth; // world width goes beyond window
    float m_worldHeight; // height depends on window resize

    // high scores
    struct HighScore {
        std::string name;
        int score;
    };
    std::vector<HighScore> m_highScores;
    static const int MAX_HIGH_SCORES = 10;

    void loadHighScores();
    void saveHighScores();
    bool isHighScore(int score) const;
    int getHighScoreIndex(int score) const;
    void addHighScore(const std::string& name, int score);
    void handleGameOverScreen();
    void handleHighScoreEntryScreen(int scoreIndex, int playerScore);
    void renderGameOverScreen();
    void renderHighScoreEntryScreen(int scoreIndex, int playerScore);
    std::string getUserInput(int maxChars);
    // END: high scores


    void handleHowToPlayEvents();
    void renderHowToPlayScreen();

    void handleEvents();
    void handleMenuEvents();
    void update(float deltaTime);
    void render();
    void renderMenu();
    void spawnOpponent();
    void checkCollisions();
    void updateCamera();
    void renderMinimap();
    void renderText(const char* text, int x, int y, const SDL_Color& color, FontSize size);
    bool pointInRect(int x, int y, const SDL_FRect& rect);
    void playGame();
};