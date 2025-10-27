#include "aggressive_opponent.h"
#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include <cmath>
#include <algorithm>
#include "../particle.h"
#include "../../core/texture_manager.h"
#include "../../core/game.h" 

AggressiveOpponent::AggressiveOpponent(float x, float y, float w, float h) 
    : BaseOpponent(x, y, w, h) {
    m_speed = 70.0f;
    m_angularSpeed = 0.0f;
    m_oscillationAmplitude = 0.0f;
    m_fireInterval = 1.8f;

    m_health = 2;
    m_scoreVal = 100;
}

void AggressiveOpponent::update(float deltaTime, const SDL_FPoint& playerPos, float cameraX, const GameStateData& state) {
    if (m_health <= 0) return;

    // targeting inaccuracy
    float targetX = playerPos.x + (static_cast<float>(rand() % 200) - 100.0f);
    float targetY = playerPos.y;

    // chase player position
    float dx = targetX - m_rect.x;
    float dy = targetY - m_rect.y;

    // go left/right
    if (std::abs(dx) > 1.0f) {
        m_rect.x += (dx > 0 ? 1 : -1) * m_speed * deltaTime;
    }
    // go up/down]
    if (std::abs(dy) > 1.0f) {
        m_rect.y += (dy > 0 ? 1 : -1) * m_speed * deltaTime;
    }

    m_fireTimer += deltaTime;
    bool opponentVisible = isOnScreen(m_rect.x + m_rect.w/2, m_rect.y, cameraX, state.screenWidth);

    if (opponentVisible && m_fireTimer >= m_fireInterval) {
        m_projectiles.emplace(
            m_rect.x + m_rect.w/2,
            m_rect.y + m_rect.h/2,
            targetX,
            targetY,
            300.0f
        );
        m_fireTimer = 0.0f;
    }
}

void AggressiveOpponent::explode(plf::colony<Particle>& gameParticles) const {
    const int numParticles = 220;

    SDL_FPoint center = { m_rect.x + m_rect.w / 2.0f, m_rect.y + m_rect.h / 2.0f };

    for (int i = 0; i < numParticles; ++i) {
        float angle = (static_cast<float>(i) / numParticles) * 2.0f * M_PI + (static_cast<float>(rand()) / RAND_MAX) * 0.5f;
        float speed = static_cast<float>(rand() % 120) + 60.0f;

        float velX = cos(angle) * speed;
        float velY = sin(angle) * speed;

        Uint8 r = static_cast<Uint8>(rand() % 100 + 100);
        Uint8 g = static_cast<Uint8>(rand() % 50);
        Uint8 b = static_cast<Uint8>(rand() % 100 + 155);

        gameParticles.emplace(center.x, center.y, velX, velY, r, g, b, 0.2f, 1.9f);
    }
}

bool AggressiveOpponent::isOnScreen(float objX, float objY, float cameraX, int screenWidth) const {
    float screenMinX = cameraX;
    float screenMaxX = cameraX + screenWidth;
    // Y is always fully visible because:
    // world height = 600px
    // window height is enforced to be >= 600px
    // no vertical camera movement
    return (objX >= screenMinX && objX <= screenMaxX);
}