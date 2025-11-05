#pragma once
#include <vector>
#include <SDL3/SDL.h>
#include "../globals.h"
#include "../config.h"

// added to move some of the methods in Game here
// holds non-owning pointers/references to the landscape

class GameHelper {
public:
    GameHelper(const std::vector<SDL_FPoint>* landscapePtr);

    float getGroundYAt(float x) const; // for landscape
    bool isOutOfWorld(const SDL_FRect& r, float mx = 100.0f, float my = 100.0f) const;

private:
    const std::vector<SDL_FPoint>* m_landscape;
};