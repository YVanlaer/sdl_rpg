#include "Entities.h"

#include <cmath>

#include "Game.h"
#include "Util.h"

/* ============================= Entity ============================= */

bool Entity::collides(const World& world, float nx, float ny) const {
    const SDL_FRect b{nx - w / 2, ny - h, w, h};
    for (const SDL_FRect& c : world.colliders) {
        if (rectsOverlap(b, c)) return true;
    }
    return false;
}

void Entity::tryMove(const World& world, float dx, float dy) {
    if (dx != 0.0f && !collides(world, x + dx, y)) x += dx;
    if (dy != 0.0f && !collides(world, x, y + dy)) y += dy;
    x = clampf(x, 8, WORLD_W - 8);
    y = clampf(y, 16, WORLD_H - 4);
}

void Entity::applyKnockback(const World& world, float dt) {
    if (std::fabs(kbx) > 1.0f || std::fabs(kby) > 1.0f) {
        tryMove(world, kbx * dt, kby * dt);
        kbx *= std::pow(0.001f, dt);
        kby *= std::pow(0.001f, dt);
    } else {
        kbx = kby = 0.0f;
    }
}

void Entity::faceTowards(float tx, float ty) {
    const float dx = tx - x, dy = ty - y;
    if (std::fabs(dx) > std::fabs(dy)) dir = dx < 0 ? Direction::Left : Direction::Right;
    else if (std::fabs(dy) > 1.0f) dir = dy < 0 ? Direction::Up : Direction::Down;
}

/* ============================= Player ============================= */

namespace {
struct HeroDef {
    const char* key;
    int frames;
    int fps;
};
constexpr HeroDef HERO_IDLE{"heroIdle", 4, 6};
constexpr HeroDef HERO_WALK{"heroWalk", 6, 11};
constexpr HeroDef HERO_RUN{"heroRun", 8, 13};
constexpr HeroDef HERO_ATTACK{"heroSword", 10, 20};
constexpr HeroDef HERO_HURT{"heroDamage", 4, 10};
constexpr HeroDef HERO_DEAD{"heroDead", 4, 5};

CharDef makeHeroDef(const Assets& assets, const HeroDef& d) {
    return CharDef{assets.get(d.key), 32, 32, d.frames, d.fps, 0, false};
}

// "E" interact hint bubble above NPCs and chests.
void drawBubble(SDL_Renderer* r, const Game& game, float x, float y) {
    const SDL_FRect box{std::round(x - game.cam.x) - 6, std::round(y - game.cam.y) - 12, 12, 12};
    const SDL_Color fill = hexColor(0xfff8e8);
    const SDL_Color border = hexColor(0x5a3d2b);
    SDL_SetRenderDrawColor(r, fill.r, fill.g, fill.b, 255);
    SDL_RenderFillRect(r, &box);
    SDL_SetRenderDrawColor(r, border.r, border.g, border.b, 255);
    SDL_RenderRect(r, &box);
    game.font.draw(r, "E", box.x + 6, box.y + 2, border);
}
}  // namespace

Player::Player(const Assets& assets, float x, float y)
    : Entity(x, y, 12, 12),
      idleDef(makeHeroDef(assets, HERO_IDLE)),
      walkDef(makeHeroDef(assets, HERO_WALK)),
      runDef(makeHeroDef(assets, HERO_RUN)),
      attackDef(makeHeroDef(assets, HERO_ATTACK)),
      hurtDef(makeHeroDef(assets, HERO_HURT)),
      deadDef(makeHeroDef(assets, HERO_DEAD)) {}

const CharDef& Player::def() const {
    switch (state) {
        case State::Move: return running ? runDef : walkDef;
        case State::Attack: return attackDef;
        case State::Hurt: return hurtDef;
        case State::Dead: return deadDef;
        case State::Idle: default: return idleDef;
    }
}

int Player::frame() const {
    const int f = static_cast<int>(std::floor(animT * def().fps));
    // looping states wrap; attack/hurt/dead clamp in drawCharFrame
    if (state == State::Idle || state == State::Move) return f % def().frames;
    return f;
}

void Player::hurt(Game& game, int dmg, float sx, float sy) {
    if (invuln > 0.0f || state == State::Dead) return;
    hp = SDL_max(0, hp - dmg);
    invuln = 1.0f;
    const float a = std::atan2(y - sy, x - sx);
    kbx = std::cos(a) * 160;
    kby = std::sin(a) * 160;
    game.audio.play("hurt");
    game.shake = 0.25f;
    animT = 0.0f;
    if (hp <= 0) {
        state = State::Dead;
        game.onPlayerDeath();
    } else {
        state = State::Hurt;
    }
}

