#include "game.h"
#include <cstdlib>
#include <ctime>
#include <algorithm>
#include <fstream> //TODO
#include <sstream> //
#include <cctype>
#include "../core/config.h" // TODO
#include "../core/globals.h"
#include "../entities/health_item.h"

// TODO:
//remove
    // float worldWidth;  // world width goes beyond window
    // float worldHeight;

Game::Game()
    : m_state{}, m_gameHelpers(m_state.landscape) { // TODO: dims go to globals?
    srand((unsigned int)time(nullptr)); // TODO: use in main instead???
    m_state.worldWidth = Config::Game::WORLD_WIDTH;
    m_state.worldHeight = Config::Game::WORLD_HEIGHT; // TODO
    m_highScores.loadHighScores(m_state);
}

void Game::startNewGame() {
    m_mixer = SoundManager::getInstance().getMixerInstance();
    if (m_mixer) 
        SoundManager::getInstance().playSound(Config::Sounds::GAME_START, m_mixer);

    m_state.opponents.clear();
    m_state.particles.clear();
    m_state.healthItems.clear();
    m_state.cameraX = 0.0f;

    m_lastWindowHeight = globals.windowHeight;
    m_state.worldHeight = globals.windowHeight;
    float px = m_state.worldWidth / 2.0f - 40.0f;
    float py = globals.windowHeight / 2.0f - 24.0f;
    m_state.player = std::make_unique<Player>(px, py, 80, 48);

    m_state.state = GameStateData::State::PLAYING;
    m_state.worldHealth = m_state.maxWorldHealth;
    m_state.playerScore = 0;
    m_opponentSpawnTimer = 0.0f;

    m_playerHealthItemSpawnTimer = 0.0f;
    m_worldHealthItemSpawnTimer = 0.0f;

    setLandscape();
}

void Game::setLandscape() {
    m_state.landscape = {
        {0, m_state.worldHeight - 20},
        {m_state.worldWidth * 0.1f, m_state.worldHeight - 28},
        {m_state.worldWidth * 0.18f, m_state.worldHeight - 38},
        {m_state.worldWidth * 0.225f, m_state.worldHeight - 50},
        {m_state.worldWidth * 0.25f, m_state.worldHeight - 40},
        {m_state.worldWidth * 0.32f, m_state.worldHeight - 120},
        {m_state.worldWidth * 0.41f, m_state.worldHeight - 100},
        {m_state.worldWidth * 0.48f, m_state.worldHeight - 140},
        {m_state.worldWidth * 0.52f, m_state.worldHeight - 95},
        {m_state.worldWidth * 0.61f, m_state.worldHeight - 120},
        {m_state.worldWidth * 0.68f, m_state.worldHeight - 80},
        {m_state.worldWidth * 0.71f, m_state.worldHeight - 110},
        {m_state.worldWidth * 0.75f, m_state.worldHeight - 90},
        {m_state.worldWidth * 0.81f, m_state.worldHeight - 70},
        {m_state.worldWidth * 0.86f, m_state.worldHeight - 110},
        {m_state.worldWidth * 0.90f, m_state.worldHeight - 75},
        {m_state.worldWidth * 0.93f, m_state.worldHeight - 90},
        {m_state.worldWidth * 0.98f, m_state.worldHeight - 60},
        {m_state.worldWidth, m_state.worldHeight - 40}
    };
}

void Game::update(float deltaTime) {
    if (m_state.state != GameStateData::State::PLAYING) return;

    // TODO: unify with other todo
    // detect window resize for landscape
    if (globals.windowHeight != m_lastWindowHeight) {
        m_lastWindowHeight = globals.windowHeight;
        m_state.worldHeight = globals.windowHeight; // for consistency, but not necessary
        setLandscape();
    }

    SDL_FRect pb;
    if (m_state.player) {
        pb = m_state.player->getBounds();
    }

    updatePlayerAndProjectiles(deltaTime, pb);

    m_gameHelpers.keepPlayerInBounds(m_state.player, pb);

    if(!updateOpponents(deltaTime, pb)) // TODO: ^^^ reorder
        return; // this means that a bomb dropped the world health to 0 - game over

    ColonyUpdateAndPrune::updateAndPruneParticles(m_state.particles, deltaTime);

    ColonyUpdateAndPrune::updateAndPruneHealthItems(m_state.healthItems, deltaTime, m_gameHelpers);

    handleSpawnsAndTimers(deltaTime);

    checkCollisions();
    updateCamera();    
}

