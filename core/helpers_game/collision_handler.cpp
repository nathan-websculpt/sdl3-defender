#include "collision_handler.h"
#include <memory>

using namespace CollisionHandler;

// renames helpers, state, mixer
namespace {
    // player projectiles hitting opponents
    void handlePlayerProjectiles(GameStateData& state, GameHelper& helpers) {
        if (!state.player) return;
        auto& pp = state.player->getProjectiles();
        for (auto p_it = pp.begin(); p_it != pp.end(); ) {
            SDL_FRect pb = p_it->getBounds();
            bool projectileHit = false;

            // for horizontal beams, find visual end X
            float beamY = p_it->getSpawnY();
            float startX = p_it->getSpawnX();
            bool goingRight = (p_it->getVelocity().x > 0);
            // landscape stops beam
            float visualEndX = helpers.getBeamVisualEndX(startX, beamY, goingRight);

            for (auto& o : state.opponents) { // o is std::unique_ptr<BaseOpponent>&
                if (!o || !o->isAlive()) continue;

                // skip if opponent is beyond the beam's visual range (landscape stopped it)
                float oppCenterX = o->getBounds().x + o->getBounds().w / 2.0f;
                if (goingRight && oppCenterX > visualEndX) continue;
                if (!goingRight && oppCenterX < visualEndX) continue;

                if (helpers.rectsIntersect(o->getBounds(), pb)) {
                    o->takeDamage(1);
                    if (!o->isAlive()) {
                        state.playerScore += o->getScoreVal();
                        o->explode(state.particles);
                    }
                    projectileHit = true;
                    break; // break inner loop
                }
            }
            if (projectileHit) {
                p_it = pp.erase(p_it); // erase using projectile iterator, assign returned iterator
            } else {
                ++p_it;
            }
        }
    }

    // player collisions with opponents and opponents projectiles collision with player
    // returns true if processing completed normally, false if it resulted in game-over
    bool handleOpponentsAndPlayer(GameStateData& state, GameHelper& helpers, HighScores& highScores, MIX_Mixer* mixer) {
        if (!state.player || !state.player->isAlive()) return true;
        for (auto o_it = state.opponents.begin(); o_it != state.opponents.end(); ) {
            auto& o = *o_it;
            if (!o || !o->isAlive()) {
                 ++o_it; // skip dead opponents
                 continue;
            }

            // check player/opponent collision
            if (helpers.rectsIntersect(state.player->getBounds(), o->getBounds())) { 
                state.player->takeDamage(1);
                o->explode(state.particles); 
                state.playerScore += o->getScoreVal();
                o_it = state.opponents.erase(o_it);
                if (!state.player->isAlive()) {
                    if (mixer) 
                            SoundManager::getInstance().playSound(Config::Sounds::GAME_OVER, mixer);
                            
                    state.state = GameStateData::State::GAME_OVER;
                    if (highScores.isHighScore(state)) {
                        state.highScoreIndex = highScores.getHighScoreIndex(state);
                        state.waitingForHighScore = true;
                        state.highScoreNameInput = ""; // initialize empty input
                    }
                    return false; // exit early if player dies
                }
                continue; // skip projectile check if opponent was destroyed by collision
            }

            // check if opponent's projectiles hit player
            auto& op = o->getProjectiles(); 
            for (auto op_it = op.begin(); op_it != op.end(); ) {
                SDL_FRect projBounds = op_it->getBounds();
                SDL_FRect playerBounds = state.player->getBounds();

                // collision check ... projectile and player
                if (helpers.rectsIntersect(projBounds, playerBounds)) {
                    state.player->takeDamage(1);
                    // erase the projectile that hit the player using the iterator
                    op_it = op.erase(op_it);
                    if (!state.player->isAlive()) {
                        if (mixer) 
                            SoundManager::getInstance().playSound(Config::Sounds::GAME_OVER, mixer);

                        state.state = GameStateData::State::GAME_OVER;
                        if (highScores.isHighScore(state)) {
                            state.highScoreIndex = highScores.getHighScoreIndex(state);
                            state.waitingForHighScore = true;
                            state.highScoreNameInput = ""; // initialize empty input
                        }
                        return false; // exit early if player dies
                    }
                } else {
                    ++op_it;
                }
            }

            // increment opponent iterator only if the opponent itself wasn't erased in the player collision check
            if (state.state != GameStateData::State::GAME_OVER && o_it != state.opponents.end()) { // check if state changed or iterator became invalid due to erase
                ++o_it;
            }
            // ... if state is GAME_OVER or o_it was invalidated by erase in the inner loop, the outer loop will terminate
        }
        return true;
    }

    // player collisions with health items
    void handleHealthItems(GameStateData& state, GameHelper& helpers) {
        if (!state.player) return;
        for (auto it = state.healthItems.begin(); it != state.healthItems.end(); ) {
            auto& item = *it;
            if (!item || !item->isAlive() || item->isBlinking()) { // don't collide if blinking or dead
                ++it;
                continue;
            }
            if (helpers.rectsIntersect(state.player->getBounds(), item->getBounds())) {
                if (item->getType() == HealthItemType::PLAYER) {
                    state.player->restoreHealth();
                } else if (item->getType() == HealthItemType::WORLD) {
                    state.worldHealth = state.maxWorldHealth;
                }
                it = state.healthItems.erase(it);
                continue;
            }
            ++it;
        }
    }

} // namespace CollisionHandler

void CollisionHandler::processAllCollisions(GameStateData& state, GameHelper& helpers, HighScores& highScores, MIX_Mixer* mixer) {
    if (!state.player) return;

    handlePlayerProjectiles(state, helpers);

    if (!handleOpponentsAndPlayer(state, helpers, highScores, mixer)) return; // player died inside the handler and state has been set to GAME_OVER
        
    handleHealthItems(state, helpers);
}

