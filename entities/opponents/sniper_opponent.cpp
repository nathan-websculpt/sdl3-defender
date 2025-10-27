#include "sniper_opponent.h"
#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include <cmath>
#include <algorithm>
#include "../particle.h"
#include "../../core/texture_manager.h"
#include "../../core/game.h" 

SniperOpponent::SniperOpponent(float x, float y, float w, float h) 
    : BaseOpponent(x, y, w, h) {
    m_speed = 20.0f;
    m_angularSpeed = 0.8f;
    m_oscillationAmplitude = 60.0f;
    m_oscillationSpeed = 1.0f;
    m_oscillationOffset = static_cast<float>(rand()) / RAND_MAX * 2.0f * M_PI;
    m_fireInterval = 4.0f;
    m_fireAccuracy = 0.1f;
    
    m_health = 1;
    m_scoreVal = 100;
}

void SniperOpponent::update(float deltaTime, const SDL_FPoint& playerPos, float cameraX, const GameStateData& state) {
    if (m_health <= 0) return;

    // simple movement
    m_rect.y += m_speed * deltaTime;
    m_angle += m_oscillationSpeed * deltaTime;
    m_rect.x = m_startX + sin(m_angle + m_oscillationOffset) * m_oscillationAmplitude;

    m_fireTimer += deltaTime;
    bool opponentVisible = isOnScreen(m_rect.x + m_rect.w/2, m_rect.y, cameraX, state.screenWidth);
    
    if (opponentVisible && m_fireTimer >= m_fireInterval) {
        m_projectiles.emplace(
            m_rect.x + m_rect.w/2,
            m_rect.y + m_rect.h/2,
            playerPos.x,
            playerPos.y,
            1800.0f
        );
        m_fireTimer = 0.0f;
    }
}

void SniperOpponent::explode(plf::colony<Particle>& gameParticles) const {
    const int numParticles = 345;
    SDL_FPoint center = { m_rect.x + m_rect.w / 2.0f, m_rect.y + m_rect.h / 2.0f };

    for (int i = 0; i < numParticles; ++i) {
        float angle = (static_cast<float>(i) / numParticles) * 2.0f * M_PI + (static_cast<float>(rand()) / RAND_MAX) * 0.2f;
        float speed = static_cast<float>(rand() % 110) + 70.0f;

        float velX = cos(angle) * speed;
        float velY = sin(angle) * speed;

        Uint8 r = static_cast<Uint8>(rand() % 100 + 55);
        Uint8 g = static_cast<Uint8>(rand() % 100 + 155);
        Uint8 b = static_cast<Uint8>(rand() % 50 + 55);

        gameParticles.emplace(center.x, center.y, velX, velY, r, g, b, 0.0001f, 1.35f);
    }
}

bool SniperOpponent::isOnScreen(float objX, float objY, float cameraX, int screenWidth) const {
    float screenMinX = cameraX;
    float screenMaxX = cameraX + screenWidth;
    // Y is always fully visible because:
    // world height = 600px
    // window height is enforced to be >= 600px
    // no vertical camera movement
    return (objX >= screenMinX && objX <= screenMaxX);
}