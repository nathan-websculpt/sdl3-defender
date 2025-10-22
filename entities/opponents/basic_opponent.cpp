#include "basic_opponent.h"
#include "../particle.h"
#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include <cmath>
#include <cstdlib>

BasicOpponent::BasicOpponent(float x, float y, float w, float h) 
    : BaseOpponent(x, y, w, h) {
    m_speed = 30.0f;
    m_angularSpeed = 1.5f;
    m_oscillationAmplitude = 80.0f;
    m_startY = y;
    m_fireInterval = 0.0f;

    m_scoreVal = 300;
}

void BasicOpponent::update(float deltaTime, const SDL_FPoint& playerPos, float cameraX, int screenWidth, int screenHeight) {
    if (m_health <= 0) return;

    m_rect.y += m_speed * deltaTime;
    m_angle += m_angularSpeed * deltaTime;
    m_rect.x = m_startX + sin(m_angle) * m_oscillationAmplitude;
}

void BasicOpponent::explode(plf::colony<Particle>& gameParticles) const {
    const int numParticles = 120;
    SDL_FPoint center = { m_rect.x + m_rect.w / 2.0f, m_rect.y + m_rect.h / 2.0f };

    for (int i = 0; i < numParticles; ++i) {
        float angle = (static_cast<float>(i) / numParticles) * 2.0f * M_PI + (static_cast<float>(rand()) / RAND_MAX) * 0.2f;
        float speed = static_cast<float>(rand() % 150) + 80.0f;
        
        float velX = cos(angle) * speed;
        float velY = sin(angle) * speed;

        Uint8 r = static_cast<Uint8>(rand() % 100 + 155);
        Uint8 g = static_cast<Uint8>(rand() % 100 + 55);
        Uint8 b = static_cast<Uint8>(rand() % 50);

        gameParticles.emplace(center.x, center.y, velX, velY, r, g, b, 0.02f, 2.8f);
    }
}