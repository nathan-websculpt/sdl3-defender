#include "render_screens.h"

void RenderScreens::renderMainMenu() {
    RenderHelper::setRenderDrawColor(RenderColors::primary);
    SDL_RenderClear(globals.renderer);      
    RenderHelper::renderText("SDL3 DEFENDER", globals.windowWidth/2 - 100, globals.windowHeight/2 - 120, RenderColors::white, FontSize::MEDIUM);

    // button positions
    int buttonWidth = 200;
    int buttonHeight = 50;
    int centerX = globals.windowWidth / 2 - buttonWidth / 2;
    int startY = globals.windowHeight / 2 - 60;
    int buttonSpacing = 60;

    RenderHelper::renderMenuButton(centerX, startY, buttonWidth, buttonHeight, RenderColors::white, "Play");
    startY += buttonSpacing;
    RenderHelper::renderMenuButton(centerX, startY, buttonWidth, buttonHeight, RenderColors::white, "How to Play");
    startY += buttonSpacing;
    RenderHelper::renderMenuButton(centerX, startY, buttonWidth, buttonHeight, RenderColors::white, "High Scores");
    startY += buttonSpacing;
    RenderHelper::renderMenuButton(centerX, startY, buttonWidth, buttonHeight, RenderColors::white, "Exit");
}

void RenderScreens::renderHowToPlayScreen() {
    RenderHelper::setRenderDrawColor(RenderColors::primary);
    SDL_RenderClear(globals.renderer);

    int y_pos = 5; // starting Y position for text
    const int line_spacing = 35;
    const int line_spacing_lg = 50;
    const int line_spacing_xl = 70;
    const int opponent_image_size = 30;

    SDL_FRect backgroundRect = { 
        static_cast<float>(globals.windowWidth/2 - 335), 
        0.0f, 
        670.0f, 
        static_cast<float>(globals.windowHeight)
    };
    
    RenderHelper::setRenderDrawColor(RenderColors::secondary);
    SDL_RenderFillRect(globals.renderer, &backgroundRect);

    const int x_start1 = globals.windowWidth/2 - 140;
    const int x_start2 = globals.windowWidth/2 - 200;
    const int x_start3 = globals.windowWidth/2 - 240;
    const int x_start4 = globals.windowWidth/2 - 255;

    RenderHelper::renderText("HOW TO PLAY", x_start1, y_pos, RenderColors::textPrimary, FontSize::LARGE);
    y_pos += line_spacing_xl;
    RenderHelper::renderText("CONTROLS:", x_start4, y_pos, RenderColors::textSecondary, FontSize::MEDIUM);
    y_pos += line_spacing;
    RenderHelper::renderText("- Move: Arrow Keys or WASD", x_start2, y_pos, RenderColors::textSecondary, FontSize::MEDIUM);
    y_pos += line_spacing;
    RenderHelper::renderText("- Shoot: Spacebar", x_start2, y_pos, RenderColors::textSecondary, FontSize::MEDIUM);
    y_pos += line_spacing;
    RenderHelper::renderText("- Boost: Hold 'C' or Shift", x_start2, y_pos, RenderColors::textSecondary, FontSize::MEDIUM);
    y_pos += line_spacing_xl;

    // bombs
    auto basicTexture = TextureManager::getInstance().getTexture(Config::Textures::BASIC_OPPONENT, globals.renderer);
    if (basicTexture) {
        SDL_FRect imageRect = { (float)x_start3, (float)y_pos, (float)opponent_image_size, (float)opponent_image_size };
        SDL_RenderTexture(globals.renderer, basicTexture.get(), nullptr, &imageRect);
    }
    RenderHelper::renderText("Bombs: worth 300 points", x_start2, y_pos, RenderColors::textSecondary, FontSize::MEDIUM);
    y_pos += line_spacing;
    RenderHelper::renderText("Do not shoot at you, but damage the", x_start4, y_pos, RenderColors::textSecondary, FontSize::MEDIUM);
    y_pos += line_spacing;
    RenderHelper::renderText("world if they reach the bottom", x_start4, y_pos, RenderColors::textSecondary, FontSize::MEDIUM);
    y_pos += line_spacing_lg;

    // aggressive
    auto aggressiveTexture = TextureManager::getInstance().getTexture(Config::Textures::AGGRESSIVE_OPPONENT, globals.renderer);
    if (aggressiveTexture) {
        SDL_FRect imageRect = { (float)x_start3, (float)y_pos, (float)opponent_image_size, (float)opponent_image_size };
        SDL_RenderTexture(globals.renderer, aggressiveTexture.get(), nullptr, &imageRect);
    }
    RenderHelper::renderText("Aggressive: worth 100 points", x_start2, y_pos, RenderColors::textSecondary, FontSize::MEDIUM);
    y_pos += line_spacing;
    RenderHelper::renderText("Chases the player and fires aimed shots,", x_start4, y_pos, RenderColors::textSecondary, FontSize::MEDIUM);
    y_pos += line_spacing;
    RenderHelper::renderText("but lacks accuracy", x_start4, y_pos, RenderColors::textSecondary, FontSize::MEDIUM);
    y_pos += line_spacing_lg;

    // sniper
    auto sniperTexture = TextureManager::getInstance().getTexture(Config::Textures::SNIPER_OPPONENT, globals.renderer);
    if (sniperTexture) {
        SDL_FRect imageRect = { (float)x_start3, (float)y_pos, (float)opponent_image_size, (float)opponent_image_size };
        SDL_RenderTexture(globals.renderer, sniperTexture.get(), nullptr, &imageRect);
    }
    RenderHelper::renderText("Sniper: worth 100 points", x_start2, y_pos, RenderColors::textSecondary, FontSize::MEDIUM);
    y_pos += line_spacing;
    RenderHelper::renderText("Fires faster and with more accuracy,", x_start4, y_pos, RenderColors::textSecondary, FontSize::MEDIUM);
    y_pos += line_spacing;
    RenderHelper::renderText("but moves slowly", x_start4, y_pos, RenderColors::textSecondary, FontSize::MEDIUM);
    y_pos += line_spacing_lg;

    RenderHelper::renderText("Goal: Destroy opponents while", x_start4, y_pos, RenderColors::textSecondary, FontSize::MEDIUM);
    y_pos += line_spacing;
    RenderHelper::renderText("preventing bombs from", x_start4, y_pos, RenderColors::textSecondary, FontSize::MEDIUM);
    y_pos += line_spacing;
    RenderHelper::renderText("damaging world", x_start4, y_pos, RenderColors::textSecondary, FontSize::MEDIUM);
    y_pos += line_spacing_xl;
    RenderHelper::renderText("Press ESC or ENTER to return to the menu.", globals.windowWidth/2 - 280, y_pos, RenderColors::textSecondary, FontSize::SMALL);

    RenderHelper::renderCloseButton();
}

