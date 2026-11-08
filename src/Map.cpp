#include "Map.h"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <iostream>

TileMap::TileMap(int mapWidth,
                 int mapHeight,
                 int sourceTileSize,
                 int worldTileSize)
    : mapWidth(mapWidth),
      mapHeight(mapHeight),
      sourceTileSize(sourceTileSize),
      worldTileSize(worldTileSize),
      cells(mapWidth * mapHeight, -1) {
}

bool TileMap::LoadTileset(const std::string& imagePath) {
    if (tilesetLoaded) {
        UnloadTexture(tileset);
        tilesetLoaded = false;
    }

    tileset = LoadTexture(imagePath.c_str());

    if (tileset.id == 0) {
        std::cerr << "Could not load tileset: " << imagePath << '\n';
        return false;
    }

    tilesetLoaded = true;
    tilesetColumns = tileset.width / sourceTileSize;

    if (tilesetColumns <= 0) {
        std::cerr << "Tileset is smaller than the configured source tile size.\n";
        Unload();
        return false;
    }

    std::cout << "Loaded tileset: " << imagePath
              << " (" << tileset.width << "x" << tileset.height << ")"
              << ", " << tilesetColumns << " columns\n";
    return true;
}

bool TileMap::LoadPlaceholderTileset(const std::vector<Color>& colours) {
    if (colours.empty()) {
        return false;
    }

    if (tilesetLoaded) {
        UnloadTexture(tileset);
        tilesetLoaded = false;
    }

    const int count = static_cast<int>(colours.size());
    Image image = GenImageColor(count * sourceTileSize, sourceTileSize, BLANK);

    for (int i = 0; i < count; ++i) {
        ImageDrawRectangle(&image, i * sourceTileSize, 0,
                           sourceTileSize, sourceTileSize, colours[i]);

        // A faint edge on each tile so individual tiles can be told apart.
        const Color edge = {
            static_cast<unsigned char>(colours[i].r * 8 / 10),
            static_cast<unsigned char>(colours[i].g * 8 / 10),
            static_cast<unsigned char>(colours[i].b * 8 / 10),
            255
        };
        ImageDrawRectangle(&image, i * sourceTileSize, 0, sourceTileSize, 1, edge);
        ImageDrawRectangle(&image, i * sourceTileSize, 0, 1, sourceTileSize, edge);
    }

    tileset = LoadTextureFromImage(image);
    UnloadImage(image);

    if (tileset.id == 0) {
        std::cerr << "Could not create placeholder tileset.\n";
        return false;
    }

    tilesetLoaded = true;
    tilesetColumns = count;
    return true;
}

void TileMap::Unload() {
    if (tilesetLoaded) {
        UnloadTexture(tileset);
        tilesetLoaded = false;
    }
    tilesetColumns = 0;
}

int TileMap::Index(int x, int y) const {
    return y * mapWidth + x;
}

void TileMap::SetTile(int x, int y, int tileID) {
    if (x < 0 || y < 0 || x >= mapWidth || y >= mapHeight) {
        return;
    }
    cells[Index(x, y)] = tileID;
}

int TileMap::GetTile(int x, int y) const {
    if (x < 0 || y < 0 || x >= mapWidth || y >= mapHeight) {
        return -1;
    }
    return cells[Index(x, y)];
}

void TileMap::Fill(int tileID) {
    for (int& cell : cells) {
        cell = tileID;
    }
}

void TileMap::Draw(Vector2 offset, bool showGrid) const {
    const Rectangle wholeMap = {
        offset.x,
        offset.y,
        static_cast<float>(mapWidth * worldTileSize),
        static_cast<float>(mapHeight * worldTileSize)
    };

    DrawView(wholeMap, offset, showGrid);
}

