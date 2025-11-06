#include "game.h"
#include <algorithm>
#include <cctype>
#include "config.h"
#include "globals.h"

void Game::handleEscapeKey() {
    if (m_state.state == GameStateData::State::MENU) {
        m_state.running = false;
    } else if (m_state.state == GameStateData::State::PLAYING) {
        m_state.state = GameStateData::State::MENU;
    } else if (m_state.state == GameStateData::State::GAME_OVER) {
        if (m_state.waitingForHighScore) {
            std::string nameToSubmit = m_state.highScoreNameInput.empty() ? "ANON" : m_state.highScoreNameInput;
            m_highScores.submitHighScore(nameToSubmit, m_state);
            m_state.waitingForHighScore = false;
            m_state.state = GameStateData::State::MENU;
        } else {
            m_state.state = GameStateData::State::MENU;
        }
    } else if (m_state.state == GameStateData::State::HOW_TO_PLAY) {
        m_state.state = GameStateData::State::MENU;
    }
}

void Game::handleInputMenu(const GameInput& input) {
    if (input.enter) {
        startNewGame();
    } else if (input.mouseClick) {
        int mx = input.mouseX;
        int my = input.mouseY;
        int w = globals.windowWidth;
        int h = globals.windowHeight;
        SDL_FRect playBtn = { (float)(w/2 - 100), (float)(h/2 - 60), 200, 50 };
        SDL_FRect howToPlayBtn = { (float)(w/2 - 100), (float)(h/2), 200, 50 };
        SDL_FRect exitBtn = { (float)(w/2 - 100), (float)(h/2 + 60), 200, 50 };
        if (mx >= playBtn.x && mx < playBtn.x + playBtn.w && my >= playBtn.y && my < playBtn.y + playBtn.h) {
            startNewGame();
        } else if (mx >= howToPlayBtn.x && mx < howToPlayBtn.x + howToPlayBtn.w && my >= howToPlayBtn.y && my < howToPlayBtn.y + howToPlayBtn.h) {
            m_state.state = GameStateData::State::HOW_TO_PLAY;
        } else if (mx >= exitBtn.x && mx < exitBtn.x + exitBtn.w && my >= exitBtn.y && my < exitBtn.y + exitBtn.h) {
            m_state.running = false;
        }
    }
}

void Game::handleInputHowToPlay(const GameInput& input) {
    if (input.enter || (input.mouseClick && input.mouseX > globals.windowWidth - 30 && input.mouseY < 30)) {
        m_state.state = GameStateData::State::MENU;
    }
}

void Game::handleInputPlaying(const GameInput& input, float deltaTime) {
    if (!m_state.player) return;
    
    m_state.player->setSpeedBoost(input.boost);

    if (input.shoot && !m_prevShootState) { // current frame: pressed, previous frame: not pressed
        m_state.player->shoot();
    }

    // update the previous state for the next frame
    m_prevShootState = input.shoot;

    float speed = m_state.player->getSpeed();
    float dx = 0, dy = 0;
    if (input.moveLeft) { dx -= speed * deltaTime; m_state.player->setFacing(Direction::LEFT); }
    if (input.moveRight) { dx += speed * deltaTime; m_state.player->setFacing(Direction::RIGHT); }
    if (input.moveUp) dy -= speed * deltaTime;
    if (input.moveDown) dy += speed * deltaTime;
    m_state.player->moveBy(dx, dy);
}

void Game::handleInputGameOver(const GameInput& input, float deltaTime) {
    if (m_state.waitingForHighScore) {
        if (input.charInputEvent) {
            char c = input.inputChar;
            if (m_state.highScoreNameInput.length() < 10 && (std::isalnum(static_cast<unsigned char>(c)) || c == ' ')) {
                m_state.highScoreNameInput += c;
            }
        }           
        
        static float backspaceCooldown = 0.0f;
        const float BACKSPACE_DELAY = 0.1f; 
        if (input.backspacePressed) { 
            if (backspaceCooldown <= 0.0f && !m_state.highScoreNameInput.empty()) {
                m_state.highScoreNameInput.pop_back();
                backspaceCooldown = BACKSPACE_DELAY;
            } else {
                backspaceCooldown = std::max(0.0f, backspaceCooldown - 1.0f/60.0f);
            }
        } else {
            backspaceCooldown = 0.0f;
        }

        // process enter/click for submission/cancellation
        if (input.enter) {
            // trim leading/trailing spaces
            std::string trimmedName = m_state.highScoreNameInput;
            if (!trimmedName.empty()) {
                size_t start = trimmedName.find_first_not_of(" \t");
                size_t end = trimmedName.find_last_not_of(" \t");
                if (start != std::string::npos && end != std::string::npos) {
                    trimmedName = trimmedName.substr(start, end - start + 1);
                } else {
                    trimmedName = "";
                }
            }
            // use "ANON" if the name is empty after trimming or was empty initially
            if (trimmedName.empty()) {
                trimmedName = "ANON";
            }
            m_highScores.submitHighScore(trimmedName, m_state);
            m_state.waitingForHighScore = false;
            m_state.state = GameStateData::State::MENU;
        } else if (input.mouseClick) {
            if (input.mouseX > globals.windowWidth - 30 && input.mouseY < 30) {
                // use "ANON" if user cancels with 'X' and input was empty
                std::string nameToSubmit = m_state.highScoreNameInput.empty() ? "ANON" : m_state.highScoreNameInput;
                m_highScores.submitHighScore(nameToSubmit, m_state);
                m_state.waitingForHighScore = false;
                m_state.state = GameStateData::State::MENU;
            }
        }
    } else { // not waiting for high score - game over screen
        if (input.enter || (input.mouseClick && input.mouseX > globals.windowWidth - 30 && input.mouseY < 30)) {
                m_state.state = GameStateData::State::MENU;
        }
    }
}