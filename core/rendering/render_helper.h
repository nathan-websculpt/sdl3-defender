#pragma once
#include <SDL3/SDL.h>
#include "../config.h"
#include "../globals.h"
#include "../game.h"
#include "../managers/texture_manager.h"
#include "../managers/font_manager.h"

class RenderHelper {
public:
    RenderHelper();

    void renderText(const char* text, int x, int y, const SDL_Color& color, FontSize size);
    void renderCloseButton();
    void renderMenuButton(int x, int y, int width, int height, SDL_Color& textColor, const std::string& text);
};