void Player::addXP(Game& game, int n) {
    xp += n;
    while (xp >= xpNext) {
        xp -= xpNext;
        level++;
        xpNext = static_cast<int>(xpNext * 1.8f);
        maxHp += 2;
        if (level % 2 == 0) dmg += 1;
        heal(maxHp);
        game.audio.play("levelup");
        game.addFloat(x, y - 30, "LEVEL " + std::to_string(level) + "!", hexColor(0xffd94a));
    }
}

void Player::update(float dt, Game& game) {
    const Game::Input& input = game.input;
    animT += dt;
    invuln = SDL_max(0.0f, invuln - dt);
    attackCd = SDL_max(0.0f, attackCd - dt);
    applyKnockback(game.world, dt);

    if (state == State::Dead) return;

    if (state == State::Attack) {
        const int f = frame();
        if (f >= 3 && !hitApplied) {
            hitApplied = true;
            game.meleeHit(*this);
        }
        if (f >= HERO_ATTACK.frames - 1) state = State::Idle;
        return;  // rooted while swinging
    }
    if (state == State::Hurt) {
        if (frame() >= HERO_HURT.frames - 1) state = State::Idle;
        return;
    }

    float dx = (input.right ? 1.0f : 0.0f) - (input.left ? 1.0f : 0.0f);
    float dy = (input.down ? 1.0f : 0.0f) - (input.up ? 1.0f : 0.0f);
    if (dx != 0.0f || dy != 0.0f) {
        const float len = std::hypot(dx, dy);
        dx /= len;
        dy /= len;
        running = input.run;
        const float sp = running ? 118.0f : 72.0f;
        tryMove(game.world, dx * sp * dt, dy * sp * dt);
        if (dx < 0) dir = Direction::Left;
        else if (dx > 0) dir = Direction::Right;
        else if (dy < 0) dir = Direction::Up;
        else dir = Direction::Down;
        state = State::Move;
    } else {
        state = State::Idle;
    }

    if (input.attackPressed && attackCd <= 0.0f) {
        state = State::Attack;
        animT = 0.0f;
        hitApplied = false;
        attackCd = 0.5f;
        game.audio.play("swing");
    }
}

void Player::draw(SDL_Renderer* r, const Game& game) {
    if (invuln > 0.0f && state != State::Dead &&
        static_cast<int>(std::floor(invuln * 12)) % 2 == 0) return;
    drawShadow(r, game.cam, x, y, 8);
    drawCharFrame(r, game.cam, def(), dir, frame(), x, y + 4);
}

/* ============================= Slime ============================= */

namespace {
// sheet row offsets per state (rows are [down, up, left] within each block)
constexpr int SLIME_ROWS[] = {0, 3, 3, 6, 9, 12};  // idle, move, chase, attack, hurt, dead
constexpr int SLIME_FPS[] = {5, 6, 6, 8, 8, 6};
}  // namespace

const SlimeKind& slimeKind(SlimeName name) {
    static const SlimeKind KINDS[] = {
        {"slimeBlue", 3, 1, 30, 90, 4, 1, 3, 1},
        {"slimeGreen", 4, 1, 34, 95, 5, 2, 4, 1},
        {"slimePink", 6, 2, 38, 100, 7, 3, 5, 1},
        {"slimeGolden", 30, 3, 30, 130, 40, 40, 60, 2},
    };
    return KINDS[static_cast<int>(name)];
}

Slime::Slime(const Assets& assets, float x, float y, SlimeName kindName)
    : Entity(x, y, 14 * slimeKind(kindName).scale, 10 * slimeKind(kindName).scale),
      k(slimeKind(kindName)),
      golden(kindName == SlimeName::Golden),
      hp(k.hp),
      tex(assets.get(k.texKey)) {
    type = golden ? EnemyType::Boss : EnemyType::Slime;
    waitT = randf() * 2.0f;
    tx = homeX = x;
    ty = homeY = y;
}

CharDef Slime::makeDef() const {
    const int s = static_cast<int>(state);
    return CharDef{tex, 32, 32, 4, SLIME_FPS[s], SLIME_ROWS[s], false};
}

