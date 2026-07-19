#include "World.h"

#include <cmath>

#include "Autotile.h"
#include "Util.h"

namespace {

// ground types
constexpr Uint8 G_A = 0, G_B = 1, G_C = 2, G_DIRT = 3, G_PLAZA = 4,
                G_FOREST = 5, G_DARK = 6, G_FOREST2 = 7;

/* --- autotile ground metadata ---------------------------------------------
 * The whole ground is drawn from 3 elements of the sheet; no flat colors.
 * Model: the base is ALWAYS grass - every tile first draws the Center piece
 * of the G_A element. Autotiled terrains then draw their piece on top, and
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
    /*G_A*/      {CLS_GRASS, 0, 0, false},  // base grass: Center piece only
    /*G_B*/      {CLS_DEEP,  1, 0, true},   // deeper grass patches
    /*G_C*/      {CLS_GRASS, 0, 0, false},  // looks like base grass
    /*G_DIRT*/   {CLS_SOIL,  0, 2, true},   // soil (paths)
    /*G_PLAZA*/  {CLS_SOIL,  0, 2, true},   // same soil asset as the paths
    /*G_FOREST*/ {CLS_DEEP,  1, 0, true},   // forest floor = deeper grass
    /*G_DARK*/   {CLS_DEEP,  1, 0, true},   // boss clearing = deeper grass
    /*G_FOREST2*/{CLS_DEEP,  1, 0, true},
};

// deterministic hash noise (same formula as the JS version)
double h2(double x, double y) {
    const double s = std::sin(x * 127.1 + y * 311.7) * 43758.5453;
    return s - std::floor(s);
}

// smooth value noise over h2: coherent blobs instead of per-tile static
double vnoise(double x, double y) {
    const int x0 = static_cast<int>(std::floor(x)), y0 = static_cast<int>(std::floor(y));
    double fx = x - x0, fy = y - y0;
    fx = fx * fx * (3 - 2 * fx);
    fy = fy * fy * (3 - 2 * fy);
    const double a = h2(x0, y0), b = h2(x0 + 1, y0), c = h2(x0, y0 + 1), d = h2(x0 + 1, y0 + 1);
    return a + (b - a) * fx + (c - a) * fy + (a - b - c + d) * fx * fy;
}

bool inMap(int x, int y) { return x >= 0 && y >= 0 && x < MAP_W && y < MAP_H; }

}  // namespace

