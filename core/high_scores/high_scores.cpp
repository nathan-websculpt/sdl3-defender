#include "high_scores.h"
#include "../game_state_data.h"
#include <fstream>
#include <sstream>

HighScores::HighScores() = default;

void HighScores::loadHighScores(GameStateData& state) {
    state.highScores.clear();
    std::ifstream file(Config::Game::HIGH_SCORES_PATH);
    if (file.is_open()) {
        std::string line;
        while (std::getline(file, line) && state.highScores.size() < state.MAX_HIGH_SCORES) {
            std::istringstream iss(line);
            std::string name;
            int score;
            if (iss >> name >> score) { // format: "NAME SCORE"
                 GameStateData::HighScore entry;
                 entry.name = name;
                 entry.score = score;
                 state.highScores.push_back(entry);
            }
        }
        file.close();
    }

    // list is sorted (highest first) and capped at MAX_HIGH_SCORES
    std::sort(state.highScores.begin(), state.highScores.end(),
              [](const GameStateData::HighScore& a, const GameStateData::HighScore& b) { return a.score > b.score; });
    if (state.highScores.size() > state.MAX_HIGH_SCORES) {
        state.highScores.resize(state.MAX_HIGH_SCORES);
    }
}

bool HighScores::isHighScore(GameStateData& state) const {
    return state.highScores.size() < state.MAX_HIGH_SCORES || state.playerScore > state.highScores.back().score;
}

int HighScores::getHighScoreIndex(GameStateData& state) const {
    // finds the index where new score should be inserted (0 is highest)
    for (size_t i = 0; i < state.highScores.size(); ++i) {
        if (state.playerScore > state.highScores[i].score) {
            return static_cast<int>(i);
        }
    }
    //if loop finishes without returning, the score is not higher than any existing score, but also need to check if the list is not full yet
    if (state.highScores.size() < state.MAX_HIGH_SCORES) {
        return static_cast<int>(state.highScores.size());
    }

    return -1;
}

void HighScores::submitHighScore(const std::string& name, GameStateData& state) {
    int index = getHighScoreIndex(state);
    if (index != -1) {
        GameStateData::HighScore newEntry;
        newEntry.name = name.empty() ? "ANON" : name;
        newEntry.score = state.playerScore;
        state.highScores.insert(state.highScores.begin() + index, newEntry);
        if (state.highScores.size() > state.MAX_HIGH_SCORES) {
            state.highScores.pop_back();
        }
        saveHighScores(state);
    }
}

void HighScores::saveHighScores(const GameStateData& state) {
    std::ofstream file(Config::Game::HIGH_SCORES_PATH);
    if (file.is_open()) {
        for (const auto& entry : state.highScores) {
            file << entry.name << " " << entry.score << "\n"; //Format: "NAME SCORE"
        }
        file.close();
        SDL_Log("High scores saved.");
    } else {
        SDL_Log("Warning: Could not save high scores to file.");
    }
}