#pragma once
#include "../config.h"

// new: needed to forward declare to avoid circular include after adding /helpers_game/collision_handler
class GameStateData;

class HighScores {
public:
    HighScores();

    void loadHighScores(GameStateData& state);
    void submitHighScore(const std::string& name, GameStateData& state); 
    bool isHighScore(GameStateData& state) const;
    int getHighScoreIndex(GameStateData& state) const;

private:
    void saveHighScores(GameStateData& state);
};