#pragma once
#include "base_opponent.h"
#include <vector>
#include <memory>
#include "../particle.h"

class AggressiveOpponent : public BaseOpponent {
public:
    AggressiveOpponent(float x, float y, float w, float h);
    ~AggressiveOpponent() = default;

    void update(float deltaTime, const SDL_FPoint& playerPos, float cameraX, const GameStateData& state) override;

    const std::string& getTextureKey() const override { return Config::Textures::AGGRESSIVE_OPPONENT; }
};