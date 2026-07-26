#pragma once

#include <SDL3/SDL.h>

#include <optional>
#include <vector>

#include "Assets.h"
#include "Camera.h"

constexpr float TILE = 16.0f;

// Ground tile types (they index the autotile metadata table in World.cpp).
constexpr Uint8 G_GRASS = 0;    // base grass
constexpr Uint8 G_GRASS_B = 1;  // deeper grass patches
constexpr Uint8 G_GRASS_C = 2;  // looks like base grass
constexpr Uint8 G_DIRT = 3;     // soil: paths, tilled fields
constexpr Uint8 G_PLAZA = 4;    // plaza soil (same asset as dirt)
constexpr Uint8 G_FOREST = 5;   // forest floor (deeper grass)
constexpr Uint8 G_DARK = 6;     // boss clearing (deeper grass)
constexpr Uint8 G_FOREST2 = 7;  // forest floor variant

// A y-sorted static sprite (tree, house, fence, ...) in the world.
// If img is nullptr the object is an invisible collider (e.g. border walls).
struct WorldObject {
    SDL_Texture* img;
    const SDL_FRect* rect;  // nullptr = whole image
    float x, y, w, h;
    float sortY;
    std::optional<SDL_FRect> solid;  // world-space collider, defined at object level
};

// A reusable tree definition: all visual/collision characteristics are declared
// once, then placed many times in maps. The solid rectangle is relative to the
// sprite's top-left corner and is translated to world space when placed.
struct TreeType {
    const char* texKey;
    const SDL_FRect* rect;
    float w, h;
    SDL_FRect solid;
};

inline TreeType makeTreeType(const char* texKey, const SDL_FRect* rect, float w, float h) {
    const float sw = SDL_max(10.0f, w * 0.28f), sh = 8.0f;
    return TreeType{texKey, rect, w, h, {w / 2 - sw / 2, h - sh - 2, sw, sh}};
}

namespace treeTypes {
extern const TreeType PINE1;
extern const TreeType PINE2;
extern const TreeType MAPLE_GREEN;
extern const TreeType MAPLE_ORANGE;
}  // namespace treeTypes

// Small 16px ground decoration (stone, flower, crop).
struct GroundDetail {
    SDL_Texture* img;
    float sx, sy;
    float x, y;
};

// The static geometry of one map: ground tiles, decoration and y-sorted objects.
// Each object may carry its own world-space solid rectangle. Dimensions are
// per-map. A map builder (see src/maps/) fills this in via the construction
// helpers below; generation is deterministic (same h2/vnoise formulas as the JS version).
class World {
public:
    // Sizes the grid and sets the ground autotile sheet.
    void init(int wTiles, int hTiles, SDL_Texture* tileset);

    int gi(int x, int y) const { return y * w + x; }
    bool inMap(int x, int y) const { return x >= 0 && y >= 0 && x < w && y < h; }
    float pixelW() const { return w * TILE; }
    float pixelH() const { return h * TILE; }

    /* ---- construction helpers for map builders ---- */

    // Static sprite; solid (optional) is its world-space collider.
    void addObject(SDL_Texture* img, const SDL_FRect* rect, float px, float py, float w, float h,
                   const SDL_FRect* solid = nullptr);
    // Place a tree defined by its type. The map only chooses the position.
    void addTree(const Assets& assets, const TreeType& type, float px, float py);
    // Dirt path, 2 tiles tall, carved in straight L-shaped segments.
    void carvePath(int x0, int y0, int x1, int y1);

    // Opening in the border: side 0=N 1=E 2=S 3=W, tile range [t0, t1].
    struct BorderGap {
        int side, t0, t1;
    };
    // Ring of border pines plus invisible hard wall objects just inside the edge.
    void addBorder(const Assets& assets, const BorderGap* gap = nullptr);

    // Fenced rectangle of tiles; the (gateX, gateY) tile is left open.
    void addFenceRect(SDL_Texture* fence, int x0, int y0, int x1, int y1, int gateX = -1,
                      int gateY = -1);

    // Stones/flowers sprinkled over the ground, skipping dirt (deterministic).
    void addGroundDetails(const Assets& assets, double chance = 0.05, double stoneBias = 0.45);

    void drawGround(SDL_Renderer* r, const Camera& cam) const;
    void drawObject(SDL_Renderer* r, const Camera& cam, const WorldObject& o) const;

    bool inView(const Camera& cam, float x, float y, float w, float h) const {
        return x + w >= cam.x && x <= cam.x + cam.w && y + h >= cam.y && y <= cam.y + cam.h;
    }

    std::vector<Uint8> ground;
    SDL_Texture* tiles = nullptr;  // ground autotile sheet, set by init()
    int w = 0, h = 0;              // size in tiles
    std::vector<GroundDetail> details;
    std::vector<WorldObject> objects;
};
