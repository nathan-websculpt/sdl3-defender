#pragma once
#include <vector>
#include <memory>
#include "managers/sound_manager.h"
#include "high_scores/high_scores.h"
#include "helpers_game/game_helper.h"
#include "helpers_game/colony_update_and_prune.h"
#include "../entities/player.h"
#include "../entities/health_item.h"
#include "../entities/opponents/base_opponent.h"
#include "../entities/opponents/basic_opponent.h"
#include "../entities/opponents/aggressive_opponent.h"
#include "../entities/opponents/sniper_opponent.h"
#include "../plf/plf_colony.h" 
#include "game_state_data.h"

class Game {
public:
    Game();
    ~Game() = default;

    void startNewGame();
    void update(float deltaTime);
    void handleInput(const GameInput& input, float deltaTime);
    const GameStateData& getState() const { return m_state; }
    GameStateData& getState() { return m_state; }    

private:
    MIX_Mixer* m_mixer;
    GameStateData m_state;
    HighScores m_highScores;
    GameHelper m_gameHelpers;

    float m_lastWindowHeight = 0.0f;
    float m_opponentSpawnTimer;
    const float OPPONENT_SPAWN_INTERVAL = 2.0f;
    bool m_prevShootState = false;

    float m_playerHealthItemSpawnTimer = 0.0f;
    float m_worldHealthItemSpawnTimer = 0.0f;
    const float PLAYER_HEALTH_ITEM_SPAWN_INTERVAL = 17.0f;
    const float WORLD_HEALTH_ITEM_SPAWN_INTERVAL = 36.0f;


    void setLandscape();
    void spawnOpponent();    
    void spawnHealthItem(HealthItemType type);  
    void updateCamera();

    // inputs
    void handleEscapeKey();
    void handleInputMenu(const GameInput& input);
    void handleInputHowToPlay(const GameInput& input);
    void handleInputPlaying(const GameInput& input, float deltaTime);
    void handleInputGameOver(const GameInput& input, float deltaTime);
    // END: inputs

    // updates
    void updatePlayerAndProjectiles(float deltaTime, const SDL_FRect& pb);
    bool updateOpponents(float deltaTime, const SDL_FRect& pb);
    void handleSpawnsAndTimers(float deltaTime);
    // END: updates
};