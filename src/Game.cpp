#include "Game.h"

#include <SDL3_image/SDL_image.h>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <iostream>

#include "Util.h"

namespace {

// UI colors (same palette as the JS version)
const SDL_Color PANEL_FILL = hexColor(0xf7e3c3);
const SDL_Color PANEL_BORDER = hexColor(0x8a4b2d);
const SDL_Color PANEL_INNER = hexColor(0x5a3d2b);
const SDL_Color TEXT_DARK = hexColor(0x3b2a2a);
const SDL_Color TEXT_SHADOW = hexColor(0x2b1d26);
const SDL_Color TEXT_LIGHT = hexColor(0xfff8e8);
const SDL_Color GOLD = hexColor(0xffd94a);
const SDL_Color GREEN = hexColor(0x7dff8a);
const SDL_Color PINK = hexColor(0xff6b81);
const SDL_Color RED = hexColor(0xe0443a);
const SDL_Color XP_BLUE = hexColor(0x6aa8ff);
const SDL_Color BOSS_GOLD = hexColor(0xd4a017);

void setColor(SDL_Renderer* r, SDL_Color c, Uint8 a = 255) {
    SDL_SetRenderDrawColor(r, c.r, c.g, c.b, a);
}

void fillRect(SDL_Renderer* r, float x, float y, float w, float h, SDL_Color c, Uint8 a = 255) {
    setColor(r, c, a);
    SDL_FRect rect{x, y, w, h};
    SDL_RenderFillRect(r, &rect);
}

// Framed parchment panel used for dialogue and the shop.
void drawPanel(SDL_Renderer* r, float x, float y, float w, float h) {
    fillRect(r, x, y, w, h, PANEL_FILL);
    setColor(r, PANEL_BORDER);
    fillRect(r, x + 1, y + 1, w - 2, 2, PANEL_BORDER);
    fillRect(r, x + 1, y + h - 3, w - 2, 2, PANEL_BORDER);
    fillRect(r, x + 1, y + 1, 2, h - 2, PANEL_BORDER);
    fillRect(r, x + w - 3, y + 1, 2, h - 2, PANEL_BORDER);
    SDL_FRect inner{x + 3.5f, y + 3.5f, w - 7, h - 7};
    setColor(r, PANEL_INNER);
    SDL_RenderRect(r, &inner);
}

void textShadow(SDL_Renderer* r, const Font& font, const std::string& text, float x, float y,
                SDL_Color color, Font::Align align = Font::Left, float scale = 1.0f) {
    font.draw(r, text, x + 1, y + 1, TEXT_SHADOW, scale, align);
    font.draw(r, text, x, y, color, scale, align);
}

}  // namespace

/* ============================= Dialogue ============================= */

void Dialogue::update(float dt, bool confirmPressed, Game& game) {
    t += dt;
    const std::string& line = lines[lineIndex];
    if (chars < static_cast<float>(line.size())) {
        chars = SDL_min(static_cast<float>(line.size()), chars + dt * 45.0f);
        if (confirmPressed) chars = static_cast<float>(line.size());
    } else if (confirmPressed) {
        lineIndex++;
        chars = 0.0f;
        if (lineIndex >= static_cast<int>(lines.size())) {
            game.state = GameState::Play;
            if (onEnd) onEnd();
        }
    }
}

