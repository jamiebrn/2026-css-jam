#pragma once

#include <cstdint>
#include <vector>

// What each map cell *is*, independent of which tilesheet draws it.
// The island is generated as Terrain values first; IslandTiles.h then turns
// each Terrain into a tile ID. That keeps all the "which tile is which"
// decisions in one table, so swapping tilesheets never touches the layout code.
enum class Terrain : std::uint8_t {
    DeepWater,
    ShallowWater,
    Sand,
    Grass,          // bright grass: crash site
    DarkGrass,      // forest floor
    Dirt,           // paths
    Cliff,          // coastal cliffs and the mountain body
    StoneFloor,
    DarkStoneFloor, // luminous room, arena inner ring
    StoneWall,
    Stairs,
    Door,
    Torch,
    Rune,
    Crystal,
    Pillar,
    BrokenPillar,
    Portal,
    Cobble,         // forest ruins paving
    Altar,          // puzzle objects
    Tree,
    PineTree,
    DeadTree,
    Bush,
    Rock,
    Log,
    Wreckage,       // plane pieces
    Debris,         // small rocks and broken objects
    Dock,
    COUNT
};

struct TilePos {
    int x = 0;
    int y = 0;
};

struct IslandLayout {
    int width = 0;
    int height = 0;
    std::vector<Terrain> cells;

    // Where the player starts (inside the plane crash site), in tiles.
    TilePos spawn;

    // Centres / entrances of each area, in tiles. Handy for placing NPCs,
    // triggers, puzzle logic and Chronos waypoints later.
    TilePos crashSite;
    TilePos mountainDoor;   // bottom of the stairs, puzzle 1
    TilePos mazeDoor;       // outside the maze entrance, puzzle 3
    TilePos mazeGoal;       // far end of the maze
    TilePos luminousRoom;   // word puzzle, room centre
    TilePos forestRuins;    // puzzle 2, altar
    TilePos arena;          // Chronos boss, portal
    TilePos dock;           // landward end of the pier

    Terrain At(int x, int y) const {
        if (x < 0 || y < 0 || x >= width || y >= height) {
            return Terrain::DeepWater;
        }
        return cells[static_cast<std::size_t>(y) * width + x];
    }
};

// Same seed gives the same island every run.
IslandLayout GenerateIslandLayout(int width, int height, std::uint32_t seed = 1234u);
