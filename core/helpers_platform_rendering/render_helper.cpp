#include "render_helper.h"

// wraps SDL_SetRenderDrawColor - easier to keep SDL_Colors in one place
// pass RenderColors::colorxyz into this to set SDL Draw Color
void RenderHelper::setRenderDrawColor(const SDL_Color& c) {
    SDL_SetRenderDrawColor(globals.renderer, c.r, c.g, c.b, c.a);
}

void RenderHelper::renderCloseButton() {
    const float size = 20.0f;
    const float y = 10.0f;
    const float x = static_cast<float>(globals.windowWidth) - size - y;
    
    SDL_FRect buttonRect = { x, y, size, size };
    
    // draw background
    setRenderDrawColor(RenderColors::greySlate);
    SDL_RenderFillRect(globals.renderer, &buttonRect);
    
    // draw border
    setRenderDrawColor(RenderColors::white);
    SDL_RenderRect(globals.renderer, &buttonRect);
    
    FontSize closeButtonFontSize = FontSize::SMALL;
    
    int textX = x + (size - 12) / 2;  // approx centering
    int textY = y + (size - 20) / 2;
    
    renderText("X", textX, textY, RenderColors::white, closeButtonFontSize);
}

void RenderHelper::renderMenuButton(int x, int y, int width, int height, const SDL_Color& textColor, const std::string& text) {
    SDL_FRect bgRect = {(float)x, (float)y, (float)width, (float)height};
    
    setRenderDrawColor(RenderColors::blueLight);
    SDL_RenderFillRect(globals.renderer, &bgRect);
    setRenderDrawColor(RenderColors::white);
    SDL_RenderRect(globals.renderer, &bgRect);
    
    //centering text
    int textX = x + (width - static_cast<int>(text.length()) * 14) / 2;
    int textY = y + (height - 24) / 2;
    
    renderText(text.c_str(), textX, textY, textColor, FontSize::MEDIUM);
}

void RenderHelper::renderText(const char* text, int x, int y, const SDL_Color& color, FontSize sizeEnum) {
    int fontSize{16}; 
    switch (sizeEnum) {
        case FontSize::SMALL:
            fontSize = 16;
            break;
        case FontSize::MEDIUM: 
            fontSize = 24;
            break;        
        case FontSize::LARGE:
            fontSize = 36;
            break;        
        case FontSize::GRANDELOCO:
            fontSize = 52;
            break;        
    }

    auto font = FontManager::getInstance().getFont(Config::Fonts::DEFAULT_FONT_FILE, fontSize);
    if (!font) {
        SDL_Log("Failed to get font from manager");
        return; 
    }

    SDL_Surface* fontSurface = TTF_RenderText_Solid(font.get(), text, strlen(text), color);
    if (!fontSurface) {
        SDL_Log("Text Render failed: %s", SDL_GetError());
        return;
    }

    SDL_Texture* fontTexture = SDL_CreateTextureFromSurface(globals.renderer, fontSurface);
    if (!fontTexture) {
        SDL_DestroySurface(fontSurface);
        SDL_Log("Failed to create texture from font surface: %s", SDL_GetError());
        return;
    }

    SDL_FRect dst = { (float)x, (float)y, (float)fontSurface->w, (float)fontSurface->h };
    SDL_RenderTexture(globals.renderer, fontTexture, nullptr, &dst);

    SDL_DestroyTexture(fontTexture);
    SDL_DestroySurface(fontSurface);
}