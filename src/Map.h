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
    void Unload();

    void Draw(Vector2 offset = {0.0f, 0.0f},
              bool showGrid = true) const;

    // Draws only the tiles that overlap `view` in world space.
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
    bool tilesetLoaded = false;
    std::vector<int> cells;

    int Index(int x, int y) const;
};
