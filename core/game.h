#pragma once
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
    GameStateData m_state;
    float m_lastWindowHeight = 0.0f;
    float m_opponentSpawnTimer;
    const float OPPONENT_SPAWN_INTERVAL = 2.0f;
    bool m_prevShootState = false;

    float m_playerHealthItemSpawnTimer = 0.0f;
    float m_worldHealthItemSpawnTimer = 0.0f;
    const float PLAYER_HEALTH_ITEM_SPAWN_INTERVAL = 17.0f;
    const float WORLD_HEALTH_ITEM_SPAWN_INTERVAL = 36.0f;

    MIX_Mixer* m_mixer;

    void setLandscape();

    void spawnHealthItem(HealthItemType type);
    void updateAndPruneHealthItems(float deltaTime);

    void updateCamera();
    void checkCollisions();
    void spawnOpponent();    

    // helpers
    bool rectsIntersect(const SDL_FRect& a, const SDL_FRect& b) const;
    bool isOutOfWorld(const SDL_FRect& r, float mx = 100.0f, float my = 100.0f) const;
    void updateAndPruneProjectiles(plf::colony<Projectile>& proj, float deltaTime);
    void updateAndPruneParticles(float deltaTime);
    float getGroundYAt(float x) const; // for landscape
    float getBeamVisualEndX(float startX, float beamY, bool goingRight) const; // landscape stops player's beam
    void keepPlayerInBounds(SDL_FRect& pb);

    HighScores m_highScores;
};