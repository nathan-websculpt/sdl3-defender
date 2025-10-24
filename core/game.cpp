#include "game.h"
#include <cstdlib>
#include <ctime>
#include <algorithm>
#include <fstream>
#include <sstream>
#include <cctype>
#include "../core/config.h"

Game::Game()
    : m_state{} {
    srand((unsigned int)time(nullptr));
    m_state.worldWidth = Config::Game::WORLD_WIDTH;
    m_state.worldHeight = Config::Game::WORLD_HEIGHT;
    loadHighScores();
}

void Game::handleInput(const GameInput& input) {
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
        if (input.escape || input.enter || input.mouseClick) {
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
            float delta = 0.016f;
            float dx = 0, dy = 0;
            if (input.moveLeft) { dx -= speed * delta; m_state.player->setFacing(Direction::LEFT); }
            if (input.moveRight) { dx += speed * delta; m_state.player->setFacing(Direction::RIGHT); }
            if (input.moveUp) dy -= speed * delta;
            if (input.moveDown) dy += speed * delta;
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

            // process enter/esc/click for submission/cancellation
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
            } else if (input.escape) { // TODO: handling 'esc' above this, could possibly remove
                // use "ANON" if user cancels with escape and input was empty
                std::string nameToSubmit = m_state.highScoreNameInput.empty() ? "ANON" : m_state.highScoreNameInput;
                submitHighScore(nameToSubmit);
                m_state.waitingForHighScore = false;
                m_state.state = GameStateData::State::MENU;
            } else if (input.mouseClick) {
                // TODO:
                // place 'X' button at top-right (20x20)
                if (input.mouseX > m_state.screenWidth - 30 && input.mouseY < 30) {
                    // use "ANON" if user cancels with 'X' and input was empty
                    std::string nameToSubmit = m_state.highScoreNameInput.empty() ? "ANON" : m_state.highScoreNameInput;
                    submitHighScore(nameToSubmit);
                    m_state.waitingForHighScore = false;
                    m_state.state = GameStateData::State::MENU;
                }
            }
        } else { // not waiting for high score
            if (input.escape || input.enter || input.mouseClick) {
                if (input.mouseClick && input.mouseX > m_state.screenWidth - 30 && input.mouseY < 30) {
                    m_state.state = GameStateData::State::MENU;
                } else {
                    m_state.state = GameStateData::State::MENU; // TODO:
                }
            }
        }
    }
}

void Game::startNewGame() {
    m_state.opponents.clear();
    m_state.particles.clear();
    m_state.cameraX = 0.0f;
    float px = m_state.worldWidth / 2.0f - 40.0f;
    float py = m_state.screenHeight / 2.0f - 24.0f;
    m_state.player = std::make_unique<Player>(px, py, 80, 48);
    m_state.state = GameStateData::State::PLAYING;
    m_state.worldHealth = 10;
    m_state.playerScore = 0;
    m_opponentSpawnTimer = 0.0f;
}

void Game::resetForMenu() {
    m_state.player.reset();
    m_state.opponents.clear();
    m_state.particles.clear();
    m_state.state = GameStateData::State::MENU;
}

