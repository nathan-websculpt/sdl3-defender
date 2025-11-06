#include "colony_update_and_prune.h"

namespace ColonyUpdateAndPrune {

    void projectiles(plf::colony<Projectile>& projectiles, float deltaTime, const GameHelper& helpers) {
        for (auto it = projectiles.begin(); it != projectiles.end(); ) {
            it->update(deltaTime);
            SDL_FRect b = it->getBounds();
            
            if (helpers.isOutOfWorld(b, 0.0f, 0.0f)) {
                it = projectiles.erase(it);
                continue;
            }

            float projCenterX = b.x + b.w / 2.0f;
            float groundY = helpers.getGroundYAt(projCenterX);
            float projBottom = b.y + b.h;

            // if projectile is at or below ground - remove it
            if (projBottom >= groundY) {
                it = projectiles.erase(it);
                continue;
            }

            ++it;
        }
    }

    void particles(plf::colony<Particle>& particles, float deltaTime) {
        for (auto it = particles.begin(); it != particles.end(); ) {
            it->update(deltaTime);
            if (!it->isAlive()) 
                it = particles.erase(it);
            else 
                ++it;
        }
    }

    void healthItems(plf::colony<std::unique_ptr<HealthItem>>& healthItems, float deltaTime, const GameHelper& helpers) {
        for (auto it = healthItems.begin(); it != healthItems.end(); ) {
            auto& item = *it;
            if (!item) {
                ++it;
                continue;
            }
            item->update(deltaTime);

            // check if item hit the landscape
            float groundY = helpers.getGroundYAt(item->getBounds().x + item->getBounds().w / 2.0f);
            float itemBottom = item->getBounds().y + item->getBounds().h;
            if (itemBottom >= groundY && !item->isBlinking()) {
                item->startBlinking();
            }

            // remove dead items (finished blinking)
            if (!item->isAlive()) {
                it = healthItems.erase(it);
                continue;
            }
            ++it;
        }
    }
}