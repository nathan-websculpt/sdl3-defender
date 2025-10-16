#pragma once
#include <SDL3/SDL.h>
#include "../core/config.h"

class Projectile {
public:
    // for player: fire horizontally
    Projectile(float spawnX, float spawnY, float direction, float speed);

    // for opponents: fire at a target point
    Projectile(float spawnX, float spawnY, float targetX, float targetY, float speed);
    ~Projectile();

    void update(float deltaTime);
    void render(SDL_Renderer* renderer, SDL_FRect* renderBounds) const; // render with camera adjustment

    SDL_FRect getBounds() const; // here this is just a hit box
    bool isOffScreen(int screenWidth, int screenHeight) const;

    // for spawn position (to calculate beam start point)
    float getSpawnX() const { return m_spawnX; }
    float getSpawnY() const { return m_spawnY; }
    
    // for current position (to calculate beam endpoint)
    float getCurrentX() const { return m_rect.x; }
    float getCurrentY() const { return m_rect.y; }

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
    const float m_worldWidth;
    const float m_worldHeight;

    bool m_isHorizontal = false; // true for player shots
};