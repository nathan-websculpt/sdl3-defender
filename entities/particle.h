#pragma once
#include <SDL3/SDL.h>

class Particle {
public:
    Particle(float x, float y, float velocityX, float velocityY, Uint8 r, Uint8 g, Uint8 b, float initialSize = 2.0f, float lifetime = 0.2f);
    ~Particle() = default;
    void update(float deltaTime);
    bool isAlive() const;
    SDL_FRect getBounds() const;

    // getters for rendering state
    float getX() const { return m_rect.x; }
    float getY() const { return m_rect.y; }
    float getCurrentSize() const { return m_rect.w; } // assuming the particle's width is same as height
    Uint8 getR() const { return m_r; }
    Uint8 getG() const { return m_g; }
    Uint8 getB() const { return m_b; }
    Uint8 getAlpha() const { return m_alpha; }

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