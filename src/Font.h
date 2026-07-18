#pragma once

#include <SDL3/SDL.h>

#include <string>

// Monospace 8x8 bitmap text renderer, matching the 8px monospace text of the
// original JS game. Glyphs are pre-rendered into a texture atlas at startup
// and tinted per draw call.
//
// Two custom glyphs replace unused control codes:
//   '\x01' = heart (used for healing float texts)
//   '\x02' = down arrow (dialogue "continue" prompt)
class Font {
public:
    enum Align { Left, Center, Right };

    ~Font() { shutdown(); }

    bool init(SDL_Renderer* renderer);

    void shutdown() {
        if (atlas) {
            SDL_DestroyTexture(atlas);
            atlas = nullptr;
        }
    }

    void draw(SDL_Renderer* r, const std::string& text, float x, float y,
              SDL_Color color, float scale = 1.0f, Align align = Left,
              Uint8 alpha = 255) const;

    // Pixel width of one line at the given scale.
    int width(const std::string& text, float scale = 1.0f) const {
        return static_cast<int>(text.size() * 8 * scale);
    }

private:
    SDL_Texture* atlas = nullptr;  // 16x8 grid of 8x8 glyphs
};
