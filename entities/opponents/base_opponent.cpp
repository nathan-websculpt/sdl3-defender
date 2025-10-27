#include "base_opponent.h"
#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include <cmath>
#include "../../core/texture_manager.h"

BaseOpponent::BaseOpponent(float x, float y, float w, float h) 
    : m_rect{x, y, w, h}, m_speed(50.0f),
      m_angle(0.0f), m_angularSpeed(2.0f), m_oscillationAmplitude(100.0f),
      m_startX(x), m_health(3), m_fireTimer(0.0f), m_fireInterval(5.0f) 
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