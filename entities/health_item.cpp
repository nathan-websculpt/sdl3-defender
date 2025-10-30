#include "health_item.h"
#include <cmath> 

const float HealthItem::BLINK_DURATION = 0.2f;

HealthItem::HealthItem(float x, float y, float w, float h, HealthItemType type, const std::string& textureKey)
    : m_rect{x, y, w, h}, m_type(type), m_textureKey(textureKey), m_velocityY(50.0f), m_hitGround(false), m_blinking(false), m_blinkTimer(0.0f), m_blinkCount(0) 
    { 
        m_doesStop = (std::rand() / (RAND_MAX + 1.0f)) < 0.25f; // ~25% chance
        if(m_doesStop)             
            m_randomStopY = 200 + (std::rand() % 101);// random between 200 and 300   
    }

void HealthItem::update(float deltaTime) {
    if (m_blinking) {
        m_blinkTimer += deltaTime;
        if (m_blinkTimer >= BLINK_DURATION) {
            m_blinkTimer = 0.0f;
            m_blinkCount++;
        }
    } else {
        if (!m_hasStopped) {
            if(m_doesStop) {
                if (m_rect.y >= m_randomStopY) {
                    m_hasStopped = true;
                }
            }
            m_rect.y += m_velocityY * deltaTime; // move downwards
        } // else leave health item where it is for player to pick-up
    }
}

SDL_FRect HealthItem::getBounds() const {
    return m_rect;
}

HealthItemType HealthItem::getType() const {
    return m_type;
}

bool HealthItem::isAlive() const {
    return m_blinkCount < MAX_BLINKS * 2; // each blink has an on/off phase
}

bool HealthItem::isBlinking() const {
    return m_blinking;
}

float HealthItem::getBlinkAlpha() const {
    if (!m_blinking) return 255.0f;
    // on/off blinking based on timer
    return (static_cast<int>(m_blinkTimer / (BLINK_DURATION / 2.0f)) % 2 == 0) ? 255.0f : 0.0f;
}

void HealthItem::startBlinking() {
    m_hitGround = true; // TODO: used?
    m_blinking = true;
    m_blinkTimer = 0.0f;
    m_blinkCount = 0;
}

const std::string& HealthItem::getTextureKey() const {
    return m_textureKey;
}