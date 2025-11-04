#pragma once

#include "render_helper.h"

class RenderHud {
public:
    static void renderHealthBars(const GameStateData& state);
    static void renderMinimap(const GameStateData& state);
    static void renderScore(const GameStateData& state);

private:
    static void renderHealthBar(const char* label, int x, int y, int width, int height, float healthRatio, const SDL_Color& labelColor = {255, 255, 255, 255});
};
