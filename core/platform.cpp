#include "platform.h"
#include <SDL3/SDL_main.h>
#include <SDL3_image/SDL_image.h>
#include <algorithm>
#include <cctype>
#include <cstring>
#include <sstream>
#include "../entities/health_item.h"

Platform::Platform() = default;

Platform::~Platform() { shutdown(); }

// public usage
bool Platform::initialize() {
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO)) {
        SDL_Log("unable to initialize sdl: %s", SDL_GetError());
        return false;
    }
    if (!TTF_Init()) {
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

    // audio device initialization
    // define the desired audio format using SDL3 enums
    SDL_AudioSpec desired_spec;
    SDL_zero(desired_spec);
    desired_spec.freq = 44100; 
    desired_spec.format = SDL_AUDIO_F32; // SDL3 enum for floating point format
    desired_spec.channels = 2; // stereo

    m_audioDeviceID = SDL_OpenAudioDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &desired_spec);
    if (!m_audioDeviceID) {
        SDL_Log("Failed to open default audio device! SDL_GetError: %s", SDL_GetError());
        SDL_Log("Desired audio spec: %d Hz, %s, %d channels", desired_spec.freq, SDL_GetAudioFormatName(desired_spec.format), desired_spec.channels);
    } else {
        m_audioSpec = desired_spec;
        SDL_Log("Successfully opened audio device %d with actual spec: %d Hz, %s, %d channels", m_audioDeviceID, m_audioSpec.freq, SDL_GetAudioFormatName(m_audioSpec.format), m_audioSpec.channels);

        SDL_PauseAudioDevice(m_audioDeviceID); // unpauses (starts) the device
        SDL_Log("Started audio device %d (unpaused).", m_audioDeviceID);
    }

    // sound manager initialization
    if (m_audioDeviceID) {
        if (!SoundManager::getInstance().initialize(m_audioDeviceID, m_audioSpec)) {
            SDL_Log("Failed to initialize SoundManager even though audio device was opened.");
        } else {
             SDL_Log("SoundManager initialized successfully.");
        }
    } else {
        SDL_Log("Skipping SoundManager initialization due to audio device failure.");
    }

    return true;
}

void Platform::shutdown() {
    if (m_textInputActive) {
        SDL_StopTextInput(m_window); // stop text input
        m_textInputActive = false;
        SDL_Log("Platform: Text input STOPPED during shutdown.");
    }

    SoundManager::getInstance().shutdown();

    TextureManager::getInstance().clearCache();
    FontManager::getInstance().clearCache();

     //audio device shutdown
    if (m_audioDeviceID) {
        // explicitly pause the audio device before closing it.
        SDL_PauseAudioDevice(m_audioDeviceID);
        SDL_Log("Paused audio device %d before closing.", m_audioDeviceID);

        SDL_CloseAudioDevice(m_audioDeviceID);
        m_audioDeviceID = 0;
        SDL_Log("Closed audio device %d.", m_audioDeviceID);
    }

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
        state.screenWidth = m_windowWidth;
        state.screenHeight = m_windowHeight;

        if (state.state == GameStateData::State::PLAYING) {
            state.worldHeight = (float)m_windowHeight; // world height depends on window resize (width does not)
        }

        updateTextInputState(state); // update text input state

        // fixed timestep update loop
        while (accumulator >= FIXED_DELTA_TIME) {
            GameInput input = pollInput(state);
            sim.handleInput(input, FIXED_DELTA_TIME);

            if (input.quit || state.running == false) m_running = false;            

            sim.update(FIXED_DELTA_TIME);
            accumulator -= FIXED_DELTA_TIME;
        }

        render(state);
    }

    // ensure text input is stopped when the loop exits
    if (m_textInputActive) {
        SDL_StopTextInput(m_window);
        m_textInputActive = false;
        SDL_Log("Platform: Text input STOPPED on shutdown.");
    }
}
// END: public usage

