#include "game.h"
#include <SDL3/SDL_main.h>
#include <SDL3_image/SDL_image.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <cstdlib>
#include <ctime>
#include <algorithm>
#include <cstring>
#include <string>
#include "font_manager.h"
#include <fstream>
#include <sstream>
#include <cctype>

Game::Game()
    : m_window(nullptr), m_renderer(nullptr),
      m_running(false), m_state(GameState::MENU), m_worldHealth(10), 
      m_playerHealth(10), m_cameraX(0.0f), 
      m_worldWidth(Config::Game::WORLD_WIDTH), 
      m_worldHeight(Config::Game::WORLD_HEIGHT), 
      m_playerScore(0) {

    srand((unsigned int)time(nullptr));

    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        SDL_Log("unable to initialize sdl: %s", SDL_GetError());
        return;
    }

    if (TTF_Init() < 0) {
        SDL_Log("unable to initialize sdl_ttf: %s", SDL_GetError());
        SDL_Quit();
        return;
    }

    m_window = SDL_CreateWindow("sdl3 defender", 800, 600, SDL_WINDOW_RESIZABLE);
    if (!m_window) {
        SDL_Log("failed to create window: %s", SDL_GetError());
        TTF_Quit();
        SDL_Quit();
        return;
    }

    m_renderer = SDL_CreateRenderer(m_window, nullptr);
    if (!m_renderer) {
        SDL_Log("failed to create renderer: %s", SDL_GetError());
        SDL_DestroyWindow(m_window);
        TTF_Quit();
        SDL_Quit();
        return;
    }
    
    m_running = true;
    loadHighScores();
}

Game::~Game() {
    // reset the player and opponents to release their shared_ptr textures
    m_player.reset(); 
    m_opponents.clear();
    m_particles.clear(); 

    // TODO: redundant but fixes segfault on close
    // clear the managers' caches before destroying the renderer
    TextureManager::getInstance().clearCache();
    FontManager::getInstance().clearCache();

    // ...now it's safe to destroy renderer and other SDL resources
    if (m_renderer) {
        SDL_DestroyRenderer(m_renderer);
        m_renderer = nullptr; //good practice
    }

    if (m_window) {
        SDL_DestroyWindow(m_window);
        m_window = nullptr; // good practice
    }

    TTF_Quit();
    SDL_Quit();
}

void Game::renderMenu() {
    SDL_SetRenderDrawColor(m_renderer, 0, 20, 40, 255);
    SDL_RenderClear(m_renderer);
    
    SDL_Color white = {255, 255, 255, 255};
    renderText("SDL3 DEFENDER", m_windowWidth/2 - 100, m_windowHeight/2 - 120, white, FontSize::MEDIUM);

    // button rectangles
    SDL_FRect playBg = { (float)(m_windowWidth/2 - 100), (float)(m_windowHeight/2 - 60), 200, 50 };
    SDL_FRect howToPlayBg = { (float)(m_windowWidth/2 - 100), (float)(m_windowHeight/2), 200, 50 };
    SDL_FRect exitBg = { (float)(m_windowWidth/2 - 100), (float)(m_windowHeight/2 + 60), 200, 50 };

    // button backgrounds and borders
    SDL_SetRenderDrawColor(m_renderer, 0, 100, 200, 200); 
    SDL_RenderFillRect(m_renderer, &playBg);
    SDL_RenderFillRect(m_renderer, &howToPlayBg); 
    SDL_RenderFillRect(m_renderer, &exitBg);
    SDL_SetRenderDrawColor(m_renderer, 255, 255, 255, 255); 
    SDL_RenderRect(m_renderer, &playBg);
    SDL_RenderRect(m_renderer, &howToPlayBg); 
    SDL_RenderRect(m_renderer, &exitBg);

    // render button text
    renderText("Play", m_windowWidth/2 - 40, m_windowHeight/2 - 50, white, FontSize::MEDIUM); 
    renderText("How to Play", m_windowWidth/2 - 80, m_windowHeight/2 + 10, white, FontSize::MEDIUM);
    renderText("Exit", m_windowWidth/2 - 40, m_windowHeight/2 + 70, white, FontSize::MEDIUM);

    SDL_RenderPresent(m_renderer);
}

