#include "texture_manager.h"
#include <SDL3_image/SDL_image.h>
#include <SDL3/SDL.h>
#include <iostream>

TextureManager& TextureManager::getInstance() {
    static TextureManager instance; //only created once (C++11)
    return instance;
}

std::shared_ptr<SDL_Texture> TextureManager::getTexture(const std::string& filepath, SDL_Renderer* renderer) {
    auto it = m_textureCache.find(filepath);
    if (it != m_textureCache.end()) {
        // SDL_Log("TextureManager: Cache HIT for '%s'. Ref count: %ld", filepath.c_str(), it->second.use_count());
        // return existing shared pointer
        return it->second;
    }

    SDL_Texture* texture = IMG_LoadTexture(renderer, filepath.c_str());
    if (!texture) {
        SDL_Log("Failed to load texture '%s': %s", filepath.c_str(), SDL_GetError());
        return nullptr; // return null shared_ptr if loading fails
    }

    // wrap raw pointer in a shared_ptr with custom deleter
    auto sharedTexture = std::shared_ptr<SDL_Texture>(texture, SDL_Texture_Deleter{});

    // store shared pointer in the cache
    m_textureCache[filepath] = sharedTexture;

    // SDL_Log("TextureManager: Cache MISS, LOADED texture '%s'. Ref count: %ld", filepath.c_str(), sharedTexture.use_count());

    return sharedTexture;
}

void TextureManager::clearCache() {
    SDL_Log("TextureManager: Clearing cache and destroying %zu textures.", m_textureCache.size());
    m_textureCache.clear(); // will call the deleter for each texture
}

TextureManager::~TextureManager() {
    clearCache(); //clean up all cached textures when manager is destroyed
}