void Dialogue::draw(SDL_Renderer* r, const Font& font) const {
    const float x = 8, y = VIEW_H - 62, w = VIEW_W - 16, h = 54;
    drawPanel(r, x, y, w, h);
    float tx = x + 8;
    if (portrait) {
        const SDL_FRect src{0, 0, 64, 64};  // first cell of the portrait sheet
        const SDL_FRect dst{x + 6, y + 6, 42, 42};
        SDL_RenderTexture(r, portrait, &src, &dst);
        tx = x + 54;
    }
    font.draw(r, name, tx, y + 4, PANEL_BORDER);
    // word wrap over max two lines
    const std::string text = lines[lineIndex].substr(0, static_cast<size_t>(chars));
    const float maxW = x + w - 10 - tx;
    std::string line;
    float ly = y + 16;
    size_t start = 0;
    while (start <= text.size()) {
        const size_t space = text.find(' ', start);
        const std::string word = text.substr(start, space - start);
        const std::string test = line.empty() ? word : line + " " + word;
        if (font.width(test) > maxW && !line.empty()) {
            font.draw(r, line, tx, ly, TEXT_DARK);
            ly += 11;
            line = word;
        } else {
            line = test;
        }
        if (space == std::string::npos) break;
        start = space + 1;
    }
    font.draw(r, line, tx, ly, TEXT_DARK);
    if (chars >= static_cast<float>(lines[lineIndex].size()) &&
        static_cast<int>(t * 3) % 2 == 0) {
        font.draw(r, "\x02", x + w - 16, y + h - 14, PANEL_BORDER);
    }
}

/* ============================= Game ============================= */

Game::~Game() {
    // Member SDL resources must be released before the renderer/SDL itself;
    // members would otherwise be destroyed after SDL_Quit().
    audio.shutdown();
    assets.unload();
    font.shutdown();
    if (renderer) SDL_DestroyRenderer(renderer);
    if (window) SDL_DestroyWindow(window);
    SDL_Quit();
}

bool Game::init() {
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO)) {
        std::cerr << "SDL_Init failed: " << SDL_GetError() << std::endl;
        return false;
    }
    window = SDL_CreateWindow("Harvest Vale", VIEW_W * 3, VIEW_H * 3, SDL_WINDOW_RESIZABLE);
    if (!window) {
        std::cerr << "SDL_CreateWindow failed: " << SDL_GetError() << std::endl;
        return false;
    }
    renderer = SDL_CreateRenderer(window, nullptr);
    if (!renderer) {
        std::cerr << "SDL_CreateRenderer failed: " << SDL_GetError() << std::endl;
        return false;
    }
    SDL_SetRenderVSync(renderer, 1);
    SDL_SetRenderLogicalPresentation(renderer, VIEW_W, VIEW_H,
                                     SDL_LOGICAL_PRESENTATION_INTEGER_SCALE);
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

    if (!assets.load(renderer)) return false;
    if (!font.init(renderer)) {
        std::cerr << "Font init failed: " << SDL_GetError() << std::endl;
        return false;
    }
    if (!audio.init()) {
        std::cerr << "Audio init failed (continuing silent): " << SDL_GetError() << std::endl;
    }

    buildMaps();
    map = maps[static_cast<size_t>(MapId::Village)].get();
    player = std::make_unique<Player>(assets, map->playerStart.x, map->playerStart.y);
    player->gold = 5;
    refreshInteractables();
    boss = findBoss();
    snapCamera();
    applyDebugEnv();
    running = true;
    return true;
}

void Game::run() {
    Uint64 last = SDL_GetPerformanceCounter();
    while (running) {
        const Uint64 now = SDL_GetPerformanceCounter();
        const float dt =
            SDL_min(0.05f, (now - last) / static_cast<float>(SDL_GetPerformanceFrequency()));
        last = now;

        processEvents();
        update(dt);
        render();
        pressed.clear();
    }
}

void Game::processEvents() {
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
        switch (e.type) {
            case SDL_EVENT_QUIT:
                running = false;
                break;
            case SDL_EVENT_KEY_DOWN:
                if (!e.key.repeat) pressed.insert(e.key.scancode);
                break;
            default:
                break;
        }
    }
}