void Game::updateCamera() {
    if (!m_state.player) return;
    SDL_FRect pb = m_state.player->getBounds();
    float target = pb.x - globals.windowWidth / 2.0f;
    if (target < 0) target = 0;
    if (target > m_state.worldWidth - globals.windowWidth) target = m_state.worldWidth - globals.windowWidth;
    m_state.cameraX = target;
}

void Game::handleInput(const GameInput& input, float deltaTime) {
    if (input.quit) {
        m_state.running = false;
        return;
    }

    // 'ESC' key processes and returns early, skipping any other input for this frame
    if (input.escape) {
        handleEscapeKey();        
        return; // exit early
    }

    if (m_state.state == GameStateData::State::MENU) {
        handleInputMenu(input);
    } else if (m_state.state == GameStateData::State::HOW_TO_PLAY) {
        handleInputHowToPlay(input);
    } else if (m_state.state == GameStateData::State::PLAYING) {
        handleInputPlaying(input, deltaTime);
    } else if (m_state.state == GameStateData::State::GAME_OVER) {
        handleInputGameOver(input, deltaTime);
    }
}

void Game::checkCollisions() {
    if (!m_state.player) return;

    // collisions between player projectile and opponent
    auto& pp = m_state.player->getProjectiles();
    for (auto p_it = pp.begin(); p_it != pp.end(); ) {
        SDL_FRect pb = p_it->getBounds();
        bool projectileHit = false;

        // for horizontal beams, find visual end X
        float beamY = p_it->getSpawnY();
        float startX = p_it->getSpawnX();
        bool goingRight = (p_it->getVelocity().x > 0);
        // landscape stops beam
        float visualEndX = m_gameHelpers.getBeamVisualEndX(startX, beamY, goingRight);

        for (auto& o : m_state.opponents) { // o is std::unique_ptr<BaseOpponent>&
            if (!o || !o->isAlive()) continue;

            // skip if opponent is beyond the beam's visual range (landscape stopped it)
            float oppCenterX = o->getBounds().x + o->getBounds().w / 2.0f;
            if (goingRight && oppCenterX > visualEndX) continue;
            if (!goingRight && oppCenterX < visualEndX) continue;

            if (m_gameHelpers.rectsIntersect(o->getBounds(), pb)) {
                o->takeDamage(1);
                if (!o->isAlive()) {
                    m_state.playerScore += o->getScoreVal();
                    o->explode(m_state.particles);
                }
                projectileHit = true;
                break; // break inner loop
            }
        }
        if (projectileHit) {
            p_it = pp.erase(p_it); // erase using projectile iterator, assign returned iterator
        } else {
            ++p_it;
        }
    } 

    // player collisions with opponents and opponent projectiles
    if (m_state.player->isAlive()) {
        for (auto o_it = m_state.opponents.begin(); o_it != m_state.opponents.end(); ) {
            auto& o = *o_it;
            if (!o || !o->isAlive()) {
                 ++o_it; // skip dead opponents
                 continue;
            }

            // check player/opponent collision
            if (m_gameHelpers.rectsIntersect(m_state.player->getBounds(), o->getBounds())) { 
                m_state.player->takeDamage(1);
                o->explode(m_state.particles); 
                m_state.playerScore += o->getScoreVal();
                o_it = m_state.opponents.erase(o_it);
                if (!m_state.player->isAlive()) {
                    if (m_mixer) 
                            SoundManager::getInstance().playSound(Config::Sounds::GAME_OVER, m_mixer);
                            
                    m_state.state = GameStateData::State::GAME_OVER;
                    if (m_highScores.isHighScore(m_state)) {
                        m_state.highScoreIndex = m_highScores.getHighScoreIndex(m_state);
                        m_state.waitingForHighScore = true;
                        m_state.highScoreNameInput = ""; // initialize empty input
                    }
                    return; // exit early if player dies
                }
                continue; // skip projectile check if opponent was destroyed by collision
            }

            // check if opponent's projectiles hit player
            auto& op = o->getProjectiles(); 
            for (auto op_it = op.begin(); op_it != op.end(); ) {
                SDL_FRect projBounds = op_it->getBounds();
                SDL_FRect playerBounds = m_state.player->getBounds();

                // collision check ... projectile and player
                if (m_gameHelpers.rectsIntersect(projBounds, playerBounds)) {
                    m_state.player->takeDamage(1);
                    // erase the projectile that hit the player using the iterator
                    op_it = op.erase(op_it);
                    if (!m_state.player->isAlive()) {
                        if (m_mixer) 
                            SoundManager::getInstance().playSound(Config::Sounds::GAME_OVER, m_mixer);

                        m_state.state = GameStateData::State::GAME_OVER;
                        if (m_highScores.isHighScore(m_state)) {
                            m_state.highScoreIndex = m_highScores.getHighScoreIndex(m_state);
                            m_state.waitingForHighScore = true;
                            m_state.highScoreNameInput = ""; // initialize empty input
                        }
                        return; // exit early if player dies
                    }
                } else {
                    ++op_it;
                }
            }

            // increment opponent iterator only if the opponent itself wasn't erased in the player collision check
            if (m_state.state != GameStateData::State::GAME_OVER && o_it != m_state.opponents.end()) { // check if state changed or iterator became invalid due to erase
                ++o_it;
            }
            // ... if state is GAME_OVER or o_it was invalidated by erase in the inner loop, the outer loop will terminate
        }

        // player / health collisions (restores player or world health)
        for (auto it = m_state.healthItems.begin(); it != m_state.healthItems.end(); ) {
            auto& item = *it;
            if (!item || !item->isAlive() || item->isBlinking()) { // don't collide if blinking or dead
                ++it;
                continue;
            }
            if (m_gameHelpers.rectsIntersect(m_state.player->getBounds(), item->getBounds())) {
                if (item->getType() == HealthItemType::PLAYER) {
                    m_state.player->restoreHealth();
                } else if (item->getType() == HealthItemType::WORLD) {
                    m_state.worldHealth = m_state.maxWorldHealth;
                }
                it = m_state.healthItems.erase(it);
                continue;
            }
            ++it;
        }
    }
}

