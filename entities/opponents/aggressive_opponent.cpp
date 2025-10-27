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

    m_explosionConfig.numParticles = 220;
    m_explosionConfig.angleJitter = 0.5f;
    m_explosionConfig.speedMin = 60.0f;
    m_explosionConfig.speedMax = 180.0f; // rand()%120 + 60
    m_explosionConfig.rMin = 100; 
    m_explosionConfig.rMax = 199;
    m_explosionConfig.gMin = 0;   
    m_explosionConfig.gMax = 49;
    m_explosionConfig.bMin = 155; 
    m_explosionConfig.bMax = 254;
    m_explosionConfig.life = 0.2f;
    m_explosionConfig.size = 1.9f;
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
    // go up/down
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