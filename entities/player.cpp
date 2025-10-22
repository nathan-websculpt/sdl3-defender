#include "player.h"
#include "particle.h"
#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include <algorithm>
#include "../core/texture_manager.h"

Player::Player(float x, float y, float w, float h) 
    : m_rect{x, y, w, h}, 
      m_normalSpeed(200.0f),
      m_speedBoostActive(false),
      m_boostMultiplier(2.1f),
      m_facing(Direction::RIGHT), 
      m_spacePressed(false) {
    m_speed = m_normalSpeed;
}

void Player::update(float deltaTime, plf::colony<Particle>& particles) {
    //TODO: make sure that these are getting erased
    // for (auto particle = m_boosterParticles.begin(); particle != m_boosterParticles.end(); ) {
    //     particle->update(deltaTime);
    //     if (!particle->isAlive()) {
    //         particle = m_boosterParticles.erase(particle);
    //     } else {
    //         ++particle;
    //     }
    // }

    spawnDefaultBoosterParticles(particles);

    if (m_speedBoostActive) 
        spawnBoosterParticles(particles);    
}

void Player::handleInput(const bool* keyboardState) {
    bool boostActive = keyboardState[SDL_SCANCODE_C] || keyboardState[SDL_SCANCODE_RSHIFT] || keyboardState[SDL_SCANCODE_LSHIFT];
    if (boostActive != m_speedBoostActive) {
        m_speedBoostActive = boostActive;
        m_speed = boostActive ? m_normalSpeed * m_boostMultiplier : m_normalSpeed;
    }
    float currentSpeed = m_speed * 0.016f;
    if (keyboardState[SDL_SCANCODE_UP] || keyboardState[SDL_SCANCODE_W]) {
        m_rect.y -= currentSpeed;
    }
    if (keyboardState[SDL_SCANCODE_DOWN] || keyboardState[SDL_SCANCODE_S]) {
        m_rect.y += currentSpeed;
    }
    if (keyboardState[SDL_SCANCODE_LEFT] || keyboardState[SDL_SCANCODE_A]) {
        m_rect.x -= currentSpeed;
        m_facing = Direction::LEFT;
    }
    if (keyboardState[SDL_SCANCODE_RIGHT] || keyboardState[SDL_SCANCODE_D]) {
        m_rect.x += currentSpeed;
        m_facing = Direction::RIGHT;
    }
    bool currentSpaceState = keyboardState[SDL_SCANCODE_SPACE];
    if (currentSpaceState && !m_spacePressed) {
        shoot();
    }
    m_spacePressed = currentSpaceState;
}

SDL_FRect Player::getBounds() const {
    return m_rect;
}

SDL_FPoint Player::getFrontCenter() const {
    SDL_FPoint frontCenter;
    if (m_facing == Direction::RIGHT) {
        frontCenter.x = m_rect.x + m_rect.w;
        frontCenter.y = m_rect.y + m_rect.h / 2.0f;
    } else {
        frontCenter.x = m_rect.x;
        frontCenter.y = m_rect.y + m_rect.h / 2.0f;
    }

    return frontCenter;
}

void Player::setPosition(float x, float y) {
    m_rect.x = x;
    m_rect.y = y;
}

plf::colony<Projectile>& Player::getProjectiles() {
    return m_projectiles;
}

void Player::shoot() {
    SDL_FPoint spawn = getFrontCenter();
    float dir = (m_facing == Direction::RIGHT) ? 1.0f : -1.0f;
    m_projectiles.emplace(spawn.x, spawn.y, dir, 600.0f);
}

bool Player::isHit(const SDL_FRect& opponentBounds) {
    SDL_FRect playerBounds = getBounds();

    // TODO: refactor
    return (playerBounds.x < opponentBounds.x + opponentBounds.w &&
            playerBounds.x + playerBounds.w > opponentBounds.x &&
            playerBounds.y < opponentBounds.y + opponentBounds.h &&
            playerBounds.y + playerBounds.h > opponentBounds.y);
}

void Player::setSpeedBoost(bool active) {
    m_speedBoostActive = active;
    m_speed = active ? m_normalSpeed * m_boostMultiplier : m_normalSpeed;
}

void Player::spawnBoosterParticles(plf::colony<Particle>& particles) {
    if (!m_speedBoostActive) return;
    
    SDL_FPoint rearCenter = getFrontCenter();
    if (m_facing == Direction::RIGHT) 
        rearCenter.x = m_rect.x;
    else 
        rearCenter.x = m_rect.x + m_rect.w;
    

    const int numParticles = 12;
    for (int i = 0; i < numParticles; ++i) {
        // random offset within a 12-unit wide by 22-unit tall rectangle centered on rearCenter
        float spawnX = rearCenter.x + (static_cast<float>(rand() % 12) - 6.0f);
        float spawnY = rearCenter.y + (static_cast<float>(rand() % 22) - 11.0f);

        float velX = (m_facing == Direction::RIGHT) ? -100.0f : 100.0f;
        velX += static_cast<float>(rand() % 40) - 20.0f;
        float velY = static_cast<float>(rand() % 40) - 20.0f;

        Uint8 r = 255;
        Uint8 g = static_cast<Uint8>(rand() % 100 + 100);
        Uint8 b = static_cast<Uint8>(rand() % 50);

        particles.emplace(spawnX, spawnY, velX, velY, r, g, b);
    }
}

void Player::spawnDefaultBoosterParticles(plf::colony<Particle>& particles) {    
    SDL_FPoint rearCenter = getFrontCenter();
    if (m_facing == Direction::RIGHT) 
        rearCenter.x = m_rect.x;
    else 
        rearCenter.x = m_rect.x + m_rect.w;
    

    // random offset within a 5-unit wide by 6-unit tall rectangle centered on rearCenter
    float spawnX = rearCenter.x + (static_cast<float>(rand() % 5) - 2.5f);
    float spawnY = rearCenter.y + (static_cast<float>(rand() % 6) - 3.0f);

    float velX = (m_facing == Direction::RIGHT) ? -40.0f : 40.0f;
    velX += static_cast<float>(rand() % 20) - 10.0f;
    float velY = static_cast<float>(rand() % 20) - 10.0f;

    Uint8 r = 255;
    Uint8 g = static_cast<Uint8>(rand() % 100 + 100);
    Uint8 b = static_cast<Uint8>(rand() % 50);

    particles.emplace(spawnX, spawnY, velX, velY, r, g, b);    
}