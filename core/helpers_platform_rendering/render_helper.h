#pragma once
#include <SDL3/SDL.h>
#include "../config.h"
#include "../globals.h"
#include "../game.h"
#include "../managers/texture_manager.h"
#include "../managers/font_manager.h"

struct RenderColors {
    static constexpr SDL_Color primary = {0, 20, 40, 255};            // game background
    static constexpr SDL_Color secondary = {46, 62, 85, 155};            
    static constexpr SDL_Color textPrimary = {220, 161, 29, 255};     // gold titles 
    static constexpr SDL_Color textSecondary = {245, 245, 245, 220};  // dim white

    static constexpr SDL_Color white = {255, 255, 255, 255};
    static constexpr SDL_Color whiteDim1 = {255, 255, 255, 100};
    static constexpr SDL_Color red = {255, 0, 0, 255};    
    static constexpr SDL_Color green = {0, 255, 0, 255};    
    static constexpr SDL_Color yellowLight = {223, 245, 39, 200};    // minimap dot for player
    static constexpr SDL_Color gold1 = {100, 80, 60, 255};           // landscape
    static constexpr SDL_Color gold2 = {180, 150, 100, 200};         // minimap landscape
    static constexpr SDL_Color blueDark1 = {0, 30, 50, 220};          // HUD background
    static constexpr SDL_Color blueDark2 = {0, 40, 80, 200};          // minimap background
    static constexpr SDL_Color greySlate = {40, 40, 40, 200};        // close btn background  
    static constexpr SDL_Color blueLight = {0, 100, 200, 200};       // menu btn background  
    static constexpr SDL_Color grey = {200, 200, 200, 255};          // HUD separator
};

class RenderHelper {
public:
    static void setRenderDrawColor(const SDL_Color& c);
    static void renderCloseButton();
    static void renderMenuButton(int x, int y, int width, int height, const SDL_Color& textColor, const std::string& text);
    static void renderText(const char* text, int x, int y, const SDL_Color& color, FontSize size);
};