void Platform::render(const GameStateData& state) {
    float cameraOffsetX = state.cameraX;
    switch (state.state) {
        case GameStateData::State::MENU:
            renderMainMenu();
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
                    renderBounds.x -= cameraOffsetX;
                    
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
                        
                        float beamY = p.getSpawnY();
                        float startX = p.getSpawnX();
                        bool goingRight = (p.getVelocity().x > 0);

                        // find visual end point
                        float rawEndX = goingRight ? state.worldWidth : 0.0f;
                        float landscapeEndX = findBeamLandscapeIntersection(startX, beamY, goingRight, state.landscape, state.worldWidth);

                        // use the closer endpoint (landscape or world edge)
                        float endX = goingRight ? std::min(rawEndX, landscapeEndX) : std::max(0.0f, landscapeEndX);

                        SDL_Color color = p.getColor();
                        SDL_SetRenderDrawColor(m_renderer, color.r, color.g, color.b, color.a);
                        SDL_RenderLine(m_renderer, startX - cameraOffsetX, beamY, endX - cameraOffsetX,beamY); // render player beam
                    }
                }
            }

            for (const auto& o : state.opponents) {
                if (!o || !o->isAlive()) continue;

                SDL_FRect renderBounds = o->getBounds();
                renderBounds.x -= cameraOffsetX;

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

                    // full intended endpoint
                    float dx = p.getCurrentX() - p.getSpawnX();
                    float dy = p.getCurrentY() - p.getSpawnY();
                    float intendedEndX = p.getSpawnX() + dx * 4.0f;
                    float intendedEndY = p.getSpawnY() + dy * 4.0f;

                    // clip to landscape
                    SDL_FPoint clipped = clipRayToLandscape(p.getSpawnX(), p.getSpawnY(), intendedEndX, intendedEndY, state.landscape);

                    // camera offset
                    SDL_FPoint start = { p.getSpawnX() - cameraOffsetX, p.getSpawnY() };
                    SDL_FPoint end   = { clipped.x - cameraOffsetX, clipped.y };

                    SDL_Color color = p.getColor();
                    SDL_SetRenderDrawColor(m_renderer, color.r, color.g, color.b, color.a);
                    SDL_RenderLine(m_renderer, start.x, start.y, end.x, end.y);
                }
            }

            // render particles
            for (const auto& particle : state.particles) {
                if (particle.isAlive()) { 
                    SDL_FRect renderBounds = { particle.getX(), particle.getY(), particle.getCurrentSize(), particle.getCurrentSize() };
                    renderBounds.x -= cameraOffsetX; // apply camera offset

                    SDL_SetRenderDrawColor(m_renderer, particle.getR(), particle.getG(), particle.getB(), particle.getAlpha());
                    SDL_RenderFillRect(m_renderer, &renderBounds);
                }
            }

            // render landscape
            if (!state.landscape.empty()) {
                SDL_SetRenderDrawColor(m_renderer, 100, 80, 60, 255);
                for (size_t i = 0; i < state.landscape.size() - 1; ++i) {
                    SDL_FPoint p1 = { state.landscape[i].x - cameraOffsetX, state.landscape[i].y };
                    SDL_FPoint p2 = { state.landscape[i + 1].x - cameraOffsetX, state.landscape[i + 1].y };
                    SDL_RenderLine(m_renderer, p1.x, p1.y, p2.x, p2.y);
                }
            }

            // render health items
            for (const auto& item : state.healthItems) {
                if (!item || !item->isAlive()) continue;

                SDL_FRect renderBounds = item->getBounds();
                renderBounds.x -= cameraOffsetX;

                auto itemTexture = TextureManager::getInstance().getTexture(item->getTextureKey(), m_renderer);
                if (itemTexture) {
                    // handle blinking
                    Uint8 originalAlpha = 255;
                    if (item->isBlinking()) {
                         originalAlpha = static_cast<Uint8>(item->getBlinkAlpha());
                    }
                    SDL_SetTextureAlphaMod(itemTexture.get(), originalAlpha);
                    SDL_RenderTexture(m_renderer, itemTexture.get(), nullptr, &renderBounds);
                    SDL_SetTextureAlphaMod(itemTexture.get(), 255); // ...resets alpha for next item
                } else {
                    // fallback rectangle
                    SDL_SetRenderDrawColor(m_renderer, 0, 255, 0, 255);
                    if (item->getType() == HealthItemType::WORLD) {
                        SDL_SetRenderDrawColor(m_renderer, 255, 255, 0, 255);
                    }
                    if (item->isBlinking()) {
                        // blinking effect
                        if (static_cast<int>(SDL_GetTicks() / (static_cast<int>(HealthItem::BLINK_DURATION * 1000) / 2)) % 2 == 0) {
                             SDL_RenderFillRect(m_renderer, &renderBounds);
                        }
                    } else {
                         SDL_RenderFillRect(m_renderer, &renderBounds);
                    }
                }
            }

            renderMinimap(state);
            renderHealthBars(state);
            renderScore(state);

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