void Game::spawnOpponent() {
    int type = rand() % 3;
    float x = (float)(rand() % (int)(m_state.worldWidth - 50));
    float y = -50.0f;
    switch (type) {
        case 0: m_state.opponents.emplace(std::make_unique<BasicOpponent>(x, y, 40, 40)); break;
        case 1: m_state.opponents.emplace(std::make_unique<AggressiveOpponent>(x, y, 45, 45)); break;
        case 2: m_state.opponents.emplace(std::make_unique<SniperOpponent>(x, y, 35, 35)); break;
    }
}

void Game::spawnHealthItem(HealthItemType type) {
    float x = static_cast<float>(rand() % static_cast<int>(m_state.worldWidth - 50)); // random X within world
    float y = -50.0f; // start from top
    float w = 30.0f;
    float h = 30.0f;
    const std::string& textureKey = (type == HealthItemType::PLAYER) ? Config::Textures::PLAYER_HEALTH_ITEM : Config::Textures::WORLD_HEALTH_ITEM;
    m_state.healthItems.emplace(std::make_unique<HealthItem>(x, y, w, h, type, textureKey));
}

// TODO: 
//      updateAndPrune methods go into sep place
//      rest of helpers separated
//      5 highscore methods
//      handleInput, update, and checkCollisions need to be broken up some