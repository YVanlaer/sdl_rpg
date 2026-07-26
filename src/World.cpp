#include "World.h"

#include <cmath>

#include "Autotile.h"
#include "Util.h"

namespace {

/* --- autotile ground metadata ---------------------------------------------
 * The whole ground is drawn from 3 elements of the sheet; no flat colors.
 * Model: the base is ALWAYS grass - every tile first draws the Center piece
 * of the G_GRASS element. Autotiled terrains then draw their piece on top, and
 * their transparent fringe blends into that grass base.
 * cls = blending class (same class = no border between tiles).
 * elemCol/elemRow = which 192x64 element of the sheet to use.
 * autotile = false -> the grass base is the whole tile (no overlay). */
enum GroundClass : Uint8 { CLS_GRASS, CLS_DEEP, CLS_SOIL };
struct GroundMeta {
    Uint8 cls;
    Uint8 elemCol, elemRow;
    bool autotile;
};
constexpr GroundMeta GROUND_META[] = {
    /*G_GRASS*/   {CLS_GRASS, 0, 0, false},  // base grass: Center piece only
    /*G_GRASS_B*/ {CLS_DEEP,  1, 0, true},   // deeper grass patches
    /*G_GRASS_C*/ {CLS_GRASS, 0, 0, false},  // looks like base grass
    /*G_DIRT*/    {CLS_SOIL,  0, 2, true},   // soil (paths)
    /*G_PLAZA*/   {CLS_SOIL,  0, 2, true},   // same soil asset as the paths
    /*G_FOREST*/  {CLS_DEEP,  1, 0, true},   // forest floor = deeper grass
    /*G_DARK*/    {CLS_DEEP,  1, 0, true},   // boss clearing = deeper grass
    /*G_FOREST2*/ {CLS_DEEP,  1, 0, true},
};

}  // namespace

namespace treeTypes {

const TreeType PINE1 = makeTreeType("pines", &spriteRects::PINE1, 36, 60);
const TreeType PINE2 = makeTreeType("pines", &spriteRects::PINE2, 36, 60);
const TreeType MAPLE_GREEN = makeTreeType("maples", &spriteRects::MAPLE_GREEN, 36, 60);
const TreeType MAPLE_ORANGE = makeTreeType("maples", &spriteRects::MAPLE_ORANGE, 36, 60);

}  // namespace treeTypes

void World::init(int wTiles, int hTiles, SDL_Texture* tileset) {
    w = wTiles;
    h = hTiles;
    tiles = tileset;
    ground.assign(w * h, 0);
}

void World::addObject(SDL_Texture* img, const SDL_FRect* rect, float px, float py, float w,
                      float h, const SDL_FRect* solid) {
    WorldObject obj{img, rect, px, py, w, h, py + h};
    if (solid) obj.solid = *solid;
    objects.push_back(obj);
}

void World::addTree(const Assets& assets, const TreeType& type, float px, float py) {
    const SDL_FRect worldSolid{type.solid.x + px, type.solid.y + py, type.solid.w, type.solid.h};
    addObject(assets.get(type.texKey), type.rect, px, py, type.w, type.h, &worldSolid);
}

void World::carvePath(int x0, int y0, int x1, int y1) {
    const auto markDirt = [&](int x, int y) {
        for (int dy = 0; dy <= 1; ++dy)
            if (inMap(x, y + dy)) ground[gi(x, y + dy)] = G_DIRT;
    };
    int x = x0, y = y0;
    while (x != x1) { markDirt(x, y); x += (x1 > x) ? 1 : -1; }
    while (y != y1) { markDirt(x, y); y += (y1 > y) ? 1 : -1; }
    markDirt(x1, y1);
}