void Game::update(float deltaTime) {
    if (m_state.state != GameStateData::State::PLAYING) return;

    if (m_state.player) {
        m_state.player->update(deltaTime, m_state.particles);
        auto& playerProjectiles = m_state.player->getProjectiles();

        for (auto& p : playerProjectiles) { 
            p.update(deltaTime);
        }

        SDL_FRect pb = m_state.player->getBounds();

        float cx = pb.x;
        float cy = pb.y;
        if (cx < 0) cx = 0;
        if (cy < 0) cy = 0;
        if (cx + pb.w > m_state.worldWidth) cx = m_state.worldWidth - pb.w;
        if (cy + pb.h > m_state.worldHeight) cy = m_state.worldHeight - pb.h;
        if (cx != pb.x || cy != pb.y) m_state.player->setPosition(cx, cy);
    }

    // update opponents
    for (auto& o : m_state.opponents) { // o is std::unique_ptr<BaseOpponent>&
        if (!o || !o->isAlive()) continue; 
        SDL_FPoint playerPos = { m_state.player->getBounds().x, m_state.player->getBounds().y };
        o->update(deltaTime, playerPos, m_state.cameraX, m_state); // remember: world width is bigger than screen - height is same 
    }

    // erase opponents
    for (auto o_it = m_state.opponents.begin(); o_it != m_state.opponents.end(); ) {
        if (!(*o_it) || (*o_it)->getBounds().y > m_state.worldHeight) {
            BasicOpponent* b = dynamic_cast<BasicOpponent*>((*o_it).get()); //get() to get raw pointer for cast
            if (b) { // only the basic opponents damage world
                m_state.worldHealth--;
                if (m_state.worldHealth <= 0) {
                    m_state.state = GameStateData::State::GAME_OVER;
                    if (isHighScore(m_state.playerScore)) {
                        m_state.highScoreIndex = getHighScoreIndex(m_state.playerScore);
                        m_state.waitingForHighScore = true;
                        m_state.highScoreNameInput = ""; // initialize empty input
                    }
                }
            }
            o_it = m_state.opponents.erase(o_it);
        } else if (!(*o_it)->isAlive()) {
            o_it = m_state.opponents.erase(o_it);
        } else {
            ++o_it; // only increment if not erased
        }
    }

    // player projectiles
    if (m_state.player) {
        auto& pp = m_state.player->getProjectiles();
        for (auto p_it = pp.begin(); p_it != pp.end(); ) {
            p_it->update(deltaTime);
            SDL_FRect b = p_it->getBounds();
            float mx = 100.0f;
            float my = 100.0f;
            // player can shoot across whole world
            if (b.x + b.w < -mx || b.x > m_state.worldWidth + mx || b.y + b.h < -my || b.y > m_state.worldHeight + my) {
                p_it = pp.erase(p_it);
            } else {
                ++p_it;
            }
        }
    }

    // opponent projectiles
    for (auto& o : m_state.opponents) { // o is std::unique_ptr<BaseOpponent>&
        if (!o) continue; 
        auto& op = o->getProjectiles(); 
        for (auto& p : op) { 
            p.update(deltaTime);
        }
        
        for (auto p_it = op.begin(); p_it != op.end(); ) {
            SDL_FRect b = p_it->getBounds();
            float mx = 100.0f, my = 100.0f;
            if (b.x + b.w < -mx || b.x > m_state.worldWidth + mx || b.y + b.h < -my || b.y > m_state.worldHeight + my) {
                p_it = op.erase(p_it); 
            } else {
                ++p_it; 
            }
        }
    }

    for (auto particle = m_state.particles.begin(); particle != m_state.particles.end(); ) {
        particle->update(deltaTime);
        if (!particle->isAlive()) {
            particle = m_state.particles.erase(particle);
        } else {
            ++particle; // only increment if not erasing
        }
    }
    checkCollisions();
    updateCamera();

    m_opponentSpawnTimer += deltaTime;
    if (m_opponentSpawnTimer >= OPPONENT_SPAWN_INTERVAL) {
        spawnOpponent();
        m_opponentSpawnTimer = 0.0f;
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

void Game::checkCollisions() {
    if (!m_state.player) return;

    // collisions between player projectile and opponent
    auto& pp = m_state.player->getProjectiles();
    for (auto p_it = pp.begin(); p_it != pp.end(); ) {
        SDL_FRect pb = p_it->getBounds();
        bool projectileHit = false;
        for (auto& o : m_state.opponents) { // o is std::unique_ptr<BaseOpponent>&
            if (!o || !o->isAlive()) continue;
            if (o->isHit(pb)) {
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
            if (m_state.player->isHit(o->getBounds())) { 
                m_state.player->takeDamage(1);
                m_state.playerHealth = m_state.player->getHealth();
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
                if (projBounds.x < playerBounds.x + playerBounds.w &&
                    projBounds.x + projBounds.w > playerBounds.x &&
                    projBounds.y < playerBounds.y + playerBounds.h &&
                    projBounds.y + projBounds.h > playerBounds.y) {
                    m_state.player->takeDamage(1);
                    m_state.playerHealth = m_state.player->getHealth();
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

// finds the index where new score should be inserted (0 is highest)
int Game::getHighScoreIndex(int score) const {
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