void World::build(const Assets& assets) {
    using namespace spriteRects;

    tiles = assets.get("grassSpring");
    ground.assign(MAP_W * MAP_H, 0);

    const auto inForest = [](int x, int y) { return x >= 54 && x <= 114 && y >= 4 && y <= 82; };
    const auto inBoss = [](int x, int y) { return x >= 90 && x <= 112 && y >= 5 && y <= 22; };
    const auto inPlaza = [](int x, int y) { return x >= 18 && x <= 34 && y >= 62 && y <= 74; };

    /* ---------- base ground ---------- */
    for (int y = 0; y < MAP_H; ++y) {
        for (int x = 0; x < MAP_W; ++x) {
            const double r = h2(x, y);
            Uint8 t = G_A;
            // patches of the second grass tone (tune size/density here)
            if (vnoise(x * 0.12, y * 0.12) > 0.65) t = G_B;
            if (r > 0.92) t = G_B;
            if (inForest(x, y)) t = (x + y) % 2 == 0 ? G_FOREST : G_FOREST2;
            if (inBoss(x, y)) t = G_DARK;
            if (inPlaza(x, y)) t = G_PLAZA;
            ground[gi(x, y)] = t;
        }
    }

    /* ---------- paths (2 tiles wide) ---------- */
    const auto markDirt = [&](int x, int y) {
        for (int dy = 0; dy <= 1; ++dy)
            if (inMap(x, y + dy)) ground[gi(x, y + dy)] = G_DIRT;
    };
    const auto carvePath = [&](int x0, int y0, int x1, int y1) {
        int x = x0, y = y0;
        while (x != x1) { markDirt(x, y); x += (x1 > x) ? 1 : -1; }
        while (y != y1) { markDirt(x, y); y += (y1 > y) ? 1 : -1; }
        markDirt(x1, y1);
    };
    carvePath(30, 66, 74, 66);    // village -> mid forest
    carvePath(74, 40, 74, 66);    // fork north to slime meadow
    carvePath(74, 66, 100, 66);   // main path east
    carvePath(100, 18, 100, 66);  // north to boss clearing
    carvePath(30, 60, 30, 66);    // plaza to main path

    /* ---------- object helpers ---------- */
    const auto addObject = [&](SDL_Texture* img, const SDL_FRect* rect, float px, float py,
                               float w, float h, const SDL_FRect* solid) {
        objects.push_back(WorldObject{img, rect, px, py, w, h, py + h});
        if (solid) colliders.push_back(*solid);
    };
    // solid base at the trunk bottom
    const auto addTreeImg = [&](SDL_Texture* img, const SDL_FRect* rect, float px, float py,
                                float w, float h) {
        const float sw = SDL_max(10.0f, w * 0.28f), sh = 8.0f;
        const SDL_FRect solid{px + w / 2 - sw / 2, py + h - sh - 2, sw, sh};
        addObject(img, rect, px, py, w, h, &solid);
    };
    const auto addTree = [&](const SDL_FRect* rect, float px, float py, float w, float h) {
        addTreeImg(assets.get("pines"), rect, px, py, w, h);
    };

    /* ---------- border tree wall ---------- */
    for (int x = 0; x < MAP_W; x += 2) {
        addTree(&PINE1, x * TILE - 10.0f, 2, 36, 60);
        addTree(&PINE2, x * TILE - 10.0f, (MAP_H - 2) * TILE - 20.0f, 36, 60);
    }
    for (int y = 0; y < MAP_H; y += 2) {
        addTree(&PINE1, -14, y * TILE, 36, 60);
        addTree(&PINE2, (MAP_W - 2) * TILE - 4.0f, y * TILE, 36, 60);
    }
    // hard walls just inside the border
    colliders.push_back({-32, -32, WORLD_W + 64, 56});
    colliders.push_back({-32, WORLD_H - 24, WORLD_W + 64, 56});
    colliders.push_back({-32, -32, 56, WORLD_H + 64});
    colliders.push_back({WORLD_W - 24, -32, 56, WORLD_H + 64});

    /* ---------- village ---------- */
    {
        const SDL_FRect c{150, 880, 116, 60};
        addObject(assets.get("houseHome"), nullptr, 144, 848, 128, 96, &c);
    }
    {
        const SDL_FRect c{406, 876, 116, 68};
        addObject(assets.get("houseCabin"), nullptr, 400, 836, 128, 112, &c);
    }
    {
        const SDL_FRect c{562, 884, 100, 58};
        addObject(assets.get("houseSmith"), &HOUSE_SMITH, 556, 840, 112, 106, &c);
    }
    {
        const SDL_FRect c{372, 1048, 24, 18};
        addObject(assets.get("well"), &WELL, 368, 1008, 32, 64, &c);
    }

    // chicken pen: fence rectangle tiles (9..15, 74..79), gate on east
    const struct { int x0, y0, x1, y1; } pen{9, 74, 15, 79};
    for (int x = pen.x0; x <= pen.x1; ++x) {
        const SDL_FRect* topRect = (x == pen.x0) ? &FENCE_NW : (x == pen.x1) ? &FENCE_NE : &FENCE_H;
        const SDL_FRect topC{x * TILE, pen.y0 * TILE + 6.0f, 16, 10};
        addObject(assets.get("fence"), topRect, x * TILE, pen.y0 * TILE, 16, 16, &topC);

        const SDL_FRect* botRect = (x == pen.x0) ? &FENCE_SW : (x == pen.x1) ? &FENCE_SE : &FENCE_H;
        const SDL_FRect botC{x * TILE, pen.y1 * TILE + 6.0f, 16, 10};
        addObject(assets.get("fence"), botRect, x * TILE, pen.y1 * TILE, 16, 16, &botC);
    }
    for (int y = pen.y0 + 1; y < pen.y1; ++y) {
        for (const int x : {pen.x0, pen.x1}) {
            if (x == pen.x1 && y == 76) continue;  // gate opening
            const SDL_FRect c{x * TILE, y * TILE + 6.0f, 16, 10};
            addObject(assets.get("fence"), &FENCE_V, x * TILE, y * TILE, 16, 16, &c);
        }
    }
    spawns.pen = {pen.x0 * TILE, pen.y0 * TILE, (pen.x1 - pen.x0) * TILE, (pen.y1 - pen.y0) * TILE};

    // torches along the main path
    const int torchPos[][2] = {{32, 64}, {42, 68}, {52, 64}, {62, 68}, {72, 64},
                               {84, 68}, {98, 64}, {100, 40}, {100, 24}, {74, 52}};
    for (const auto& [tx, ty] : torchPos) {
        const SDL_FRect c{tx * TILE + 5.0f, ty * TILE + 10.0f, 6, 6};
        addObject(assets.get("props"), &TORCH, tx * TILE, ty * TILE, 16, 16, &c);
    }

    /* ---------- forest trees ---------- */
    const auto onPath = [&](int x, int y) {
        return inMap(x, y) && ground[gi(x, y)] == G_DIRT;
    };
    const SDL_FRect* treeRects[] = {&PINE1, &PINE2};
    for (int y = 5; y <= 81; ++y) {
        for (int x = 55; x <= 113; ++x) {
            if (onPath(x, y) || onPath(x, y + 1) || onPath(x + 1, y)) continue;
            if (inBoss(x, y)) continue;
            // clearings with fewer trees
            const bool inMeadow = x >= 58 && x <= 78 && y >= 36 && y <= 58;
            const bool inDeep = x >= 88 && x <= 110 && y >= 40 && y <= 62;
            double density = 0.34;
            if (inMeadow) density = 0.10;
            if (inDeep) density = 0.16;
            const double r = h2(x * 3 + 7, y * 5 + 13);
            if (r < density) {
                const double rr = h2(x * 11 + 3, y * 7 + 1);
                const float px = x * TILE + std::floor(h2(x, y * 9) * 8) - 10.0f;
                const float py = y * TILE - 26.0f + std::floor(h2(x * 4, y) * 10);
                if (rr < 0.6) {
                    addTree(treeRects[rr < 0.3 ? 0 : 1], px, py, 36, 60);
                } else {
                    const SDL_FRect* mrect = rr < 0.8 ? &MAPLE_GREEN : &MAPLE_ORANGE;
                    addTreeImg(assets.get("maples"), mrect, px, py - 4, 36, 70);
                }
            }
        }
    }

    /* ---------- ground details (stones, flowers) ---------- */
    for (int y = 1; y < MAP_H - 1; ++y) {
        for (int x = 1; x < MAP_W - 1; ++x) {
            const Uint8 g = ground[gi(x, y)];
            if (g == G_DIRT) continue;
            const double r = h2(x * 13 + 5, y * 17 + 3);
            if (r < 0.05) {
                const double pick = h2(x * 3, y * 7);
                SDL_Texture* img;
                SDL_FPoint cell;
                if (g == G_FOREST || g == G_DARK || pick < 0.45) {
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

    /* ---------- spawn points ---------- */
    spawns.player = {240, 1010};
    spawns.bonfire = {304, 1048};
    spawns.smith = {614, 972};
    spawns.merchant = {480, 1044};
    spawns.chestHome = {184, 986};
    spawns.boss = {1616, 240};
    spawns.chestBoss = {1616, 176};
    spawns.chickens = {{176, 1216}, {200, 1244}, {160, 1252}};
    spawns.foxes = {{1000, 1130}, {1060, 1180}};
    spawns.slimes = {{1020, 700}, {1100, 760}, {1180, 660}, {1120, 850}, {1230, 780}};
    spawns.greens = {{1190, 300}, {1260, 380}, {1130, 420}, {1300, 250}};
    spawns.pinks = {{1520, 760}, {1640, 860}, {1560, 940}};
    spawns.goblins = {{1500, 880}, {1700, 760}, {1620, 990}};
}

void World::drawGround(SDL_Renderer* r, const Camera& cam) const {
    const int x0 = SDL_max(0, static_cast<int>(std::floor(cam.x / TILE)));
    const int y0 = SDL_max(0, static_cast<int>(std::floor(cam.y / TILE)));
    const int x1 = SDL_min(MAP_W - 1, static_cast<int>(std::ceil((cam.x + cam.w) / TILE)));
    const int y1 = SDL_min(MAP_H - 1, static_cast<int>(std::ceil((cam.y + cam.h) / TILE)));
    // same-class test for the autotile mask; outside the map counts as "same"
    // so no borders are drawn along the world edge
    const auto sameClass = [&](int x, int y, Uint8 cls) {
        return !inMap(x, y) || GROUND_META[ground[gi(x, y)]].cls == cls;
    };
    for (int y = y0; y <= y1; ++y) {
        for (int x = x0; x <= x1; ++x) {
            const Uint8 t = ground[gi(x, y)];
            const GroundMeta& m = GROUND_META[t];
            const SDL_FRect dst{x * TILE - std::round(cam.x), y * TILE - std::round(cam.y), TILE, TILE};
            if (!tiles) continue;
            // the base is always grass
            const SDL_FRect bsrc = autotile::srcRect(GROUND_META[G_A].elemCol,
                                                     GROUND_META[G_A].elemRow, autotile::Center);
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
