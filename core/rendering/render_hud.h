#pragma once

#include "render_helper.h"

class RenderHud {
public:
    RenderHud();

        // HUD (top bar)
    void renderHealthBars(const GameStateData& state);
    void renderHealthBar(const char* label, int x, int y, int width, int height, float healthRatio, const SDL_Color& labelColor = {255, 255, 255, 255});
    void renderMinimap(const GameStateData& state);
    void renderScore(const GameStateData& state);

    RenderHelper m_renderHelper;
};
