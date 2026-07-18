#pragma once

#include <SDL3/SDL.h>

#include <string>
#include <unordered_map>

// Texture store for the Farm RPG Tiny Asset Pack, plus the hand-measured
// source rectangles for sprites that live on shared sheets (trees, fences,
// chests, UI, ...). All character/enemy sheets use 32x32 frames with rows
// [down, up, left]; right is left flipped horizontally.
class Assets {
public:
    ~Assets();

    // Loads every texture in the manifest. Returns false on failure and
    // prints the offending path to stderr.
    bool load(SDL_Renderer* renderer);

    // Destroys all textures. Safe to call more than once; textures must be
    // destroyed before the renderer they belong to.
    void unload();

    SDL_Texture* get(const std::string& key) const;

private:
    std::unordered_map<std::string, SDL_Texture*> textures;
};

// Fixed sprite source rects on their sheets (hand-measured).
namespace spriteRects {
extern const SDL_FRect PINE1;
extern const SDL_FRect PINE2;
extern const SDL_FRect MAPLE_GREEN;
extern const SDL_FRect MAPLE_ORANGE;
extern const SDL_FRect MAPLE_BIRCH;
extern const SDL_FRect TREE_BIG_GREEN;
extern const SDL_FRect TREE_PINK;
extern const SDL_FRect TREE_TEAL;

extern const SDL_FRect HOUSE_SMITH;  // first house on the blacksmith sheet

extern const SDL_FRect WELL;
extern const SDL_FRect CHEST_CLOSED;
extern const SDL_FRect CHEST_OPEN;

extern const SDL_FRect FENCE_POST;
extern const SDL_FRect FENCE_H;
extern const SDL_FRect FENCE_CORNER;

extern const SDL_FRect TORCH;

extern const SDL_FRect STALL_TOP;
extern const SDL_FRect STALL_COUNTER;

extern const SDL_FRect HEART_FULL;
extern const SDL_FRect HEART_HALF;
extern const SDL_FRect HEART_EMPTY;
extern const SDL_FRect COIN;

// 16x16 ground-detail cells (top-left corners only).
extern const SDL_FPoint STONES[8];
extern const SDL_FPoint FLOWERS[9];
}  // namespace spriteRects