void Game::handleMenuEvents() {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_EVENT_QUIT) {
            m_running = false;
        } else if (event.type == SDL_EVENT_KEY_DOWN) {
            if (event.key.key == SDLK_ESCAPE) {
                m_running = false;
            } else if (event.key.key == SDLK_RETURN) {
                playGame();
            }
        } else if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
            int mx = event.button.x;
            int my = event.button.y;
            SDL_FRect playBtn = { (float)(m_windowWidth/2 - 100), (float)(m_windowHeight/2 - 60), 200, 50 };
            SDL_FRect howToPlayBtn = { (float)(m_windowWidth/2 - 100), (float)(m_windowHeight/2), 200, 50 };
            SDL_FRect exitBtn = { (float)(m_windowWidth/2 - 100), (float)(m_windowHeight/2 + 60), 200, 50 }; 

            if (pointInRect(mx, my, playBtn)) {
                playGame();
            } else if (pointInRect(mx, my, howToPlayBtn)) { 
                m_state = GameState::HOW_TO_PLAY;
            } else if (pointInRect(mx, my, exitBtn)) {
                m_running = false;
            }
        }
    }
}

void Game::playGame() {
    m_opponents.clear();
    m_particles.clear();
    m_cameraX = 0.0f;
    float px = m_worldWidth / 2.0f - 40;
    m_player = std::make_unique<Player>(px, 500, 80, 48, m_renderer);
    m_state = GameState::PLAYING;
    m_worldHealth = 10.0f; // TODO: use a maxHealth
}

void Game::run() {
    const int TARGET_FPS = 60;
    const float FRAME_TARGET_TIME = 1000.0f / TARGET_FPS;
    Uint64 last_frame_time = SDL_GetTicks();

    float opponentSpawnTimer = 0.0f;
    const float OPPONENT_SPAWN_INTERVAL = 2.0f; // TODO: change as game progresses?
    
    while (m_running) {
        SDL_GetWindowSize(m_window, &m_windowWidth, &m_windowHeight);
        if(m_worldHeight != m_windowHeight) m_worldHeight = m_windowHeight; // world height depends on window resize (width does not)
        
        Uint64 frame_start = SDL_GetTicks();
        float deltaTime = (frame_start - last_frame_time) / 1000.0f;
        last_frame_time = frame_start;

        if (m_state == GameState::MENU) {
            m_playerScore = 0; // TODO: reset in a func
            handleMenuEvents();
            renderMenu();
        } else if (m_state == GameState::HOW_TO_PLAY) { 
            handleHowToPlayEvents();
            renderHowToPlayScreen();
        } else if (m_state == GameState::PLAYING) {
            handleEvents();
            update(deltaTime);
            opponentSpawnTimer += deltaTime;
            if (opponentSpawnTimer >= OPPONENT_SPAWN_INTERVAL) { spawnOpponent(); opponentSpawnTimer = 0.0f; }
            render();
        } else if (m_state == GameState::GAME_OVER) {
            handleGameOverScreen();
        }

        Uint64 frame_end = SDL_GetTicks();
        int frame_time = (int)(frame_end - frame_start);
        if (frame_time < FRAME_TARGET_TIME) SDL_Delay((int)(FRAME_TARGET_TIME - frame_time));
    }
}

void Game::handleEvents() {
    SDL_Event event;

    if (m_state != GameState::PLAYING) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) {
                m_running = false;
            } else if (event.type == SDL_EVENT_KEY_DOWN) {
                 if (event.key.key == SDLK_ESCAPE) {
                    if (m_state == GameState::GAME_OVER) {
                         m_state = GameState::MENU; // ESC skips to menu from game over
                         return; // exit event handling for this frame
                    } else if (m_state == GameState::PLAYING) {
                        m_state = GameState::MENU;
                        return;
                    }
                }
            }
        }
        
        return; // do not process player input if not playing
    }

    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_EVENT_QUIT) {
            m_running = false;
        } else if (event.type == SDL_EVENT_KEY_DOWN) {
            if (event.key.key == SDLK_ESCAPE) {
                m_state = GameState::MENU;
                return;
            }
        }
    }

    const bool* keys = SDL_GetKeyboardState(nullptr);

    if (m_player) m_player->handleInput(keys);
}

bool Game::pointInRect(int x, int y, const SDL_FRect& r) {
    return x >= r.x && x < r.x + r.w && y >= r.y && y < r.y + r.h;
}

