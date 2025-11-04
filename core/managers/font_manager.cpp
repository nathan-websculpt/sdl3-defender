#include "font_manager.h"
#include <SDL3_ttf/SDL_ttf.h>
#include <iostream>
#include <sstream>

FontManager& FontManager::getInstance() {
    static FontManager instance;
    return instance;
}

std::shared_ptr<TTF_Font> FontManager::getFont(const std::string& filepath, int size) {

    //a unique key for filepath and size combination
    std::ostringstream keyStream;
    keyStream << filepath << "_" << size;
    std::string key = keyStream.str();

    auto it = m_fontCache.find(key);
    if (it != m_fontCache.end()) {
        // SDL_Log("FontManager: Cache HIT for '%s'. Ref count: %ld", key.c_str(), it->second.use_count());
        return it->second;
    }

    TTF_Font* font = TTF_OpenFont(filepath.c_str(), size);
    if (!font) {
        SDL_Log("FontManager: Failed to load font '%s' with size %d: %s", filepath.c_str(), size, SDL_GetError());
        return nullptr; 
    }

    // wrap raw pointer in a shared_ptr with custom deleter
    auto sharedFont = std::shared_ptr<TTF_Font>(font, TTF_Font_Deleter{});

    // store shared pointer in the cache using the combined key
    m_fontCache[key] = sharedFont;

    // SDL_Log("FontManager: Cache MISS, LOADED font '%s'. Ref count: %ld", key.c_str(), sharedFont.use_count());

    return sharedFont;
}

void FontManager::clearCache() {
    SDL_Log("FontManager: Clearing cache and closing %zu fonts.", m_fontCache.size());
    m_fontCache.clear(); // will automatically call the deleter for each font
}

FontManager::~FontManager() {
    clearCache(); 
}