Game::Input Game::makeInput() const {
    Input inp;
    const bool* keys = SDL_GetKeyboardState(nullptr);
    inp.up = keys[SDL_SCANCODE_W] || keys[SDL_SCANCODE_UP];
    inp.down = keys[SDL_SCANCODE_S] || keys[SDL_SCANCODE_DOWN];
    inp.left = keys[SDL_SCANCODE_A] || keys[SDL_SCANCODE_LEFT];
    inp.right = keys[SDL_SCANCODE_D] || keys[SDL_SCANCODE_RIGHT];
    inp.run = keys[SDL_SCANCODE_LSHIFT] || keys[SDL_SCANCODE_RSHIFT];

    const auto hit = [&](SDL_Scancode k) { return pressed.count(k) > 0; };
    inp.attackPressed = hit(SDL_SCANCODE_SPACE) || hit(SDL_SCANCODE_J) || hit(SDL_SCANCODE_Z);
    inp.interactPressed = hit(SDL_SCANCODE_E) || hit(SDL_SCANCODE_X);
    inp.confirmPressed =
        hit(SDL_SCANCODE_RETURN) || hit(SDL_SCANCODE_SPACE) || hit(SDL_SCANCODE_E);
    inp.potionPressed = hit(SDL_SCANCODE_Q);
    inp.mutePressed = hit(SDL_SCANCODE_M);
    inp.cancelPressed = hit(SDL_SCANCODE_ESCAPE);
    inp.num = hit(SDL_SCANCODE_1) ? 1 : hit(SDL_SCANCODE_2) ? 2 : hit(SDL_SCANCODE_3) ? 3 : 0;
    return inp;
}

void Game::buildMaps() {
    // Register every map here, in the same order as the MapId enum (Map.h).
    maps.push_back(buildVillage(assets));
    maps.push_back(buildFarm(assets));
}

// Moves the player to another map, placing them at `entry`. The old map keeps
// its entities as they were; the new map resumes from its own state.
void Game::switchMap(MapId id, SDL_FPoint entry) {
    map = maps[static_cast<size_t>(id)].get();
    player->x = entry.x;
    player->y = entry.y;
    player->kbx = player->kby = 0.0f;
    texts.clear();
    refreshInteractables();
    boss = findBoss();
    mapCooldown = 0.6f;
    snapCamera();
    addFloat(player->x, player->y - 40, map->name, GOLD);
}

void Game::refreshInteractables() {
    interactables.clear();
    for (const auto& n : map->npcs) interactables.push_back(n.get());
    for (const auto& p : map->props) {
        if (dynamic_cast<Chest*>(p.get())) interactables.push_back(p.get());
    }
}

// The boss-bar target, if the active map holds a (living) boss.
Slime* Game::findBoss() {
    for (const auto& e : map->enemies)
        if (e->type == EnemyType::Boss) return static_cast<Slime*>(e.get());
    return nullptr;
}

void Game::interact() {
    Entity* best = nullptr;
    float bd = 28.0f;
    for (Entity* it : interactables) {
        const float d = distf(player->x, player->y, it->x, it->y);
        if (d < bd) {
            bd = d;
            best = it;
        }
    }
    if (best) best->interact(*this);
}

void Game::meleeHit(Player& attacker) {
    float dvx = 0, dvy = 0;
    switch (attacker.dir) {
        case Direction::Down: dvy = 1; break;
        case Direction::Up: dvy = -1; break;
        case Direction::Left: dvx = -1; break;
        case Direction::Right: dvx = 1; break;
    }
    bool hitAny = false;
    for (const auto& e : map->enemies) {
        if (e->isDead()) continue;
        const float dx = e->x - attacker.x, dy = e->y - attacker.y;
        const float d = std::hypot(dx, dy);
        if (d > 30) continue;
        const float dot = d == 0 ? 1.0f : (dx * dvx + dy * dvy) / d;
        if (dot > 0.25f) {
            e->hurt(*this, attacker.dmg, attacker.x, attacker.y);
            hitAny = true;
        }
    }
    if (hitAny) shake = SDL_max(shake, 0.08f);
}

