#include "platform.h"
#include <SDL3/SDL_main.h>
#include <SDL3_image/SDL_image.h>
#include <algorithm>
#include <cctype>
#include <cstring>
#include <sstream>

Platform::Platform() = default;

Platform::~Platform() { shutdown(); }

bool Platform::initialize() {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        SDL_Log("unable to initialize sdl: %s", SDL_GetError());
        return false;
    }
    if (TTF_Init() < 0) {
        SDL_Log("unable to initialize sdl_ttf: %s", SDL_GetError());
        SDL_Quit();
        return false;
    }

    m_window = SDL_CreateWindow("sdl3 defender", 800, 600, SDL_WINDOW_RESIZABLE);
    if (!m_window) {
        SDL_Log("failed to create window: %s", SDL_GetError());
        TTF_Quit();
        SDL_Quit();
        return false;
    }

    m_renderer = SDL_CreateRenderer(m_window, nullptr);
    if (!m_renderer) {
        SDL_Log("failed to create renderer: %s", SDL_GetError());
        SDL_DestroyWindow(m_window);
        TTF_Quit();
        SDL_Quit();
        return false;
    }

    // attempt to enable VSync using SDL_SetRenderVSync
    if (SDL_SetRenderVSync(m_renderer, 1) != 0) { // 1 enables VSync, 0 disables
        // if setting VSync fails, log it but continue (maybe VSync isn't supported on this display/driver)
        SDL_Log("Warning: Failed to enable VSync: %s. Running without VSync.", SDL_GetError());
    } else {
        SDL_Log("VSync successfully enabled.");
    }

    SDL_GetWindowSize(m_window, &m_windowWidth, &m_windowHeight);
    return true;
}

void Platform::shutdown() {
    if (m_textInputActive) {
        SDL_StopTextInput(m_window); // stop text input
        m_textInputActive = false;
        SDL_Log("Platform: Text input STOPPED during shutdown.");
    }

    TextureManager::getInstance().clearCache();
    FontManager::getInstance().clearCache();

    if (m_renderer) {
        SDL_DestroyRenderer(m_renderer);
        m_renderer = nullptr;
    }
    if (m_window) {
        SDL_DestroyWindow(m_window);
        m_window = nullptr;
    }
    TTF_Quit();
    SDL_Quit();
}

void Platform::run(Game& sim) {
    const int TARGET_FPS = 60;
    const float FRAME_TARGET_TIME_MS = 1000.0f / TARGET_FPS;
    const float FIXED_DELTA_TIME = 1.0f / TARGET_FPS; // delta time for updates

    // 64-bit integers for time values
    Uint64 previousFrameTime = SDL_GetTicks(); // time of previous frame start
    float accumulator = 0.0f; // accumulates elapsed time to control update frequency

    m_running = true;
    while (m_running) {
        Uint64 currentTime = SDL_GetTicks();
        float deltaTimeMS = static_cast<float>(currentTime - previousFrameTime);
        previousFrameTime = currentTime;

        // prevents "spiral of death" ... if frame takes too long
        if (deltaTimeMS > 200.0f) 
            deltaTimeMS = 200.0f; // cap at 200ms (5 FPS)
        
        accumulator += deltaTimeMS / 1000.0f; // convert to seconds, add to accumulator

        SDL_GetWindowSize(m_window, &m_windowWidth, &m_windowHeight);

        auto& state = sim.getState();
        if (state.state == GameStateData::State::PLAYING) {
            state.worldHeight = (float)m_windowHeight; // world height depends on window resize (width does not)
        }

        updateTextInputState(state); // update text input state

        GameInput input = pollInput(state);
        sim.handleInput(input);

        if (input.quit || state.running == false) {
            m_running = false;
        }

        // fixed timestep update loop
        while (accumulator >= FIXED_DELTA_TIME) {
            sim.update(FIXED_DELTA_TIME, m_windowWidth); // pass the fixed delta time
            accumulator -= FIXED_DELTA_TIME;
        }

        // TODO: implement cap here, too?? ^^^
        render(state);
    }

    // ensure text input is stopped when the loop exits
    if (m_textInputActive) {
        SDL_StopTextInput(m_window);
        m_textInputActive = false;
        SDL_Log("Platform: Text input STOPPED on shutdown.");
    }
}

