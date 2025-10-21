#include "base_opponent.h"
#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include <cmath>
#include "../../core/texture_manager.h"

BaseOpponent::BaseOpponent(float x, float y, float w, float h) 
    : m_rect{x, y, w, h}, m_speed(50.0f),
      m_angle(0.0f), m_angularSpeed(2.0f), m_oscillationAmplitude(100.0f),
      m_startX(x), m_maxHealth(3), m_health(3), m_fireTimer(0.0f), m_fireInterval(5.0f) 
      {}

BaseOpponent::~BaseOpponent() {
    
}

void BaseOpponent::takeDamage(int damage) {
    m_health -= damage;
    if (m_health < 0) m_health = 0;
}

void BaseOpponent::reset() {
    m_health = m_maxHealth;
}

SDL_FRect BaseOpponent::getBounds() const {
    return m_rect;
}

bool BaseOpponent::isOffScreen(int screenHeight) const {
    return m_rect.y > screenHeight;
}

bool BaseOpponent::isHit(const SDL_FRect& projectileBounds) {
    SDL_FRect oppBounds = getBounds();
 
    // TODO: refactor?
    return (projectileBounds.x < oppBounds.x + oppBounds.w &&
            projectileBounds.x + projectileBounds.w > oppBounds.x &&
            projectileBounds.y < oppBounds.y + oppBounds.h &&
            projectileBounds.y + projectileBounds.h > oppBounds.y);
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