int Slime::frame() const {
    const int f = static_cast<int>(std::floor(animT * SLIME_FPS[static_cast<int>(state)]));
    return (state == State::Dead || state == State::Attack) ? SDL_min(3, f) : f % 4;
}

void Slime::hurt(Game& game, int dmg, float sx, float sy) {
    if (state == State::Dead) return;
    hp -= dmg;
    const float a = std::atan2(y - sy, x - sx);
    const float kb = golden ? 30.0f : 130.0f;
    kbx = std::cos(a) * kb;
    kby = std::sin(a) * kb;
    game.audio.play("hit");
    game.addFloat(x, y - 20, "-" + std::to_string(dmg), hexColor(0xffffff));
    if (golden) enraged = true;
    animT = 0.0f;
    if (hp <= 0) {
        state = State::Dead;
        game.audio.play("squish");
    } else {
        state = State::Hurt;
    }
}

void Slime::update(float dt, Game& game) {
    animT += dt;
    stateT += dt;
    applyKnockback(game.world, dt);
    Player& p = *game.player;
    const float d = distf(x, y, p.x, p.y);

    switch (state) {
        case State::Dead:
            if (animT > 0.7f) { remove = true; game.onEnemyKilled(*this); }
            return;
        case State::Hurt:
            if (animT > 0.35f) { state = State::Chase; animT = 0.0f; }
            return;
        case State::Attack:
            if (frame() == 2 && !attackDidHit) {
                attackDidHit = true;
                const float reach = 24 * k.scale + 6;
                if (distf(x, y, p.x, p.y) < reach) p.hurt(game, k.dmg, x, y);
            }
            if (animT > 0.55f) { state = State::Chase; animT = 0.0f; }
            return;
        default: break;
    }

    const bool aggro = enraged || d < k.aggro;
    if (state == State::Idle) {
        if (stateT > waitT) {
            if (aggro) {
                state = State::Chase;
            } else {
                state = State::Move;
                const float a = randf() * 6.28318f, r = 20 + randf() * 40;
                tx = homeX + std::cos(a) * r;
                ty = homeY + std::sin(a) * r;
            }
            stateT = 0.0f;
            animT = 0.0f;
        }
    } else {  // Move or Chase
        if (aggro) {
            state = State::Chase;
            tx = p.x;
            ty = p.y;
            const float reach = 20 * k.scale;
            if (d < reach) {
                state = State::Attack;
                animT = stateT = 0.0f;
                attackDidHit = false;
                faceTowards(p.x, p.y);
                return;
            }
        } else if (state == State::Chase) {
            state = State::Move;  // lost interest, wander home-ish
        }
        const float dd = distf(x, y, tx, ty);
        if (dd < 4.0f) {
            state = State::Idle;
            stateT = animT = 0.0f;
            waitT = 0.8f + randf() * 2.0f;
        } else {
            const float sp = state == State::Chase ? k.speed : k.speed * 0.5f;
            faceTowards(tx, ty);
            tryMove(game.world, (tx - x) / dd * sp * dt, (ty - y) / dd * sp * dt);
        }
    }
}

void Slime::draw(SDL_Renderer* r, const Game& game) {
    const float s = k.scale;
    drawShadow(r, game.cam, x, y, 7 * s);
    drawCharFrame(r, game.cam, makeDef(), dir, frame(), x, y + 2 * s, s);
}

/* ============================= Goblin ============================= */

Goblin::Goblin(const Assets& assets, float x, float y)
    : Entity(x, y, 12, 14),
      idleDef{assets.get("gobIdle"), 32, 32, 4, 6, 0, false},
      moveDef{assets.get("gobWalk"), 32, 32, 6, 10, 0, false},
      chaseDef{assets.get("gobRun"), 32, 32, 8, 12, 0, false},
      attackDef{assets.get("gobAttack"), 32, 32, 6, 12, 0, false},
      hurtDef{assets.get("gobDamage"), 32, 32, 4, 8, 0, false},
      deadDef{assets.get("gobDead"), 32, 32, 4, 6, 0, false} {
    type = EnemyType::Goblin;
    waitT = randf() * 2.0f;
    tx = homeX = x;
    ty = homeY = y;
}

const CharDef& Goblin::def() const {
    switch (state) {
        case State::Move: return moveDef;
        case State::Chase: return chaseDef;
        case State::Attack: return attackDef;
        case State::Hurt: return hurtDef;
        case State::Dead: return deadDef;
        case State::Idle: default: return idleDef;
    }
}

