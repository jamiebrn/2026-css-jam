#pragma once

#include "raylib.h"
#include <string>
#include <vector>

// A small editable tile map for the island game.
// Tile IDs are read left-to-right, top-to-bottom from the tilesheet:
// 0 = top-left tile, 1 = next tile, etc.
// Use -1 for an empty map cell.
class TileMap {
public:
    TileMap(int mapWidth = 50,
            int mapHeight = 40,
            int sourceTileSize = 32,
            int worldTileSize = 32);

    bool LoadTileset(const std::string& imagePath);

    // Builds a tileset in code: one flat-colour tile per entry, in one row.
    // Tile ID n is colours[n]. For testing a layout before the real art exists.
    bool LoadPlaceholderTileset(const std::vector<Color>& colours);

    // Colour multiplied over every tile when drawing (WHITE = unchanged).
    void SetTint(Color newTint) { tilesetTint = newTint; }
    void Unload();

    void Draw(Vector2 offset = {0.0f, 0.0f},
              bool showGrid = true) const;

    // Draws only the tiles that overlap `view`, a rectangle in the same
    // coordinate space as `offset` (i.e. world space). Use this with a camera
    // so a big map doesn't draw thousands of off-screen tiles every frame.
    void DrawView(Rectangle view,
                  Vector2 offset = {0.0f, 0.0f},
                  bool showGrid = false) const;

    void SetTile(int x, int y, int tileID);
    int GetTile(int x, int y) const;
    void Fill(int tileID);

    // Paints the cell under the mouse. Pass the same offset used in Draw().
    void PaintAt(Vector2 mouseScreenPosition,
                 Vector2 offset,
                 int tileID);

    bool Save(const std::string& filePath) const;
    bool Load(const std::string& filePath);

    int GetWidth() const { return mapWidth; }
    int GetHeight() const { return mapHeight; }
    int GetWorldTileSize() const { return worldTileSize; }
    int GetTilesetColumns() const { return tilesetColumns; }

private:
    int mapWidth;
    int mapHeight;
    int sourceTileSize;
    int worldTileSize;
    int tilesetColumns = 0;

    Texture2D tileset{};
    Color tilesetTint{70, 85, 80, 255};
    bool tilesetLoaded = false;
    std::vector<int> cells;

    int Index(int x, int y) const;
};