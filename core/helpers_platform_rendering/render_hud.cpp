#include "render_hud.h"

void RenderHud::renderHudBackground() {
    // background
    RenderHelper::setRenderDrawColor(RenderColors::blueDark1);
    SDL_FRect hudBg = {0.0f, 0.0f, static_cast<float>(globals.windowWidth), static_cast<float>(Config::Game::HUD_HEIGHT)};
    SDL_RenderFillRect(globals.renderer, &hudBg);

    // separator line
    RenderHelper::setRenderDrawColor(RenderColors::grey);
    SDL_RenderLine(globals.renderer, 0.0f, static_cast<float>(Config::Game::HUD_HEIGHT), static_cast<float>(globals.windowWidth), static_cast<float>(Config::Game::HUD_HEIGHT));
}

void RenderHud::renderHealthBars(const GameStateData& state) {
    const int barW = 200;
    const int barH = 10;
    const int barX = 2;
    const int barY = 2;
    const int spacing = 5;

    float pHealth = (float)state.player->getHealth();
    float pMaxHealth = (float)state.player->getMaxHealth();
    float playerHealthRatio = pHealth / pMaxHealth;
    
    renderHealthBar("Player Health:", barX, barY, barW, barH, playerHealthRatio, RenderColors::white);
    
    float worldHealthRatio = (float)state.worldHealth / 10.0f;
    int worldBarY = barY + 20 + barH + spacing;
    renderHealthBar("World Health:", barX, worldBarY, barW, barH, worldHealthRatio, RenderColors::white);    
}

void RenderHud::renderHealthBar(const char* label, int x, int y, int width, int height, float healthRatio, const SDL_Color& labelColor) {
    RenderHelper::renderText(label, x, y, labelColor, FontSize::SMALL);
    
    float fillWidth = std::max(0.0f, width * healthRatio);
    
    RenderHelper::setRenderDrawColor(RenderColors::red);
    SDL_FRect bgRect = {(float)x, (float)(y + 20), (float)width, (float)height};
    SDL_RenderFillRect(globals.renderer, &bgRect);
    
    RenderHelper::setRenderDrawColor(RenderColors::green);
    SDL_FRect fillRect = {(float)x, (float)(y + 20), fillWidth, (float)height};
    SDL_RenderFillRect(globals.renderer, &fillRect);
    
    RenderHelper::setRenderDrawColor(RenderColors::white);
    SDL_RenderRect(globals.renderer, &bgRect);
}

void RenderHud::renderMinimap(const GameStateData& state) {
    const int mmW = 210;
    const int mmH = 42;
    const int mmX = (globals.windowWidth - mmW)/2;
    const int mmY = 20;

    RenderHelper::setRenderDrawColor(RenderColors::blueDark2);
    SDL_FRect mm = {(float)mmX, (float)mmY, (float)mmW, (float)mmH};
    SDL_RenderFillRect(globals.renderer, &mm);
    RenderHelper::setRenderDrawColor(RenderColors::blueLight);
    SDL_RenderRect(globals.renderer, &mm);

    float sx = (float)mmW / Config::Game::WORLD_WIDTH;
    float sy = (float)mmH / globals.windowHeight;

    // light yellow dot for player
    if (state.player) {
        SDL_FRect pb = state.player->getBounds();
        float px = (pb.x * sx + mmX) - 1.0f;
        float py = pb.y * sy + mmY;
        RenderHelper::setRenderDrawColor(RenderColors::yellowLight);
        SDL_FRect pd = {px, py, 3, 3};
        SDL_RenderFillRect(globals.renderer, &pd);
    }

    // red dots for opponents
    for (const auto& o : state.opponents) {
        if (o && o->isAlive()) {
            SDL_FRect ob = o->getBounds();
            float ox = (ob.x * sx + mmX) - 1.0f;
            float oy = ob.y * sy + mmY;
            RenderHelper::setRenderDrawColor(RenderColors::red);
            SDL_FRect od = {ox, oy, 3, 3};
            SDL_RenderFillRect(globals.renderer, &od);
        }
    }

    // green dots for health
    for (const auto& h : state.healthItems) {
        if (h && h->isAlive()) {
            SDL_FRect hb = h->getBounds();
            float hx = (hb.x * sx + mmX) - 1.0f;
            float hy = hb.y * sy + mmY;
            RenderHelper::setRenderDrawColor(RenderColors::green);
            SDL_FRect hd = {hx, hy, 3, 3};
            SDL_RenderFillRect(globals.renderer, &hd);
        }
    }

    // render landscape
    if (!state.landscape.empty()) {
        RenderHelper::setRenderDrawColor(RenderColors::gold2);
        for (size_t i = 0; i < state.landscape.size() - 1; ++i) {
            float x1 = state.landscape[i].x * sx + mmX;
            float y1 = state.landscape[i].y * sy + mmY;
            float x2 = state.landscape[i + 1].x * sx + mmX;
            float y2 = state.landscape[i + 1].y * sy + mmY;
            SDL_RenderLine(globals.renderer, x1, y1, x2, y2);
        }
    }

    float vx = state.cameraX * sx + mmX;
    float vw = globals.windowWidth * sx;
    RenderHelper::setRenderDrawColor(RenderColors::whiteDim1);
    SDL_FRect vr = {vx, (float)mmY, vw, (float)mmH};
    SDL_RenderRect(globals.renderer, &vr);
}

void RenderHud::renderScore(const GameStateData& state) {
    const int barY = 10;    
    float rightOffset = globals.windowWidth - 150;
    
    RenderHelper::renderText("Score:", rightOffset, barY, RenderColors::white, FontSize::SMALL);
    std::string scoreStr = std::to_string(state.playerScore);
    RenderHelper::renderText(scoreStr.c_str(), globals.windowWidth - 90, barY, RenderColors::white, FontSize::SMALL);
}