void TileMap::DrawView(Rectangle view, Vector2 offset, bool showGrid) const {
    // Turn the visible rectangle into a range of tile coordinates, clamped to
    // the map, so only tiles that can actually be seen are drawn.
    const int firstX = std::max(
        0, static_cast<int>(std::floor((view.x - offset.x) / worldTileSize)));
    const int firstY = std::max(
        0, static_cast<int>(std::floor((view.y - offset.y) / worldTileSize)));
    const int lastX = std::min(
        mapWidth - 1,
        static_cast<int>(std::floor((view.x + view.width - offset.x) / worldTileSize)));
    const int lastY = std::min(
        mapHeight - 1,
        static_cast<int>(std::floor((view.y + view.height - offset.y) / worldTileSize)));

    if (firstX > lastX || firstY > lastY) {
        return;
    }

    for (int y = firstY; y <= lastY; ++y) {
        for (int x = firstX; x <= lastX; ++x) {
            const Rectangle destination = {
                offset.x + x * worldTileSize,
                offset.y + y * worldTileSize,
                static_cast<float>(worldTileSize),
                static_cast<float>(worldTileSize)
            };

            const Color checker = ((x + y) % 2 == 0)
                ? Color{24, 29, 43, 255}
                : Color{30, 35, 51, 255};

            DrawRectangleRec(destination, checker);

            const int tileID = cells[Index(x, y)];

            if (!tilesetLoaded || tileID < 0 || tilesetColumns <= 0) {
                continue;
            }

            const int sourceX = (tileID % tilesetColumns) * sourceTileSize;
            const int sourceY = (tileID / tilesetColumns) * sourceTileSize;

            // Ignore IDs that point outside the tilesheet.
            if (sourceX + sourceTileSize > tileset.width ||
                sourceY + sourceTileSize > tileset.height) {
                continue;
            }

            const Rectangle source = {
                static_cast<float>(sourceX),
                static_cast<float>(sourceY),
                static_cast<float>(sourceTileSize),
                static_cast<float>(sourceTileSize)
            };

            DrawTexturePro(
                tileset,
                source,
                destination,
                {0.0f, 0.0f},
                0.0f,
                tilesetTint
            );
        }
    }

    if (showGrid) {
        const Color gridColour{5, 8, 15, 100};

        // Grid lines only span the visible block of tiles.
        const int left   = static_cast<int>(offset.x + firstX * worldTileSize);
        const int right  = static_cast<int>(offset.x + (lastX + 1) * worldTileSize);
        const int top    = static_cast<int>(offset.y + firstY * worldTileSize);
        const int bottom = static_cast<int>(offset.y + (lastY + 1) * worldTileSize);

        for (int x = firstX; x <= lastX + 1; ++x) {
            const int screenX = static_cast<int>(offset.x + x * worldTileSize);
            DrawLine(screenX, top, screenX, bottom, gridColour);
        }

        for (int y = firstY; y <= lastY + 1; ++y) {
            const int screenY = static_cast<int>(offset.y + y * worldTileSize);
            DrawLine(left, screenY, right, screenY, gridColour);
        }
    }
}

void TileMap::PaintAt(Vector2 mouseScreenPosition,
                      Vector2 offset,
                      int tileID) {
    const int x = static_cast<int>(
        (mouseScreenPosition.x - offset.x) / worldTileSize
    );
    const int y = static_cast<int>(
        (mouseScreenPosition.y - offset.y) / worldTileSize
    );

    SetTile(x, y, tileID);
}

bool TileMap::Save(const std::string& filePath) const {
    std::ofstream file(filePath);

    if (!file.is_open()) {
        std::cerr << "Could not save map: " << filePath << '\n';
        return false;
    }

    file << mapWidth << ' ' << mapHeight << '\n';

    for (int y = 0; y < mapHeight; ++y) {
        for (int x = 0; x < mapWidth; ++x) {
            file << GetTile(x, y);
            if (x + 1 < mapWidth) file << ' ';
        }
        file << '\n';
    }

    return true;
}

bool TileMap::Load(const std::string& filePath) {
    std::ifstream file(filePath);

    if (!file.is_open()) {
        std::cerr << "Could not load map: " << filePath << '\n';
        return false;
    }

    int fileWidth = 0;
    int fileHeight = 0;
    file >> fileWidth >> fileHeight;

    if (!file || fileWidth != mapWidth || fileHeight != mapHeight) {
        std::cerr << "Map dimensions do not match this TileMap.\n";
        return false;
    }

    for (int y = 0; y < mapHeight; ++y) {
        for (int x = 0; x < mapWidth; ++x) {
            int tileID = -1;
            file >> tileID;

            if (!file) {
                std::cerr << "Map file ended unexpectedly.\n";
                return false;
            }

            SetTile(x, y, tileID);
        }
    }

    return true;
}