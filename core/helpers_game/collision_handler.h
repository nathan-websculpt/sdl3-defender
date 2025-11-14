#pragma once
#include "../game_state_data.h"
#include "game_helper.h"
#include "../high_scores/high_scores.h"
#include "../managers/sound_manager.h"
#include "../config.h"
#include "../../entities/health_item.h"

namespace CollisionHandler {
    void processAllCollisions(GameStateData& state, const GameHelper& helpers, const HighScores& highScores, const SDL_FRect& playerBounds);
}