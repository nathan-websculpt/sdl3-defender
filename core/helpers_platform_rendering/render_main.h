#pragma once
#include "../game.h"
#include "../globals.h"

class RenderMain {
public:
    static void render(const GameStateData& state);

private:
    static void renderPlaying(const GameStateData& state);
    static void renderPlayerAndProjectiles(const GameStateData& state, float cameraOffsetX);
    static void renderOpponentsAndProjectiles(const GameStateData& state, float cameraOffsetX);
    static void renderParticles(const GameStateData& state, float cameraOffsetX);
    static void renderLandscape(const GameStateData& state, float cameraOffsetX);
    static void renderHealthItems(const GameStateData& state, float cameraOffsetX);
};