void RenderScreens::renderGameOverScreen(const GameStateData& state) {
    RenderHelper::setRenderDrawColor(RenderColors::primary);
    SDL_RenderClear(globals.renderer);
    
    RenderHelper::renderText("GAME OVER", globals.windowWidth / 2 - 130, globals.windowHeight / 2 - 60, RenderColors::textPrimary, FontSize::LARGE);
    RenderHelper::renderText(("Score: " + std::to_string(state.playerScore)).c_str(), globals.windowWidth / 2 - 80, globals.windowHeight / 2, RenderColors::textSecondary, FontSize::MEDIUM);

    RenderHelper::renderCloseButton();
}

void RenderScreens::renderHighScoreEntryScreen(const GameStateData& state) {
    RenderHelper::setRenderDrawColor(RenderColors::primary);
    SDL_RenderClear(globals.renderer);
    RenderHelper::renderText("NEW HIGH SCORE!", globals.windowWidth / 2 - 175, globals.windowHeight / 2 - 130, RenderColors::textPrimary, FontSize::LARGE);
    RenderHelper::renderText(("Position: #" + std::to_string(state.highScoreIndex + 1)).c_str(), globals.windowWidth / 2 - 85, globals.windowHeight / 2 - 60, RenderColors::textSecondary, FontSize::MEDIUM);
    RenderHelper::renderText(("Score: " + std::to_string(state.playerScore)).c_str(), globals.windowWidth / 2 - 65, globals.windowHeight / 2 - 20, RenderColors::textSecondary, FontSize::MEDIUM);
    RenderHelper::renderText("Enter Name (max 10 chars):", globals.windowWidth / 2 - 125, globals.windowHeight / 2 + 50, RenderColors::textSecondary, FontSize::SMALL);
    RenderHelper::renderText((state.highScoreNameInput + "_").c_str(), globals.windowWidth / 2 - 25, globals.windowHeight / 2 + 70, RenderColors::textSecondary, FontSize::MEDIUM);

    RenderHelper::renderCloseButton();
}

void RenderScreens::renderViewHighScoresScreen(const GameStateData& state) {
    RenderHelper::setRenderDrawColor(RenderColors::primary);
    SDL_RenderClear(globals.renderer);

    int y_pos = globals.windowHeight / 2 - 150.0f;
    int titleY = y_pos - 40;
    int listStartY = y_pos + 20;
    int listEndY = listStartY + 300.0f;
    
    SDL_FRect backgroundRect = { 
        static_cast<float>(globals.windowWidth/2 - 180), 
        static_cast<float>(titleY - 30), 
        380.0f, 
        static_cast<float>(listEndY - titleY + 150) 
    };
    
    RenderHelper::setRenderDrawColor(RenderColors::secondary);
    SDL_RenderFillRect(globals.renderer, &backgroundRect);

    RenderHelper::renderText("HIGH SCORES", globals.windowWidth/2 - 130, titleY, RenderColors::textPrimary, FontSize::LARGE);

    int currentY = listStartY;
    for (int i = 0; i < static_cast<int>(state.highScores.size()); ++i) {
        const auto& entry = state.highScores[i];
        std::string rankStr = std::to_string(i + 1) + ". " + entry.name + " - " + std::to_string(entry.score);
        RenderHelper::renderText(rankStr.c_str(), globals.windowWidth/2 - 130, currentY, RenderColors::textSecondary, FontSize::MEDIUM);
        currentY += 40;
    }

    if (state.highScores.empty()) {
        RenderHelper::renderText("NO HIGH SCORES YET", globals.windowWidth/2 - 140, currentY, RenderColors::textSecondary, FontSize::MEDIUM);
    }

    RenderHelper::renderCloseButton();
}