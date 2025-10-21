#include "projectile.h"
#include <SDL3/SDL.h>
#include <cmath>

// player projectile constructor shoots horizontally
Projectile::Projectile(float spawnX, float spawnY, float direction, float speed)
    : m_spawnX(spawnX), m_spawnY(spawnY), m_rect{spawnX, spawnY, 2.0f, 2.0f}, // m_rect is a small hitbox
      m_direction(direction), m_speed(speed), m_age(0.0f), m_lifetime(0.5f), // lifetime for beam effect
      m_worldWidth(Config::Game::WORLD_WIDTH), m_worldHeight(Config::Game::WORLD_HEIGHT) { // same world dimensions as in game class (TODO: refactor)
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

Projectile::~Projectile() {
    // TODO: will there ever be dynamic resources here?
}

void Projectile::update(float deltaTime) {
    m_age += deltaTime;
    m_rect.x += m_velocity.x * deltaTime;
    m_rect.y += m_velocity.y * deltaTime;
}

void Projectile::render(SDL_Renderer* renderer, SDL_FRect* renderBounds) const {
    if (m_age >= m_lifetime) return;

    float lifeRatio = (m_lifetime - m_age) / m_lifetime;
    Uint8 alpha = static_cast<Uint8>(255 * lifeRatio * 0.7f);
    float pulseAlpha = 0.7f + 0.3f * std::sin(m_age * 25.0f);
    alpha = static_cast<Uint8>(alpha * pulseAlpha);

    Uint8 r = static_cast<Uint8>(200 + 55 * std::sin(m_age * 30.0f));
    Uint8 g = static_cast<Uint8>(200 + 55 * std::sin(m_age * 25.0f + M_PI));
    Uint8 b = static_cast<Uint8>(0 + 55 * std::sin(m_age * 35.0f + M_PI/2));

    SDL_SetRenderDrawColor(renderer, r, g, b, alpha);

    float offsetX = m_rect.x - renderBounds->x;
    float offsetY = m_rect.y - renderBounds->y;

    if (m_isHorizontal) {
        // player beam: from spawn to world edge
        float startX = m_spawnX - offsetX;
        float startY = m_spawnY - offsetY;
        float endX = (m_velocity.x > 0) ? (m_worldWidth - offsetX) : (0.0f - offsetX);
        float endY = startY;

        SDL_RenderLine(renderer, startX, startY, endX, endY);
    } else {
       float dx = m_rect.x - m_spawnX;
        float dy = m_rect.y - m_spawnY;

        //extend 3x further in same direction
        float endX = m_spawnX + dx * 4.0f; 
        float endY = m_spawnY + dy * 4.0f;

        SDL_FPoint start = { m_spawnX - offsetX, m_spawnY - offsetY };
        SDL_FPoint end   = { endX - offsetX, endY - offsetY };
        SDL_RenderLine(renderer, start.x, start.y, end.x, end.y);
    }
}

SDL_FRect Projectile::getBounds() const {
    return m_rect; // small hitbox
}

// check if the current position is off-screen
bool Projectile::isOffScreen(int screenWidth, int screenHeight) const {
    return (m_rect.x < -50 || m_rect.x > m_worldWidth + 50 || m_rect.y < -50 || m_rect.y > m_worldHeight + 50);
}