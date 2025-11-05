#pragma once
#include "../../plf/plf_colony.h" 
#include "game_helper.h"
#include "../../entities/projectile.h"
#include "../../entities/particle.h"
#include "../../entities/health_item.h"

namespace ColonyUpdateAndPrune {
    void updateAndPruneProjectiles(plf::colony<Projectile>& projectiles, float deltaTime, const GameHelper& helpers);
    void updateAndPruneParticles(plf::colony<Particle>& particles, float deltaTime);
    void updateAndPruneHealthItems(plf::colony<std::unique_ptr<HealthItem>>& healthItems, float deltaTime, const GameHelper& helpers);
}