void Platform::render(const GameStateData& state) {
    switch (state.state) {
        case GameStateData::State::MENU:
            renderMenu(state);
            break;
        case GameStateData::State::HOW_TO_PLAY:
            renderHowToPlayScreen();
            break;
        case GameStateData::State::PLAYING:
            SDL_SetRenderDrawColor(m_renderer, 0, 20, 40, 255);
            SDL_RenderClear(m_renderer);

            if (state.player) {
                // render player
                auto playerTexture = TextureManager::getInstance().getTexture(Config::Textures::PLAYER, m_renderer);
                if (playerTexture) {
                    SDL_FRect renderBounds = state.player->getBounds();
                    renderBounds.x -= state.cameraX;
                    
                    // apply flip based on player's facing-direction
                    SDL_FRect drawRect = renderBounds;
                    if (state.player->getFacing() == Direction::LEFT) {
                        drawRect.x += drawRect.w;
                        drawRect.w = -drawRect.w;
                    }

                    SDL_RenderTexture(m_renderer, playerTexture.get(), nullptr, &drawRect);

                    // render player projectiles
                    const auto& pp = state.player->getProjectiles();
                    for (const auto& p : pp) {
                        if (p.getAge() >= p.getLifetime()) continue;
                        
                        float cameraOffsetX = state.cameraX;

                        SDL_Color color = p.getColor();
                        SDL_SetRenderDrawColor(m_renderer, color.r, color.g, color.b, color.a);

                        // player beam: from spawn to world edge
                        float startX = p.getSpawnX() - cameraOffsetX;
                        float endX = (p.getVelocity().x > 0) ? (p.getWorldWidth() - cameraOffsetX) : (0.0f - cameraOffsetX);
                        SDL_RenderLine(m_renderer, startX, p.getSpawnY(), endX, p.getSpawnY()); // spawn x/y, end x/y                       
                    }
                }
            }

            for (const auto& o : state.opponents) {
                if (!o || !o->isAlive()) continue;

                SDL_FRect renderBounds = o->getBounds();
                renderBounds.x -= state.cameraX;

                // render opponent texture
                auto opponentTexture = TextureManager::getInstance().getTexture(o->getTextureKey(), m_renderer);
                if (opponentTexture) {
                    SDL_RenderTexture(m_renderer, opponentTexture.get(), nullptr, &renderBounds);
                } else {
                    // fallback rect
                    SDL_SetRenderDrawColor(m_renderer, 255, 0, 255, 255);
                    SDL_RenderFillRect(m_renderer, &renderBounds);
                }

                //render opponent projectiles
                const auto& op = o->getProjectiles();
                for (const auto& p : op) { 
                    if (p.getAge() >= p.getLifetime()) continue;
                    float cameraOffsetX = state.cameraX;

                    SDL_Color color = p.getColor();
                    SDL_SetRenderDrawColor(m_renderer, color.r, color.g, color.b, color.a);

                    // opponent beam ... from spawn to current pos, extended
                    float dx = p.getCurrentX() - p.getSpawnX();
                    float dy = p.getCurrentY() - p.getSpawnY();
                    float endX = p.getSpawnX() + dx * 4.0f; // 4x extension
                    float endY = p.getSpawnY() + dy * 4.0f;
                    SDL_FPoint start = { p.getSpawnX() - cameraOffsetX, p.getSpawnY() };
                    SDL_FPoint end   = { endX - cameraOffsetX, endY };
                    SDL_RenderLine(m_renderer, start.x, start.y, end.x, end.y);
                }
            }

            // render particles
            for (const auto& particle : state.particles) {
                if (particle.isAlive()) { 
                    SDL_FRect renderBounds = { particle.getX(), particle.getY(), particle.getCurrentSize(), particle.getCurrentSize() };
                    renderBounds.x -= state.cameraX; // apply camera offset

                    SDL_SetRenderDrawColor(m_renderer, particle.getR(), particle.getG(), particle.getB(), particle.getAlpha());
                    SDL_RenderFillRect(m_renderer, &renderBounds);
                }
            }

            renderMinimap(state);
            renderHealthBars(state);

            break;
        case GameStateData::State::GAME_OVER:
            if (state.waitingForHighScore) {
                renderHighScoreEntryScreen(state);
            } else {
                renderGameOverScreen(state);
            }
            break;
    }
    SDL_RenderPresent(m_renderer);
}