void Game::onEnemyKilled(Entity& e) {
    // drops
    int g0, g1, xp;
    if (e.type == EnemyType::Goblin) {
        Goblin& g = static_cast<Goblin&>(e);
        g0 = 3;
        g1 = 6;
        xp = g.xp;
    } else {
        Slime& s = static_cast<Slime&>(e);
        g0 = s.k.gold0;
        g1 = s.k.gold1;
        xp = s.k.xp;
    }
    const int total = randInt(g0, g1);
    const int coins = SDL_min(3, total);
    for (int i = 0; i < coins; ++i) {
        const float a = randf() * 6.28318f;
        map->pickups.push_back(std::make_unique<Pickup>(
            assets, e.x + std::cos(a) * 8, e.y - 4 + std::sin(a) * 6, Pickup::Type::Coin,
            SDL_max(1, static_cast<int>(std::lround(static_cast<float>(total) / coins)))));
    }
    if (randf() < 0.18f)
        map->pickups.push_back(
            std::make_unique<Pickup>(assets, e.x, e.y - 8, Pickup::Type::Heart, 2));
    player->addXP(*this, xp);

    Quest& q = quests.slime;
    if (e.type == EnemyType::Slime && q.state == Quest::State::Active) {
        q.count = SDL_min(q.target, q.count + 1);
        if (q.count >= q.target) {
            q.state = Quest::State::Ready;
            audio.play("quest");
            addFloat(player->x, player->y - 40, "Quest done! See Bram", GREEN);
        }
    }
    if (e.type == EnemyType::Boss) {
        bossDown = true;
        boss = nullptr;
        quests.boss.state = Quest::State::Done;
        audio.play("win");
        state = GameState::Win;
        winT = 0.0f;
    }
}

void Game::onPlayerDeath() {
    audio.play("death");
    deathTimer = 1.2f;
}

void Game::startDialogue(std::unique_ptr<Dialogue> d) {
    dialogue = std::move(d);
    state = GameState::Dialog;
}

void Game::talkToSmith() {
    SDL_Texture* portrait = assets.get("smithPortrait");
    if (quests.intro.state == Quest::State::Active) {
        quests.intro.state = Quest::State::Done;
        quests.slime.state = Quest::State::Active;
        audio.play("quest");
        startDialogue(std::make_unique<Dialogue>(
            "Bram the Blacksmith",
            std::vector<std::string>{
                "Well met, traveler! Welcome to Harvest Vale... what's left of it.",
                "Slimes have overrun the forest east of the village. We cannot chop wood or "
                "forage safely anymore.",
                "You look handy with that sword. Clear out 5 slimes for us, and I will pay you "
                "fairly.",
            },
            portrait, [this] { audio.play("quest"); }));
    } else if (quests.slime.state == Quest::State::Active) {
        startDialogue(std::make_unique<Dialogue>(
            "Bram the Blacksmith",
            std::vector<std::string>{
                "The forest still crawls with slimes. That is " +
                    std::to_string(quests.slime.count) + " of " +
                    std::to_string(quests.slime.target) + " down - keep at it!",
                "Follow the path east from the village. The blue ones are the weakest.",
            },
            portrait));
    } else if (quests.slime.state == Quest::State::Ready) {
        quests.slime.state = Quest::State::Done;
        quests.boss.state = Quest::State::Active;
        player->gold += 20;
        audio.play("quest");
        startDialogue(std::make_unique<Dialogue>(
            "Bram the Blacksmith",
            std::vector<std::string>{
                "Ha! Fine work! Here is your pay - 20 gold, as promised.",
                "But listen... the slimes have a king. A huge GOLDEN slime laired in the "
                "clearing north-east, past the deep woods.",
                "Strike it down and the Vale is free. The beasts guard a treasure chest up "
                "there, too.",
            },
            portrait));
    } else if (quests.boss.state == Quest::State::Active) {
        startDialogue(std::make_unique<Dialogue>(
            "Bram the Blacksmith",
            std::vector<std::string>{
                "The golden slime still lives. Take the east path, then head north at the "
                "torch-lined fork.",
                "It hits hard - bring potions. Grub the merchant sells them at the plaza stall.",
            },
            portrait));
    } else if (bossDown) {
        startDialogue(std::make_unique<Dialogue>(
            "Bram the Blacksmith",
            std::vector<std::string>{
                "You actually did it. The golden menace is gone - the Vale owes you everything, "
                "hero.",
                "Stay as long as you like. The forest is ours again!",
            },
            portrait));
    }
}

void Game::openShop() {
    state = GameState::Shop;
}

