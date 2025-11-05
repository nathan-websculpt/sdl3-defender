#include "game_helper.h"

// TODO:
//      do not deref pointers without null-checking

GameHelper::GameHelper(const std::vector<SDL_FPoint>* landscapePtr)
    : m_landscape(landscapePtr) {}

bool GameHelper::isOutOfWorld(const SDL_FRect& r, float mx, float my) const {
    return (r.x + r.w < -mx || r.x > Config::Game::WORLD_WIDTH + mx ||
            r.y + r.h < -my || r.y > globals.windowHeight + my);
}

float GameHelper::getGroundYAt(float x) const {
    const auto& land = (*m_landscape);
    if (land.empty()) return globals.windowHeight;

    // clamp x to landscape bounds
    if (x <= land.front().x) return land.front().y;
    if (x >= land.back().x) return land.back().y;

    for (size_t i = 0; i < land.size() - 1; ++i) {
        if (x >= land[i].x && x <= land[i + 1].x) {
            // linear interpolation between land[i] and land[i+1]
            float t = (x - land[i].x) / (land[i + 1].x - land[i].x);
            return land[i].y + t * (land[i + 1].y - land[i].y);
        }
    }
    return land.back().y; // fallback
}


