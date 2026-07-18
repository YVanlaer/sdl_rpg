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
const SDL_FRect PINE1{16 * 4, 0, 16 * 2, 16 * 3};
const SDL_FRect PINE2{16 * 6, 0, 16 * 2, 16 * 3};
const SDL_FRect MAPLE_GREEN{0, 46, 36, 70};
const SDL_FRect MAPLE_ORANGE{68, 46, 30, 70};
const SDL_FRect MAPLE_BIRCH{108, 46, 36, 70};
const SDL_FRect TREE_BIG_GREEN{0, 144, 32, 48};
const SDL_FRect TREE_PINK{0, 192, 32, 48};
const SDL_FRect TREE_TEAL{64, 144, 32, 48};

const SDL_FRect HOUSE_SMITH{0, 0, 112, 106};

const SDL_FRect WELL{0, 0, 32, 64};
const SDL_FRect CHEST_CLOSED{6, 6, 20, 14};
const SDL_FRect CHEST_OPEN{6, 28, 20, 14};

const SDL_FRect FENCE_POST{0, 16, 16, 16};
const SDL_FRect FENCE_H{32, 0, 16, 16};
const SDL_FRect FENCE_CORNER{0, 0, 16, 16};

const SDL_FRect TORCH{0, 142, 16, 16};

const SDL_FRect STALL_TOP{230, 66, 28, 18};
const SDL_FRect STALL_COUNTER{230, 84, 28, 36};

const SDL_FRect HEART_FULL{0, 0, 16, 16};
const SDL_FRect HEART_HALF{32, 0, 16, 16};
const SDL_FRect HEART_EMPTY{0, 16, 16, 16};
const SDL_FRect COIN{0, 0, 16, 16};

const SDL_FPoint STONES[8] = {{0, 0},  {16, 0},  {32, 0},  {48, 0},
                              {0, 16}, {16, 16}, {32, 16}, {48, 16}};
const SDL_FPoint FLOWERS[9] = {{0, 106},  {16, 106}, {32, 106}, {48, 106}, {64, 106},
                               {0, 120},  {16, 120}, {32, 120}, {48, 120}};

}  // namespace spriteRects
