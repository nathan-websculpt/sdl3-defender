#include "basic_opponent.h"
#include "../particle.h"
#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include <cmath>
#include <cstdlib>
#include "../../core/game.h" 

BasicOpponent::BasicOpponent(float x, float y, float w, float h) 
    : BaseOpponent(x, y, w, h) {
    m_speed = 30.0f;
    m_angularSpeed = 1.5f;
    m_oscillationAmplitude = 80.0f;
    m_startY = y;
    m_fireInterval = 0.0f;

    m_scoreVal = 300;

    m_explosionConfig.numParticles = 450;
    m_explosionConfig.angleJitter = 0.2f;
    m_explosionConfig.speedMin = 80.0f;
    m_explosionConfig.speedMax = 230.0f; // rand()%150 + 80
    m_explosionConfig.rMin = 155; 
    m_explosionConfig.rMax = 254;
    m_explosionConfig.gMin = 55;  
    m_explosionConfig.gMax = 154;
    m_explosionConfig.bMin = 0;   
    m_explosionConfig.bMax = 49;
    m_explosionConfig.life = 0.005f;
    m_explosionConfig.size = 2.2f;
}

void BasicOpponent::update(float deltaTime, const SDL_FPoint& playerPos, float cameraX, const GameStateData& state) {
    if (m_health <= 0) return;

    m_rect.y += m_speed * deltaTime;
    m_angle += m_angularSpeed * deltaTime;
    m_rect.x = m_startX + sin(m_angle) * m_oscillationAmplitude;
}