void Game::shopBuy(int n) {
    if (n == 1) {  // potion, 15g
        if (player->gold >= 15) {
            player->gold -= 15;
            player->potions++;
            audio.play("coin");
            addFloat(player->x, player->y - 30, "+1 potion", GREEN);
        } else {
            audio.play("deny");
        }
    } else if (n == 2) {  // heart container, 50g
        if (shopBought) {
            audio.play("deny");
            return;
        }
        if (player->gold >= 50) {
            player->gold -= 50;
            player->maxHp += 2;
            player->heal(player->maxHp);
            shopBought = true;
            audio.play("levelup");
            addFloat(player->x, player->y - 30, "+1 heart!", PINK);
        } else {
            audio.play("deny");
        }
    } else if (n == 3) {
        state = GameState::Play;
    }
}

void Game::addFloat(float x, float y, std::string text, SDL_Color color) {
    texts.emplace_back(x, y, std::move(text), color);
}

const Quest* Game::currentQuest() const {
    if (quests.intro.state == Quest::State::Active) return &quests.intro;
    if (quests.slime.state == Quest::State::Active ||
        quests.slime.state == Quest::State::Ready)
        return &quests.slime;
    if (quests.boss.state == Quest::State::Active) return &quests.boss;
    return nullptr;
}

void Game::snapCamera() {
    cam.x = clampf(player->x - VIEW_W / 2.0f, 0.0f, world().pixelW() - VIEW_W);
    cam.y = clampf(player->y - VIEW_H / 2.0f, 0.0f, world().pixelH() - VIEW_H);
}

void Game::update(float dt) {
    time += dt;
    input = makeInput();
    if (input.mutePressed) audio.toggleMute();

    switch (state) {
        case GameState::Title:
            if (input.confirmPressed) state = GameState::Play;
            return;
        case GameState::Dialog:
            dialogue->update(dt, input.confirmPressed, *this);
            return;
        case GameState::Shop:
            if (input.num) shopBuy(input.num);
            if (input.cancelPressed || input.interactPressed) state = GameState::Play;
            return;
        case GameState::Dead:
            if (input.confirmPressed) {
                // wake up at the map that holds the respawn point (the bonfire)
                Map* home = map;
                for (const auto& m : maps)
                    if (m->hasRespawn) {
                        home = m.get();
                        break;
                    }
                switchMap(home->id, home->respawn);
                player->hp = player->maxHp;
                player->state = Player::State::Idle;
                player->invuln = 1.5f;
                player->gold = static_cast<int>(player->gold * 0.85f);
                deathTimer = -1.0f;
                state = GameState::Play;
            }
            return;
        case GameState::Win:
            winT += dt;
            if (winT > 1.0f && input.confirmPressed) state = GameState::Play;
            break;  // the world stays alive behind the banner
        case GameState::Play:
            break;
    }

    // play (& win banner overlays a live world)
    player->update(dt, *this);

    if (state == GameState::Play) {
        if (input.interactPressed) interact();
        if (input.potionPressed && player->potions > 0 && player->hp < player->maxHp &&
            !player->isDead()) {
            player->potions--;
            player->heal(4);
            audio.play("potion");
            addFloat(player->x, player->y - 30, "+2 \x01", PINK);
        }
    }

    // map exits: walking into a trigger area teleports to another map
    mapCooldown = SDL_max(0.0f, mapCooldown - dt);
    if (state == GameState::Play && mapCooldown <= 0.0f && !player->isDead()) {
        const SDL_FRect pb = player->box();
        for (const MapExit& ex : map->exits) {
            if (rectsOverlap(pb, ex.area)) {
                switchMap(ex.target, ex.entry);
                break;
            }
        }
    }

    for (const auto& e : map->enemies) e->update(dt, *this);
    for (const auto& n : map->npcs) n->update(dt, *this);
    for (const auto& a : map->animals) a->update(dt, *this);
    for (const auto& p : map->props) p->update(dt, *this);
    for (const auto& pk : map->pickups) pk->update(dt, *this);
    for (FloatText& t : texts) t.update(dt);

    // enemy separation (soft)
    for (size_t i = 0; i < map->enemies.size(); ++i) {
        for (size_t j = i + 1; j < map->enemies.size(); ++j) {
            Entity& a = *map->enemies[i];
            Entity& b = *map->enemies[j];
            if (a.isDead() || b.isDead()) continue;
            const float dx = b.x - a.x, dy = b.y - a.y;
            const float d = std::hypot(dx, dy);
            const float minD = 14.0f;
            if (d > 0.01f && d < minD) {
                const float push = (minD - d) / 2;
                const float nx = dx / d, ny = dy / d;
                a.x -= nx * push;
                a.y -= ny * push;
                b.x += nx * push;
                b.y += ny * push;
            }
        }
    }

    const auto isRemoved = [](const auto& e) { return e->remove; };
    map->enemies.erase(std::remove_if(map->enemies.begin(), map->enemies.end(), isRemoved),
                       map->enemies.end());
    map->pickups.erase(std::remove_if(map->pickups.begin(), map->pickups.end(), isRemoved),
                       map->pickups.end());
    texts.erase(
        std::remove_if(texts.begin(), texts.end(), [](const FloatText& t) { return t.remove; }),
        texts.end());

    if (deathTimer > 0.0f) {
        deathTimer -= dt;
        if (deathTimer <= 0.0f && player->isDead()) state = GameState::Dead;
    }

    // camera
    shake = SDL_max(0.0f, shake - dt);
    snapCamera();
    if (shake > 0.0f) {
        cam.x += (randf() - 0.5f) * 3;
        cam.y += (randf() - 0.5f) * 3;
    }
}

