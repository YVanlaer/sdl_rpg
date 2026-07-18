#pragma once

#include <SDL3/SDL.h>

#include <cmath>
#include <random>

// Small shared helpers used across the game.

inline float clampf(float v, float lo, float hi) {
    return v < lo ? lo : (v > hi ? hi : v);
}

inline float distf(float ax, float ay, float bx, float by) {
    return std::hypot(bx - ax, by - ay);
}

inline bool rectsOverlap(const SDL_FRect& a, const SDL_FRect& b) {
    return a.x < b.x + b.w && a.x + a.w > b.x && a.y < b.y + b.h && a.y + a.h > b.y;
}

// 0xRRGGBB -> SDL_Color (opaque)
constexpr SDL_Color hexColor(unsigned int c) {
    return SDL_Color{static_cast<Uint8>(c >> 16), static_cast<Uint8>((c >> 8) & 0xFF),
                     static_cast<Uint8>(c & 0xFF), 255};
}

inline std::mt19937& rng() {
    static std::mt19937 gen(std::random_device{}());
    return gen;
}

// Random float in [0, 1)
inline float randf() {
    static std::uniform_real_distribution<float> d(0.0f, 1.0f);
    return d(rng());
}

// Random float in [lo, hi)
inline float randRange(float lo, float hi) {
    return lo + randf() * (hi - lo);
}

// Random int in [lo, hi]
inline int randInt(int lo, int hi) {
    return lo + static_cast<int>(std::floor(randf() * (hi - lo + 1)));
}
