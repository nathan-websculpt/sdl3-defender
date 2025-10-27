#include "base_opponent.h"
#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include <cmath>
#include "../../core/texture_manager.h"

BaseOpponent::BaseOpponent(float x, float y, float w, float h) 
    : m_rect{x, y, w, h},
      m_angle(0.0f), m_startX(x), m_health(3), m_fireTimer(0.0f)
      {}

void BaseOpponent::takeDamage(int damage) {
    m_health -= damage;
    if (m_health < 0) m_health = 0;
}

SDL_FRect BaseOpponent::getBounds() const {
    return m_rect;
}

plf::colony<Projectile>& BaseOpponent::getProjectiles() {
    return m_projectiles;
}

const plf::colony<Projectile>& BaseOpponent::getProjectiles() const {
    return m_projectiles;
}

const int& BaseOpponent::getScoreVal() const {
    return m_scoreVal;
}

bool BaseOpponent::isOnScreen(float objX, float objY, float cameraX, int screenWidth) const {
    float screenMinX = cameraX;
    float screenMaxX = cameraX + screenWidth;
    // Y is always fully visible because:
    // world height = 600px
    // window height is enforced to be >= 600px
    // no vertical camera movement
    return (objX >= screenMinX && objX <= screenMaxX);
}

void BaseOpponent::explode(plf::colony<Particle>& gameParticles) const {
    SDL_FPoint center = { m_rect.x + m_rect.w / 2.0f, m_rect.y + m_rect.h / 2.0f };
    const ExplosionConfig& cfg = m_explosionConfig;

    for (int i = 0; i < cfg.numParticles; ++i) {
        float baseAngle = (static_cast<float>(i) / cfg.numParticles) * 2.0f * M_PI;
        float angle = baseAngle + (static_cast<float>(rand()) / RAND_MAX) * cfg.angleJitter;
        float speed = cfg.speedMin + static_cast<float>(rand()) / RAND_MAX * (cfg.speedMax - cfg.speedMin);

        float velX = cosf(angle) * speed;
        float velY = sinf(angle) * speed;

        Uint8 r = static_cast<Uint8>(cfg.rMin + (rand() % (cfg.rMax - cfg.rMin + 1)));
        Uint8 g = static_cast<Uint8>(cfg.gMin + (rand() % (cfg.gMax - cfg.gMin + 1)));
        Uint8 b = static_cast<Uint8>(cfg.bMin + (rand() % (cfg.bMax - cfg.bMin + 1)));

        gameParticles.emplace(center.x, center.y, velX, velY, r, g, b, cfg.life, cfg.size);
    }
}