#pragma once

#include <SDL3/SDL.h>

#include <string>

#include "Assets.h"
#include "Sprites.h"
#include "World.h"

class Game;

enum class EnemyType { None, Slime, Goblin, Boss };

// Base class for everything that moves or lives in the world.
// Positions are feet-center (bottom-center of the sprite); the hitbox extends
// w/2 to each side and h up from the feet.
class Entity {
public:
    Entity(float x, float y, float w, float h) : x(x), y(y), w(w), h(h) {}
    virtual ~Entity() = default;

    virtual void update(float dt, Game& game) {}
    virtual void draw(SDL_Renderer* r, const Game& game) {}
    virtual void interact(Game& game) {}
    virtual void hurt(Game& game, int dmg, float sx, float sy) {}
    virtual bool isDead() const { return false; }

    SDL_FRect box() const { return {x - w / 2, y - h, w, h}; }

    bool collides(const World& world, float nx, float ny) const;
    void tryMove(const World& world, float dx, float dy);
    void applyKnockback(const World& world, float dt);
    void faceTowards(float tx, float ty);

    float x, y;              // feet center
    float w, h;              // hitbox
    Direction dir = Direction::Down;
    float animT = 0.0f;
    bool remove = false;
    float kbx = 0.0f, kby = 0.0f;  // knockback velocity
    EnemyType type = EnemyType::None;
};

/* ============================= PLAYER ============================= */

class Player : public Entity {
public:
    enum class State { Idle, Move, Attack, Hurt, Dead };

    Player(const Assets& assets, float x, float y);

    void update(float dt, Game& game) override;
    void draw(SDL_Renderer* r, const Game& game) override;
    void hurt(Game& game, int dmg, float sx, float sy) override;
    bool isDead() const override { return state == State::Dead; }

    void heal(int n) { hp = SDL_min(maxHp, hp + n); }
    void addXP(Game& game, int n);

    const CharDef& def() const;
    int frame() const;

    int maxHp = 6, hp = 6;  // half-hearts
    int gold = 0;
    int xp = 0, level = 1, xpNext = 12;
    int dmg = 1;
    int potions = 0;
    State state = State::Idle;
    bool running = false;
    float invuln = 0.0f, attackCd = 0.0f;
    bool hitApplied = false;

private:
    CharDef idleDef, walkDef, runDef, attackDef, hurtDef, deadDef;
};

/* ============================= ENEMIES ============================= */

struct SlimeKind {
    const char* texKey;
    int hp, dmg;
    float speed;
    int aggro;
    int xp;
    int gold0, gold1;
    float scale;
};

enum class SlimeName { Blue, Green, Pink, Golden };
const SlimeKind& slimeKind(SlimeName name);

class Slime : public Entity {
public:
    enum class State { Idle, Move, Chase, Attack, Hurt, Dead };

    Slime(const Assets& assets, float x, float y, SlimeName kindName);

    void update(float dt, Game& game) override;
    void draw(SDL_Renderer* r, const Game& game) override;
    void hurt(Game& game, int dmg, float sx, float sy) override;
    bool isDead() const override { return state == State::Dead; }

    int frame() const;

    const SlimeKind& k;
    const bool golden;
    int hp;
    State state = State::Idle;
    float stateT = 0.0f, waitT;
    float tx, ty, homeX, homeY;  // wander target / anchor
    bool attackDidHit = false;
    bool enraged = false;  // golden slime: aggroes forever once hit

private:
    CharDef makeDef() const;
    SDL_Texture* tex;
};

class Goblin : public Entity {
public:
    enum class State { Idle, Move, Chase, Attack, Hurt, Dead };

    Goblin(const Assets& assets, float x, float y);

    void update(float dt, Game& game) override;
    void draw(SDL_Renderer* r, const Game& game) override;
    void hurt(Game& game, int dmg, float sx, float sy) override;
    bool isDead() const override { return state == State::Dead; }

    int hp = 6, maxHp = 6;
    int dmg = 2;
    float speed = 48;
    int xp = 8;
    State state = State::Idle;
    float stateT = 0.0f, waitT;
    float tx, ty, homeX, homeY;
    bool attackDidHit = false;

private:
    const CharDef& def() const;
    int frame() const;
    CharDef idleDef, moveDef, chaseDef, attackDef, hurtDef, deadDef;
};

/* ============================= NPCs ============================= */

class NPC : public Entity {
public:
    enum class Kind { Smith, Merchant };

    NPC(float x, float y, std::string name, const CharDef& def, Kind kind);

    void update(float dt, Game& game) override;
    void draw(SDL_Renderer* r, const Game& game) override;
    void interact(Game& game) override;

    std::string name;
    CharDef def;
    Kind kind;
};

/* ============================= ANIMALS ============================= */

class Chicken : public Entity {
public:
    Chicken(float x, float y, SDL_Texture* img, const SDL_FRect& pen);
    void update(float dt, Game& game) override;
    void draw(SDL_Renderer* r, const Game& game) override;

private:
    enum class State { Idle, Walk, Peck };
    SDL_Texture* img;
    SDL_FRect pen;
    State state = State::Idle;
    float stateT = 0.0f, waitT;
    float tx, ty;
    bool flip = false;
};

class Fox : public Entity {
public:
    Fox(float x, float y, SDL_Texture* img);
    void update(float dt, Game& game) override;
    void draw(SDL_Renderer* r, const Game& game) override;

private:
    SDL_Texture* img;
    float homeX, homeY;
    bool walking = false;
    float stateT = 0.0f, waitT;
    float tx, ty;
    bool flip = false;
};

/* ============================= CHEST / BONFIRE / PICKUPS ============================= */

class Chest : public Entity {
public:
    Chest(const Assets& assets, float x, float y, bool big);
    void draw(SDL_Renderer* r, const Game& game) override;
    void interact(Game& game) override;

    bool big;
    bool opened = false;

private:
    SDL_Texture* tex;
};

class Bonfire : public Entity {
public:
    Bonfire(const Assets& assets, float x, float y);
    void update(float dt, Game& game) override { animT += dt; }
    void draw(SDL_Renderer* r, const Game& game) override;

private:
    SDL_Texture* tex;
};

class Pickup : public Entity {
public:
    enum class Type { Coin, Heart };

    Pickup(const Assets& assets, float x, float y, Type type, int value);
    void update(float dt, Game& game) override;
    void draw(SDL_Renderer* r, const Game& game) override;

    Type type;
    int value;

private:
    float vx, vy;
    SDL_Texture* moneyTex;
    SDL_Texture* barsTex;
};

// Rising, fading combat/status text.
struct FloatText {
    FloatText(float x, float y, std::string text, SDL_Color color)
        : x(x), y(y), text(std::move(text)), color(color) {}

    void update(float dt) {
        t += dt;
        if (t > life) remove = true;
    }
    void draw(SDL_Renderer* r, const Game& game) const;

    float x, y;
    std::string text;
    SDL_Color color;
    float t = 0.0f;
    float life = 1.1f;
    bool remove = false;
};
