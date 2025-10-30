#pragma once
#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <memory>
#include <string>
#include "game.h"
#include "texture_manager.h"
#include "font_manager.h"
#include "sound_manager.h"

class Platform {
public:
    Platform();
    ~Platform();

    bool initialize();
    void run(Game& sim);
    void shutdown();

private:
    SDL_Window* m_window = nullptr;
    SDL_Renderer* m_renderer = nullptr;
    int m_windowWidth = 800;
    int m_windowHeight = 600;
    bool m_running = true;
    bool m_textInputActive = false; // track if text input is currently active

    SDL_AudioDeviceID m_audioDeviceID = 0;
    SDL_AudioSpec m_audioSpec;

    void render(const GameStateData& state);

    // input
    GameInput pollInput(const GameStateData& state);
    void updateTextInputState(const GameStateData& state);

    // menus and screens
    void renderMainMenu();
    void renderHowToPlayScreen();
    void renderGameOverScreen(const GameStateData& state);
    void renderHighScoreEntryScreen(const GameStateData& state);

    // HUD (top bar)
    void renderHealthBars(const GameStateData& state);
    void renderHealthBar(const char* label, int x, int y, int width, int height, float healthRatio, const SDL_Color& labelColor = {255, 255, 255, 255});
    void renderMinimap(const GameStateData& state);
    void renderScore(const GameStateData& state);

    // helpers
    void renderText(const char* text, int x, int y, const SDL_Color& color, FontSize size);
    void renderMenuButton(int x, int y, int width, int height, SDL_Color& textColor, const std::string& text);
    void renderCloseButton();
    float findBeamLandscapeIntersection(float startX, float beamY, bool goingRight, const std::vector<SDL_FPoint>& landscape, float worldWidth);
};