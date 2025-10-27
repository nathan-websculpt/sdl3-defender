#pragma once
#include <SDL3/SDL.h>
#include <vector>
#include <memory>
#include "../projectile.h"
#include "../particle.h" 
#include "../../core/config.h"
#include "../../plf/plf_colony.h" 

struct GameStateData; // forward declaration

class BaseOpponent {
public:
    BaseOpponent(float x, float y, float w, float h);
    virtual ~BaseOpponent() = default;

    virtual void update(float deltaTime, const SDL_FPoint& playerPos, float cameraX, const GameStateData& state) = 0;

    SDL_FRect getBounds() const;

    bool isAlive() const { return m_health > 0; }
    void takeDamage(int damage);

    plf::colony<Projectile>& getProjectiles();
    const plf::colony<Projectile>& getProjectiles() const;

    const int& getScoreVal() const;

    virtual void explode(plf::colony<Particle>& gameParticles) const;

    virtual const std::string& getTextureKey() const = 0;

    bool isOnScreen(float objX, float objY, float cameraX, int screenWidth) const;

protected:
    struct ExplosionConfig {
        int numParticles = 200;
        float speedMin = 50.0f;
        float speedMax = 150.0f;
        float angleJitter = 0.25f;
        Uint8 rMin = 100;
        Uint8 rMax = 255;
        Uint8 gMin = 0;
        Uint8 gMax = 255;
        Uint8 bMin = 0; 
        Uint8 bMax = 255;
        float life = 0.1f;
        float size = 1.0f;
    };

    ExplosionConfig m_explosionConfig;

    SDL_FRect m_rect;

    float m_speed;
    float m_angle;
    float m_angularSpeed;
    float m_oscillationAmplitude;
    float m_startX;
    int m_health;

    plf::colony<Projectile> m_projectiles;
    float m_fireTimer;
    float m_fireInterval;

    int m_scoreVal;
};