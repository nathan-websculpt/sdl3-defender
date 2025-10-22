#pragma once

#include <SDL3_ttf/SDL_ttf.h>
#include <string>
#include <unordered_map>
#include <memory>

// TODO: since copying/moving these managers is explicitly forbidden, the rule-of-five doesn't directly apply to the managers themselves?

struct TTF_Font;

class FontManager {
public:
    // delete copy constructor and assignment operator to enforce singleton
    FontManager(const FontManager&) = delete;
    FontManager& operator=(const FontManager&) = delete;

    // get singleton instance
    static FontManager& getInstance();

    std::shared_ptr<TTF_Font> getFont(const std::string& filepath, int size);

    void clearCache();

private:
    FontManager() = default;
    ~FontManager();

    std::unordered_map<std::string, std::shared_ptr<TTF_Font>> m_fontCache;
};

struct TTF_Font_Deleter {
    void operator()(TTF_Font* font) const {
        if (font) {
            TTF_CloseFont(font);
        }
    }
};