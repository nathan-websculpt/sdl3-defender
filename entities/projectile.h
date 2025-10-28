#pragma once
#include <SDL3/SDL.h>
#include "../core/config.h"
#include <cmath>

class Projectile {
public:
    // for player: fire horizontally
    Projectile(float spawnX, float spawnY, float direction, float speed);

    // for opponents: fire at a target point
    Projectile(float spawnX, float spawnY, float targetX, float targetY, float speed);
    ~Projectile() = default;

    void update(float deltaTime);

    SDL_FRect getBounds() const; // just a hit box

    // for spawn position (to calculate beam start point)
    float getSpawnX() const { return m_spawnX; }
    float getSpawnY() const { return m_spawnY; }
    
    // for current position (to calculate beam endpoint)
    float getCurrentX() const { return m_rect.x; }
    float getCurrentY() const { return m_rect.y; }

    float getAge() const { return m_age; }
    float getLifetime() const { return m_lifetime; }
    SDL_FPoint getVelocity() const { return m_velocity; } // for direction of beam
    bool isHorizontal() const { return m_isHorizontal; } // for beam type

    // helper to calculate color based on age
    SDL_Color getColor() const {
        float age = getAge();
        Uint8 r = static_cast<Uint8>(200 + 55 * std::sin(age * 30.0f));
        Uint8 g = static_cast<Uint8>(200 + 55 * std::sin(age * 25.0f + M_PI));
        Uint8 b = static_cast<Uint8>(0 + 55 * std::sin(age * 35.0f + M_PI/2));
        float lifeRatio = (getLifetime() - age) / getLifetime();
        Uint8 alpha = static_cast<Uint8>(255 * lifeRatio * 0.7f);
        float pulseAlpha = 0.7f + 0.3f * std::sin(age * 25.0f);
        alpha = static_cast<Uint8>(alpha * pulseAlpha);
        return {r, g, b, alpha};
    }

private:
    SDL_FRect m_rect;
    SDL_FPoint m_velocity;

    // position where the projectile was fired
    float m_spawnX;
    float m_spawnY; 
    
    float m_direction; // 1 is right, -1 is left
    float m_speed;

    float m_age; // how long projectile has existed
    const float m_lifetime; // lifetime of the projectile

    bool m_isHorizontal = false; // true for player shots
};