// input
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
// END: input

// screens and menus
void Platform::renderMainMenu() {
    SDL_SetRenderDrawColor(m_renderer, 0, 20, 40, 255);
    SDL_RenderClear(m_renderer);    
    SDL_Color white = {255, 255, 255, 255};    
    renderText("SDL3 DEFENDER", m_windowWidth/2 - 100, m_windowHeight/2 - 120, white, FontSize::MEDIUM);

    // button positions
    int buttonWidth = 200;
    int buttonHeight = 50;
    int centerX = m_windowWidth / 2 - buttonWidth / 2;
    int startY = m_windowHeight / 2 - 60;
    int buttonSpacing = 60;

    renderMenuButton(centerX, startY, buttonWidth, buttonHeight, white, "Play");
    renderMenuButton(centerX, startY + buttonSpacing, buttonWidth, buttonHeight, white, "How to Play");
    renderMenuButton(centerX, startY + buttonSpacing * 2, buttonWidth, buttonHeight, white, "Exit");
}

void Platform::renderHowToPlayScreen() {
    SDL_SetRenderDrawColor(m_renderer, 0, 20, 40, 255);
    SDL_RenderClear(m_renderer);

    SDL_Color white = {255, 255, 255, 255};
    SDL_Color yellow = {255, 255, 0, 255};

    int y_pos = 50; // starting Y position for text
    const int line_spacing = 30;
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

    renderCloseButton();
}

void Platform::renderGameOverScreen(const GameStateData& state) {
    SDL_Color white = {255, 255, 255, 255};
    SDL_Color red = {255, 0, 0, 255};

    SDL_SetRenderDrawColor(m_renderer, 0, 0, 0, 255);
    SDL_RenderClear(m_renderer);
    
    renderText("GAME OVER", m_windowWidth / 2 - 100, m_windowHeight / 2 - 60, red, FontSize::LARGE);
    renderText(("Score: " + std::to_string(state.playerScore)).c_str(), m_windowWidth / 2 - 60, m_windowHeight / 2, white, FontSize::MEDIUM);

    renderCloseButton();
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

    renderCloseButton();
}
// END: screens and menus

// HUD (top-bar)
void Platform::renderHealthBars(const GameStateData& state) {
    const int barW = 200;
    const int barH = 10;
    const int barX = 10;
    const int barY = 10;
    const int spacing = 5;
    
    SDL_Color white = {255, 255, 255, 255};
    float pHealth = (float)state.player->getHealth();
    float pMaxHealth = (float)state.player->getMaxHealth();
    float playerHealthRatio = pHealth / pMaxHealth;
    
    renderHealthBar("Player Health:", barX, barY, barW, barH, playerHealthRatio, white);
    
    float worldHealthRatio = (float)state.worldHealth / 10.0f;
    int worldBarY = barY + 20 + barH + spacing;
    renderHealthBar("World Health:", barX, worldBarY, barW, barH, worldHealthRatio, white);    
}

