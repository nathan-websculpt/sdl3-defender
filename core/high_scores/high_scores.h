#pragma once

#include "../config.h"
#include "../game_state_data.h" // for the GameStateData forward declaration

struct GameStateData; // forward declaration needed because Game and HighScores both need the GameStateData::HighScore struct

class HighScores {
public:
    HighScores();

    void loadHighScores(GameStateData& state);
    void submitHighScore(const std::string& name, GameStateData& state); // TODO: remove
    bool isHighScore(GameStateData& state) const;
    int getHighScoreIndex(GameStateData& state) const;

private:
    void saveHighScores(GameStateData& state);
};