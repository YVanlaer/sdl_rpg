#pragma once

#include <SDL3/SDL.h>

#include <functional>
#include <memory>
#include <string>
#include <unordered_set>
#include <vector>

#include "Assets.h"
#include "Camera.h"
#include "Entities.h"
#include "Font.h"
#include "Sfx.h"
#include "World.h"

constexpr int VIEW_W = 480;
constexpr int VIEW_H = 270;

enum class GameState { Title, Play, Dialog, Shop, Dead, Win };

struct Quest {
    enum class State { Locked, Active, Ready, Done };

    std::string name, desc;
    State state = State::Locked;
    int count = 0, target = 0;
};

// Typewriter dialogue box with an optional portrait, shown over the world.
class Dialogue {
public:
    Dialogue(std::string name, std::vector<std::string> lines, SDL_Texture* portrait,
             std::function<void()> onEnd = nullptr)
        : name(std::move(name)),
          lines(std::move(lines)),
          portrait(portrait),
          onEnd(std::move(onEnd)) {}

    void update(float dt, bool confirmPressed, Game& game);
    void draw(SDL_Renderer* r, const Font& font) const;

private:
    std::string name;
    std::vector<std::string> lines;
    SDL_Texture* portrait;
    std::function<void()> onEnd;
    int lineIndex = 0;
    float chars = 0.0f;
    float t = 0.0f;
};

class Game {
public:
    struct Input {
        bool up = false, down = false, left = false, right = false, run = false;
        bool attackPressed = false, interactPressed = false, confirmPressed = false;
        bool potionPressed = false, mutePressed = false, cancelPressed = false;
        int num = 0;  // shop hotkeys 1..3
    };

    Game() = default;
    ~Game();

    bool init();
    void run();

    // entity callbacks
    void meleeHit(Player& attacker);
    void onEnemyKilled(Entity& e);
    void onPlayerDeath();
    void talkToSmith();
    void openShop();
    void addFloat(float x, float y, std::string text, SDL_Color color);

    // shared state used by entities
    Assets assets;
    Font font;
    Sfx audio;
    World world;
    Camera cam;
    Input input;
    GameState state = GameState::Title;
    float time = 0.0f;
    float shake = 0.0f;

    std::unique_ptr<Player> player;
    std::vector<std::unique_ptr<Entity>> enemies, npcs, animals, props, pickups;
    std::vector<FloatText> texts;

private:
    void processEvents();
    Input makeInput() const;
    void update(float dt);
    void render();

    void spawnWorld();
    void interact();
    void shopBuy(int n);
    void startDialogue(std::unique_ptr<Dialogue> d);
    const Quest* currentQuest() const;
    void snapCamera();
    void applyDebugEnv();

    void drawWorld();
    void drawHUD();
    void drawTitle();
    void drawDead();
    void drawWin();
    void drawShop();

    SDL_Window* window = nullptr;
    SDL_Renderer* renderer = nullptr;
    bool running = false;
    std::unordered_set<SDL_Scancode> pressed;  // edge-triggered keys this frame

    std::vector<Entity*> interactables;
    struct {
        Quest intro{"A New Arrival", "Talk to Bram the blacksmith", Quest::State::Active};
        Quest slime{"Slime Trouble", "Defeat slimes in the forest", Quest::State::Locked, 0, 5};
        Quest boss{"The Golden Menace", "Defeat the golden slime (NE clearing)"};
    } quests;
    std::unique_ptr<Dialogue> dialogue;
    bool shopBought = false;
    Slime* boss = nullptr;  // non-owning; cleared on kill
    bool bossDown = false;
    float winT = 0.0f;
    float deathTimer = -1.0f;

    // headless screenshot hook (HV_SCREENSHOT=/path/out.png)
    std::string shotPath;
    int shotFrames = -1;
};