void Game::renderText(const char* text, int x, int y, const SDL_Color& color, FontSize sizeEnum) {
    int fontSize{16}; 
    switch (sizeEnum) {
        case FontSize::SMALL:
            fontSize = 16;
            break;
        case FontSize::MEDIUM: 
            fontSize = 24;
            break;        
        case FontSize::LARGE:
            fontSize = 36;
            break;        
        case FontSize::GRANDELOCO:
            fontSize = 52;
            break;        
    }

    auto font = FontManager::getInstance().getFont(Config::Fonts::DEFAULT_FONT_FILE, fontSize);
    if (!font) {
        SDL_Log("Failed to get font from manager");
        return; 
    }

    SDL_Surface* fontSurface = TTF_RenderText_Solid(font.get(), text, strlen(text), color);
    if (!fontSurface) {
        SDL_Log("Text Render failed: %s", SDL_GetError());
        return;
    }

    SDL_Texture* fontTexture = SDL_CreateTextureFromSurface(m_renderer, fontSurface);
    if (!fontTexture) {
        SDL_DestroySurface(fontSurface);
        SDL_Log("Failed to create texture from font surface: %s", SDL_GetError());
        return;
    }

    SDL_FRect dst = { (float)x, (float)y, (float)fontSurface->w, (float)fontSurface->h };
    SDL_RenderTexture(m_renderer, fontTexture, nullptr, &dst);

    SDL_DestroyTexture(fontTexture);
    SDL_DestroySurface(fontSurface);
}

void Game::update(float deltaTime) {
    if (m_player) {
        m_player->update(deltaTime);
        auto& playerProjectiles = m_player->getProjectiles();

        for (auto& p : playerProjectiles) { 
            p.update(deltaTime);
        }

        SDL_FRect pb = m_player->getBounds();

        float cx = pb.x;
        float cy = pb.y;
        if (cx < 0) cx = 0;
        if (cy < 0) cy = 0;
        if (cx + pb.w > m_worldWidth) cx = m_worldWidth - pb.w;
        if (cy + pb.h > m_worldHeight) cy = m_worldHeight - pb.h;
        if (cx != pb.x || cy != pb.y) m_player->setPosition(cx, cy);
    }

    // update opponents
    for (auto& o : m_opponents) { // o is std::unique_ptr<BaseOpponent>&
        if (!o || !o->isAlive()) continue; 
        SDL_FPoint playerPos = { m_player->getBounds().x, m_player->getBounds().y };
        o->update(deltaTime, playerPos, m_cameraX, m_windowWidth); 
    }

    // erase opponents
    for (auto o_it = m_opponents.begin(); o_it != m_opponents.end(); ) {
        if (!(*o_it) || (*o_it)->getBounds().y > m_worldHeight) {
            BasicOpponent* b = dynamic_cast<BasicOpponent*>((*o_it).get()); //get() to get raw pointer for cast
            if (b) { // only the basic opponents damage world
                m_worldHealth--;
                if (m_worldHealth <= 0) m_state = GameState::GAME_OVER;
            }
            o_it = m_opponents.erase(o_it);
        } else if (!(*o_it)->isAlive()) {
            o_it = m_opponents.erase(o_it);
        } else {
            ++o_it; // only increment if not erased
        }
    }

    // player projectiles
    if (m_player) {
        auto& pp = m_player->getProjectiles();
        for (auto p_it = pp.begin(); p_it != pp.end(); ) {
            p_it->update(deltaTime);
            SDL_FRect b = p_it->getBounds();
            float mx = 100.0f;
            float my = 100.0f;
            if (b.x + b.w < -mx || b.x > m_worldWidth + mx || b.y + b.h < -my || b.y > m_worldHeight + my) {
                p_it = pp.erase(p_it);
            } else {
                ++p_it;
            }
        }
    }

    // opponent projectiles
    for (auto& o : m_opponents) { // o is std::unique_ptr<BaseOpponent>&
        if (!o) continue; 
        auto& op = o->getProjectiles(); 
        for (auto& p : op) { 
            p.update(deltaTime);
        }
        
        for (auto p_it = op.begin(); p_it != op.end(); ) {
            SDL_FRect b = p_it->getBounds();
            float mx = 100.0f, my = 100.0f;
            if (b.x + b.w < -mx || b.x > m_worldWidth + mx || b.y + b.h < -my || b.y > m_worldHeight + my) {
                p_it = op.erase(p_it); 
            } else {
                ++p_it; 
            }
        }
    }

    for (auto particle = m_particles.begin(); particle != m_particles.end(); ) {
        particle->update(deltaTime);
        if (!particle->isAlive()) {
            particle = m_particles.erase(particle);
        } else {
            ++particle; // only increment if not erasing
        }
    }
    checkCollisions();
    updateCamera();
}

