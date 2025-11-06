#include "game.h"
#include <cstdlib>
#include <ctime>
#include <algorithm>
#include <fstream> //TODO
#include <sstream> //
#include <cctype>
#include "../core/config.h" // TODO
#include "../core/globals.h"
#include "../entities/health_item.h"
#include "helpers_game/collision_handler.h"

Game::Game()
    : m_state{}, m_gameHelpers(m_state.landscape) { // TODO: dims go to globals?
    srand((unsigned int)time(nullptr)); // TODO: use in main instead???
    // globals.windowHeight = Config::Game::WORLD_HEIGHT; // TODO: ???
    m_highScores.loadHighScores(m_state);
}

void Game::startNewGame() {
    m_mixer = SoundManager::getInstance().getMixerInstance();
    if (m_mixer) 
        SoundManager::getInstance().playSound(Config::Sounds::GAME_START, m_mixer);

    m_state.opponents.clear();
    m_state.particles.clear();
    m_state.healthItems.clear();
    m_state.cameraX = 0.0f;

    m_lastWindowHeight = globals.windowHeight;
    globals.windowHeight = globals.windowHeight;
    float px = Config::Game::WORLD_WIDTH / 2.0f - 40.0f;
    float py = globals.windowHeight / 2.0f - 24.0f;
    m_state.player = std::make_unique<Player>(px, py, 80, 48);

    m_state.state = GameStateData::State::PLAYING;
    m_state.worldHealth = m_state.maxWorldHealth;
    m_state.playerScore = 0;
    m_opponentSpawnTimer = 0.0f;

    m_playerHealthItemSpawnTimer = 0.0f;
    m_worldHealthItemSpawnTimer = 0.0f;

    setLandscape();
}

void Game::setLandscape() {
    m_state.landscape = {
        {0, globals.windowHeight - 20.0f},
        {Config::Game::WORLD_WIDTH * 0.1f, globals.windowHeight - 28.0f},
        {Config::Game::WORLD_WIDTH * 0.18f, globals.windowHeight - 38.0f},
        {Config::Game::WORLD_WIDTH * 0.225f, globals.windowHeight - 50.0f},
        {Config::Game::WORLD_WIDTH * 0.25f, globals.windowHeight - 40.0f},
        {Config::Game::WORLD_WIDTH * 0.32f, globals.windowHeight - 120.0f},
        {Config::Game::WORLD_WIDTH * 0.41f, globals.windowHeight - 100.0f},
        {Config::Game::WORLD_WIDTH * 0.48f, globals.windowHeight - 140.0f},
        {Config::Game::WORLD_WIDTH * 0.52f, globals.windowHeight - 95.0f},
        {Config::Game::WORLD_WIDTH * 0.61f, globals.windowHeight - 120.0f},
        {Config::Game::WORLD_WIDTH * 0.68f, globals.windowHeight - 80.0f},
        {Config::Game::WORLD_WIDTH * 0.71f, globals.windowHeight - 110.0f},
        {Config::Game::WORLD_WIDTH * 0.75f, globals.windowHeight - 90.0f},
        {Config::Game::WORLD_WIDTH * 0.81f, globals.windowHeight - 70.0f},
        {Config::Game::WORLD_WIDTH * 0.86f, globals.windowHeight - 110.0f},
        {Config::Game::WORLD_WIDTH * 0.90f, globals.windowHeight - 75.0f},
        {Config::Game::WORLD_WIDTH * 0.93f, globals.windowHeight - 90.0f},
        {Config::Game::WORLD_WIDTH * 0.98f, globals.windowHeight - 60.0f},
        {Config::Game::WORLD_WIDTH, globals.windowHeight - 40.0f}
    };
}

void Game::update(float deltaTime) {
    if (m_state.state != GameStateData::State::PLAYING) return;

    if (globals.windowHeight != m_lastWindowHeight) {
        m_lastWindowHeight = globals.windowHeight;
        setLandscape();
    }

    SDL_FRect pb;
    if (m_state.player) {
        pb = m_state.player->getBounds();
    }

    updatePlayerAndProjectiles(deltaTime, pb);

    m_gameHelpers.keepPlayerInBounds(m_state.player, pb);

    if(!updateOpponents(deltaTime, pb)) // TODO: ^^^ reorder
        return; // this means that a bomb dropped the world health to 0 - game over

    ColonyUpdateAndPrune::updateAndPruneParticles(m_state.particles, deltaTime);

    ColonyUpdateAndPrune::updateAndPruneHealthItems(m_state.healthItems, deltaTime, m_gameHelpers);

    handleSpawnsAndTimers(deltaTime);

    CollisionHandler::processAllCollisions(m_state, m_gameHelpers, m_highScores, m_mixer);
    updateCamera();    
}

void Game::updateCamera() {
    if (!m_state.player) return;
    SDL_FRect pb = m_state.player->getBounds();
    float target = pb.x - globals.windowWidth / 2.0f;
    if (target < 0) target = 0;
    if (target > Config::Game::WORLD_WIDTH - globals.windowWidth) target = Config::Game::WORLD_WIDTH - globals.windowWidth;
    m_state.cameraX = target;
}

void Game::handleInput(const GameInput& input, float deltaTime) {
    if (input.quit) {
        m_state.running = false;
        return;
    }

    // 'ESC' key processes and returns early, skipping any other input for this frame
    if (input.escape) {
        handleEscapeKey();        
        return; // exit early
    }

    if (m_state.state == GameStateData::State::MENU) {
        handleInputMenu(input);
    } else if (m_state.state == GameStateData::State::HOW_TO_PLAY) {
        handleInputHowToPlay(input);
    } else if (m_state.state == GameStateData::State::PLAYING) {
        handleInputPlaying(input, deltaTime);
    } else if (m_state.state == GameStateData::State::GAME_OVER) {
        handleInputGameOver(input, deltaTime);
    }
}

// TODO: move to update_handler
void Game::spawnOpponent() {
    int type = rand() % 3;
    float x = (float)(rand() % (int)(Config::Game::WORLD_WIDTH - 50));
    float y = -50.0f;
    switch (type) {
        case 0: m_state.opponents.emplace(std::make_unique<BasicOpponent>(x, y, 40, 40)); break;
        case 1: m_state.opponents.emplace(std::make_unique<AggressiveOpponent>(x, y, 45, 45)); break;
        case 2: m_state.opponents.emplace(std::make_unique<SniperOpponent>(x, y, 35, 35)); break;
    }
}

// TODO: move to update_handler
void Game::spawnHealthItem(HealthItemType type) {
    float x = static_cast<float>(rand() % static_cast<int>(Config::Game::WORLD_WIDTH - 50)); // random X within world
    float y = -50.0f; // start from top
    float w = 30.0f;
    float h = 30.0f;
    const std::string& textureKey = (type == HealthItemType::PLAYER) ? Config::Textures::PLAYER_HEALTH_ITEM : Config::Textures::WORLD_HEALTH_ITEM;
    m_state.healthItems.emplace(std::make_unique<HealthItem>(x, y, w, h, type, textureKey));
}