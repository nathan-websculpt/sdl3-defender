#include "game_helper.h"

GameHelper::GameHelper(const std::vector<SDL_FPoint>& landscape)
    : m_landscape(landscape) {}

bool GameHelper::isOutOfWorld(const SDL_FRect& r, float mx, float my) const {
    return (r.x + r.w < -mx || r.x > Config::Game::WORLD_WIDTH + mx ||
            r.y + r.h < -my || r.y > globals.windowHeight + my);
}

float GameHelper::getGroundYAt(float x) const {
    const auto& land = m_landscape;
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

bool GameHelper::rectsIntersect(const SDL_FRect& a, const SDL_FRect& b) const {
    return (a.x < b.x + b.w &&
            a.x + a.w > b.x &&
            a.y < b.y + b.h &&
            a.y + a.h > b.y);
}

float GameHelper::getBeamVisualEndX(float startX, float beamY, bool goingRight) const {
    if (m_landscape.empty()) {
        return goingRight ? Config::Game::WORLD_WIDTH : 0.0f;
    }

    const auto& land = m_landscape;

    if (goingRight) {
        // find first segment where x >= startX
        for (size_t i = 0; i < land.size() - 1; ++i) {
            float x0 = land[i].x;
            float x1 = land[i + 1].x;
            if (x1 < startX) continue;

            float y0 = land[i].y;
            float y1 = land[i + 1].y;

            // if beam is above both points, it passes through
            if (beamY < y0 && beamY < y1) {
                continue;
            }

            // if beam is below or at both, it hits at segment start
            if (beamY >= y0 && beamY >= y1) {
                return std::max(startX, x0);
            }

            // interpolate intersection
            // find X where the horizontal beam crosses the straight line segment between (x0,y0) and (x1,y1) 
            if (y1 != y0) {
                float t = (beamY - y0) / (y1 - y0);
                if (t >= 0.0f && t <= 1.0f) {
                    float intersectX = x0 + t * (x1 - x0);
                    if (intersectX >= startX) {
                        return intersectX;
                    }
                }
            }
        }
        return Config::Game::WORLD_WIDTH;
    } else {
        // going left
        for (size_t i = land.size() - 1; i > 0; --i) {
            float x0 = land[i - 1].x;
            float x1 = land[i].x;
            if (x0 > startX) continue;

            float y0 = land[i - 1].y;
            float y1 = land[i].y;

            if (beamY < y0 && beamY < y1) {
                continue;
            }
            if (beamY >= y0 && beamY >= y1) {
                return std::min(startX, x1);
            }

            if (y1 != y0) {
                float t = (beamY - y0) / (y1 - y0);
                if (t >= 0.0f && t <= 1.0f) {
                    float intersectX = x0 + t * (x1 - x0);
                    if (intersectX <= startX) {
                        return intersectX;
                    }
                }
            }
        }
        return 0.0f;
    }
}

void GameHelper::keepPlayerInBounds(std::unique_ptr<Player>& player, SDL_FRect& pb) {
    // keeps player beneath HUD, above landscape, and in-world

    float desiredX = pb.x;
    float desiredY = pb.y;

    // left and right (world) boundaries
    if (desiredX < 0) desiredX = 0;
    if (desiredX + pb.w > Config::Game::WORLD_WIDTH) desiredX = Config::Game::WORLD_WIDTH - pb.w;

    // top (HUD) boundary
    desiredY = std::max(desiredY, static_cast<float>(Config::Game::HUD_HEIGHT));

    // landscape constraint (bottom)
    float playerBottomX = desiredX + pb.w / 2.0f; 
    float groundYAtPlayerX = getGroundYAt(playerBottomX);
    float absoluteWorldBottom = globals.windowHeight - pb.h; // absolute bottom of the world

    // player's bottom Y should not exceed the landscape height at their X position
    float effectiveGroundY = std::min(groundYAtPlayerX, absoluteWorldBottom);
    float maxAllowedY = effectiveGroundY - pb.h;

    // ensure desiredY is not below the calculated maximum
    desiredY = std::min(desiredY, maxAllowedY);
    
    if (pb.x != desiredX || pb.y != desiredY) {
        player->setPosition(desiredX, desiredY); // apply the final calculated position
    }
}


