#include "game.h"
#include <cstdlib>
#include <ctime>
#include <algorithm>
#include <fstream>
#include <sstream>
#include <cctype>
#include "../core/config.h"
#include "../entities/health_item.h"

Game::Game()
    : m_state{} {
    srand((unsigned int)time(nullptr));
    m_state.worldWidth = Config::Game::WORLD_WIDTH;
    m_state.worldHeight = Config::Game::WORLD_HEIGHT;
    loadHighScores();
}

void Game::startNewGame() {
    m_state.opponents.clear();
    m_state.particles.clear();
    m_state.healthItems.clear();
    m_state.cameraX = 0.0f;

    m_lastWindowHeight = m_state.screenHeight;
    float px = m_state.worldWidth / 2.0f - 40.0f;
    float py = m_state.screenHeight / 2.0f - 24.0f;
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

    // detect window resize for landscape
    if (m_state.screenHeight != m_lastWindowHeight) {
        m_lastWindowHeight = m_state.screenHeight;
        m_state.worldHeight = m_state.screenHeight; // for consistency, but not necessary
        setLandscape();
    }

    SDL_FRect pb;
    if (m_state.player) {
        pb = m_state.player->getBounds();
    }

    m_state.player->update(deltaTime, m_state.particles);

    // player projectiles
    auto& playerProjectiles = m_state.player->getProjectiles();        
    updateAndPruneProjectiles(playerProjectiles, deltaTime);   

    // TODO: place after all other checks
    // new: top boundary (for HUD)
    if (pb.y < static_cast<float>(Config::Game::HUD_HEIGHT)) {
        m_state.player->setPosition(pb.x, static_cast<float>(Config::Game::HUD_HEIGHT));
    } // TODO: unify with old code below

    // keep player in world and on-screen
    float cx = pb.x;
    float cy = pb.y;
    if (cx < 0) cx = 0;
    if (cy < 0) cy = 0;
    if (cx + pb.w > m_state.worldWidth) cx = m_state.worldWidth - pb.w;
    if (cy + pb.h > m_state.worldHeight) cy = m_state.worldHeight - pb.h;
    if (cx != pb.x || cy != pb.y) m_state.player->setPosition(cx, cy);

    // TODO: ^^^ really no longer needs to check the bottom of the world
    // new: player can't go below the landscape
    float groundY = getGroundYAt(pb.x + pb.w / 2.0f);
    float playerBottom = pb.y + pb.h;
    if (playerBottom > groundY) {
        m_state.player->setPosition(pb.x, groundY - pb.h);
    }

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
            updateAndPruneProjectiles(oppPtr->getProjectiles(), deltaTime);
        }

        // new: check if opponent hit landscape
        SDL_FRect oppBounds = oppPtr->getBounds();
        float oppCenterX = oppBounds.x + oppBounds.w / 2.0f;
        float groundY = getGroundYAt(oppCenterX);
        if (oppBounds.y + oppBounds.h >= groundY) {
            BasicOpponent* b = dynamic_cast<BasicOpponent*>(oppPtr.get());
            if (b) { // only basic opponents damage world
                m_state.worldHealth--;
                if (m_state.worldHealth <= 0) {
                    // world health too low; game over
                    m_state.state = GameStateData::State::GAME_OVER;
                    if (isHighScore(m_state.playerScore)) {
                        m_state.highScoreIndex = getHighScoreIndex(m_state.playerScore);
                        m_state.waitingForHighScore = true;
                        m_state.highScoreNameInput.clear();
                    }
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

    updateAndPruneParticles(deltaTime);

    updateAndPruneHealthItems(deltaTime);

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
    float target = pb.x - m_state.screenWidth / 2.0f;
    if (target < 0) target = 0;
    if (target > m_state.worldWidth - m_state.screenWidth) target = m_state.worldWidth - m_state.screenWidth;
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
                submitHighScore(nameToSubmit);
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
            int w = m_state.screenWidth;
            int h = m_state.screenHeight;
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
        if (input.enter || (input.mouseClick && input.mouseX > m_state.screenWidth - 30 && input.mouseY < 30)) {
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
                submitHighScore(trimmedName);
                m_state.waitingForHighScore = false;
                m_state.state = GameStateData::State::MENU;
            } else if (input.mouseClick) {
                if (input.mouseX > m_state.screenWidth - 30 && input.mouseY < 30) {
                    // use "ANON" if user cancels with 'X' and input was empty
                    std::string nameToSubmit = m_state.highScoreNameInput.empty() ? "ANON" : m_state.highScoreNameInput;
                    submitHighScore(nameToSubmit);
                    m_state.waitingForHighScore = false;
                    m_state.state = GameStateData::State::MENU;
                }
            }
        } else { // not waiting for high score - game over screen
            if (input.enter || (input.mouseClick && input.mouseX > m_state.screenWidth - 30 && input.mouseY < 30)) {
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
                    m_state.state = GameStateData::State::GAME_OVER;
                    if (isHighScore(m_state.playerScore)) {
                        m_state.highScoreIndex = getHighScoreIndex(m_state.playerScore);
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
                        m_state.state = GameStateData::State::GAME_OVER;
                        if (isHighScore(m_state.playerScore)) {
                            m_state.highScoreIndex = getHighScoreIndex(m_state.playerScore);
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

// handle high scores
void Game::loadHighScores() {
    m_state.highScores.clear();
    std::ifstream file(Config::Game::HIGH_SCORES_PATH);
    if (file.is_open()) {
        std::string line;
        while (std::getline(file, line) && m_state.highScores.size() < m_state.MAX_HIGH_SCORES) {
            std::istringstream iss(line);
            std::string name;
            int score;
            if (iss >> name >> score) { // format: "NAME SCORE"
                 GameStateData::HighScore entry;
                 entry.name = name;
                 entry.score = score;
                 m_state.highScores.push_back(entry);
            }
        }
        file.close();
    }

    // list is sorted (highest first) and capped at MAX_HIGH_SCORES
    std::sort(m_state.highScores.begin(), m_state.highScores.end(),
              [](const GameStateData::HighScore& a, const GameStateData::HighScore& b) { return a.score > b.score; });
    if (m_state.highScores.size() > m_state.MAX_HIGH_SCORES) {
        m_state.highScores.resize(m_state.MAX_HIGH_SCORES);
    }
}

void Game::saveHighScores() {
    std::ofstream file(Config::Game::HIGH_SCORES_PATH);
    if (file.is_open()) {
        for (const auto& entry : m_state.highScores) {
            file << entry.name << " " << entry.score << "\n"; //Format: "NAME SCORE"
        }
        file.close();
        SDL_Log("High scores saved.");
    } else {
        SDL_Log("Warning: Could not save high scores to file.");
    }
}

bool Game::isHighScore(int score) const {
    return m_state.highScores.size() < m_state.MAX_HIGH_SCORES || score > m_state.highScores.back().score;
}

int Game::getHighScoreIndex(int score) const {
    // finds the index where new score should be inserted (0 is highest)
    for (size_t i = 0; i < m_state.highScores.size(); ++i) {
        if (score > m_state.highScores[i].score) {
            return static_cast<int>(i);
        }
    }
    //if loop finishes without returning, the score is not higher than any existing score, but also need to check if the list is not full yet
    if (m_state.highScores.size() < m_state.MAX_HIGH_SCORES) {
        return static_cast<int>(m_state.highScores.size());
    }

    return -1;
}

void Game::submitHighScore(const std::string& name) {
    int index = getHighScoreIndex(m_state.playerScore);
    if (index != -1) {
        GameStateData::HighScore newEntry;
        newEntry.name = name.empty() ? "ANON" : name;
        newEntry.score = m_state.playerScore;
        m_state.highScores.insert(m_state.highScores.begin() + index, newEntry);
        if (m_state.highScores.size() > m_state.MAX_HIGH_SCORES) {
            m_state.highScores.pop_back();
        }
        saveHighScores();
    }
}
// END: handle high scores

// helpers
bool Game::rectsIntersect(const SDL_FRect& a, const SDL_FRect& b) const {
    return (a.x < b.x + b.w &&
            a.x + a.w > b.x &&
            a.y < b.y + b.h &&
            a.y + a.h > b.y);
}

bool Game::isOutOfWorld(const SDL_FRect& r, float mx, float my) const {
    return (r.x + r.w < -mx || r.x > m_state.worldWidth + mx ||
            r.y + r.h < -my || r.y > m_state.worldHeight + my);
}

void Game::updateAndPruneProjectiles(plf::colony<Projectile>& projectiles, float deltaTime) {
    for (auto it = projectiles.begin(); it != projectiles.end(); ) {
        it->update(deltaTime);
        SDL_FRect b = it->getBounds();
        
        if (isOutOfWorld(b, 0.0f, 0.0f)) {
            it = projectiles.erase(it);
            continue;
        }

        // TODO: this part could be restricted to !it->isHorizontal because this is just for opponent projectiles
        float projCenterX = b.x + b.w / 2.0f;
        float groundY = getGroundYAt(projCenterX);
        float projBottom = b.y + b.h;

        // if projectile is at or below ground - remove it
        if (projBottom >= groundY) {
            it = projectiles.erase(it);
            continue;
        }

        ++it;
    }
}

void Game::updateAndPruneParticles(float deltaTime) {
    for (auto it = m_state.particles.begin(); it != m_state.particles.end(); ) {
        it->update(deltaTime);
        if (!it->isAlive()) 
            it = m_state.particles.erase(it);
        else 
            ++it;
    }
}

float Game::getGroundYAt(float x) const {
    const auto& land = m_state.landscape;
    if (land.empty()) return m_state.worldHeight;

    // clamp x to landscape bounds
    if (x <= land.front().x) return land.front().y;
    if (x >= land.back().x) return land.back().y;

    for (size_t i = 0; i < land.size() - 1; ++i) {
        if (x >= land[i].x && x <= land[i + 1].x) {
            // linear interpolation between land[i] and land[i+1]
            float t = (x - land[i].x) / (land[i + 1].x - land[i].x);
            return land[i].y + t * (land[i + 1].y - land[i].y);
        }
    }
    return land.back().y; // fallback
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

void Game::updateAndPruneHealthItems(float deltaTime) {
    for (auto it = m_state.healthItems.begin(); it != m_state.healthItems.end(); ) {
        auto& item = *it;
        if (!item) {
             ++it;
             continue;
        }
        item->update(deltaTime);

        // check if item hit the landscape
        float groundY = getGroundYAt(item->getBounds().x + item->getBounds().w / 2.0f);
        float itemBottom = item->getBounds().y + item->getBounds().h;
        if (itemBottom >= groundY && !item->isBlinking()) {
            item->startBlinking();
        }

        // remove dead items (finished blinking)
        if (!item->isAlive()) {
            it = m_state.healthItems.erase(it);
            continue;
        }
        ++it;
    }
}
// END: helpers