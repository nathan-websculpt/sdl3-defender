#include "sound_manager.h"
#include <iostream>
#include <algorithm>

SoundManager& SoundManager::getInstance() {
    static SoundManager instance;
    return instance;
}

bool SoundManager::initialize(SDL_AudioDeviceID deviceID, const SDL_AudioSpec& spec) {
    if (m_initialized) {
        SDL_Log("SoundManager: Already initialized.");
        return true;
    }

    bool init_success = MIX_Init();
    if (!init_success) { 
        SDL_Log("SDL_mixer could not initialize! MIX_Init Error: %s", SDL_GetError());
        return false;
    }

    MIX_Mixer* mixer = MIX_CreateMixerDevice(deviceID, &spec);
    if (!mixer) {
        SDL_Log("SDL_mixer could not create mixer for device %d! SDL_GetError: %s", deviceID, SDL_GetError());
        MIX_Quit();
        return false;
    }

    m_mixerInstance = std::shared_ptr<MIX_Mixer>(mixer, MIX_Mixer_Deleter{});

    m_initialized = true;
    SDL_Log("SoundManager: SDL_mixer initialized successfully with device %d.", deviceID);
    return true;
}

void SoundManager::shutdown() {
    if (m_initialized) {
        clearCache();
        m_mixerInstance.reset();
        m_initialized = false;
        SDL_Log("SoundManager: SDL_mixer shut down.");
        MIX_Quit();
    }
}

std::shared_ptr<MIX_Audio> SoundManager::getSound(const std::string& filepath) {
    if (!m_initialized || !m_mixerInstance) {
        SDL_Log("SoundManager: Not initialized or mixer not available! Cannot load sound: %s", filepath.c_str());
        return nullptr;
    }

    auto it = m_soundCache.find(filepath);
    if (it != m_soundCache.end()) {
        SDL_Log("SoundManager: Cache HIT for sound '%s'.", filepath.c_str());
        return it->second;
    }

    MIX_Audio* audio = MIX_LoadAudio(m_mixerInstance.get(), filepath.c_str(), true);
    if (!audio) {
        SDL_Log("Failed to load sound '%s': %s", filepath.c_str(), SDL_GetError());
        return nullptr; 
    }

    auto sharedAudio = std::shared_ptr<MIX_Audio>(audio, MIX_Audio_Deleter{});
    m_soundCache[filepath] = sharedAudio;
    SDL_Log("SoundManager: Cache MISS, LOADED sound '%s'.", filepath.c_str());
    return sharedAudio;
}

void SoundManager::clearCache() {
    if (m_initialized) {
        SDL_Log("SoundManager: Clearing sound cache and destroying %zu audio objects.", m_soundCache.size());
        m_soundCache.clear(); // will call the deleter for each MIX_Audio
    }
}

SoundManager::~SoundManager() {
    shutdown(); 
}

bool SoundManager::playSound(const std::string& filepath, MIX_Mixer* mixer) {
    if (!m_initialized || !mixer) {
         SDL_Log("SoundManager: Cannot play sound, not initialized or mixer is null.");
         return false;
    }

    auto audioSharedPtr = getSound(filepath);
    if (!audioSharedPtr) {
        SDL_Log("SoundManager: Cannot play sound '%s', failed to load or retrieve.", filepath.c_str());
        return false;
    }

    // create a temporary track for playback
    MIX_Track* track = MIX_CreateTrack(mixer);
    if (!track) {
        SDL_Log("SoundManager: Failed to create track for sound '%s': %s", filepath.c_str(), SDL_GetError());
        return false;
    }

    auto sharedTrack = std::shared_ptr<MIX_Track>(track, MIX_Track_Deleter{});

    if (!MIX_SetTrackAudio(sharedTrack.get(), audioSharedPtr.get())) {
        SDL_Log("SoundManager: Failed to assign audio to track for sound '%s': %s", filepath.c_str(), SDL_GetError());
        return false; 
    }

    SDL_PropertiesID options = SDL_CreateProperties();
    if (!options) {
        SDL_Log("SoundManager: Failed to create properties for sound '%s': %s", filepath.c_str(), SDL_GetError());
        return false;
    }

    // play once
    bool playSuccess = MIX_PlayTrack(sharedTrack.get(), options);
    SDL_DestroyProperties(options);

    if (!playSuccess) {
        SDL_Log("SoundManager: Failed to play track for sound '%s': %s", filepath.c_str(), SDL_GetError());
        return false;
    }

    SDL_Log("SoundManager: Played sound '%s'.", filepath.c_str());
    return true;
}