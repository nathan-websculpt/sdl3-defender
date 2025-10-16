#pragma once
#include <SDL3/SDL.h>
#include <vector>
#include <memory>
#include "../projectile.h"
#include "../particle.h" 
#include "../../core/config.h"

class BaseOpponent {
public:
    BaseOpponent(float x, float y, float w, float h, SDL_Renderer* renderer);
    virtual ~BaseOpponent();

    virtual void update(float deltaTime, const SDL_FPoint& playerPos, float cameraX, int screenWidth) = 0;
    virtual void render(SDL_Renderer* renderer, SDL_FRect* renderBounds) const = 0;

    SDL_FRect getBounds() const;
    bool isOffScreen(int screenHeight) const;

    bool isAlive() const { return m_health > 0; }
    void takeDamage(int damage);
    void reset();
    bool isHit(const SDL_FRect& projectileBounds);

    std::vector<std::unique_ptr<Projectile>>& getProjectiles();
    const std::vector<std::unique_ptr<Projectile>>& getProjectiles() const;

    const int& getScoreVal() const;

    virtual void explode(std::vector<std::unique_ptr<Particle>>& gameParticles) const = 0;

protected:
    SDL_FRect m_rect;
    std::shared_ptr<SDL_Texture> m_texture;

    float m_speed;
    float m_angle;
    float m_angularSpeed;
    float m_oscillationAmplitude;
    float m_startX;
    int m_health;
    int m_maxHealth; //TODO: remove

    std::vector<std::unique_ptr<Projectile>> m_projectiles;
    float m_fireTimer;
    float m_fireInterval;

    int m_scoreVal;
};