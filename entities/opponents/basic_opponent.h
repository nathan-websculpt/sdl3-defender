#pragma once
#include "base_opponent.h"
#include <vector>
#include <memory>
#include "../particle.h"

class BasicOpponent : public BaseOpponent {
public:
    BasicOpponent(float x, float y, float w, float h);
    ~BasicOpponent() = default;

    void update(float deltaTime, const SDL_FPoint& playerPos, float cameraX, int screenWidth, int screenHeight) override;
    void explode(plf::colony<Particle>& gameParticles) const override;

    const std::string& getTextureKey() const override { return Config::Textures::BASIC_OPPONENT; }

private:
    float m_startY;
};