#include "game.h"
#include <cstdlib>
#include <ctime>
#include <algorithm>
#include "config.h"
#include "globals.h"
#include "../entities/health_item.h"

void Game::updatePlayerAndProjectiles(float deltaTime, const SDL_FRect& playerBounds) {
    if (!m_state.player) return;
    
    m_state.player->update(deltaTime, m_state.particles);

    // player projectiles
    auto& playerProjectiles = m_state.player->getProjectiles();        
    ColonyUpdateAndPrune::projectiles(playerProjectiles, deltaTime, m_gameHelpers); 
}

bool Game::updateOpponents(float deltaTime, const SDL_FRect& playerBounds) {
    for (auto opp_iter = m_state.opponents.begin(); opp_iter != m_state.opponents.end(); ) {
        auto& oppPtr = *opp_iter;
        if(!oppPtr) {
            opp_iter = m_state.opponents.erase(opp_iter);
            continue;
        }

        if(oppPtr->isAlive()) {
            SDL_FPoint playerPos = { playerBounds.x, playerBounds.y };
            oppPtr->update(deltaTime, playerPos, m_state.cameraX, m_state); // remember: world width is bigger than screen - height is same 
            ColonyUpdateAndPrune::projectiles(oppPtr->getProjectiles(), deltaTime, m_gameHelpers);
        }

        // check if opponent hit landscape
        SDL_FRect opplayerBoundsounds = oppPtr->getBounds();
        float oppCenterX = opplayerBoundsounds.x + opplayerBoundsounds.w / 2.0f;
        float groundY = m_gameHelpers.getGroundYAt(oppCenterX);
        if (opplayerBoundsounds.y + opplayerBoundsounds.h >= groundY) {
            BasicOpponent* b = dynamic_cast<BasicOpponent*>(oppPtr.get());
            if (b) { // only basic opponents damage world
                m_state.worldHealth--;
                if (m_state.worldHealth <= 0) {
                    // world health too low; game over
                    if (m_mixer) 
                        SoundManager::getInstance().playSound(Config::Sounds::GAME_OVER, m_mixer);

                        m_state.state = GameStateData::State::GAME_OVER;
                    if (m_highScores.isHighScore(m_state)) {
                        m_state.highScoreIndex = m_highScores.getHighScoreIndex(m_state);
                        m_state.waitingForHighScore = true;
                        m_state.highScoreNameInput.clear();
                    }
                    return false; // exit early if world health too low
                }
            }
            // opponent touched ground - explode
            oppPtr->explode(m_state.particles);
            opp_iter = m_state.opponents.erase(opp_iter);

            continue;
        }

        if (!oppPtr->isAlive()) {
            opp_iter = m_state.opponents.erase(opp_iter);
            continue;
        }

        ++opp_iter;
    }
    return true;
}

void Game::handleSpawnsAndTimers(float deltaTime) {
    m_playerHealthItemSpawnTimer += deltaTime;
    if (m_playerHealthItemSpawnTimer >= PLAYER_HEALTH_ITEM_SPAWN_INTERVAL) {
        spawnHealthItem(HealthItemType::PLAYER);
        m_playerHealthItemSpawnTimer = 0.0f;
    }

    m_worldHealthItemSpawnTimer += deltaTime;
    if (m_worldHealthItemSpawnTimer >= WORLD_HEALTH_ITEM_SPAWN_INTERVAL) {
        spawnHealthItem(HealthItemType::WORLD);
        m_worldHealthItemSpawnTimer = 0.0f;
    }

    m_opponentSpawnTimer += deltaTime;
    if (m_opponentSpawnTimer >= OPPONENT_SPAWN_INTERVAL) {
        spawnOpponent();
        m_opponentSpawnTimer = 0.0f;
    }
}

void Game::spawnOpponent() {
    int type = rand() % 3;
    float x = (float)(rand() % (int)(Config::Game::WORLD_WIDTH - 50));
    float y = -50.0f;
    switch (type) {
        case 0: m_state.opponents.emplace(std::make_unique<BasicOpponent>(x, y, 40, 40)); break;
        case 1: m_state.opponents.emplace(std::make_unique<AggressiveOpponent>(x, y, 45, 45)); break;
        case 2: m_state.opponents.emplace(std::make_unique<SniperOpponent>(x, y, 35, 35)); break;
    }
}

void Game::spawnHealthItem(HealthItemType type) {
    float x = static_cast<float>(rand() % static_cast<int>(Config::Game::WORLD_WIDTH - 50)); // random X within world
    float y = -50.0f; // start from top
    float w = 30.0f;
    float h = 30.0f;
    const std::string& textureKey = (type == HealthItemType::PLAYER) ? Config::Textures::PLAYER_HEALTH_ITEM : Config::Textures::WORLD_HEALTH_ITEM;
    m_state.healthItems.emplace(std::make_unique<HealthItem>(x, y, w, h, type, textureKey));
}