void World::addBorder(const Assets& assets, const BorderGap* gap) {
    using namespace treeTypes;
    // trees skip two extra rows around the gap so the opening reads clearly
    const auto skip = [&](int side, int t) {
        return gap && gap->side == side && t >= gap->t0 - 2 && t <= gap->t1 + 2;
    };
    for (int x = 0; x < w; x += 2) {
        if (!skip(0, x)) addTree(assets, PINE1, x * TILE - 10.0f, 2);
        if (!skip(2, x)) addTree(assets, PINE2, x * TILE - 10.0f, (h - 2) * TILE - 20.0f);
    }
    for (int y = 0; y < h; y += 2) {
        if (!skip(3, y)) addTree(assets, PINE1, -14, y * TILE);
        if (!skip(1, y)) addTree(assets, PINE2, (w - 2) * TILE - 4.0f, y * TILE);
    }
    // hard walls just inside the border, split around the gap
    const auto addWall = [&](float wx, float wy, float ww, float wh) {
        const SDL_FRect solid{wx, wy, ww, wh};
        addObject(nullptr, nullptr, wx, wy, ww, wh, &solid);
    };
    const float W = pixelW(), H = pixelH();
    if (!gap || gap->side != 0) addWall(-32, -32, W + 64, 56);
    if (!gap || gap->side != 2) addWall(-32, H - 24, W + 64, 56);
    if (!gap || gap->side != 3) addWall(-32, -32, 56, H + 64);
    if (!gap || gap->side != 1) addWall(W - 24, -32, 56, H + 64);
    if (gap) {
        const float g0 = gap->t0 * TILE, g1 = (gap->t1 + 1) * TILE;
        switch (gap->side) {
            case 0:
                addWall(-32, -32, g0 + 32, 56);
                addWall(g1, -32, W + 64 - g1, 56);
                break;
            case 2:
                addWall(-32, H - 24, g0 + 32, 56);
                addWall(g1, H - 24, W + 64 - g1, 56);
                break;
            case 3:
                addWall(-32, -32, 56, g0 + 32);
                addWall(-32, g1, 56, H + 64 - g1);
                break;
            case 1:
                addWall(W - 24, -32, 56, g0 + 32);
                addWall(W - 24, g1, 56, H + 64 - g1);
                break;
            default: break;
        }
    }
}

void World::addFenceRect(SDL_Texture* fence, int x0, int y0, int x1, int y1, int gateX,
                         int gateY) {
    using namespace spriteRects;
    const auto piece = [&](const SDL_FRect* rect, int tx, int ty) {
        const SDL_FRect c{tx * TILE, ty * TILE + 6.0f, TILE, 10};
        addObject(fence, rect, tx * TILE, ty * TILE, TILE, TILE, &c);
    };
    for (int x = x0; x <= x1; ++x) {
        if (!(x == gateX && y0 == gateY))
            piece(x == x0 ? &FENCE_NW : x == x1 ? &FENCE_NE : &FENCE_H, x, y0);
        if (!(x == gateX && y1 == gateY))
            piece(x == x0 ? &FENCE_SW : x == x1 ? &FENCE_SE : &FENCE_H, x, y1);
    }
    for (int y = y0 + 1; y < y1; ++y) {
        for (const int x : {x0, x1}) {
            if (!(x == gateX && y == gateY)) piece(&FENCE_V, x, y);
        }
    }
}

void World::addGroundDetails(const Assets& assets, double chance, double stoneBias) {
    using namespace spriteRects;
    for (int y = 1; y < h - 1; ++y) {
        for (int x = 1; x < w - 1; ++x) {
            const Uint8 g = ground[gi(x, y)];
            if (g == G_DIRT) continue;
            const double r = h2(x * 13 + 5, y * 17 + 3);
            if (r < chance) {
                const double pick = h2(x * 3, y * 7);
                SDL_Texture* img;
                SDL_FPoint cell;
                if (g == G_FOREST || g == G_DARK || pick < stoneBias) {
                    img = assets.get("stones");
                    cell = STONES[static_cast<int>(pick * 2.2 * 8) % 8];
                } else {
                    img = assets.get("props");
                    cell = FLOWERS[static_cast<int>(h2(x, y) * 9) % 9];
                }
                details.push_back(GroundDetail{img, cell.x, cell.y, x * TILE, y * TILE});
            }
        }
    }
}