void Game::render() {
    SDL_SetRenderDrawColor(m_renderer, 0, 20, 40, 255);
    SDL_RenderClear(m_renderer);

    if (m_player) {
        SDL_FRect renderBounds = m_player->getBounds();
        renderBounds.x -= m_cameraX;
        m_player->render(m_renderer, &renderBounds);

        // render player projectiles
        const auto& pp = m_player->getProjectiles();
        for (const auto& p : pp) {
            SDL_FRect pr = p.getBounds();
            pr.x -= m_cameraX;
            p.render(m_renderer, &pr);
        }
    }

    for (const auto& o : m_opponents) {
        if (o) {
            // render opponent
            SDL_FRect or_ = o->getBounds(); 
            or_.x -= m_cameraX;
            o->render(m_renderer, &or_);

            // render opponent projectiles
            const auto& op = o->getProjectiles();
            for (const auto& p : op) { 
                SDL_FRect pr = p.getBounds();
                pr.x -= m_cameraX;
                p.render(m_renderer, &pr);
            }
        }
    }

    // render particles
    for (const auto& particle : m_particles) { // particle is the object itself
        SDL_FRect pr = particle.getBounds();
        pr.x -= m_cameraX;
        particle.render(m_renderer, &pr);
    }

    renderMinimap();

    // render health bars
    const int barW = 200;
    const int barH = 10;
    const int barX = 10;
    const int barY = 10;
    const int spacing = 5;
    SDL_Color white = {255,255,255,255};

    renderText("Player Health:", barX, barY, white, FontSize::SMALL);
    float phr = m_player ? (float)m_player->getHealth() / 10.0f : 1.0f; // TODO: change 10 to a maxHealth
    SDL_SetRenderDrawColor(m_renderer, 255,0,0,255);
    SDL_FRect pbBg = {(float)barX, (float)(barY+20), (float)barW, (float)barH};
    SDL_RenderFillRect(m_renderer, &pbBg);
    SDL_SetRenderDrawColor(m_renderer, 0,255,0,255);
    SDL_FRect pbFill = {(float)barX, (float)(barY+20), (float)(barW * phr), (float)barH};
    SDL_RenderFillRect(m_renderer, &pbFill);
    SDL_SetRenderDrawColor(m_renderer, 255,255,255,255);
    SDL_RenderRect(m_renderer, &pbBg);

    renderText("World Health:", barX, barY + 20 + barH + spacing, white, FontSize::SMALL);
    float whr = (float)m_worldHealth / 10.0f; // TODO: change 10 to a maxHealth
    SDL_SetRenderDrawColor(m_renderer, 255,0,0,255);
    SDL_FRect wbBg = {(float)barX, (float)(barY + 20 + barH + spacing + 20), (float)barW, (float)barH};
    SDL_RenderFillRect(m_renderer, &wbBg);
    SDL_SetRenderDrawColor(m_renderer, 0,255,0,255);
    SDL_FRect wbFill = {(float)barX, (float)(barY + 20 + barH + spacing + 20), (float)(barW * whr), (float)barH};
    SDL_RenderFillRect(m_renderer, &wbFill);
    SDL_SetRenderDrawColor(m_renderer, 255,255,255,255);
    SDL_RenderRect(m_renderer, &wbBg);

    // render player score
    float rightOffset = m_windowWidth - 150;
    renderText("Score:", rightOffset, barY, white, FontSize::SMALL);
    std::string scoreStr = std::to_string(m_playerScore);
    renderText(scoreStr.c_str(), m_windowWidth - 90, barY, white, FontSize::SMALL);

    SDL_RenderPresent(m_renderer);
}

void Game::spawnOpponent() {
    int type = rand() % 3;
    float x = (float)(rand() % (int)(m_worldWidth - 50));
    float y = -50.0f;
    switch (type) {
        case 0: m_opponents.emplace(std::make_unique<BasicOpponent>(x, y, 40, 40, m_renderer)); break;
        case 1: m_opponents.emplace(std::make_unique<AggressiveOpponent>(x, y, 45, 45, m_renderer)); break;
        case 2: m_opponents.emplace(std::make_unique<SniperOpponent>(x, y, 35, 35, m_renderer)); break;
    }
}

