#pragma once
#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <memory>
#include <string>
#include "game.h"
#include "managers/texture_manager.h"
#include "managers/font_manager.h"
#include "managers/sound_manager.h"
#include "rendering/render_hud.h"
#include "rendering/render_screens.h"

class Platform {
public:
    Platform();
    ~Platform();

    bool initialize();
    void run(Game& sim);
    void shutdown();

private:
    bool m_running = true;
    bool m_textInputActive = false; // track if text input is currently active

    SDL_AudioDeviceID m_audioDeviceID = 0;
    SDL_AudioSpec m_audioSpec;

    void render(const GameStateData& state);

    // input
    GameInput pollInput(const GameStateData& state);
    void updateTextInputState(const GameStateData& state);

    // helpers
    float findBeamLandscapeIntersection(float startX, float beamY, bool goingRight, const std::vector<SDL_FPoint>& landscape, float worldWidth); // for player beams (horizontal)
    SDL_FPoint clipRayToLandscape(float startX, float startY, float endX, float endY, const std::vector<SDL_FPoint>& landscape) const; // for opponent projectiles

    // RenderScreens m_renderScreens;
    RenderHud m_renderHud;
};