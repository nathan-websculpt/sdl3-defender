#pragma once

#include <SDL3/SDL.h>
#include <SDL3_mixer/SDL_mixer.h>
#include <string>
#include <unordered_map>
#include <memory>

// custom deleter for MIX_Audio
struct MIX_Audio_Deleter {
    void operator()(MIX_Audio* audio) const {
        if (audio) {
            MIX_DestroyAudio(audio);
        }
    }
};

// custom deleter for MIX_Mixer
struct MIX_Mixer_Deleter {
    void operator()(MIX_Mixer* mixer) const {
        if (mixer) {
            MIX_DestroyMixer(mixer);
        }
    }
};

// custom deleter for MIX_Track
struct MIX_Track_Deleter {
    void operator()(MIX_Track* track) const {
        if (track) {
            MIX_DestroyTrack(track);
        }
    }
};

class SoundManager {
public:
    SoundManager(const SoundManager&) = delete;
    SoundManager& operator=(const SoundManager&) = delete;

    static SoundManager& getInstance();
    std::shared_ptr<MIX_Audio> getSound(const std::string& filepath);
    void clearCache();
    bool initialize(SDL_AudioDeviceID deviceID, const SDL_AudioSpec& spec);
    void shutdown();

    bool playSound(const std::string& filepath, MIX_Mixer* mixer);

    MIX_Mixer* getMixerInstance() const { return m_mixerInstance.get(); }

private:
    SoundManager() = default;
    ~SoundManager();
    std::unordered_map<std::string, std::shared_ptr<MIX_Audio>> m_soundCache;
    bool m_initialized = false; 
    // store the mixer instance created during initialization
    std::shared_ptr<MIX_Mixer> m_mixerInstance;
};