void Game::checkCollisions() {
    if (!m_player) return;

    // collisions between player projectile and opponent
    auto& pp = m_player->getProjectiles();
    for (auto p_it = pp.begin(); p_it != pp.end(); ) {
        SDL_FRect pb = p_it->getBounds();
        bool projectileHit = false;
        for (auto& o : m_opponents) { // o is std::unique_ptr<BaseOpponent>&
            if (!o || !o->isAlive()) continue;
            if (o->isHit(pb)) {
                o->takeDamage(1);
                if (!o->isAlive()) {
                    m_playerScore += o->getScoreVal();
                    o->explode(m_particles);
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
    if (m_player->isAlive()) {
        for (auto o_it = m_opponents.begin(); o_it != m_opponents.end(); ) {
            auto& o = *o_it;
            if (!o || !o->isAlive()) {
                 ++o_it; // skip dead opponents
                 continue;
            }

            // check player/opponent collision
            if (m_player->isHit(o->getBounds())) { 
                m_player->takeDamage(1);
                m_playerHealth = m_player->getHealth();
                o->explode(m_particles); 
                m_playerScore += o->getScoreVal();
                o_it = m_opponents.erase(o_it);
                if (!m_player->isAlive()) {
                    m_state = GameState::GAME_OVER;
                    return; // exit early if player dies
                }
                continue; // skip projectile check if opponent was destroyed by collision
            }

            // check if opponent's projectiles hit player
            auto& op = o->getProjectiles(); 
            for (auto op_it = op.begin(); op_it != op.end(); ) {
                SDL_FRect projBounds = op_it->getBounds();
                SDL_FRect playerBounds = m_player->getBounds();

                // collision check ... projectile and player
                if (projBounds.x < playerBounds.x + playerBounds.w &&
                    projBounds.x + projBounds.w > playerBounds.x &&
                    projBounds.y < playerBounds.y + playerBounds.h &&
                    projBounds.y + projBounds.h > playerBounds.y) {
                    m_player->takeDamage(1);
                    m_playerHealth = m_player->getHealth();
                    // erase the projectile that hit the player using the iterator
                    op_it = op.erase(op_it);
                    if (!m_player->isAlive()) {
                        m_state = GameState::GAME_OVER;
                        return; // exit early if player dies
                    }
                } else {
                    ++op_it;
                }
            }

            // increment opponent iterator only if the opponent itself wasn't erased in the player collision check
            if (m_state != GameState::GAME_OVER && o_it != m_opponents.end()) { // check if state changed or iterator became invalid due to erase
                ++o_it;
            }
            // ... if state is GAME_OVER or o_it was invalidated by erase in the inner loop, the outer loop will terminate
        }
    }
}

void Game::updateCamera() {
    if (!m_player) return;

    SDL_FRect pb = m_player->getBounds();
    float target = pb.x - m_windowWidth/2.0f;

    if (target < 0) target = 0;
    if (target > m_worldWidth - m_windowWidth) target = m_worldWidth - m_windowWidth;
    m_cameraX = target;
}

void Game::renderMinimap() {
    const int mmW = 210;
    const int mmH = 42;
    const int mmX = (m_windowWidth - mmW)/2, mmY = 10;
    SDL_SetRenderDrawColor(m_renderer, 0, 40, 80, 200);
    SDL_FRect mm = {(float)mmX, (float)mmY, (float)mmW, (float)mmH};
    SDL_RenderFillRect(m_renderer, &mm);
    SDL_SetRenderDrawColor(m_renderer, 0, 100, 200, 255);
    SDL_RenderRect(m_renderer, &mm);

    float sx = (float)mmW / m_worldWidth;
    float sy = (float)mmH / m_worldHeight;

    if (m_player) {
        SDL_FRect pb = m_player->getBounds();
        float px = pb.x * sx + mmX;
        float py = pb.y * sy + mmY;
        SDL_SetRenderDrawColor(m_renderer, 0, 255, 0, 255);
        SDL_FRect pd = {px, py, 4, 4};
        SDL_RenderFillRect(m_renderer, &pd);
    }

    for (const auto& o : m_opponents) {
        if (o && o->isAlive()) {
            SDL_FRect ob = o->getBounds();
            float ox = ob.x * sx + mmX;
            float oy = ob.y * sy + mmY;
            SDL_SetRenderDrawColor(m_renderer, 255, 0, 0, 255);
            SDL_FRect od = {ox, oy, 3, 3};
            SDL_RenderFillRect(m_renderer, &od);
        }
    }

    float vx = m_cameraX * sx + mmX;
    float vw = (float)m_windowWidth * sx;
    SDL_SetRenderDrawColor(m_renderer, 255, 255, 255, 100);
    SDL_FRect vr = {vx, (float)mmY, vw, (float)mmH};
    SDL_RenderRect(m_renderer, &vr);
}


// high scores

void Game::loadHighScores() {
    m_highScores.clear();
    std::ifstream file(Config::Game::HIGH_SCORES_PATH);
    if (file.is_open()) {
        std::string line;
        while (std::getline(file, line) && m_highScores.size() < MAX_HIGH_SCORES) {
            std::istringstream iss(line);
            std::string name;
            int score;
            if (iss >> name >> score) { // format: "NAME SCORE"
                 HighScore entry;
                 entry.name = name;
                 entry.score = score;
                 m_highScores.push_back(entry);
            }
        }
        file.close();
    }

    // list is sorted (highest first) and capped at MAX_HIGH_SCORES
    std::sort(m_highScores.begin(), m_highScores.end(),
              [](const HighScore& a, const HighScore& b) { return a.score > b.score; });
    if (m_highScores.size() > MAX_HIGH_SCORES) {
        m_highScores.resize(MAX_HIGH_SCORES);
    }
}

// saves high scores to  file
void Game::saveHighScores() {
    std::ofstream file(Config::Game::HIGH_SCORES_PATH);
    if (file.is_open()) {
        for (const auto& entry : m_highScores) {
            file << entry.name << " " << entry.score << "\n"; //Format: "NAME SCORE"
        }
        file.close();
        SDL_Log("High scores saved.");
    } else {
        SDL_Log("Warning: Could not save high scores to file.");
    }
}

bool Game::isHighScore(int score) const {
    return m_highScores.size() < MAX_HIGH_SCORES || score > m_highScores.back().score;
}

// finds the index where new score should be inserted (0 is highest)
int Game::getHighScoreIndex(int score) const {
    for (size_t i = 0; i < m_highScores.size(); ++i) {
        if (score > m_highScores[i].score) {
            return static_cast<int>(i);
        }
    }
    //if loop finishes without returning, the score is not higher than any existing score, but also need to check if the list is not full yet
    if (m_highScores.size() < MAX_HIGH_SCORES) {
        return static_cast<int>(m_highScores.size());
    }

    return -1;
}

void Game::addHighScore(const std::string& name, int score) {
    int index = getHighScoreIndex(score);
    if (index != -1) { // valid high score position
        HighScore newEntry;
        newEntry.name = name.empty() ? "ANON" : name;
        newEntry.score = score;
        m_highScores.insert(m_highScores.begin() + index, newEntry);

        if (m_highScores.size() > MAX_HIGH_SCORES) {
            m_highScores.pop_back();
        }
        saveHighScores();
    }
}

//TODO: make resizeable
void Game::handleGameOverScreen() {
    int scoreIndex = getHighScoreIndex(m_playerScore);

    if (scoreIndex != -1) {
        handleHighScoreEntryScreen(scoreIndex, m_playerScore);
    } else {
        // display game over screen until user interacts
        SDL_Color white = {255, 255, 255, 255};
        SDL_Color red = {255, 0, 0, 255};
        SDL_Color black = {0, 0, 0, 255};
        SDL_Event event;

        // 'X' button
        const int xButtonSize = 20;
        SDL_FRect xButtonRect = {
            static_cast<float>(m_windowWidth - xButtonSize - 10), 
            10.0f,
            static_cast<float>(xButtonSize),
            static_cast<float>(xButtonSize)
        };

        bool waitingForInput = true;
        // clear any pending events before starting the input wait
        SDL_FlushEvents(SDL_EVENT_FIRST, SDL_EVENT_LAST);

        while (waitingForInput && m_running) {
            // render the game over screen
            SDL_SetRenderDrawColor(m_renderer, 0, 0, 0, 255);
            SDL_RenderClear(m_renderer);

            renderText("GAME OVER", m_windowWidth / 2 - 100, m_windowHeight / 2 - 60, red, FontSize::LARGE);
            renderText(("Score: " + std::to_string(m_playerScore)).c_str(), m_windowWidth / 2 - 60, m_windowHeight / 2, white, FontSize::MEDIUM);

            // draw 'X' button
            SDL_SetRenderDrawColor(m_renderer, 255, 0, 0, 255); // red
            SDL_RenderLine(m_renderer, xButtonRect.x, xButtonRect.y, xButtonRect.x + xButtonRect.w, xButtonRect.y + xButtonRect.h);
            SDL_RenderLine(m_renderer, xButtonRect.x, xButtonRect.y + xButtonRect.h, xButtonRect.x + xButtonRect.w, xButtonRect.y);
            SDL_SetRenderDrawColor(m_renderer, 255, 255, 255, 255); // white border
            SDL_RenderRect(m_renderer, &xButtonRect);

            SDL_RenderPresent(m_renderer);

            while (SDL_PollEvent(&event)) {
                if (event.type == SDL_EVENT_QUIT) {
                    m_running = false;
                    waitingForInput = false;
                } else if (event.type == SDL_EVENT_KEY_DOWN) {
                    if (event.key.key == SDLK_RETURN || event.key.key == SDLK_ESCAPE) {
                        waitingForInput = false; // close the game-over screen
                    }
                } else if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
                    if (event.button.button == SDL_BUTTON_LEFT) {
                        // check for click within the 'X' button rectangle
                        if (pointInRect(event.button.x, event.button.y, xButtonRect)) {
                            waitingForInput = false; // close game-over screen
                        }
                    }
                }
            }
            
            SDL_Delay(10);//small delay to prevent the loop from consuming too much CPU
        }
    }
    m_state = GameState::MENU;
}

// screen where player enters their name
void Game::handleHighScoreEntryScreen(int scoreIndex, int playerScore) {
    SDL_Color white = {255, 255, 255, 255};
    SDL_Color yellow = {255, 255, 0, 255};
    std::string playerName = "";
    SDL_Event event;

    // clear any pending events before starting input
    SDL_FlushEvents(SDL_EVENT_FIRST, SDL_EVENT_LAST);

    bool inputting = true;
    while (inputting && m_running) {
        SDL_SetRenderDrawColor(m_renderer, 0, 0, 0, 255);
        SDL_RenderClear(m_renderer);
        renderText("NEW HIGH SCORE!", m_windowWidth / 2 - 120, m_windowHeight / 2 - 100, yellow, FontSize::LARGE);
        renderText(("Position: #" + std::to_string(scoreIndex + 1)).c_str(), m_windowWidth / 2 - 80, m_windowHeight / 2 - 50, white, FontSize::MEDIUM);
        renderText(("Score: " + std::to_string(playerScore)).c_str(), m_windowWidth / 2 - 60, m_windowHeight / 2 - 20, white, FontSize::MEDIUM);
        renderText("Enter Name (max 10 chars):", m_windowWidth / 2 - 140, m_windowHeight / 2 + 20, white, FontSize::SMALL);
        renderText((playerName + "_").c_str(), m_windowWidth / 2 - 40, m_windowHeight / 2 + 50, white, FontSize::MEDIUM); // cursor effect
        SDL_RenderPresent(m_renderer);

        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) {
                m_running = false; // exit game if window closed
                inputting = false; // stop input loop
            } else if (event.type == SDL_EVENT_KEY_DOWN) {
                SDL_Keycode key = event.key.key;
                if (key != SDLK_UP && key != SDLK_DOWN && key != SDLK_LEFT && key != SDLK_RIGHT) {
                    if (key == SDLK_RETURN) {
                        inputting = false; 
                    } else if (key == SDLK_BACKSPACE && !playerName.empty()) {
                        playerName.pop_back();
                    } else if (playerName.length() < 10 && (std::isalnum(static_cast<unsigned char>(key)))) {
                        playerName += static_cast<char>(key);
                    } else if (key == SDLK_ESCAPE) {
                        playerName = "ANON";
                        inputting = false; // cancel input and exit loop
                    }
                }
            }
        }
        
        SDL_Delay(10);// small delay to prevent the loop from consuming too much CPU
    }

    // trim spaces (after input loop ends)
    if (!playerName.empty()) {
        size_t start = playerName.find_first_not_of(" \t");
        size_t end = playerName.find_last_not_of(" \t");
        if (start != std::string::npos && end != std::string::npos) {
            playerName = playerName.substr(start, end - start + 1);
        } else {
            playerName = "";
        }
        if (playerName.empty()) playerName = "ANON";
    } else if (playerName.empty()) {
         playerName = "ANON";
    }


    addHighScore(playerName, playerScore);

    // show confirmation message for a short time
    Uint64 confirmStartTime = SDL_GetTicks();
    const Uint32 CONFIRM_TIME_MS = 2000;

    SDL_FlushEvents(SDL_EVENT_FIRST, SDL_EVENT_LAST);

    while (SDL_GetTicks() - confirmStartTime < CONFIRM_TIME_MS && m_running) {
        SDL_SetRenderDrawColor(m_renderer, 0, 0, 0, 255);
        SDL_RenderClear(m_renderer);
        renderText("CONGRATULATIONS!", m_windowWidth / 2 - 130, m_windowHeight / 2 - 60, yellow, FontSize::LARGE);
        renderText((playerName + " - #" + std::to_string(scoreIndex + 1) + "!").c_str(), m_windowWidth / 2 - 150, m_windowHeight / 2, white, FontSize::MEDIUM);
        renderText(("Score: " + std::to_string(playerScore)).c_str(), m_windowWidth / 2 - 60, m_windowHeight / 2 + 40, white, FontSize::MEDIUM);
        SDL_RenderPresent(m_renderer);
        SDL_Delay(16);
    }
}

// END: high scores

// how to play

void Game::handleHowToPlayEvents() {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_EVENT_QUIT) {
            m_running = false;
        } else if (event.type == SDL_EVENT_KEY_DOWN) {
             if (event.key.key == SDLK_ESCAPE || event.key.key == SDLK_RETURN) {
                m_state = GameState::MENU;
            }
        } else if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
            // for now, clicking anywhere goes back to menu
            m_state = GameState::MENU;
        }
    }
}

