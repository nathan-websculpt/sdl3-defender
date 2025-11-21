#pragma once
#include <SDL3_ttf/SDL_ttf.h>
#include <string>
#include <unordered_map>
#include <memory>

struct TTF_Font;

class FontManager {
public:
    // enforce singleton
    FontManager(const FontManager&) = delete;
    FontManager& operator=(const FontManager&) = delete;
    FontManager(FontManager&&) = delete;
    FontManager& operator=(FontManager&&) = delete;


    // get singleton instance
    static FontManager& getInstance();

    std::shared_ptr<TTF_Font> getFont(const std::string& filepath, int size);

    void clearCache();

private:
    FontManager() = default;
    ~FontManager();

    std::unordered_map<std::string, std::shared_ptr<TTF_Font>> m_fontCache;
    bool m_cleanedUp = false;
};

struct TTF_Font_Deleter {
    void operator()(TTF_Font* font) const {
        if (font) {
            TTF_CloseFont(font);
        }
    }
};