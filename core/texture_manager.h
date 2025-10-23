#pragma once
#include <SDL3/SDL.h>
#include <string>
#include <unordered_map>
#include <memory>

struct SDL_Texture;

class TextureManager {
public:
    // delete copy constructor and assignment operator to enforce singleton
    TextureManager(const TextureManager&) = delete;
    TextureManager& operator=(const TextureManager&) = delete;

    // get singleton instance
    static TextureManager& getInstance();

    std::shared_ptr<SDL_Texture> getTexture(const std::string& filepath, SDL_Renderer* renderer);

    void clearCache();

private:
    TextureManager() = default; 
    ~TextureManager(); // handles SDL_DestroyTexture

    std::unordered_map<std::string, std::shared_ptr<SDL_Texture>> m_textureCache;
};

// helper
struct SDL_Texture_Deleter {
    void operator()(SDL_Texture* texture) const {
        if (texture) {
            SDL_DestroyTexture(texture);
        }
    }
};