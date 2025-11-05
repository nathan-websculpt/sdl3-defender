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

    m_state.player->update(deltaTime, m_state.particles);

    // player projectiles
    auto& playerProjectiles = m_state.player->getProjectiles();        
    ColonyUpdateAndPrune::updateAndPruneProjectiles(playerProjectiles, deltaTime, m_gameHelpers);   

    keepPlayerInBounds(pb);

    // TODO: move out
    // opponents / projectiles
    for (auto opp_iter = m_state.opponents.begin(); opp_iter != m_state.opponents.end(); ) {
        auto& oppPtr = *opp_iter;
        if(!oppPtr) {
            opp_iter = m_state.opponents.erase(opp_iter);
            continue;
        }

        if(oppPtr->isAlive()) {
            SDL_FPoint playerPos = { pb.x, pb.y };
            oppPtr->update(deltaTime, playerPos, m_state.cameraX, m_state); // remember: world width is bigger than screen - height is same 
            ColonyUpdateAndPrune::updateAndPruneProjectiles(oppPtr->getProjectiles(), deltaTime, m_gameHelpers);
        }

        // new: check if opponent hit landscape
        SDL_FRect oppBounds = oppPtr->getBounds();
        float oppCenterX = oppBounds.x + oppBounds.w / 2.0f;
        float groundY = m_gameHelpers.getGroundYAt(oppCenterX);
        if (oppBounds.y + oppBounds.h >= groundY) {
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
                    return; // exit early if world health too low
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

    ColonyUpdateAndPrune::updateAndPruneParticles(m_state.particles, deltaTime);

    ColonyUpdateAndPrune::updateAndPruneHealthItems(m_state.healthItems, deltaTime, m_gameHelpers);

    // TODO: move with spawn opps
    // spawn health items
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

    checkCollisions();
    updateCamera();

    m_opponentSpawnTimer += deltaTime;
    if (m_opponentSpawnTimer >= OPPONENT_SPAWN_INTERVAL) {
        spawnOpponent();
        m_opponentSpawnTimer = 0.0f;
    }
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

    // 'ESC' key
    if (input.escape) {
        if (m_state.state == GameStateData::State::MENU) {
            m_state.running = false;
        } else if (m_state.state == GameStateData::State::PLAYING) {
            m_state.state = GameStateData::State::MENU;
        } else if (m_state.state == GameStateData::State::GAME_OVER) {
            if (m_state.waitingForHighScore) {
                std::string nameToSubmit = m_state.highScoreNameInput.empty() ? "ANON" : m_state.highScoreNameInput;
                m_highScores.submitHighScore(nameToSubmit, m_state);
                m_state.waitingForHighScore = false;
                m_state.state = GameStateData::State::MENU;
            } else {
                m_state.state = GameStateData::State::MENU;
            }
        } else if (m_state.state == GameStateData::State::HOW_TO_PLAY) {
             m_state.state = GameStateData::State::MENU;
        }
        return; // exit early, skipping any other input for this frame
    }
    // END: 'ESC' key

    if (m_state.state == GameStateData::State::MENU) {
        if (input.enter) {
            startNewGame();
        } else if (input.mouseClick) {
            int mx = input.mouseX;
            int my = input.mouseY;
            int w = globals.windowWidth;
            int h = globals.windowHeight;
            SDL_FRect playBtn = { (float)(w/2 - 100), (float)(h/2 - 60), 200, 50 };
            SDL_FRect howToPlayBtn = { (float)(w/2 - 100), (float)(h/2), 200, 50 };
            SDL_FRect exitBtn = { (float)(w/2 - 100), (float)(h/2 + 60), 200, 50 };
            if (mx >= playBtn.x && mx < playBtn.x + playBtn.w && my >= playBtn.y && my < playBtn.y + playBtn.h) {
                startNewGame();
            } else if (mx >= howToPlayBtn.x && mx < howToPlayBtn.x + howToPlayBtn.w && my >= howToPlayBtn.y && my < howToPlayBtn.y + howToPlayBtn.h) {
                m_state.state = GameStateData::State::HOW_TO_PLAY;
            } else if (mx >= exitBtn.x && mx < exitBtn.x + exitBtn.w && my >= exitBtn.y && my < exitBtn.y + exitBtn.h) {
                m_state.running = false;
            }
        }
    } else if (m_state.state == GameStateData::State::HOW_TO_PLAY) {
        if (input.enter || (input.mouseClick && input.mouseX > globals.windowWidth - 30 && input.mouseY < 30)) {
            m_state.state = GameStateData::State::MENU;
        }
    } else if (m_state.state == GameStateData::State::PLAYING) {
        if (m_state.player) {
            m_state.player->setSpeedBoost(input.boost);

            if (input.shoot && !m_prevShootState) { // current frame: pressed, previous frame: not pressed
                m_state.player->shoot();
            }
            // update the previous state for the next frame
            m_prevShootState = input.shoot;

            float speed = m_state.player->getSpeed();
            float dx = 0, dy = 0;
            if (input.moveLeft) { dx -= speed * deltaTime; m_state.player->setFacing(Direction::LEFT); }
            if (input.moveRight) { dx += speed * deltaTime; m_state.player->setFacing(Direction::RIGHT); }
            if (input.moveUp) dy -= speed * deltaTime;
            if (input.moveDown) dy += speed * deltaTime;
            m_state.player->moveBy(dx, dy);
        }    
    } else if (m_state.state == GameStateData::State::GAME_OVER) {
        if (m_state.waitingForHighScore) {
            if (input.charInputEvent) {
                char c = input.inputChar;
                if (m_state.highScoreNameInput.length() < 10 && (std::isalnum(static_cast<unsigned char>(c)) || c == ' ')) {
                    m_state.highScoreNameInput += c;
                }
            }           
            
            static float backspaceCooldown = 0.0f;
            const float BACKSPACE_DELAY = 0.1f; 
            if (input.backspacePressed) { 
                if (backspaceCooldown <= 0.0f && !m_state.highScoreNameInput.empty()) {
                    m_state.highScoreNameInput.pop_back();
                    backspaceCooldown = BACKSPACE_DELAY;
                } else {
                    backspaceCooldown = std::max(0.0f, backspaceCooldown - 1.0f/60.0f);
                }
            } else {
                backspaceCooldown = 0.0f;
            }

            // process enter/click for submission/cancellation
            if (input.enter) {
                // trim leading/trailing spaces
                std::string trimmedName = m_state.highScoreNameInput;
                if (!trimmedName.empty()) {
                    size_t start = trimmedName.find_first_not_of(" \t");
                    size_t end = trimmedName.find_last_not_of(" \t");
                    if (start != std::string::npos && end != std::string::npos) {
                        trimmedName = trimmedName.substr(start, end - start + 1);
                    } else {
                        trimmedName = "";
                    }
                }
                // use "ANON" if the name is empty after trimming or was empty initially
                if (trimmedName.empty()) {
                    trimmedName = "ANON";
                }
                m_highScores.submitHighScore(trimmedName, m_state);
                m_state.waitingForHighScore = false;
                m_state.state = GameStateData::State::MENU;
            } else if (input.mouseClick) {
                if (input.mouseX > globals.windowWidth - 30 && input.mouseY < 30) {
                    // use "ANON" if user cancels with 'X' and input was empty
                    std::string nameToSubmit = m_state.highScoreNameInput.empty() ? "ANON" : m_state.highScoreNameInput;
                    m_highScores.submitHighScore(nameToSubmit, m_state);
                    m_state.waitingForHighScore = false;
                    m_state.state = GameStateData::State::MENU;
                }
            }
        } else { // not waiting for high score - game over screen
            if (input.enter || (input.mouseClick && input.mouseX > globals.windowWidth - 30 && input.mouseY < 30)) {
                    m_state.state = GameStateData::State::MENU;
            }
        }
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
        // new: landscape stops beam
        float visualEndX = getBeamVisualEndX(startX, beamY, goingRight);

        for (auto& o : m_state.opponents) { // o is std::unique_ptr<BaseOpponent>&
            if (!o || !o->isAlive()) continue;

            // new: skip if opponent is beyond the beam's visual range (landscape stopped it)
            float oppCenterX = o->getBounds().x + o->getBounds().w / 2.0f;
            if (goingRight && oppCenterX > visualEndX) continue;
            if (!goingRight && oppCenterX < visualEndX) continue;

            if (rectsIntersect(o->getBounds(), pb)) {
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
            if (rectsIntersect(m_state.player->getBounds(), o->getBounds())) { 
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
                if (rectsIntersect(projBounds, playerBounds)) {
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
            if (rectsIntersect(m_state.player->getBounds(), item->getBounds())) {
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



// helpers
bool Game::rectsIntersect(const SDL_FRect& a, const SDL_FRect& b) const {
    return (a.x < b.x + b.w &&
            a.x + a.w > b.x &&
            a.y < b.y + b.h &&
            a.y + a.h > b.y);
}

float Game::getBeamVisualEndX(float startX, float beamY, bool goingRight) const {
    if (m_state.landscape.empty()) {
        return goingRight ? m_state.worldWidth : 0.0f;
    }

    const auto& land = m_state.landscape;

    if (goingRight) {
        // find first segment where x >= startX
        for (size_t i = 0; i < land.size() - 1; ++i) {
            float x0 = land[i].x;
            float x1 = land[i + 1].x;
            if (x1 < startX) continue;

            float y0 = land[i].y;
            float y1 = land[i + 1].y;

            // if beam is above both points, it passes through
            if (beamY < y0 && beamY < y1) {
                continue;
            }

            // if beam is below or at both, it hits at segment start
            if (beamY >= y0 && beamY >= y1) {
                return std::max(startX, x0);
            }

            // interpolate intersection
            // find X where the horizontal beam crosses the straight line segment between (x0,y0) and (x1,y1) 
            if (y1 != y0) {
                float t = (beamY - y0) / (y1 - y0);
                if (t >= 0.0f && t <= 1.0f) {
                    float intersectX = x0 + t * (x1 - x0);
                    if (intersectX >= startX) {
                        return intersectX;
                    }
                }
            }
        }
        return m_state.worldWidth;
    } else {
        // going left
        for (size_t i = land.size() - 1; i > 0; --i) {
            float x0 = land[i - 1].x;
            float x1 = land[i].x;
            if (x0 > startX) continue;

            float y0 = land[i - 1].y;
            float y1 = land[i].y;

            if (beamY < y0 && beamY < y1) {
                continue;
            }
            if (beamY >= y0 && beamY >= y1) {
                return std::min(startX, x1);
            }

            if (y1 != y0) {
                float t = (beamY - y0) / (y1 - y0);
                if (t >= 0.0f && t <= 1.0f) {
                    float intersectX = x0 + t * (x1 - x0);
                    if (intersectX <= startX) {
                        return intersectX;
                    }
                }
            }
        }
        return 0.0f;
    }
}

void Game::keepPlayerInBounds(SDL_FRect& pb) {
    // keeps player beneath HUD, above landscape, and in-world

    float desiredX = pb.x;
    float desiredY = pb.y;

    // left and right (world) boundaries
    if (desiredX < 0) desiredX = 0;
    if (desiredX + pb.w > m_state.worldWidth) desiredX = m_state.worldWidth - pb.w;

    // top (HUD) boundary
    desiredY = std::max(desiredY, static_cast<float>(Config::Game::HUD_HEIGHT));

    // landscape constraint (bottom)
    float playerBottomX = desiredX + pb.w / 2.0f; 
    float groundYAtPlayerX = m_gameHelpers.getGroundYAt(playerBottomX);
    float absoluteWorldBottom = m_state.worldHeight - pb.h; // absolute bottom of the world

    // player's bottom Y should not exceed the landscape height at their X position
    float effectiveGroundY = std::min(groundYAtPlayerX, absoluteWorldBottom);
    float maxAllowedY = effectiveGroundY - pb.h;

    // ensure desiredY is not below the calculated maximum
    desiredY = std::min(desiredY, maxAllowedY);
    
    if (pb.x != desiredX || pb.y != desiredY) {
        m_state.player->setPosition(desiredX, desiredY); // apply the final calculated position
    }
}
// END: helpers

// TODO: 
//      updateAndPrune methods go into sep place
//      rest of helpers separated
//      5 highscore methods
//      handleInput, update, and checkCollisions need to be broken up some