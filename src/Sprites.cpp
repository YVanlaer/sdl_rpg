#include "Sprites.h"

#include <cmath>

void drawSprite(SDL_Renderer* r, SDL_Texture* tex, const SDL_FRect* src,
                float x, float y, float w, float h, bool flip) {
    SDL_FRect dst{std::round(x), std::round(y), w, h};
    SDL_RenderTextureRotated(r, tex, src, &dst, 0.0, nullptr,
                             flip ? SDL_FLIP_HORIZONTAL : SDL_FLIP_NONE);
}

void drawSpriteW(SDL_Renderer* r, const Camera& cam, SDL_Texture* tex, const SDL_FRect* src,
                 float x, float y, float w, float h, bool flip) {
    drawSprite(r, tex, src, x - std::round(cam.x), y - std::round(cam.y), w, h, flip);
}

static int dirRow(Direction dir) {
    if (dir == Direction::Up) return 1;
    if (dir == Direction::Down) return 0;
    return 2;  // left; right reuses it flipped
}

void drawCharFrame(SDL_Renderer* r, const Camera& cam, const CharDef& def,
                   Direction dir, int frame, float x, float y, float scale) {
    const int f = SDL_clamp(frame, 0, def.frames - 1);
    const int row = def.fixedRow ? def.rowBase : def.rowBase + dirRow(dir);
    const bool flip = !def.fixedRow && dir == Direction::Right;
    const float w = def.fw * scale;
    const float h = def.fh * scale;
    SDL_FRect src{static_cast<float>(f * def.fw), static_cast<float>(row * def.fh),
                  static_cast<float>(def.fw), static_cast<float>(def.fh)};
    drawSpriteW(r, cam, def.img, &src, x - w / 2, y - h, w, h, flip);
}

void drawShadow(SDL_Renderer* r, const Camera& cam, float x, float y, float w) {
    SDL_SetRenderDrawColor(r, 0, 0, 0, 46);
    fillEllipse(r, std::round(x - cam.x), std::round(y - cam.y) - 1, w, w * 0.35f);
}

void fillEllipse(SDL_Renderer* r, float cx, float cy, float rx, float ry) {
    for (int dy = static_cast<int>(-ry); dy <= static_cast<int>(ry); ++dy) {
        const float t = static_cast<float>(dy) / ry;
        const float halfW = rx * std::sqrt(1.0f - t * t);
        SDL_FRect line{cx - halfW, cy + dy, halfW * 2.0f, 1.0f};
        SDL_RenderFillRect(r, &line);
    }
}
