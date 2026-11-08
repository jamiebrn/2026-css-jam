#pragma once

// IMPORTANT:
// These are PLACEHOLDER source-tile indices for assets/Tileset.png.
// Tile index 0 means the top-left 32x32 cell, index 1 the next cell, etc.
// Open your actual atlas and adjust these numbers to match its layout.
// If your atlas uses 16x16 cells, change the TileMap source tile size in main.cpp.

namespace TileID {
    constexpr int GRASS        = 0;
    constexpr int DARK_GRASS   = 1;
    constexpr int FLOWER_GRASS = 2;
    constexpr int DIRT         = 3;
    constexpr int STONE_PATH   = 4;
    constexpr int SAND         = 5;
    constexpr int WATER        = 6;
    constexpr int CLIFF        = 7;
    constexpr int STONE_FLOOR  = 8;
    constexpr int STONE_WALL   = 9;
    constexpr int MOUNTAIN     = 10;
    constexpr int TREE         = 11;
    constexpr int PINE_TREE    = 12;
    constexpr int ROCK         = 13;
    constexpr int RUIN         = 14;
    constexpr int RUNE         = 15;
    constexpr int TORCH        = 16;
    constexpr int BRIDGE       = 17;
    constexpr int WRECKAGE     = 18;
    constexpr int PURPLE_MAGIC = 19;
}
