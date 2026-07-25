#pragma once

#include <SDL3/SDL.h>

#include <memory>
#include <string>
#include <vector>

#include "Entities.h"
#include "World.h"

// Every map in the game. To add a new one:
//   1. add an entry to this enum,
//   2. write a builder in src/maps/ (copy Farm.cpp as a starting point),
//   3. register it in Game::buildMaps() (same order as this enum),
//   4. connect it to the other maps with MapExits on both sides.
enum class MapId { Village, Farm };

// Walking into `area` teleports the player to `entry` in map `target`.
struct MapExit {
    SDL_FRect area;
    MapId target;
    SDL_FPoint entry;
};

// One self-contained map: static geometry, its own entities and its exits.
// Inactive maps keep their entities in memory, so a map is exactly as you
// left it (dead enemies stay dead, opened chests stay open) when you return.
class Map {
public:
    MapId id = MapId::Village;
    std::string key;          // short name, used by the HV_MAP debug env var
    std::string name;         // flashed on screen when entering the map
    World world;
    SDL_FPoint playerStart{0, 0};  // where the player spawns if the game starts here
    bool hasRespawn = false;       // one map should set this: death respawn point
    SDL_FPoint respawn{0, 0};
    std::vector<std::unique_ptr<Entity>> enemies, npcs, animals, props, pickups;
    std::vector<MapExit> exits;
};

// Map builders — one file per map in src/maps/.
std::unique_ptr<Map> buildVillage(const Assets& assets);
std::unique_ptr<Map> buildFarm(const Assets& assets);
