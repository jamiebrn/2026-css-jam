#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

#include "IslandLayout.h"
#include "Map.h"

// Which tile IDs draw each Terrain. IDs are read left-to-right, top-to-bottom
// from the tilesheet (0 = top-left), exactly like TileMap expects.
//
// Several IDs for one Terrain = variants; one is picked per cell, stable
// from frame to frame. An empty list = no tile (the cell shows the dark
// checker background until you assign one).
//
// FILL IN THE TODO ROWS once the tilesheet(s) are chosen. Only water, sand
// and grass are known (taken from the old generateIsland), and dirt/dark
// grass reuse sand/grass so the map is readable in the meantime.
struct IslandTileSet {
    std::array<std::vector<int>, static_cast<std::size_t>(Terrain::COUNT)> ids;

    std::vector<int>& operator[](Terrain t) { return ids[static_cast<std::size_t>(t)]; }
    const std::vector<int>& operator[](Terrain t) const { return ids[static_cast<std::size_t>(t)]; }
};

inline IslandTileSet DefaultIslandTileSet() {
    IslandTileSet s;

    // Known from the beach/island sheet.
    s[Terrain::DeepWater]    = {14};
    s[Terrain::ShallowWater] = {14};
    s[Terrain::Sand]         = {0, 1, 2};
    s[Terrain::Grass]        = {5, 6, 7};

    // Placeholders so the layout is visible before real tiles are chosen.
    s[Terrain::DarkGrass]    = {5, 6, 7};   // TODO dark grass (area 2)
    s[Terrain::Dirt]         = {0, 1, 2};   // TODO dirt path

    // TODO: everything below is unassigned.
    // Cliff, StoneFloor, DarkStoneFloor, StoneWall, Stairs, Door, Torch,
    // Rune, Crystal, Pillar, BrokenPillar, Portal, Cobble, Altar,
    // Tree, PineTree, DeadTree, Bush, Rock, Log, Wreckage, Debris, Dock
    return s;
}

// Copies a generated layout into the TileMap (single layer).
inline void ApplyIslandLayout(TileMap& map,
                              const IslandLayout& layout,
                              const IslandTileSet& tiles) {
    for (int y = 0; y < layout.height; ++y) {
        for (int x = 0; x < layout.width; ++x) {
            const std::vector<int>& options = tiles[layout.At(x, y)];
            if (options.empty()) {
                map.SetTile(x, y, -1);
                continue;
            }
            // Cheap position hash so variants look random but never flicker.
            const std::uint32_t h =
                static_cast<std::uint32_t>(x) * 73856093u ^
                static_cast<std::uint32_t>(y) * 19349663u;
            map.SetTile(x, y, options[(h >> 8) % options.size()]);
        }
    }
}