void Platform::renderHealthBar(const char* label, int x, int y, int width, int height, float healthRatio, const SDL_Color& labelColor) {
    renderText(label, x, y, labelColor, FontSize::SMALL);
    
    float fillWidth = std::max(0.0f, width * healthRatio);
    
    SDL_SetRenderDrawColor(m_renderer, 255, 0, 0, 255);
    SDL_FRect bgRect = {(float)x, (float)(y + 20), (float)width, (float)height};
    SDL_RenderFillRect(m_renderer, &bgRect);
    
    SDL_SetRenderDrawColor(m_renderer, 0, 255, 0, 255);
    SDL_FRect fillRect = {(float)x, (float)(y + 20), fillWidth, (float)height};
    SDL_RenderFillRect(m_renderer, &fillRect);
    
    SDL_SetRenderDrawColor(m_renderer, 255, 255, 255, 255);
    SDL_RenderRect(m_renderer, &bgRect);
}

void Platform::renderMinimap(const GameStateData& state) {
    const int mmW = 210;
    const int mmH = 42;
    const int mmX = (state.screenWidth - mmW)/2, mmY = 10;
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

    // render landscape
    if (!state.landscape.empty()) {
        SDL_SetRenderDrawColor(m_renderer, 180, 150, 100, 200);
        float sx = (float)mmW / state.worldWidth;
        float sy = (float)mmH / state.worldHeight;
        for (size_t i = 0; i < state.landscape.size() - 1; ++i) {
            float x1 = state.landscape[i].x * sx + mmX;
            float y1 = state.landscape[i].y * sy + mmY;
            float x2 = state.landscape[i + 1].x * sx + mmX;
            float y2 = state.landscape[i + 1].y * sy + mmY;
            SDL_RenderLine(m_renderer, x1, y1, x2, y2);
        }
    }

    float vx = state.cameraX * sx + mmX;
    float vw = state.screenWidth * sx;
    SDL_SetRenderDrawColor(m_renderer, 255, 255, 255, 100);
    SDL_FRect vr = {vx, (float)mmY, vw, (float)mmH};
    SDL_RenderRect(m_renderer, &vr);
}

void Platform::renderScore(const GameStateData& state) {
    const int barY = 10;    
    SDL_Color white = {255, 255, 255, 255};
    float rightOffset = m_windowWidth - 150;
    
    renderText("Score:", rightOffset, barY, white, FontSize::SMALL);
    std::string scoreStr = std::to_string(state.playerScore);
    renderText(scoreStr.c_str(), m_windowWidth - 90, barY, white, FontSize::SMALL);
}
// END: HUD (top-bar)

// helpers
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

void Platform::renderMenuButton(int x, int y, int width, int height, SDL_Color& textColor, const std::string& text) {
    SDL_FRect bgRect = {(float)x, (float)y, (float)width, (float)height};
    
    SDL_SetRenderDrawColor(m_renderer, 0, 100, 200, 200);
    SDL_RenderFillRect(m_renderer, &bgRect);
    SDL_SetRenderDrawColor(m_renderer, 255, 255, 255, 255);
    SDL_RenderRect(m_renderer, &bgRect);
    
    //centering text
    int textX = x + (width - static_cast<int>(text.length()) * 14) / 2;
    int textY = y + (height - 24) / 2;
    
    renderText(text.c_str(), textX, textY, textColor, FontSize::MEDIUM);
}

void Platform::renderCloseButton() {
    const float size = 20.0f;
    const float y = 10.0f;
    const float x = static_cast<float>(m_windowWidth) - size - y;

    SDL_Color white = {255, 255, 255, 255};
    
    SDL_FRect buttonRect = { x, y, size, size };
    
    // draw background
    SDL_SetRenderDrawColor(m_renderer, 40, 40, 40, 200);
    SDL_RenderFillRect(m_renderer, &buttonRect);
    
    // draw border
    SDL_SetRenderDrawColor(m_renderer, 255, 255, 255, 255);
    SDL_RenderRect(m_renderer, &buttonRect);
    
    FontSize closeButtonFontSize = FontSize::SMALL;
    
    int textX = x + (size - 12) / 2;  // approx centering
    int textY = y + (size - 20) / 2;
    
    renderText("X", textX, textY, white, closeButtonFontSize);
}

