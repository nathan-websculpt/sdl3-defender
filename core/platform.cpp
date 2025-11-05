#include "platform.h"
#include <SDL3/SDL_main.h>
#include <SDL3_image/SDL_image.h>
#include <algorithm>
#include <cctype>
#include <cstring>
#include <sstream>
#include "../entities/health_item.h"
#include "globals.h"

Platform::Platform() = default;

Platform::~Platform() { shutdown(); } // TODO: main could just call this

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

    globals.window = SDL_CreateWindow("sdl3 defender", globals.initialWindowWidth, globals.initialWindowHeight, SDL_WINDOW_RESIZABLE);
    if (!globals.window) {
        SDL_Log("failed to create window: %s", SDL_GetError());
        TTF_Quit();
        SDL_Quit();
        return false;
    }

    globals.renderer = SDL_CreateRenderer(globals.window, nullptr);
    if (!globals.renderer) {
        SDL_Log("failed to create renderer: %s", SDL_GetError());
        SDL_DestroyWindow(globals.window);
        TTF_Quit();
        SDL_Quit();
        return false;
    }

    // attempt to enable VSync using SDL_SetRenderVSync
    if (SDL_SetRenderVSync(globals.renderer, 1) != 0) { // 1 enables VSync, 0 disables
        // if setting VSync fails, log it but continue (maybe VSync isn't supported on this display/driver)
        SDL_Log("Warning: Failed to enable VSync: %s. Running without VSync.", SDL_GetError());
    } else {
        SDL_Log("VSync successfully enabled.");
    }

    SDL_GetWindowSize(globals.window, &globals.windowWidth, &globals.windowHeight); // TODO: count occurances

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
        SDL_StopTextInput(globals.window); // stop text input
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

    if (globals.renderer) {
        SDL_DestroyRenderer(globals.renderer);
        globals.renderer = nullptr;
    }
    if (globals.window) {
        SDL_DestroyWindow(globals.window);
        globals.window = nullptr;
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

        SDL_GetWindowSize(globals.window, &globals.windowWidth, &globals.windowHeight); // TODO:

        auto& state = sim.getState();

        if (state.state == GameStateData::State::PLAYING) {
            state.worldHeight = (float)globals.windowHeight; // world height depends on window resize (width does not)
            // TODO: unify and use if like in game.cpp
        }

        updateTextInputState(state); // update text input state

        // timestep update loop
        while (accumulator >= FIXED_DELTA_TIME) {
            GameInput input = pollInput(state);
            sim.handleInput(input, FIXED_DELTA_TIME);

            if (input.quit || state.running == false) m_running = false;            

            sim.update(FIXED_DELTA_TIME);
            accumulator -= FIXED_DELTA_TIME;
        }

        RenderMain::render(state); // todo: should render go into while loop? ^^^
    }

    // ensure text input is stopped when the loop exits
    if (m_textInputActive) {
        SDL_StopTextInput(globals.window);
        m_textInputActive = false;
        SDL_Log("Platform: Text input STOPPED on shutdown.");
    }
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
        SDL_StartTextInput(globals.window);
        m_textInputActive = true;
        SDL_Log("Platform: Text input STARTED for high score entry.");
    } else if (!shouldTextInputBeActive && m_textInputActive) {
        // stop text input
        SDL_StopTextInput(globals.window);
        m_textInputActive = false;
        SDL_Log("Platform: Text input STOPPED.");
    }
}