/* ============================= render ============================= */

void Game::drawWorld() {
    world().drawGround(renderer, cam);

    // y-sorted: world objects + entities
    struct DrawItem {
        float sortY;
        const WorldObject* obj;
        Entity* ent;
    };
    std::vector<DrawItem> drawList;
    for (const WorldObject& o : world().objects) {
        if (world().inView(cam, o.x, o.y, o.w, o.h)) drawList.push_back({o.sortY, &o, nullptr});
    }
    const auto addEnts = [&](std::vector<std::unique_ptr<Entity>>& list) {
        for (const auto& e : list) drawList.push_back({e->y, nullptr, e.get()});
    };
    addEnts(map->props);
    addEnts(map->animals);
    addEnts(map->npcs);
    addEnts(map->enemies);
    drawList.push_back({player->y, nullptr, player.get()});
    std::sort(drawList.begin(), drawList.end(),
              [](const DrawItem& a, const DrawItem& b) { return a.sortY < b.sortY; });

    for (const DrawItem& item : drawList) {
        if (item.obj) world().drawObject(renderer, cam, *item.obj);
        else item.ent->draw(renderer, *this);
    }

    for (const auto& pk : map->pickups) pk->draw(renderer, *this);
    for (const FloatText& t : texts) t.draw(renderer, *this);
}

void Game::render() {
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);

    drawWorld();
    if (state != GameState::Title) drawHUD();
    switch (state) {
        case GameState::Title: drawTitle(); break;
        case GameState::Dialog: dialogue->draw(renderer, font); break;
        case GameState::Shop: drawShop(); break;
        case GameState::Dead: drawDead(); break;
        case GameState::Win: drawWin(); break;
        default: break;
    }

    if (shotFrames > 0 && --shotFrames == 0) {
        if (SDL_Surface* surf = SDL_RenderReadPixels(renderer, nullptr)) {
            if (IMG_SavePNG(surf, shotPath.c_str()))
                SDL_Log("Screenshot saved to %s", shotPath.c_str());
            else
                SDL_Log("Screenshot failed: %s", SDL_GetError());
            SDL_DestroySurface(surf);
        }
        running = false;
    }

    SDL_RenderPresent(renderer);
}