// for player beams
float Platform::findBeamLandscapeIntersection(float startX, float beamY, bool goingRight, const std::vector<SDL_FPoint>& landscape, float worldWidth) {
    if (landscape.empty()) return goingRight ? worldWidth : 0.0f;

    // clamp beamY
    if (beamY <= 0) return goingRight ? worldWidth : 0.0f;

    // determine search range
    size_t startIdx = 0;
    size_t endIdx = landscape.size() - 1;

    if (goingRight) {
        // find first segment where x >= startX
        for (size_t i = 0; i < landscape.size() - 1; ++i) {
            float x0 = landscape[i].x;
            float x1 = landscape[i + 1].x;
            if (x1 < startX) continue;

            // this segment or next may contain intersection
            float y0 = landscape[i].y;
            float y1 = landscape[i + 1].y;

            // if beam is above both points, beam continues
            if (beamY < y0 && beamY < y1) {
                // no intersection in this segment
                continue;
            }

            // if beam is below both, it is already on ground - shouldn't happen if projectile was alive
            if (beamY >= y0 && beamY >= y1)                 
                return std::max(startX, x0);// beam hits ground at segment start
            
            // otherwise... interpolate intersection
            // find X where the horizontal beam crosses the straight line segment between (x0,y0) and (x1,y1) 
            // ground line: y = y0 + t*(y1 - y0), x = x0 + t*(x1 - x0)
            float t = (beamY - y0) / (y1 - y0);
            if (t >= 0.0f && t <= 1.0f) {
                float intersectX = x0 + t * (x1 - x0);
                if (intersectX >= startX) {
                    return intersectX;
                }
            }
        }
        // if no intersection found, the beam goes to world edge
        return worldWidth;
    } else {
        // going left: search backward
        for (size_t i = landscape.size() - 1; i > 0; --i) {
            float x0 = landscape[i - 1].x;
            float x1 = landscape[i].x;
            if (x0 > startX) continue;

            float y0 = landscape[i - 1].y;
            float y1 = landscape[i].y;

            if (beamY < y0 && beamY < y1) {
                continue;
            }
            if (beamY >= y0 && beamY >= y1) {
                return std::min(startX, x1);
            }

            float t = (beamY - y0) / (y1 - y0);
            if (t >= 0.0f && t <= 1.0f) {
                float intersectX = x0 + t * (x1 - x0);
                if (intersectX <= startX) {
                    return intersectX;
                }
            }
        }
        return 0.0f;
    }
}

// for opponent projectiles
SDL_FPoint Platform::clipRayToLandscape(float startX, float startY, float endX, float endY, const std::vector<SDL_FPoint>& landscape) const {
    if (landscape.empty()) return {endX, endY};

    // ray: from (startX, startY) to (endX, endY)
    float rayDx = endX - startX;
    float rayDy = endY - startY;
    float bestT = 1.0f; // full length

    // check intersection with each landscape segment
    for (size_t i = 0; i < landscape.size() - 1; ++i) {
        float x0 = landscape[i].x;
        float y0 = landscape[i].y;
        float x1 = landscape[i + 1].x;
        float y1 = landscape[i + 1].y;

        // landscape segment vector
        float segDx = x1 - x0;
        float segDy = y1 - y0;

        // solve: 
        //      startX + t1*rayDx = x0 + t2*segDx
        //      startY + t1*rayDy = y0 + t2*segDy
        float denom = rayDx * segDy - rayDy * segDx;
        if (std::abs(denom) < 1e-6f) continue; // parallel

        float t2 = (rayDx * (startY - y0) - rayDy * (startX - x0)) / denom;
        if (t2 < 0.0f || t2 > 1.0f) continue; // intersection not on segment

        float t1 = (x0 + t2 * segDx - startX) / rayDx;
        if (std::abs(rayDx) < 1e-6f) t1 = (y0 + t2 * segDy - startY) / rayDy;

        if (t1 >= 0.0f && t1 < bestT) {
            bestT = t1;
        }
    }

    // return clipped endpoint
    return {
        startX + bestT * rayDx,
        startY + bestT * rayDy
    };
}
// END: helpers