void Platform::renderText(const char* text, int x, int y, const SDL_Color& color, FontSize sizeEnum) {
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

GameInput Platform::pollInput(const GameStateData& state) {
    GameInput input{};
    SDL_Event event;

    // always poll quit/escape/enter/mouse
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_EVENT_QUIT) {
            input.quit = true;
        } else if (event.type == SDL_EVENT_KEY_DOWN) {
            if (event.key.key == SDLK_ESCAPE) input.escape = true;
            else if (event.key.key == SDLK_RETURN) input.enter = true;
        } else if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
            if (event.button.button == SDL_BUTTON_LEFT) {
                input.mouseClick = true;
                input.mouseX = event.button.x;
                input.mouseY = event.button.y;
            }
        } else if (event.type == SDL_EVENT_TEXT_INPUT) { // for text input
            if (event.text.text[0] != '\0' && event.text.text[1] == '\0') { // ensure it's a single character
                char c = event.text.text[0];
                if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9')) {
                    input.charInputEvent = true;
                    input.inputChar = c;
                }
            }
        }
    }

    // only poll movement/shoot/boost if playing
    if (state.state == GameStateData::State::PLAYING) {
        const bool* keys = SDL_GetKeyboardState(nullptr);
        input.moveLeft  = keys[SDL_SCANCODE_LEFT] || keys[SDL_SCANCODE_A];
        input.moveRight = keys[SDL_SCANCODE_RIGHT] || keys[SDL_SCANCODE_D];
        input.moveUp    = keys[SDL_SCANCODE_UP] || keys[SDL_SCANCODE_W];
        input.moveDown  = keys[SDL_SCANCODE_DOWN] || keys[SDL_SCANCODE_S];
        input.shoot     = keys[SDL_SCANCODE_SPACE];
        input.boost     = keys[SDL_SCANCODE_C] || keys[SDL_SCANCODE_LSHIFT] || keys[SDL_SCANCODE_RSHIFT];
    }
    else if (state.state == GameStateData::State::GAME_OVER && state.waitingForHighScore) {
        const bool* keys = SDL_GetKeyboardState(nullptr);
        // poll for backspace/delete specifically on the high score screen
        if (keys[SDL_SCANCODE_BACKSPACE] || keys[SDL_SCANCODE_DELETE]) {
             input.backspacePressed = true;
        }
    }

    return input;
}

void Platform::updateTextInputState(const GameStateData& state) {
    bool shouldTextInputBeActive = (state.state == GameStateData::State::GAME_OVER && state.waitingForHighScore);

    if (shouldTextInputBeActive && !m_textInputActive) {
        // start text input
        SDL_StartTextInput(m_window);
        m_textInputActive = true;
        SDL_Log("Platform: Text input STARTED for high score entry.");
    } else if (!shouldTextInputBeActive && m_textInputActive) {
        // stop text input
        SDL_StopTextInput(m_window);
        m_textInputActive = false;
        SDL_Log("Platform: Text input STOPPED.");
    }
}

bool Platform::pointInRect(int x, int y, const SDL_FRect& r) {
    return x >= r.x && x < r.x + r.w && y >= r.y && y < r.y + r.h;
}

// TODO: doesn't need state here
void Platform::renderMenu(const GameStateData& state) {
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
}

void Platform::renderHowToPlayScreen() {
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
}

void Platform::renderGameOverScreen(const GameStateData& state) {
    SDL_Color white = {255, 255, 255, 255};
    SDL_Color red = {255, 0, 0, 255};

    const int xButtonSize = 20;
    SDL_FRect xButtonRect = {
        static_cast<float>(m_windowWidth - xButtonSize - 10),
        10.0f,
        static_cast<float>(xButtonSize),
        static_cast<float>(xButtonSize)
    };

    SDL_SetRenderDrawColor(m_renderer, 0, 0, 0, 255);
    SDL_RenderClear(m_renderer);
    renderText("GAME OVER", m_windowWidth / 2 - 100, m_windowHeight / 2 - 60, red, FontSize::LARGE);
    renderText(("Score: " + std::to_string(state.playerScore)).c_str(), m_windowWidth / 2 - 60, m_windowHeight / 2, white, FontSize::MEDIUM);

    SDL_SetRenderDrawColor(m_renderer, 255, 0, 0, 255);
    SDL_RenderLine(m_renderer, xButtonRect.x, xButtonRect.y, xButtonRect.x + xButtonRect.w, xButtonRect.y + xButtonRect.h);
    SDL_RenderLine(m_renderer, xButtonRect.x, xButtonRect.y + xButtonRect.h, xButtonRect.x + xButtonRect.w, xButtonRect.y);
    SDL_SetRenderDrawColor(m_renderer, 255, 255, 255, 255);
    SDL_RenderRect(m_renderer, &xButtonRect);
}

