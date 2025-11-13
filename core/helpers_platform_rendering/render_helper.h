#pragma once
#include <SDL3/SDL.h>
#include "../config.h"
#include "../globals.h"
#include "../game.h"
#include "../managers/texture_manager.h"
#include "../managers/font_manager.h"

struct RenderColors {
    static constexpr SDL_Color white = {255, 255, 255, 255};
    static constexpr SDL_Color yellow = {255, 255, 0, 255};
    static constexpr SDL_Color red = {255, 0, 0, 255};

};

class RenderHelper {
public:
    static void renderCloseButton();
    static void renderMenuButton(int x, int y, int width, int height, const SDL_Color& textColor, const std::string& text);
    static void renderText(const char* text, int x, int y, const SDL_Color& color, FontSize size);
};