int Goblin::frame() const {
    const CharDef& d = def();
    const int f = static_cast<int>(std::floor(animT * d.fps));
    return (state == State::Dead || state == State::Attack) ? SDL_min(d.frames - 1, f)
                                                            : f % d.frames;
}

void Goblin::hurt(Game& game, int dmg, float sx, float sy) {
    if (state == State::Dead) return;
    hp -= dmg;
    const float a = std::atan2(y - sy, x - sx);
    kbx = std::cos(a) * 140;
    kby = std::sin(a) * 140;
    game.audio.play("hit");
    game.addFloat(x, y - 24, "-" + std::to_string(dmg), hexColor(0xffffff));
    animT = 0.0f;
    if (hp <= 0) {
        state = State::Dead;
        game.audio.play("squish");
    } else {
        state = State::Hurt;
    }
}

void Goblin::update(float dt, Game& game) {
    animT += dt;
    stateT += dt;
    applyKnockback(game.world, dt);
    Player& p = *game.player;
    const float d = distf(x, y, p.x, p.y);

    switch (state) {
        case State::Dead:
            if (animT > 0.8f) { remove = true; game.onEnemyKilled(*this); }
            return;
        case State::Hurt:
            if (animT > 0.35f) { state = State::Chase; animT = 0.0f; }
            return;
        case State::Attack: {
            const int f = frame();
            if (f >= 1 && f <= 3) {  // lunge
                const float dd = SDL_max(1.0f, distf(x, y, p.x, p.y));
                tryMove(game.world, (p.x - x) / dd * 95 * dt, (p.y - y) / dd * 95 * dt);
            }
            if (f == 2 && !attackDidHit) {
                attackDidHit = true;
                if (distf(x, y, p.x, p.y) < 30) p.hurt(game, dmg, x, y);
            }
            if (animT > 0.55f) { state = State::Chase; animT = 0.0f; }
            return;
        }
        default: break;
    }

    if (state == State::Idle) {
        if (stateT > waitT) {
            state = d < 100 ? State::Chase : State::Move;
            const float a = randf() * 6.28318f, r = 24 + randf() * 40;
            tx = homeX + std::cos(a) * r;
            ty = homeY + std::sin(a) * r;
            stateT = animT = 0.0f;
        }
    } else {  // Move or Chase
        if (d < 100) state = State::Chase;
        else if (state == State::Chase && d > 140) state = State::Move;
        if (state == State::Chase) { tx = p.x; ty = p.y; }
        if (state == State::Chase && d < 26) {
            state = State::Attack;
            animT = stateT = 0.0f;
            attackDidHit = false;
            faceTowards(p.x, p.y);
            return;
        }
        const float dd = distf(x, y, tx, ty);
        if (dd < 4.0f) {
            state = State::Idle;
            stateT = animT = 0.0f;
            waitT = 0.6f + randf() * 1.6f;
        } else {
            const float sp = state == State::Chase ? speed : speed * 0.45f;
            faceTowards(tx, ty);
            tryMove(game.world, (tx - x) / dd * sp * dt, (ty - y) / dd * sp * dt);
        }
    }
}

void Goblin::draw(SDL_Renderer* r, const Game& game) {
    drawShadow(r, game.cam, x, y, 8);
    drawCharFrame(r, game.cam, def(), dir, frame(), x, y + 4);
    if (hp < maxHp && state != State::Dead) {
        const float bx = std::round(x - game.cam.x) - 10;
        const float by = std::round(y - game.cam.y) - 34;
        const SDL_FRect bg{bx, by, 20, 3};
        const SDL_FRect fg{bx, by, std::round(20.0f * hp / maxHp), 3};
        const SDL_Color dark = hexColor(0x2b1d26), red = hexColor(0xe0443a);
        SDL_SetRenderDrawColor(r, dark.r, dark.g, dark.b, 255);
        SDL_RenderFillRect(r, &bg);
        SDL_SetRenderDrawColor(r, red.r, red.g, red.b, 255);
        SDL_RenderFillRect(r, &fg);
    }
}

/* ============================= NPC ============================= */

NPC::NPC(float x, float y, std::string name, const CharDef& def, Kind kind)
    : Entity(x, y, 12, 12), name(std::move(name)), def(def), kind(kind) {
    animT = randf() * 2.0f;
}

void NPC::update(float dt, Game& game) {
    animT += dt;
    const Player& p = *game.player;
    if (distf(x, y, p.x, p.y) < 48) faceTowards(p.x, p.y);
    else dir = Direction::Down;
}

