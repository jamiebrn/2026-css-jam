#pragma once
#include "Map.h"

// Builds the complete 120 x 90 island layout.
// Call after constructing TileMap and before entering the game loop.
void BuildIsland2Map(TileMap& map);

// Tile ID at (x, y) of the layout built by BuildIsland2Map, or -1 if out of range.
int Island2TileAt(int x, int y);
