#pragma once
#include <vector>
#include <SDL3/SDL.h>
#include "../config.h"

namespace ProjectileClipping {
    float findBeamLandscapeIntersection(float startX, float beamY, bool goingRight, const std::vector<SDL_FPoint>& landscape); // for player beams (horizontal)
    SDL_FPoint clipRayToLandscape(float startX, float startY, float endX, float endY, const std::vector<SDL_FPoint>& landscape); // for opponent projectiles
}