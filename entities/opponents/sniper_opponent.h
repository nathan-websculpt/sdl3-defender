#pragma once
#include "base_opponent.h"
#include <vector>
#include <memory>
#include "../particle.h"

class SniperOpponent : public BaseOpponent {
public:
    SniperOpponent(float x, float y, float w, float h);
    ~SniperOpponent() = default;

    void update(float deltaTime, const SDL_FPoint& playerPos, float cameraX, const GameStateData& state) override;

    const std::string& getTextureKey() const override { return Config::Textures::SNIPER_OPPONENT; }

private:
    float m_oscillationSpeed;
    float m_oscillationOffset;
};