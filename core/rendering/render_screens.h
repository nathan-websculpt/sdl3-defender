#pragma once

#include "render_helper.h"

class RenderScreens {
public:
    RenderScreens();
    
    void renderMainMenu();
    void renderHowToPlayScreen();
    void renderGameOverScreen(const GameStateData& state);
    void renderHighScoreEntryScreen(const GameStateData& state);
};