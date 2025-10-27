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

    m_explosionConfig.numParticles = 345;
    m_explosionConfig.angleJitter = 0.2f;
    m_explosionConfig.speedMin = 70.0f;
    m_explosionConfig.speedMax = 180.0f; // rand()%110 + 70
    m_explosionConfig.rMin = 55;  
    m_explosionConfig.rMax = 154;
    m_explosionConfig.gMin = 155; 
    m_explosionConfig.gMax = 254;
    m_explosionConfig.bMin = 55;  
    m_explosionConfig.bMax = 104;
    m_explosionConfig.life = 0.0001f;
    m_explosionConfig.size = 1.35f;
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