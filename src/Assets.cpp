#include "Assets.h"

#include <SDL3_image/SDL_image.h>

#include <filesystem>
#include <iostream>

namespace {

const char* PACK_DIR = "Farm RPG - Tiny Asset Pack - (All in One)";

// key -> path inside the asset pack (same manifest as the original JS game)
const std::pair<const char*, const char*> MANIFEST[] = {
    // player (Alex)
    {"heroIdle", "Character and Portrait - Tiny Asset Pack/Character/Pre-made/Alex/Idle.png"},
    {"heroWalk", "Character and Portrait - Tiny Asset Pack/Character/Pre-made/Alex/Walk.png"},
    {"heroRun", "Character and Portrait - Tiny Asset Pack/Character/Pre-made/Alex/Run.png"},
    {"heroSword", "Character and Portrait - Tiny Asset Pack/Character/Pre-made/Alex/Sword.png"},
    {"heroDamage", "Character and Portrait - Tiny Asset Pack/Character/Pre-made/Alex/Damage.png"},
    {"heroDead", "Character and Portrait - Tiny Asset Pack/Character/Pre-made/Alex/Dead.png"},

    // slimes: all-in-one sheet, 4 cols x 15 rows of 32x32
    {"slimeBlue", "Enemy - Tiny Asset Pack/Slimes/Blue/Slime.png"},
    {"slimeGreen", "Enemy - Tiny Asset Pack/Slimes/Green/Slime.png"},
    {"slimePink", "Enemy - Tiny Asset Pack/Slimes/Pink/Slime.png"},
    {"slimeGolden", "Enemy - Tiny Asset Pack/Slimes/Golden/Big Slime.png"},

    // spear goblin
    {"gobIdle", "Enemy - Tiny Asset Pack/Goblins/Spear Goblin/Idle.png"},
    {"gobWalk", "Enemy - Tiny Asset Pack/Goblins/Spear Goblin/Walk.png"},
    {"gobRun", "Enemy - Tiny Asset Pack/Goblins/Spear Goblin/Run.png"},
    {"gobAttack", "Enemy - Tiny Asset Pack/Goblins/Spear Goblin/Spear.png"},
    {"gobDamage", "Enemy - Tiny Asset Pack/Goblins/Spear Goblin/Damage.png"},
    {"gobDead", "Enemy - Tiny Asset Pack/Goblins/Spear Goblin/Dead.png"},

    // goblin merchant (shopkeeper), 12x2 of 32x32
    {"merchant", "Enemy - Tiny Asset Pack/Goblins/Goblin merchant/1.png"},

    // blacksmith NPC + portrait
    {"smithIdle", "Character and Portrait - Tiny Asset Pack/NPC'S/Blacksmith/Idle.png"},
    {"smithPortrait", "Character and Portrait - Tiny Asset Pack/NPC'S/Blacksmith/Portrait.png"},

    // animals
    {"chickenWhite", "Farm Animals- Tiny Asset Pack/Chicken/Chicken White.png"},
    {"chickenBrown", "Farm Animals- Tiny Asset Pack/Chicken/Chicken Brown White.png"},
    {"fox", "Forest Animals - Tiny Asset Pack/Fox/Red Fox.png"},

    // world props
    {"houseHome", "Exterior - Tiny Asset Pack/Houses/7.png"},
    {"houseCabin", "Exterior - Tiny Asset Pack/Houses/4.png"},
    {"houseSmith", "Exterior - Tiny Asset Pack/Houses/NPCS houses/Blacksmith.png"},
    {"well", "Exterior - Tiny Asset Pack/Well .png"},
    {"bonfire", "Exterior - Tiny Asset Pack/bonfire.png"},
    {"chest", "Exterior - Tiny Asset Pack/chest.png"},
    {"fence", "Exterior - Tiny Asset Pack/Fence and Bridge/Fence Wood.png"},
    {"pines", "Farm - Tiny Asse Pack/Tree/Common/Shadow/Pine Tree.png"},
    {"maples", "Farm - Tiny Asse Pack/Tree/Common/Shadow/Maple Tree.png"},
    {"stones", "Farm - Tiny Asse Pack/Props/Spring/Ground stones.png"},
    {"props", "Exterior - Tiny Asset Pack/Exterior.png"},

    // UI
    {"money", "UI - Tiny Asset Pack/Money.png"},
    {"bars", "UI - Tiny Asset Pack/Bars.png"},
};

// Locates the asset pack relative to the executable: inside the macOS app
// bundle, next to the executable, or in the project root when running from
// the build directory.
std::string findAssetRoot() {
    const char* baseRaw = SDL_GetBasePath();
    const std::string base = baseRaw ? baseRaw : "";

    const std::string candidates[] = {
        base + "../Resources/" + PACK_DIR,  // sdl3_game.app/Contents/Resources
        base + PACK_DIR,                    // pack next to the executable
        base + "../" + PACK_DIR,            // executable inside build/
    };
    for (const std::string& dir : candidates) {
        if (std::filesystem::exists(dir + "/" + MANIFEST[0].second)) return dir;
    }
    std::cerr << "Could not locate asset pack '" << PACK_DIR << "' near " << base << std::endl;
    return candidates[0];
}

}  // namespace

