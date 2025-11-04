#include "render_screens.h"

RenderScreens::RenderScreens() = default;

void RenderScreens::renderMainMenu() {
    SDL_SetRenderDrawColor(globals.renderer, 0, 20, 40, 255);
    SDL_RenderClear(globals.renderer);    
    SDL_Color white = {255, 255, 255, 255};    
    RenderHelper::renderText("SDL3 DEFENDER", globals.windowWidth/2 - 100, globals.windowHeight/2 - 120, white, FontSize::MEDIUM);

    // button positions
    int buttonWidth = 200;
    int buttonHeight = 50;
    int centerX = globals.windowWidth / 2 - buttonWidth / 2;
    int startY = globals.windowHeight / 2 - 60;
    int buttonSpacing = 60;

    RenderHelper::renderMenuButton(centerX, startY, buttonWidth, buttonHeight, white, "Play");
    RenderHelper::renderMenuButton(centerX, startY + buttonSpacing, buttonWidth, buttonHeight, white, "How to Play");
    RenderHelper::renderMenuButton(centerX, startY + buttonSpacing * 2, buttonWidth, buttonHeight, white, "Exit");
}

void RenderScreens::renderHowToPlayScreen() {
    SDL_SetRenderDrawColor(globals.renderer, 0, 20, 40, 255);
    SDL_RenderClear(globals.renderer);

    SDL_Color white = {255, 255, 255, 255};
    SDL_Color yellow = {255, 255, 0, 255};

    int y_pos = 50; // starting Y position for text
    const int line_spacing = 30;
    const int opponent_image_size = 30;

    RenderHelper::renderText("HOW TO PLAY", globals.windowWidth/2 - 100, y_pos, yellow, FontSize::MEDIUM);
    y_pos += line_spacing + 20;
    RenderHelper::renderText("CONTROLS:", globals.windowWidth/2 - 80, y_pos, white, FontSize::SMALL);
    y_pos += line_spacing;
    RenderHelper::renderText("- Move: Arrow Keys or WASD", globals.windowWidth/2 - 150, y_pos, white, FontSize::SMALL);
    y_pos += line_spacing;
    RenderHelper::renderText("- Shoot: Spacebar", globals.windowWidth/2 - 150, y_pos, white, FontSize::SMALL);
    y_pos += line_spacing;
    RenderHelper::renderText("- Boost: Hold 'C' or Shift", globals.windowWidth/2 - 150, y_pos, white, FontSize::SMALL);
    y_pos += line_spacing + 10;
    RenderHelper::renderText("OPPONENTS:", globals.windowWidth/2 - 80, y_pos, white, FontSize::SMALL);
    y_pos += line_spacing;

    // bombs
    auto basicTexture = TextureManager::getInstance().getTexture(Config::Textures::BASIC_OPPONENT, globals.renderer);
    if (basicTexture) {
        SDL_FRect imageRect = { (float)(globals.windowWidth/2 - 430), (float)y_pos, (float)opponent_image_size, (float)opponent_image_size };
        SDL_RenderTexture(globals.renderer, basicTexture.get(), nullptr, &imageRect);
    }
    RenderHelper::renderText("Bombs: Do not shoot at you, but damage the world if they reach the bottom - worth 300 points.", globals.windowWidth/2 - 390, y_pos, white, FontSize::SMALL);
    y_pos += line_spacing + 5;

    // aggressive
    auto aggressiveTexture = TextureManager::getInstance().getTexture(Config::Textures::AGGRESSIVE_OPPONENT, globals.renderer);
    if (aggressiveTexture) {
        SDL_FRect imageRect = { (float)(globals.windowWidth/2 - 430), (float)y_pos, (float)opponent_image_size, (float)opponent_image_size };
        SDL_RenderTexture(globals.renderer, aggressiveTexture.get(), nullptr, &imageRect);
    }
    RenderHelper::renderText("Aggressive: Chases the player, fires aimed shots - worth 100 points.", globals.windowWidth/2 - 390, y_pos, white, FontSize::SMALL);
    y_pos += line_spacing + 5; 

    // sniper
    auto sniperTexture = TextureManager::getInstance().getTexture(Config::Textures::SNIPER_OPPONENT, globals.renderer);
    if (sniperTexture) {
        SDL_FRect imageRect = { (float)(globals.windowWidth/2 - 430), (float)y_pos, (float)opponent_image_size, (float)opponent_image_size };
        SDL_RenderTexture(globals.renderer, sniperTexture.get(), nullptr, &imageRect);
    }
    RenderHelper::renderText("Sniper: Moves slowly, fires faster with more accuracy - worth 100 points.", globals.windowWidth/2 - 390, y_pos, white, FontSize::SMALL);
    y_pos += line_spacing + 30; 

    RenderHelper::renderText("Goal: Destroy opponents, prevent bombs from damaging world.", globals.windowWidth/2 - 200, y_pos, white, FontSize::SMALL);
    y_pos += line_spacing + 20;
    RenderHelper::renderText("Press ESC or ENTER to return to the menu.", globals.windowWidth/2 - 150, y_pos, white, FontSize::SMALL);

    RenderHelper::renderCloseButton();
}

void RenderScreens::renderGameOverScreen(const GameStateData& state) {
    SDL_Color white = {255, 255, 255, 255};
    SDL_Color red = {255, 0, 0, 255};

    SDL_SetRenderDrawColor(globals.renderer, 0, 0, 0, 255);
    SDL_RenderClear(globals.renderer);
    
    RenderHelper::renderText("GAME OVER", globals.windowWidth / 2 - 100, globals.windowHeight / 2 - 60, red, FontSize::LARGE);
    RenderHelper::renderText(("Score: " + std::to_string(state.playerScore)).c_str(), globals.windowWidth / 2 - 60, globals.windowHeight / 2, white, FontSize::MEDIUM);

    RenderHelper::renderCloseButton();
}

void RenderScreens::renderHighScoreEntryScreen(const GameStateData& state) {
    SDL_Color white = {255, 255, 255, 255};
    SDL_Color yellow = {255, 255, 0, 255};

    SDL_SetRenderDrawColor(globals.renderer, 0, 0, 0, 255);
    SDL_RenderClear(globals.renderer);
    RenderHelper::renderText("NEW HIGH SCORE!", globals.windowWidth / 2 - 120, globals.windowHeight / 2 - 100, yellow, FontSize::LARGE);
    RenderHelper::renderText(("Position: #" + std::to_string(state.highScoreIndex + 1)).c_str(), globals.windowWidth / 2 - 80, globals.windowHeight / 2 - 50, white, FontSize::MEDIUM);
    RenderHelper::renderText(("Score: " + std::to_string(state.playerScore)).c_str(), globals.windowWidth / 2 - 60, globals.windowHeight / 2 - 20, white, FontSize::MEDIUM);
    RenderHelper::renderText("Enter Name (max 10 chars):", globals.windowWidth / 2 - 140, globals.windowHeight / 2 + 20, white, FontSize::SMALL);
    RenderHelper::renderText((state.highScoreNameInput + "_").c_str(), globals.windowWidth / 2 - 40, globals.windowHeight / 2 + 50, white, FontSize::MEDIUM);

    RenderHelper::renderCloseButton();
}