void NPC::draw(SDL_Renderer* r, const Game& game) {
    drawShadow(r, game.cam, x, y, 8);
    const int f = static_cast<int>(std::floor(animT * def.fps)) % def.frames;
    drawCharFrame(r, game.cam, def, dir, f, x, y + 4);
    const Player& p = *game.player;
    if (distf(x, y, p.x, p.y) < 26 && game.state == GameState::Play) {
        drawBubble(r, game, x, y - 38);
    }
}

void NPC::interact(Game& game) {
    if (kind == Kind::Smith) game.talkToSmith();
    else game.openShop();
}

/* ============================= Chicken ============================= */

Chicken::Chicken(float x, float y, SDL_Texture* img, const SDL_FRect& pen)
    : Entity(x, y, 8, 6), img(img), pen(pen) {
    animT = randf() * 3.0f;
    waitT = 1 + randf() * 2.0f;
    tx = x;
    ty = y;
}

void Chicken::update(float dt, Game& game) {
    animT += dt;
    stateT += dt;
    if (state == State::Walk) {
        const float dd = distf(x, y, tx, ty);
        if (dd < 3.0f) {
            state = randf() < 0.5f ? State::Peck : State::Idle;
            stateT = 0.0f;
            waitT = 1 + randf() * 2.5f;
        } else {
            flip = tx < x;
            tryMove(game.world, (tx - x) / dd * 14 * dt, (ty - y) / dd * 14 * dt);
        }
    } else if (stateT > waitT) {
        state = State::Walk;
        stateT = 0.0f;
        tx = pen.x + 8 + randf() * (pen.w - 16);
        ty = pen.y + 12 + randf() * (pen.h - 16);
    }
}

void Chicken::draw(SDL_Renderer* r, const Game& game) {
    // rows: 0 peck, 1 idle, 2 walk — 16px cells, 4 frames
    const int row = state == State::Peck ? 0 : state == State::Idle ? 1 : 2;
    const int f = static_cast<int>(std::floor(animT * 5)) % 4;
    drawShadow(r, game.cam, x, y, 5);
    const SDL_FRect src{static_cast<float>(f * 16), static_cast<float>(row * 16), 16, 16};
    drawSpriteW(r, game.cam, img, &src, x - 8, y - 13, 16, 16, flip);
}

/* ============================= Fox ============================= */

Fox::Fox(float x, float y, SDL_Texture* img) : Entity(x, y, 12, 8), img(img) {
    homeX = x;
    homeY = y;
    animT = randf() * 3.0f;
    waitT = 2 + randf() * 3.0f;
    tx = x;
    ty = y;
}

void Fox::update(float dt, Game& game) {
    animT += dt;
    stateT += dt;
    if (walking) {
        const float dd = distf(x, y, tx, ty);
        if (dd < 3.0f) {
            walking = false;
            stateT = 0.0f;
            waitT = 2 + randf() * 4.0f;
        } else {
            flip = tx < x;
            tryMove(game.world, (tx - x) / dd * 26 * dt, (ty - y) / dd * 26 * dt);
        }
    } else if (stateT > waitT) {
        walking = true;
        stateT = 0.0f;
        const float a = randf() * 6.28318f, r = 30 + randf() * 60;
        tx = homeX + std::cos(a) * r;
        ty = homeY + std::sin(a) * r;
    }
}

void Fox::draw(SDL_Renderer* r, const Game& game) {
    // rows: 0 side walk, 2 front sit — 32px cells, 4 frames
    const int row = walking ? 0 : 2;
    const int f = static_cast<int>(std::floor(animT * 5)) % 4;
    drawShadow(r, game.cam, x, y, 8);
    const SDL_FRect src{static_cast<float>(f * 32), static_cast<float>(row * 32), 32, 32};
    drawSpriteW(r, game.cam, img, &src, x - 16, y - 26, 32, 32, flip);
}

/* ============================= Chest ============================= */

Chest::Chest(const Assets& assets, float x, float y, bool big)
    : Entity(x, y, 20, 12), big(big), tex(assets.get("chest")) {}

