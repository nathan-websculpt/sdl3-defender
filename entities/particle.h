#pragma once
#include <SDL3/SDL.h>

class Particle {
public:
    Particle(float x, float y, float velocityX, float velocityY, Uint8 r, Uint8 g, Uint8 b, float initialSize = 2.0f, float lifetime = 0.2f);
    ~Particle();
    void update(float deltaTime);
    void render(SDL_Renderer* renderer, SDL_FRect* renderBounds) const;
    bool isAlive() const;
    SDL_FRect getBounds() const;

private:
    SDL_FRect m_rect;
    SDL_FPoint m_velocity;
    
    // color
    Uint8 m_r;
    Uint8 m_g;
    Uint8 m_b; 
    Uint8 m_alpha;// alpha is for fading

    float m_age;
    float m_lifetime;
    float m_initialSize;
    float m_growRate;
    float m_fadeRate;
};