void Game::drawHUD() {
    // hearts
    for (int i = 0; i < player->maxHp / 2; ++i) {
        const int hp2 = player->hp - i * 2;
        const SDL_FRect& rect = hp2 >= 2   ? spriteRects::HEART_FULL
                                : hp2 == 1 ? spriteRects::HEART_HALF
                                           : spriteRects::HEART_EMPTY;
        drawSprite(renderer, assets.get("bars"), &rect, 6 + i * 17, 5, 16, 16);
    }
    // gold
    const int coinFrame = static_cast<int>(std::floor(time * 8)) % 6;
    const SDL_FRect coinSrc{static_cast<float>(coinFrame * 16), 0, 16, 16};
    drawSprite(renderer, assets.get("money"), &coinSrc, 6, 24, 16, 16);
    font.draw(renderer, std::to_string(player->gold), 25, 28, TEXT_LIGHT);

    // potions
    if (player->potions > 0) {
        fillRect(renderer, 8, 44, 8, 10, hexColor(0xe05a7a));
        fillRect(renderer, 9, 44, 6, 3, PANEL_FILL);
        font.draw(renderer, "x" + std::to_string(player->potions) + " [Q]", 20, 45, TEXT_LIGHT);
    }

    // XP bar
    fillRect(renderer, 6, VIEW_H - 12, 64, 6, TEXT_SHADOW);
    fillRect(renderer, 7, VIEW_H - 11, std::round(62.0f * player->xp / player->xpNext), 4, XP_BLUE);
    font.draw(renderer, "Lv " + std::to_string(player->level), 74, VIEW_H - 13, TEXT_LIGHT);

    // quest tracker (top right)
    if (const Quest* q = currentQuest()) {
        textShadow(renderer, font, q->name, VIEW_W - 8, 4, GOLD, Font::Right);
        std::string prog =
            q->state == Quest::State::Ready ? "Return to Bram" : q->desc;
        if (q->target > 0 && q->state != Quest::State::Ready)
            prog += "  " + std::to_string(q->count) + "/" + std::to_string(q->target);
        textShadow(renderer, font, prog, VIEW_W - 8, 15, TEXT_LIGHT, Font::Right);
    }

    // boss bar
    if (boss && !bossDown && boss->enraged && !boss->isDead()) {
        fillRect(renderer, VIEW_W / 2 - 70, 8, 140, 8, TEXT_SHADOW);
        fillRect(renderer, VIEW_W / 2 - 68, 10,
                 std::round(136.0f * SDL_max(0, boss->hp) / boss->k.hp), 4, BOSS_GOLD);
        font.draw(renderer, "GOLDEN SLIME", VIEW_W / 2, 17, TEXT_LIGHT, 1.0f, Font::Center);
    }
}

void Game::drawTitle() {
    fillRect(renderer, 0, 0, VIEW_W, VIEW_H, hexColor(0x140c18), 184);
    font.draw(renderer, "HARVEST VALE", VIEW_W / 2 + 2, 54, TEXT_SHADOW, 3.0f, Font::Center);
    font.draw(renderer, "HARVEST VALE", VIEW_W / 2, 52, GOLD, 3.0f, Font::Center);
    font.draw(renderer, "a tiny farm RPG", VIEW_W / 2, 88, GREEN, 1.0f, Font::Center);
    font.draw(renderer, "WASD / arrows - move      SHIFT - run", VIEW_W / 2, 122, TEXT_LIGHT,
              1.0f, Font::Center);
    font.draw(renderer, "SPACE / J - sword          E - talk / open", VIEW_W / 2, 134,
              TEXT_LIGHT, 1.0f, Font::Center);
    font.draw(renderer, "Q - drink potion           M - mute", VIEW_W / 2, 146, TEXT_LIGHT,
              1.0f, Font::Center);
    if (static_cast<int>(time * 2) % 2 == 0) {
        font.draw(renderer, "- PRESS ENTER -", VIEW_W / 2, 177, GOLD, 1.0f, Font::Center);
    }
    font.draw(renderer, "art: EmanuelleDev (emanuelledev.itch.io)", VIEW_W / 2, VIEW_H - 17,
              hexColor(0x8d8494), 1.0f, Font::Center);
}

