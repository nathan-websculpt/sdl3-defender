#pragma once
#include <SDL3/SDL.h>
#include <vector>
#include <memory>
#include "projectile.h"
#include "particle.h"
#include "../core/config.h"

enum class Direction {
    RIGHT,
    LEFT
};

class Player {
public:
    Player(float x, float y, float w, float h, SDL_Renderer* renderer);
    ~Player();

    void update(float deltaTime);
    void render(SDL_Renderer* renderer, SDL_FRect* renderBounds) const;

    void handleInput(const bool* keyboardState);

    SDL_FRect getBounds() const;
    SDL_FPoint getFrontCenter() const;
    void setPosition(float x, float y);

    std::vector<std::unique_ptr<Projectile>>& getProjectiles();

    void shoot();
    bool isAlive() const { return m_health > 0; }
    void takeDamage(int damage) { m_health -= damage; if (m_health < 0) m_health = 0; }
    int getHealth() const { return m_health; }
    bool isHit(const SDL_FRect& opponentBounds);
    void setSpeedBoost(bool active);

private:
    SDL_FRect m_rect;
    float m_speed;
    Direction m_facing;
    std::shared_ptr<SDL_Texture> m_texture;

    std::vector<std::unique_ptr<Projectile>> m_projectiles;
    std::vector<std::unique_ptr<Particle>> m_boosterParticles;

    bool m_spacePressed;
    int m_health = 10; // TODO: use the var in Game instead
    float m_normalSpeed;
    float m_boostMultiplier;
    bool m_speedBoostActive;

    void spawnBoosterParticles();
    void spawnDefaultBoosterParticles();
};