void Platform::renderHighScoreEntryScreen(const GameStateData& state) {
    SDL_Color white = {255, 255, 255, 255};
    SDL_Color yellow = {255, 255, 0, 255};

    SDL_SetRenderDrawColor(m_renderer, 0, 0, 0, 255);
    SDL_RenderClear(m_renderer);
    renderText("NEW HIGH SCORE!", m_windowWidth / 2 - 120, m_windowHeight / 2 - 100, yellow, FontSize::LARGE);
    renderText(("Position: #" + std::to_string(state.highScoreIndex + 1)).c_str(), m_windowWidth / 2 - 80, m_windowHeight / 2 - 50, white, FontSize::MEDIUM);
    renderText(("Score: " + std::to_string(state.playerScore)).c_str(), m_windowWidth / 2 - 60, m_windowHeight / 2 - 20, white, FontSize::MEDIUM);
    renderText("Enter Name (max 10 chars):", m_windowWidth / 2 - 140, m_windowHeight / 2 + 20, white, FontSize::SMALL);
    renderText((state.highScoreNameInput + "_").c_str(), m_windowWidth / 2 - 40, m_windowHeight / 2 + 50, white, FontSize::MEDIUM);
}

void Platform::renderHealthBars(const GameStateData& state) {
    const int barW = 200;
    const int barH = 10;
    const int barX = 10;
    const int barY = 10;
    const int spacing = 5;
    SDL_Color white = {255,255,255,255};

    renderText("Player Health:", barX, barY, white, FontSize::SMALL);
    float phr = state.player ? (float)state.player->getHealth() / 10.0f : 1.0f; // TODO: change 10 to a maxHealth
    SDL_SetRenderDrawColor(m_renderer, 255,0,0,255);
    SDL_FRect pbBg = {(float)barX, (float)(barY+20), (float)barW, (float)barH};
    SDL_RenderFillRect(m_renderer, &pbBg);
    SDL_SetRenderDrawColor(m_renderer, 0,255,0,255);
    SDL_FRect pbFill = {(float)barX, (float)(barY+20), (float)(barW * phr), (float)barH};
    SDL_RenderFillRect(m_renderer, &pbFill);
    SDL_SetRenderDrawColor(m_renderer, 255,255,255,255);
    SDL_RenderRect(m_renderer, &pbBg);

    renderText("World Health:", barX, barY + 20 + barH + spacing, white, FontSize::SMALL);
    float whr = (float)state.worldHealth / 10.0f; // TODO: change 10 to a maxHealth
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
    std::string scoreStr = std::to_string(state.playerScore);
    renderText(scoreStr.c_str(), m_windowWidth - 90, barY, white, FontSize::SMALL);
}

void Platform::renderMinimap(const GameStateData& state) {
    const int mmW = 210;
    const int mmH = 42;
    const int mmX = (m_windowWidth - mmW)/2, mmY = 10;
    SDL_SetRenderDrawColor(m_renderer, 0, 40, 80, 200);
    SDL_FRect mm = {(float)mmX, (float)mmY, (float)mmW, (float)mmH};
    SDL_RenderFillRect(m_renderer, &mm);
    SDL_SetRenderDrawColor(m_renderer, 0, 100, 200, 255);
    SDL_RenderRect(m_renderer, &mm);

    float sx = (float)mmW / state.worldWidth;
    float sy = (float)mmH / state.worldHeight;

    if (state.player) {
        SDL_FRect pb = state.player->getBounds();
        float px = pb.x * sx + mmX;
        float py = pb.y * sy + mmY;
        SDL_SetRenderDrawColor(m_renderer, 0, 255, 0, 255);
        SDL_FRect pd = {px, py, 4, 4};
        SDL_RenderFillRect(m_renderer, &pd);
    }

    for (const auto& o : state.opponents) {
        if (o && o->isAlive()) {
            SDL_FRect ob = o->getBounds();
            float ox = ob.x * sx + mmX;
            float oy = ob.y * sy + mmY;
            SDL_SetRenderDrawColor(m_renderer, 255, 0, 0, 255);
            SDL_FRect od = {ox, oy, 3, 3};
            SDL_RenderFillRect(m_renderer, &od);
        }
    }

    float vx = state.cameraX * sx + mmX;
    float vw = (float)m_windowWidth * sx;
    SDL_SetRenderDrawColor(m_renderer, 255, 255, 255, 100);
    SDL_FRect vr = {vx, (float)mmY, vw, (float)mmH};
    SDL_RenderRect(m_renderer, &vr);
}