void Game::drawDead() {
    fillRect(renderer, 0, 0, VIEW_W, VIEW_H, hexColor(0x1e0810), 153);
    font.draw(renderer, "YOU COLLAPSED...", VIEW_W / 2, VIEW_H / 2 - 26, RED, 2.0f,
              Font::Center);
    font.draw(renderer, "The bonfire keeps you. (Lost 15% of your gold)", VIEW_W / 2,
              VIEW_H / 2 + 2, TEXT_LIGHT, 1.0f, Font::Center);
    if (static_cast<int>(time * 2) % 2 == 0) {
        font.draw(renderer, "- press ENTER to wake up -", VIEW_W / 2, VIEW_H / 2 + 18,
                  TEXT_LIGHT, 1.0f, Font::Center);
    }
}

void Game::drawWin() {
    const Uint8 a = static_cast<Uint8>(SDL_min(0.55f, winT * 0.5f) * 255);
    fillRect(renderer, 0, 0, VIEW_W, VIEW_H, hexColor(0x142814), a);
    font.draw(renderer, "THE VALE IS SAFE!", VIEW_W / 2, VIEW_H / 2 - 28, GOLD, 2.0f,
              Font::Center);
    font.draw(renderer, "The golden slime is defeated. Claim the treasure north!", VIEW_W / 2,
              VIEW_H / 2, TEXT_LIGHT, 1.0f, Font::Center);
    if (winT > 1.0f && static_cast<int>(time * 2) % 2 == 0) {
        font.draw(renderer, "- press ENTER to keep exploring -", VIEW_W / 2, VIEW_H / 2 + 16,
                  TEXT_LIGHT, 1.0f, Font::Center);
    }
}

void Game::drawShop() {
    const float x = 8, y = VIEW_H - 78, w = VIEW_W - 16, h = 70;
    drawPanel(renderer, x, y, w, h);
    // merchant sprite as portrait
    const SDL_FRect msrc{0, 0, 32, 32};
    drawSprite(renderer, assets.get("merchant"), &msrc, x + 10, y + 10, 40, 40);
    font.draw(renderer, "Grub the Merchant", x + 58, y + 5, PANEL_BORDER);
    font.draw(renderer, "\"Shiny things for shiny coin! Take a look:\"", x + 58, y + 17,
              TEXT_DARK);
    font.draw(renderer,
              "[1] Potion ....... 15g  (Q to drink, heals 2 hearts)   x" +
                  std::to_string(player->potions),
              x + 58, y + 30, TEXT_DARK);
    font.draw(renderer,
              std::string("[2] Heart Container 50g  (+1 max heart)") +
                  (shopBought ? "  SOLD" : ""),
              x + 58, y + 41, TEXT_DARK);
    font.draw(renderer, "[3] Leave", x + 58, y + 52, TEXT_DARK);
    // gold
    const int coinFrame = static_cast<int>(std::floor(time * 8)) % 6;
    const SDL_FRect coinSrc{static_cast<float>(coinFrame * 16), 0, 16, 16};
    drawSprite(renderer, assets.get("money"), &coinSrc, x + w - 60, y + 6, 16, 16);
    font.draw(renderer, std::to_string(player->gold) + "g", x + w - 42, y + 10, PANEL_BORDER);
}

/* ---------------- debug hooks (headless screenshots) ---------------- */

void Game::applyDebugEnv() {
    if (SDL_getenv("HV_PLAY")) state = GameState::Play;
    if (const char* mk = SDL_getenv("HV_MAP")) {  // start on another map (Map::key)
        for (const auto& m : maps) {
            if (m->key == mk) {
                switchMap(m->id, m->playerStart);
                break;
            }
        }
    }
    if (const char* pos = SDL_getenv("HV_POS")) {
        float px, py;
        if (std::sscanf(pos, "%f,%f", &px, &py) == 2) {
            player->x = px;
            player->y = py;
            snapCamera();
        }
    }
    if (const char* act = SDL_getenv("HV_ACT")) {
        const std::string a = act;
        if (a == "dialog") talkToSmith();
        else if (a == "shop") openShop();
    }
    if (const char* shot = SDL_getenv("HV_SCREENSHOT")) {
        shotPath = shot;
        shotFrames = 45;
        if (const char* n = SDL_getenv("HV_SCREENSHOT_FRAMES")) shotFrames = std::atoi(n);
    }
}
