#pragma once
#include <SDL3/SDL.h>
#include <string>

enum class HealthItemType {
    PLAYER,
    WORLD
};

class HealthItem {
public:
    HealthItem(float x, float y, float w, float h, HealthItemType type, const std::string& textureKey);

    void update(float deltaTime);
    SDL_FRect getBounds() const;
    HealthItemType getType() const;
    bool isAlive() const; // false after blinking finishes
    bool isBlinking() const; // for rendering
    float getBlinkAlpha() const;

    // when the item hits the landscape
    void startBlinking();

    static const float BLINK_DURATION;
    const std::string& getTextureKey() const; 

private:
    SDL_FRect m_rect;
    HealthItemType m_type;
    std::string m_textureKey;
    float m_velocityY; // falling speed
    bool m_hitGround;
    bool m_blinking;
    float m_blinkTimer;
    int m_blinkCount;
    static const int MAX_BLINKS = 3;
    bool m_doesStop = false; // 25% chance it stops in the world
    bool m_hasStopped = false;
    int m_randomStopY;
};