void World::drawGround(SDL_Renderer* r, const Camera& cam) const {
    const int x0 = SDL_max(0, static_cast<int>(std::floor(cam.x / TILE)));
    const int y0 = SDL_max(0, static_cast<int>(std::floor(cam.y / TILE)));
    const int x1 = SDL_min(w - 1, static_cast<int>(std::ceil((cam.x + cam.w) / TILE)));
    const int y1 = SDL_min(h - 1, static_cast<int>(std::ceil((cam.y + cam.h) / TILE)));
    // same-class test for the autotile mask; outside the map counts as "same"
    // so no borders are drawn along the world edge
    const auto sameClass = [&](int x, int y, Uint8 cls) {
        return !inMap(x, y) || GROUND_META[ground[gi(x, y)]].cls == cls;
    };
    for (int y = y0; y <= y1; ++y) {
        for (int x = x0; x <= x1; ++x) {
            const Uint8 t = ground[gi(x, y)];
            const GroundMeta& m = GROUND_META[t];
            const SDL_FRect dst{x * TILE - std::round(cam.x), y * TILE - std::round(cam.y), TILE,
                                TILE};
            if (!tiles) continue;
            // the base is always grass
            const SDL_FRect bsrc = autotile::srcRect(GROUND_META[G_GRASS].elemCol,
                                                     GROUND_META[G_GRASS].elemRow,
                                                     autotile::Center);
            SDL_RenderTexture(r, tiles, &bsrc, &dst);
            // non-autotiled tiles (base grass) are already drawn by the base
            if (!m.autotile) continue;
            // 8-neighbor mask of same-class tiles
            Uint8 mask = 0;
            if (sameClass(x, y - 1, m.cls)) mask |= autotile::N;
            if (sameClass(x + 1, y, m.cls)) mask |= autotile::E;
            if (sameClass(x, y + 1, m.cls)) mask |= autotile::S;
            if (sameClass(x - 1, y, m.cls)) mask |= autotile::W;
            if (sameClass(x + 1, y - 1, m.cls)) mask |= autotile::NE;
            if (sameClass(x + 1, y + 1, m.cls)) mask |= autotile::SE;
            if (sameClass(x - 1, y + 1, m.cls)) mask |= autotile::SW;
            if (sameClass(x - 1, y - 1, m.cls)) mask |= autotile::NW;
            const SDL_FRect src = autotile::srcRect(m.elemCol, m.elemRow, autotile::caseForMask(mask));
            SDL_RenderTexture(r, tiles, &src, &dst);
        }
    }
    for (const GroundDetail& d : details) {
        if (!inView(cam, d.x, d.y, 16, 16)) continue;
        const SDL_FRect src{d.sx, d.sy, 16, 16};
        const SDL_FRect dst{d.x - std::round(cam.x), d.y - std::round(cam.y), 16, 16};
        SDL_RenderTexture(r, d.img, &src, &dst);
    }
}

void World::drawObject(SDL_Renderer* r, const Camera& cam, const WorldObject& o) const {
    if (!o.img) return;  // invisible collider (e.g. border walls)
    SDL_FRect full{0, 0, 0, 0};
    const SDL_FRect* srcPtr = o.rect;
    if (!srcPtr) {
        // whole-image draw: use the texture's natural size as the source
        SDL_GetTextureSize(o.img, &full.w, &full.h);
        srcPtr = &full;
    }
    const SDL_FRect dst{std::round(o.x - cam.x), std::round(o.y - cam.y), o.w, o.h};
    SDL_RenderTexture(r, o.img, srcPtr, &dst);
}
