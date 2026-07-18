#pragma once

#include <SDL3/SDL.h>

#include <vector>

#include "Assets.h"
#include "Camera.h"

constexpr float TILE = 16.0f;
constexpr int MAP_W = 120;
constexpr int MAP_H = 90;
constexpr float WORLD_W = MAP_W * TILE;   // 1920
constexpr float WORLD_H = MAP_H * TILE;   // 1440

// A y-sorted static sprite (tree, house, fence, ...) in the world.
struct WorldObject {
    SDL_Texture* img;
    const SDL_FRect* rect;  // nullptr = whole image
    float x, y, w, h;
    float sortY;
};

// Small 16px ground decoration (stone, flower).
struct GroundDetail {
    SDL_Texture* img;
    float sx, sy;
    float x, y;
};

struct Spawns {
    SDL_FPoint player, bonfire, smith, merchant, chestHome, boss, chestBoss;
    SDL_FRect pen;
    std::vector<SDL_FPoint> slimes, greens, pinks, goblins, chickens, foxes;
};

// The static world: ground tiles, decoration, y-sorted objects and colliders.
// Generated deterministically at startup (same layout as the JS version).
class World {
public:
    void build(const Assets& assets);

    void drawGround(SDL_Renderer* r, const Camera& cam) const;
    void drawObject(SDL_Renderer* r, const Camera& cam, const WorldObject& o) const;

    bool inView(const Camera& cam, float x, float y, float w, float h) const {
        return x + w >= cam.x && x <= cam.x + cam.w && y + h >= cam.y && y <= cam.y + cam.h;
    }

    std::vector<Uint8> ground;
    std::vector<GroundDetail> details;
    std::vector<WorldObject> objects;
    std::vector<SDL_FRect> colliders;
    Spawns spawns;

    static int gi(int x, int y) { return y * MAP_W + x; }
};