void Game::renderHowToPlayScreen() {
    SDL_SetRenderDrawColor(m_renderer, 0, 20, 40, 255);
    SDL_RenderClear(m_renderer);

    SDL_Color white = {255, 255, 255, 255};
    SDL_Color yellow = {255, 255, 0, 255}; // for titles

    int y_pos = 50; // starting Y position for text
    const int line_spacing = 30; // space between lines of text
    const int opponent_image_size = 30;

    renderText("HOW TO PLAY", m_windowWidth/2 - 100, y_pos, yellow, FontSize::MEDIUM);
    y_pos += line_spacing + 20;

    renderText("CONTROLS:", m_windowWidth/2 - 80, y_pos, white, FontSize::SMALL);
    y_pos += line_spacing;
    renderText("- Move: Arrow Keys or WASD", m_windowWidth/2 - 150, y_pos, white, FontSize::SMALL);
    y_pos += line_spacing;
    renderText("- Shoot: Spacebar", m_windowWidth/2 - 150, y_pos, white, FontSize::SMALL);
    y_pos += line_spacing;
    renderText("- Boost: Hold 'C' or Shift", m_windowWidth/2 - 150, y_pos, white, FontSize::SMALL);
    y_pos += line_spacing + 10;

    renderText("OPPONENTS:", m_windowWidth/2 - 80, y_pos, white, FontSize::SMALL);
    y_pos += line_spacing;

    // bombs
    auto basicTexture = TextureManager::getInstance().getTexture(Config::Textures::BASIC_OPPONENT, m_renderer);
    if (basicTexture) {
        SDL_FRect imageRect = { (float)(m_windowWidth/2 - 430), (float)y_pos, (float)opponent_image_size, (float)opponent_image_size };
        SDL_RenderTexture(m_renderer, basicTexture.get(), nullptr, &imageRect);
    }
    renderText("Bombs: Do not shoot at you, but damage the world if they reach the bottom - worth 300 points.", m_windowWidth/2 - 390, y_pos, white, FontSize::SMALL);
    y_pos += line_spacing + 5;

    // aggressive
    auto aggressiveTexture = TextureManager::getInstance().getTexture(Config::Textures::AGGRESSIVE_OPPONENT, m_renderer);
    if (aggressiveTexture) {
        SDL_FRect imageRect = { (float)(m_windowWidth/2 - 430), (float)y_pos, (float)opponent_image_size, (float)opponent_image_size };
        SDL_RenderTexture(m_renderer, aggressiveTexture.get(), nullptr, &imageRect);
    }
    renderText("Aggressive: Chases the player, fires aimed shots - worth 100 points.", m_windowWidth/2 - 390, y_pos, white, FontSize::SMALL);
    y_pos += line_spacing + 5; 

    // sniper
    auto sniperTexture = TextureManager::getInstance().getTexture(Config::Textures::SNIPER_OPPONENT, m_renderer);
    if (sniperTexture) {
        SDL_FRect imageRect = { (float)(m_windowWidth/2 - 430), (float)y_pos, (float)opponent_image_size, (float)opponent_image_size };
        SDL_RenderTexture(m_renderer, sniperTexture.get(), nullptr, &imageRect);
    }
    renderText("Sniper: Moves slowly, fires faster with more accuracy - worth 100 points.", m_windowWidth/2 - 390, y_pos, white, FontSize::SMALL);
    y_pos += line_spacing + 30; 

    renderText("Goal: Destroy opponents, prevent bombs from damaging world.", m_windowWidth/2 - 200, y_pos, white, FontSize::SMALL);

    y_pos += line_spacing + 20;

    renderText("Press ESC or ENTER to return to the menu.", m_windowWidth/2 - 150, y_pos, white, FontSize::SMALL);

    SDL_RenderPresent(m_renderer);
}

// END: how to play