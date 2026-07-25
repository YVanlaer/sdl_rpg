// Harvest Vale — the starting map: village, eastern forest and boss clearing.
// Ported 1:1 from the original single-map World::build()/Game::spawnWorld(),
// plus a west path leading out to Sunnybrook Farm.

#include "../Map.h"

#include <cmath>

#include "../Util.h"

std::unique_ptr<Map> buildVillage(const Assets& assets) {
    using namespace spriteRects;

    auto m = std::make_unique<Map>();
    m->id = MapId::Village;
    m->key = "village";
    m->name = "Harvest Vale";
    World& world = m->world;
    world.init(120, 90, assets.get("grassSpring"));

    const auto inForest = [](int x, int y) { return x >= 54 && x <= 114 && y >= 4 && y <= 82; };
    const auto inBoss = [](int x, int y) { return x >= 90 && x <= 112 && y >= 5 && y <= 22; };
    const auto inPlaza = [](int x, int y) { return x >= 18 && x <= 34 && y >= 62 && y <= 74; };

    /* ---------- base ground ---------- */
    for (int y = 0; y < 90; ++y) {
        for (int x = 0; x < 120; ++x) {
            const double r = h2(x, y);
            Uint8 t = G_GRASS;
            // patches of the second grass tone (tune size/density here)
            if (vnoise(x * 0.12, y * 0.12) > 0.65) t = G_GRASS_B;
            if (r > 0.92) t = G_GRASS_B;
            if (inForest(x, y)) t = (x + y) % 2 == 0 ? G_FOREST : G_FOREST2;
            if (inBoss(x, y)) t = G_DARK;
            if (inPlaza(x, y)) t = G_PLAZA;
            world.ground[world.gi(x, y)] = t;
        }
    }

    /* ---------- paths (2 tiles wide) ---------- */
    world.carvePath(30, 66, 74, 66);   // village -> mid forest
    world.carvePath(74, 40, 74, 66);   // fork north to slime meadow
    world.carvePath(74, 66, 100, 66);  // main path east
    world.carvePath(100, 18, 100, 66); // north to boss clearing
    world.carvePath(30, 60, 30, 66);   // plaza to main path
    world.carvePath(0, 66, 30, 66);    // west exit to Sunnybrook Farm

    /* ---------- border tree wall (west opening to the farm) ---------- */
    const World::BorderGap westGap{3, 66, 67};
    world.addBorder(assets.get("pines"), &westGap);

    /* ---------- village ---------- */
    {
        const SDL_FRect c{150, 880, 116, 60};
        world.addObject(assets.get("houseHome"), nullptr, 144, 848, 128, 96, &c);
    }
    {
        const SDL_FRect c{406, 876, 116, 68};
        world.addObject(assets.get("houseCabin"), nullptr, 400, 836, 128, 112, &c);
    }
    {
        const SDL_FRect c{562, 884, 100, 58};
        world.addObject(assets.get("houseSmith"), &HOUSE_SMITH, 556, 840, 112, 106, &c);
    }
    {
        const SDL_FRect c{372, 1048, 24, 18};
        world.addObject(assets.get("well"), &WELL, 368, 1008, 32, 64, &c);
    }

    // chicken pen: fence rectangle tiles (9..15, 74..79), gate on east
    world.addFenceRect(assets.get("fence"), 9, 74, 15, 79, 15, 76);
    const SDL_FRect pen{9 * TILE, 74 * TILE, 6 * TILE, 5 * TILE};

    // torches along the main path
    const int torchPos[][2] = {{32, 64}, {42, 68}, {52, 64}, {62, 68}, {72, 64}, {84, 68},
                               {98, 64}, {100, 40}, {100, 24}, {74, 52}, {4, 64}};
    for (const auto& [tx, ty] : torchPos) {
        const SDL_FRect c{tx * TILE + 5.0f, ty * TILE + 10.0f, 6, 6};
        world.addObject(assets.get("props"), &TORCH, tx * TILE, ty * TILE, 16, 16, &c);
    }

    /* ---------- forest trees ---------- */
    const auto onPath = [&](int x, int y) {
        return world.inMap(x, y) && world.ground[world.gi(x, y)] == G_DIRT;
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
                    world.addTree(assets.get("pines"), treeRects[rr < 0.3 ? 0 : 1], px, py, 36, 60);
                } else {
                    const SDL_FRect* mrect = rr < 0.8 ? &MAPLE_GREEN : &MAPLE_ORANGE;
                    world.addTree(assets.get("maples"), mrect, px, py - 4, 36, 70);
                }
            }
        }
    }

    /* ---------- ground details (stones, flowers) ---------- */
    world.addGroundDetails(assets);

    /* ---------- entities ---------- */
    m->playerStart = {240, 1010};
    m->hasRespawn = true;
    m->respawn = {304 + 10, 1048 + 20};  // at the bonfire

    for (const SDL_FPoint& p : {SDL_FPoint{1020, 700}, {1100, 760}, {1180, 660}, {1120, 850},
                                {1230, 780}})
        m->enemies.push_back(std::make_unique<Slime>(assets, p.x, p.y, SlimeName::Blue));
    for (const SDL_FPoint& p : {SDL_FPoint{1190, 300}, {1260, 380}, {1130, 420}, {1300, 250}})
        m->enemies.push_back(std::make_unique<Slime>(assets, p.x, p.y, SlimeName::Green));
    for (const SDL_FPoint& p : {SDL_FPoint{1520, 760}, {1640, 860}, {1560, 940}})
        m->enemies.push_back(std::make_unique<Slime>(assets, p.x, p.y, SlimeName::Pink));
    for (const SDL_FPoint& p : {SDL_FPoint{1500, 880}, {1700, 760}, {1620, 990}})
        m->enemies.push_back(std::make_unique<Goblin>(assets, p.x, p.y));
    m->enemies.push_back(std::make_unique<Slime>(assets, 1616, 240, SlimeName::Golden));

    m->npcs.push_back(std::make_unique<NPC>(614, 972, "Bram",
                                            CharDef{assets.get("smithIdle"), 32, 32, 4, 5},
                                            NPC::Kind::Smith));
    m->npcs.push_back(std::make_unique<NPC>(
        480, 1044, "Grub", CharDef{assets.get("merchant"), 16 * 4, 16 * 3, 4, 6, 0, true},
        NPC::Kind::Merchant));

    m->animals.push_back(std::make_unique<Chicken>(176, 1216, assets.get("chickenWhite"), pen));
    m->animals.push_back(std::make_unique<Chicken>(200, 1244, assets.get("chickenBrown"), pen));
    m->animals.push_back(std::make_unique<Chicken>(160, 1252, assets.get("chickenWhite"), pen));
    for (const SDL_FPoint& p : {SDL_FPoint{1000, 1130}, {1060, 1180}})
        m->animals.push_back(std::make_unique<Fox>(p.x, p.y, assets.get("fox")));

    m->props.push_back(std::make_unique<Bonfire>(assets, 304, 1048));
    m->props.push_back(std::make_unique<Chest>(assets, 184, 986, false));
    m->props.push_back(std::make_unique<Chest>(assets, 1616, 176, true));

    /* ---------- exits ---------- */
    m->exits.push_back({{0, 66 * TILE, 10, 2 * TILE}, MapId::Farm, {928, 368}});

    return m;
}
