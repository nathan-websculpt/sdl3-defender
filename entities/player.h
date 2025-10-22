#pragma once
#include <SDL3/SDL.h>
#include <vector>
#include <memory>
#include "projectile.h"
#include "particle.h"
#include "../core/config.h"
#include "../plf/plf_colony.h"

enum class Direction {
    RIGHT,
    LEFT
};

class Player {
public:
    Player(float x, float y, float w, float h);
    ~Player() = default;

    void update(float deltaTime, plf::colony<Particle>& particles);

    void handleInput(const bool* keyboardState);

    SDL_FRect getBounds() const;
    SDL_FPoint getFrontCenter() const;
    void setPosition(float x, float y);

    plf::colony<Projectile>& getProjectiles();

    void shoot();
    bool isAlive() const { return m_health > 0; }
    void takeDamage(int damage) { m_health -= damage; if (m_health < 0) m_health = 0; }
    int getHealth() const { return m_health; }
    bool isHit(const SDL_FRect& opponentBounds);
    void setSpeedBoost(bool active);

    float getSpeed() const { return m_speed; }
    void setFacing(Direction dir) { m_facing = dir; } // TODO: needed?
    Direction getFacing() const { return m_facing; }
    void moveBy(float dx, float dy) {
        m_rect.x += dx;
        m_rect.y += dy;
    }

private:
    SDL_FRect m_rect;
    float m_speed;
    Direction m_facing;

    plf::colony<Projectile> m_projectiles;

    bool m_spacePressed;
    int m_health = 10; // TODO: use the var in Game instead
    float m_normalSpeed;
    float m_boostMultiplier;
    bool m_speedBoostActive;

    void spawnBoosterParticles(plf::colony<Particle>& particles);
    void spawnDefaultBoosterParticles(plf::colony<Particle>& particles);
};