Assets::~Assets() {
    unload();
}

void Assets::unload() {
    for (auto& [key, tex] : textures) SDL_DestroyTexture(tex);
    textures.clear();
}

bool Assets::load(SDL_Renderer* renderer) {
    const std::string root = findAssetRoot();
    for (const auto& [key, rel] : MANIFEST) {
        const std::string path = root + "/" + rel;
        SDL_Texture* tex = IMG_LoadTexture(renderer, path.c_str());
        if (!tex) {
            std::cerr << "Failed to load " << path << ": " << SDL_GetError() << std::endl;
            return false;
        }
        SDL_SetTextureScaleMode(tex, SDL_SCALEMODE_NEAREST);
        textures.emplace(key, tex);
    }
    return true;
}

SDL_Texture* Assets::get(const std::string& key) const {
    auto it = textures.find(key);
    return it != textures.end() ? it->second : nullptr;
}

namespace spriteRects {

// Sheets have white silhouette rows at the bottom — rects stop above them.
const SDL_FRect PINE1{16 * 4, 16 * 0, 16 * 2, 16 * 3};
const SDL_FRect PINE2{16 * 6, 16 * 0, 16 * 2, 16 * 3};
const SDL_FRect MAPLE_GREEN{16 * 0, 16 * 3, 16 * 2, 16 * 4};
const SDL_FRect MAPLE_ORANGE{16 * 4, 16 * 3, 16 * 2, 16 * 4};
const SDL_FRect MAPLE_BIRCH{16 * 7, 16 * 3, 16 * 2, 16 * 4};
const SDL_FRect TREE_BIG_GREEN{16 * 0, 16 * 9, 16 * 2, 16 * 3};
const SDL_FRect TREE_PINK{16 * 0, 16 * 12, 16 * 2, 16 * 3};
const SDL_FRect TREE_TEAL{16 * 4, 16 * 9, 16 * 2, 16 * 3};

const SDL_FRect HOUSE_SMITH{16 * 5, 16 * 0, 16 * 9, 16 * 6};

const SDL_FRect WELL{16 * 0, 16 * 0, 16 * 2, 16 * 4};
const SDL_FRect CHEST_CLOSED{16 * 0, 16 * 2, 16 * 2, 16 * 1};
const SDL_FRect CHEST_OPEN{16 * 0, 16 * 3, 16 * 2, 16 * 1};

const SDL_FRect FENCE_POST{16 * 0, 16 * 1, 16 * 1, 16 * 1};
const SDL_FRect FENCE_H{16 * 2, 16 * 0, 16 * 1, 16 * 1};
const SDL_FRect FENCE_CORNER{16 * 0, 16 * 0, 16 * 1, 16 * 1};

const SDL_FRect TORCH{16 * 0, 16 * 9, 16 * 1, 16 * 1};

const SDL_FRect STALL_TOP{16 * 14, 16 * 4, 16 * 2, 16 * 1};
const SDL_FRect STALL_COUNTER{16 * 14, 16 * 5, 16 * 2, 16 * 2};

const SDL_FRect HEART_FULL{16 * 0, 16 * 0, 16 * 1, 16 * 1};
const SDL_FRect HEART_HALF{16 * 2, 16 * 0, 16 * 1, 16 * 1};
const SDL_FRect HEART_EMPTY{16 * 0, 16 * 1, 16 * 1, 16 * 1};
const SDL_FRect COIN{16 * 0, 16 * 0, 16 * 1, 16 * 1};

const SDL_FPoint STONES[8] = {{16 * 0, 16 * 0},  {16 * 1, 16 * 0},  {16 * 2, 16 * 0},  {16 * 3, 16 * 0},
                              {16 * 0, 16 * 1}, {16 * 1, 16 * 1}, {16 * 2, 16 * 1}, {16 * 3, 16 * 1}};
const SDL_FPoint FLOWERS[9] = {{16 * 0, 16 * 7},  {16 * 1, 16 * 7}, {16 * 2, 16 * 7}, {16 * 3, 16 * 7}, {16 * 4, 16 * 7},
                               {16 * 0, 16 * 8},  {16 * 1, 16 * 8}, {16 * 2, 16 * 8}, {16 * 3, 16 * 8}};

}  // namespace spriteRects
