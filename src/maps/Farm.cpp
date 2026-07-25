// Sunnybrook Farm — a small farmstead west of Harvest Vale: farmhouse, tilled
// crop field, chicken pen and woodlands. Demonstrates the multi-map system;
// copy this file (and register it per Map.h) to add another map.

#include "../Map.h"

#include <cmath>

#include "../Util.h"

std::unique_ptr<Map> buildFarm(const Assets& assets) {
    using namespace spriteRects;

    auto m = std::make_unique<Map>();
    m->id = MapId::Farm;
    m->key = "farm";
    m->name = "Sunnybrook Farm";
    World& world = m->world;
    world.init(60, 45, assets.get("grassSpring"));  // 960x720 px

    /* ---------- base ground ---------- */
    for (int y = 0; y < 45; ++y) {
        for (int x = 0; x < 60; ++x) {
            Uint8 t = G_GRASS;
            if (vnoise(x * 0.12, y * 0.12) > 0.65) t = G_GRASS_B;
            if (h2(x, y) > 0.92) t = G_GRASS_B;
            if (x >= 31 && x <= 43 && y >= 28 && y <= 34) t = G_DIRT;  // tilled field
            world.ground[world.gi(x, y)] = t;
        }
    }

    /* ---------- path: farmhouse -> east edge (to Harvest Vale) ---------- */
    world.carvePath(14, 22, 59, 22);

    /* ---------- border tree wall (east opening to the village) ---------- */
    const World::BorderGap eastGap{1, 22, 23};
    world.addBorder(assets.get("pines"), &eastGap);

    /* ---------- farmhouse + well ---------- */
    {
        const SDL_FRect c{182, 272, 116, 60};
        world.addObject(assets.get("houseHome"), nullptr, 176, 240, 128, 96, &c);
    }
    {
        const SDL_FRect c{372, 336, 24, 18};
        world.addObject(assets.get("well"), &WELL, 368, 296, 32, 64, &c);
    }

    /* ---------- crop field (gate north) and chicken pen (gate west) ---------- */
    world.addFenceRect(assets.get("fence"), 30, 27, 44, 35, 37, 27);
    world.addFenceRect(assets.get("fence"), 19, 28, 25, 33, 19, 30);
    const SDL_FRect pen{19 * TILE, 28 * TILE, 6 * TILE, 5 * TILE};

    // crops planted in the field
    for (int y = 28; y <= 34; ++y) {
        for (int x = 31; x <= 43; ++x) {
            if (h2(x * 7 + 1, y * 3 + 5) < 0.75) {
                const SDL_FPoint cell = CROPS[static_cast<int>(h2(x, y * 5) * 6) % 6];
                world.details.push_back(
                    GroundDetail{assets.get("crops"), cell.x, cell.y, x * TILE, y * TILE});
            }
        }
    }

    /* ---------- torches along the path ---------- */
    const int torchPos[][2] = {{18, 21}, {28, 24}, {38, 21}, {48, 24}, {56, 21}};
    for (const auto& [tx, ty] : torchPos) {
        const SDL_FRect c{tx * TILE + 5.0f, ty * TILE + 10.0f, 6, 6};
        world.addObject(assets.get("props"), &TORCH, tx * TILE, ty * TILE, 16, 16, &c);
    }

    /* ---------- woodlands (south + north-west) ---------- */
    const SDL_FRect* treeRects[] = {&PINE1, &PINE2};
    const auto scatterTrees = [&](int x0, int y0, int x1, int y1, double density) {
        for (int y = y0; y <= y1; ++y) {
            for (int x = x0; x <= x1; ++x) {
                if (x >= 10 && x <= 20 && y >= 13 && y <= 22) continue;  // keep the yard open
                const double r = h2(x * 3 + 7, y * 5 + 13);
                if (r < density) {
                    const double rr = h2(x * 11 + 3, y * 7 + 1);
                    const float px = x * TILE + std::floor(h2(x, y * 9) * 8) - 10.0f;
                    const float py = y * TILE - 26.0f + std::floor(h2(x * 4, y) * 10);
                    if (rr < 0.6) {
                        world.addTree(assets.get("pines"), treeRects[rr < 0.3 ? 0 : 1], px, py,
                                      36, 60);
                    } else {
                        const SDL_FRect* mrect = rr < 0.8 ? &MAPLE_GREEN : &MAPLE_ORANGE;
                        world.addTree(assets.get("maples"), mrect, px, py - 4, 36, 70);
                    }
                }
            }
        }
    };
    scatterTrees(3, 36, 56, 42, 0.16);  // south woods
    scatterTrees(2, 3, 12, 19, 0.22);   // north-west woods

    /* ---------- ground details (flowery meadow) ---------- */
    world.addGroundDetails(assets, 0.05, 0.2);

    /* ---------- entities ---------- */
    m->playerStart = {928, 368};  // just inside the east opening

    m->enemies.push_back(std::make_unique<Slime>(assets, 640, 160, SlimeName::Blue));
    m->enemies.push_back(std::make_unique<Slime>(assets, 720, 220, SlimeName::Blue));
    m->enemies.push_back(std::make_unique<Slime>(assets, 680, 120, SlimeName::Green));
    m->enemies.push_back(std::make_unique<Goblin>(assets, 200, 620));

    m->animals.push_back(std::make_unique<Chicken>(330, 480, assets.get("chickenWhite"), pen));
    m->animals.push_back(std::make_unique<Chicken>(352, 500, assets.get("chickenBrown"), pen));
    m->animals.push_back(std::make_unique<Chicken>(322, 508, assets.get("chickenWhite"), pen));
    m->animals.push_back(std::make_unique<Fox>(500, 640, assets.get("fox")));

    m->props.push_back(std::make_unique<Chest>(assets, 656, 520, false));

    /* ---------- exits ---------- */
    m->exits.push_back({{950, 22 * TILE, 10, 2 * TILE}, MapId::Village, {34, 1072}});

    return m;
}
