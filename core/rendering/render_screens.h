#pragma once

#include "render_helper.h"

class RenderScreens {
public:
    RenderScreens();
    
    // menus and screens
    void renderMainMenu();
    void renderHowToPlayScreen();
    void renderGameOverScreen(const GameStateData& state);
    void renderHighScoreEntryScreen(const GameStateData& state);

    RenderHelper m_renderHelper;
};