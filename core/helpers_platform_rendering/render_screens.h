#pragma once
#include "render_helper.h"

class RenderScreens {
public:
    static void renderMainMenu();
    static void renderHowToPlayScreen();
    static void renderGameOverScreen(const GameStateData& state);
    static void renderHighScoreEntryScreen(const GameStateData& state);
};