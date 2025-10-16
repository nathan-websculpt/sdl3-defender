#pragma once
#include "base_opponent.h"
#include <vector>
#include <memory>
#include "../particle.h"
class BasicOpponent : public BaseOpponent {
public:
    BasicOpponent(float x, float y, float w, float h, SDL_Renderer* renderer);
    ~BasicOpponent() = default;

    void update(float deltaTime, const SDL_FPoint& playerPos, float cameraX, int screenWidth) override;
    void render(SDL_Renderer* renderer, SDL_FRect* renderBounds) const override;
    void explode(std::vector<std::unique_ptr<Particle>>& gameParticles) const override;

private:
    float m_startY;
};