# Harvest Vale — a tiny farm RPG (C++/SDL3)

A 2D top-down action RPG built with C++17 + SDL3, using the
Farm RPG Tiny Asset Pack (art: EmanuelleDev, emanuelledev.itch.io).

This is a port of the vanilla JS/HTML5 canvas version found at
`~/Documents/vibe/rpg_game`.

## Prerequisites

- A C++ compiler (like g++ or Clang)
- CMake (version 3.16 or higher)
- SDL3 and SDL3_image

```bash
brew install sdl3 sdl3_image
```

## Building and Running

```bash
mkdir build && cd build
cmake ..
make
open ./sdl3_game.app
```

Alternatively, run the executable directly:

```bash
./sdl3_game.app/Contents/MacOS/sdl3_game
```

## Controls

- WASD / arrows — move (Shift to run)
- Space / J — sword attack
- E — talk / open chests / close shop
- Q — drink potion
- M — mute
- Enter — start / advance dialogue

## Game

Talk to **Bram the blacksmith** (east of the plaza) to start the quest line:
clear the slimes from the eastern forest, then defeat the **golden slime**
in the north-east clearing. **Grub the merchant** (plaza stall) sells potions
and a heart container. Kills grant XP (level up: +max HP, +damage) and gold.
Death sends you back to the bonfire minus 15% gold.

## Code layout

- `src/Assets.*` — asset manifest, texture loading, hand-measured sprite rects
- `src/Sprites.*` — sprite/character-frame drawing helpers (rows = down/up/left,
  right = flipped)
- `src/Font.*` — 8x8 bitmap text renderer (public-domain font8x8)
- `src/Sfx.*` — procedural WebAudio-style tone synth for SFX
- `src/World.*` — one map's static geometry (ground, objects, colliders) plus the
  construction helpers map builders use (paths, borders, fences, trees, details)
- `src/Map.h` — `Map` (world + its entities + exits), the `MapId` enum, builder decls
- `src/maps/*` — one file per map (Village, Sunnybrook Farm): geometry + entities + exits
- `src/Entities.*` — Player, Slime/Goblin/Boss AI, NPCs, animals, chests, pickups
- `src/Game.*` — loop, input, camera, map switching, combat glue, dialogue/quests/shop,
  HUD, screens

## Maps

The game is a collection of self-contained maps (`src/maps/`). Each map owns its
static geometry (`World`), its entities (enemies, NPCs, animals, props, pickups)
and its exits. Only the active map updates and draws; inactive maps keep their
state, so a map is exactly as you left it when you return. Walking into a
`MapExit` area teleports the player to the exit's entry point in the target map.

To add a new map:

1. Add an entry to the `MapId` enum in `src/Map.h`.
2. Create `src/maps/MyMap.cpp` with a `buildMyMap(const Assets&)` builder
   (copy `Farm.cpp` as a starting point; declare it in `src/Map.h`) and add the
   file to `CMakeLists.txt`.
3. Register the builder in `Game::buildMaps()` (`src/Game.cpp`), in the same
   order as the `MapId` enum.
4. Connect it: add `MapExit`s on both sides (`area` = trigger zone in world
   pixels, `target`/`entry` = where the player goes). Use `World::addBorder`
   with a `BorderGap` if the exit sits at the map edge.

## Debug hooks (env vars)

- `HV_PLAY=1` — skip the title screen
- `HV_MAP=village|farm` — start on another map (`Map::key`)
- `HV_POS=x,y` — warp the player/camera to (x, y)
- `HV_ACT=dialog|shop` — trigger an action at start
- `HV_SCREENSHOT=/path/out.png` — save a screenshot and quit
  (after `HV_SCREENSHOT_FRAMES` frames, default 45)

Handy for headless verification:

```bash
SDL_VIDEODRIVER=dummy SDL_RENDER_DRIVER=software SDL_AUDIODRIVER=dummy \
  HV_PLAY=1 HV_SCREENSHOT=/tmp/shot.png ./sdl3_game.app/Contents/MacOS/sdl3_game
```