void Chest::interact(Game& game) {
    if (opened) return;
    opened = true;
    game.audio.play("open");
    const int n = big ? 8 : 3;
    for (int i = 0; i < n; ++i) {
        const float a = randf() * 6.28318f;
        game.pickups.push_back(std::make_unique<Pickup>(
            game.assets, x + std::cos(a) * 14, y - 6 + std::sin(a) * 10,
            Pickup::Type::Coin, big ? 10 : 1 + randInt(0, 2)));
    }
    game.pickups.push_back(
        std::make_unique<Pickup>(game.assets, x, y - 14, Pickup::Type::Heart, 2));
    game.addFloat(x, y - 24, big ? "Treasure!" : "Loot!", hexColor(0xffd94a));
    if (big) game.addFloat(x, y - 34, "The Vale is safe!", hexColor(0x7dff8a));
}

void Chest::draw(SDL_Renderer* r, const Game& game) {
    const SDL_FRect& rect = opened ? spriteRects::CHEST_OPEN : spriteRects::CHEST_CLOSED;
    drawShadow(r, game.cam, x, y, 8);
    drawSpriteW(r, game.cam, tex, &rect, x - 10, y - 14, 20, 14);
    const Player& p = *game.player;
    if (!opened && distf(x, y, p.x, p.y) < 26 && game.state == GameState::Play) {
        drawBubble(r, game, x, y - 22);
    }
}

/* ============================= Bonfire ============================= */

Bonfire::Bonfire(const Assets& assets, float x, float y)
    : Entity(x, y, 14, 8), tex(assets.get("bonfire")) {}

void Bonfire::draw(SDL_Renderer* r, const Game& game) {
    // bonfire sheet: 6 cells of 16x32 — flicker between the two big ones
    const int f = static_cast<int>(std::floor(animT * 6)) % 2;
    drawShadow(r, game.cam, x, y, 9);
    const SDL_FRect src{32.0f + f * 16, 0, 16, 32};
    drawSpriteW(r, game.cam, tex, &src, x - 8, y - 26, 16, 32);
}

/* ============================= Pickup ============================= */

Pickup::Pickup(const Assets& assets, float x, float y, Type type, int value)
    : Entity(x, y, 8, 8),
      type(type),
      value(value),
      moneyTex(assets.get("money")),
      barsTex(assets.get("bars")) {
    animT = randf() * 2.0f;
    vx = (randf() - 0.5f) * 40;
    vy = (randf() - 0.5f) * 40;
}

void Pickup::update(float dt, Game& game) {
    animT += dt;
    x += vx * dt;
    y += vy * dt;
    vx *= std::pow(0.01f, dt);
    vy *= std::pow(0.01f, dt);
    Player& p = *game.player;
    const float d = distf(x, y, p.x, p.y - 6);
    if (d > 0.01f && d < 30.0f) {
        const float sp = 110.0f;
        x += (p.x - x) / d * sp * dt;
        y += (p.y - 6 - y) / d * sp * dt;
    }
    if (d < 9.0f) {
        remove = true;
        if (type == Type::Coin) {
            p.gold += value;
            game.audio.play("coin");
            game.addFloat(p.x, p.y - 24, "+" + std::to_string(value) + "g", hexColor(0xffd94a));
        } else {
            p.heal(value);
            game.audio.play("heal");
            game.addFloat(p.x, p.y - 24, "+" + std::to_string(value / 2) + " \x01",
                          hexColor(0xff6b81));
        }
    }
}

void Pickup::draw(SDL_Renderer* r, const Game& game) {
    const float bob = std::sin(animT * 4) * 1.5f;
    if (type == Type::Coin) {
        const int f = static_cast<int>(std::floor(animT * 8)) % 6;
        const SDL_FRect src{static_cast<float>(f * 16), 0, 16, 16};
        drawSpriteW(r, game.cam, moneyTex, &src, x - 8, y - 12 + bob, 16, 16);
    } else {
        drawSpriteW(r, game.cam, barsTex, &spriteRects::HEART_FULL, x - 8, y - 12 + bob, 16, 16);
    }
}

/* ============================= FloatText ============================= */

void FloatText::draw(SDL_Renderer* r, const Game& game) const {
    const float a = 1.0f - t / life;
    const float sy = y - t * 18;
    const Uint8 alpha = static_cast<Uint8>(clampf(a, 0.0f, 1.0f) * 255);
    const float sx = x - std::round(game.cam.x);
    const float syScreen = sy - std::round(game.cam.y);
    game.font.draw(r, text, sx + 1, syScreen + 1, hexColor(0x2b1d26), 1.0f, Font::Center, alpha);
    game.font.draw(r, text, sx, syScreen, color, 1.0f, Font::Center, alpha);
}
