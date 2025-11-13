#include "projectile_clipping.h"

namespace ProjectileClipping {
    // for player beams
    float findBeamLandscapeIntersection(float startX, float beamY, bool goingRight, const std::vector<SDL_FPoint>& landscape) {
        if (landscape.empty()) return goingRight ? Config::Game::WORLD_WIDTH : 0.0f;

        // clamp beamY
        if (beamY <= 0) return goingRight ? Config::Game::WORLD_WIDTH : 0.0f;

        // determine search range
        size_t startIdx = 0;

        if (goingRight) {
            // find first segment where x >= startX
            for (size_t i = 0; i < landscape.size() - 1; ++i) {
                float x0 = landscape[i].x;
                float x1 = landscape[i + 1].x;
                if (x1 < startX) continue;

                // this segment or next may contain intersection
                float y0 = landscape[i].y;
                float y1 = landscape[i + 1].y;

                // if beam is above both points, beam continues
                if (beamY < y0 && beamY < y1) {
                    // no intersection in this segment
                    continue;
                }

                // if beam is below both, it is already on ground - shouldn't happen if projectile was alive
                if (beamY >= y0 && beamY >= y1)                 
                    return std::max(startX, x0);// beam hits ground at segment start
                
                // otherwise... interpolate intersection
                // find X where the horizontal beam crosses the straight line segment between (x0,y0) and (x1,y1) 
                // ground line: y = y0 + t*(y1 - y0), x = x0 + t*(x1 - x0)
                float t = (beamY - y0) / (y1 - y0);
                if (t >= 0.0f && t <= 1.0f) {
                    float intersectX = x0 + t * (x1 - x0);
                    if (intersectX >= startX) {
                        return intersectX;
                    }
                }
            }
            // if no intersection found, the beam goes to world edge
            return Config::Game::WORLD_WIDTH;
        } else {
            // going left: search backward
            for (size_t i = landscape.size() - 1; i > 0; --i) {
                float x0 = landscape[i - 1].x;
                float x1 = landscape[i].x;
                if (x0 > startX) continue;

                float y0 = landscape[i - 1].y;
                float y1 = landscape[i].y;

                if (beamY < y0 && beamY < y1) {
                    continue;
                }
                if (beamY >= y0 && beamY >= y1) {
                    return std::min(startX, x1);
                }

                float t = (beamY - y0) / (y1 - y0);
                if (t >= 0.0f && t <= 1.0f) {
                    float intersectX = x0 + t * (x1 - x0);
                    if (intersectX <= startX) {
                        return intersectX;
                    }
                }
            }
            return 0.0f;
        }
    }

    // for opponent projectiles
    SDL_FPoint clipRayToLandscape(float startX, float startY, float endX, float endY, const std::vector<SDL_FPoint>& landscape) {
        if (landscape.empty()) return {endX, endY};

        // ray: from (startX, startY) to (endX, endY)
        float rayDx = endX - startX;
        float rayDy = endY - startY;
        float bestT = 1.0f; // full length

        // check intersection with each landscape segment
        for (size_t i = 0; i < landscape.size() - 1; ++i) {
            float x0 = landscape[i].x;
            float y0 = landscape[i].y;
            float x1 = landscape[i + 1].x;
            float y1 = landscape[i + 1].y;

            // landscape segment vector
            float segDx = x1 - x0;
            float segDy = y1 - y0;

            // solve: 
            //      startX + t1*rayDx = x0 + t2*segDx
            //      startY + t1*rayDy = y0 + t2*segDy
            float denom = rayDx * segDy - rayDy * segDx;
            if (std::abs(denom) < 1e-6f) continue; // parallel

            float t2 = (rayDx * (startY - y0) - rayDy * (startX - x0)) / denom;
            if (t2 < 0.0f || t2 > 1.0f) continue; // intersection not on segment

            float t1 = (x0 + t2 * segDx - startX) / rayDx;
            if (std::abs(rayDx) < 1e-6f) t1 = (y0 + t2 * segDy - startY) / rayDy;

            if (t1 >= 0.0f && t1 < bestT) {
                bestT = t1;
            }
        }

        // return clipped endpoint
        return {
            startX + bestT * rayDx,
            startY + bestT * rayDy
        };
    }
}