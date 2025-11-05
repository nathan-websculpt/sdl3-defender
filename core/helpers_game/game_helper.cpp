#include "game_helper.h"

// TODO:
//      get the world dims into globals?
//      do not deref pointers without null-checking

GameHelper::GameHelper(const std::vector<SDL_FPoint>* landscapePtr, const float* worldWPtr, const float* worldHPtr)
    : m_landscape(landscapePtr), m_worldWidth(worldWPtr), m_worldHeight(worldHPtr) {}

bool GameHelper::isOutOfWorld(const SDL_FRect& r, float mx, float my) const {
    return (r.x + r.w < -mx || r.x > (*m_worldWidth) + mx ||
            r.y + r.h < -my || r.y > (*m_worldHeight) + my);
}

float GameHelper::getGroundYAt(float x) const {
    const auto& land = (*m_landscape);
    if (land.empty()) return (*m_worldHeight);

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


