#pragma once
#include "base_opponent.h"
#include <vector>
#include <memory>
#include "../particle.h"

class AggressiveOpponent : public BaseOpponent {
public:
    AggressiveOpponent(float x, float y, float w, float h);
    ~AggressiveOpponent() = default;

    void update(float deltaTime, const SDL_FPoint& playerPos, float cameraX, int screenWidth) override;
    void explode(plf::colony<Particle>& gameParticles) const override;

    const std::string& getTextureKey() const override { return Config::Textures::AGGRESSIVE_OPPONENT; }

private:
    float m_fireInterval = 15.8f;

    bool isOnScreen(float objX, float objY, float cameraX, int screenWidth) const;
};