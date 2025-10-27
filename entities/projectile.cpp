#include "projectile.h"
#include <SDL3/SDL.h>

// player projectile constructor shoots horizontally
Projectile::Projectile(float spawnX, float spawnY, float direction, float speed)
    : m_spawnX(spawnX), m_spawnY(spawnY), m_rect{spawnX, spawnY, 2.0f, 2.0f}, // m_rect is a small hitbox
      m_direction(direction), m_speed(speed), m_age(0.0f), m_lifetime(0.5f), // lifetime for beam effect
      m_worldWidth(Config::Game::WORLD_WIDTH), m_worldHeight(Config::Game::WORLD_HEIGHT) { // same world dimensions as in game class (TODO: fix this)
    // velocity based on direction
    m_velocity.x = m_direction * m_speed;
    m_velocity.y = 0.0f; // horizontal
    m_isHorizontal = true;
}

// opponent projectile constructor is for aimed shots
Projectile::Projectile(float spawnX, float spawnY, float targetX, float targetY, float speed)
    : m_spawnX(spawnX), m_spawnY(spawnY),
      m_rect{spawnX, spawnY, 4.0f, 4.0f},
      m_speed(speed), m_age(0.0f), m_lifetime(0.5f),
      m_worldWidth(Config::Game::WORLD_WIDTH), m_worldHeight(Config::Game::WORLD_HEIGHT) {

    float dx = targetX - spawnX;
    float dy = targetY - spawnY;
    float dist = std::sqrt(dx*dx + dy*dy);

    if (dist > 0.001f) {
        m_velocity.x = (dx / dist) * m_speed;
        m_velocity.y = (dy / dist) * m_speed;
    } else {
        m_velocity = {0, 0};
    }
}

void Projectile::update(float deltaTime) {
    m_age += deltaTime;
    m_rect.x += m_velocity.x * deltaTime;
    m_rect.y += m_velocity.y * deltaTime;
}

SDL_FRect Projectile::getBounds() const {
    return m_rect; // small hitbox
}