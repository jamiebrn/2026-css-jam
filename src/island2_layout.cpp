#include "island2_layout.h"
#include "TilePalette.h"

#include <algorithm>
#include <cmath>
#include <vector>

namespace {
    constexpr int W = 120;
    constexpr int H = 90;

    // Mirror of every tile written, so game code can ask "what is at (x, y)?"
    // (for collision) without needing TileMap::GetTile.
    std::vector<int> g_tiles(W * H, -1);

    void Set(TileMap& map, int x, int y, int id) {
        if (x >= 0 && x < W && y >= 0 && y < H) g_tiles[y * W + x] = id;
        map.SetTile(x, y, id);
    }

    void FillRect(TileMap& map, int x, int y, int w, int h, int id) {
        for (int yy = y; yy < y + h; ++yy)
            for (int xx = x; xx < x + w; ++xx)
                Set(map, xx, yy, id);
    }

    void BorderRect(TileMap& map, int x, int y, int w, int h, int id) {
        for (int xx = x; xx < x + w; ++xx) {
            Set(map, xx, y, id);
            Set(map, xx, y + h - 1, id);
        }
        for (int yy = y; yy < y + h; ++yy) {
            Set(map, x, yy, id);
            Set(map, x + w - 1, yy, id);
        }
    }

    void Line(TileMap& map, int x0, int y0, int x1, int y1, int id, int radius = 1) {
        // Bresenham line, thickened by radius, for winding dirt/stone paths.
        int dx = std::abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
        int dy = -std::abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
        int error = dx + dy;

        while (true) {
            for (int oy = -radius; oy <= radius; ++oy)
                for (int ox = -radius; ox <= radius; ++ox)
                    if (ox * ox + oy * oy <= radius * radius + 1)
                        Set(map, x0 + ox, y0 + oy, id);

            if (x0 == x1 && y0 == y1) break;
            const int e2 = 2 * error;
            if (e2 >= dy) { error += dy; x0 += sx; }
            if (e2 <= dx) { error += dx; y0 += sy; }
        }
    }

    void Scatter(TileMap& map, int x0, int y0, int x1, int y1,
                 int tileID, int spacing, int seedOffset = 0) {
        // Deterministic decoration pattern. No random runtime dependency.
        for (int y = y0; y < y1; ++y) {
            for (int x = x0; x < x1; ++x) {
                const int hash = (x * 37 + y * 71 + seedOffset * 19) % 97;
                if (hash < 5 && x % spacing == 0 && y % spacing == 0)
                    Set(map, x, y, tileID);
            }
        }
    }

    bool InsideIsland(int x, int y) {
        const float nx = (x - 60.0f) / 52.0f;
        const float ny = (y - 45.0f) / 39.0f;
        float value = nx * nx + ny * ny;

        // Irregular coastline: small coves/notches.
        value += 0.045f * std::sin(x * 0.31f)
               + 0.035f * std::cos(y * 0.43f)
               + 0.025f * std::sin((x + y) * 0.22f);

        // A narrow inlet on the east and a small southern cove.
        if (x > 103 && y > 35 && y < 52) value += 0.18f;
        if (y > 79 && x > 45 && x < 72) value += 0.12f;

        return value < 1.0f;
    }

    void BuildCoastAndGround(TileMap& map) {
        for (int y = 0; y < H; ++y)
            for (int x = 0; x < W; ++x)
                Set(map, x, y, TileID::WATER);

        for (int y = 0; y < H; ++y) {
            for (int x = 0; x < W; ++x) {
                if (!InsideIsland(x, y)) continue;

                bool nearEdge = false;
                for (int oy = -1; oy <= 1; ++oy)
                    for (int ox = -1; ox <= 1; ++ox)
                        if (!InsideIsland(x + ox, y + oy))
                            nearEdge = true;

                Set(map, x, y, nearEdge ? TileID::SAND : TileID::GRASS);

                // A second darker band just inside the beach gives the shore depth.
                if (!nearEdge) {
                    bool nextToBeach = false;
                    for (int oy = -2; oy <= 2; ++oy)
                        for (int ox = -2; ox <= 2; ++ox)
                            if (InsideIsland(x + ox, y + oy)) {
                                // check whether any nearby cell is beach
                                for (int by = -1; by <= 1; ++by)
                                    for (int bx = -1; bx <= 1; ++bx)
                                        if (!InsideIsland(x + ox + bx, y + oy + by))
                                            nextToBeach = true;
                            }
                    if (nextToBeach) Set(map, x, y, TileID::DARK_GRASS);
                }
            }
        }
    }

    void BuildMainPaths(TileMap& map) {
        // Route order: crash -> forest trial -> mountain -> maze ->
        // forest ruins -> luminous chamber -> Chronos arena.
        Line(map, 24, 25, 39, 18, TileID::DIRT, 2);
        Line(map, 39, 18, 54, 22, TileID::DIRT, 2);
        Line(map, 54, 22, 65, 29, TileID::STONE_PATH, 2);
        Line(map, 65, 29, 79, 35, TileID::DIRT, 2);
        Line(map, 79, 35, 91, 39, TileID::STONE_PATH, 2);
        Line(map, 91, 39, 82, 52, TileID::DIRT, 2);
        Line(map, 82, 52, 64, 58, TileID::DIRT, 2);
        Line(map, 64, 58, 46, 65, TileID::STONE_PATH, 2);
        Line(map, 46, 65, 32, 66, TileID::DIRT, 2);
        Line(map, 46, 65, 64, 72, TileID::DIRT, 2);
        Line(map, 64, 72, 84, 70, TileID::STONE_PATH, 2);
        Line(map, 84, 70, 96, 66, TileID::STONE_PATH, 2);

        // Short access paths into each landmark.
        Line(map, 24, 25, 20, 31, TileID::DIRT, 1);
        Line(map, 65, 29, 65, 23, TileID::STONE_PATH, 1);
        Line(map, 91, 39, 92, 32, TileID::STONE_PATH, 1);
        Line(map, 64, 58, 64, 52, TileID::DIRT, 1);
        Line(map, 32, 66, 28, 61, TileID::STONE_PATH, 1);
        Line(map, 96, 66, 100, 72, TileID::STONE_PATH, 1);
    }

    void BuildArea1CrashSite(TileMap& map) {
        // 1. Plane crash / opening scene, west side.
        FillRect(map, 15, 20, 17, 11, TileID::DARK_GRASS);
        FillRect(map, 17, 22, 13, 7, TileID::DIRT);
        // A broken landing strip / scattered wreckage.
        Line(map, 16, 24, 29, 28, TileID::STONE_PATH, 1);
        Set(map, 19, 23, TileID::WRECKAGE);
        Set(map, 21, 25, TileID::WRECKAGE);
        Set(map, 25, 27, TileID::WRECKAGE);
        Set(map, 28, 23, TileID::ROCK);
        Set(map, 17, 28, TileID::ROCK);
        // Emergency camp / crates.
        Set(map, 14, 19, TileID::TORCH);
        Set(map, 31, 20, TileID::TORCH);
        Set(map, 29, 30, TileID::WRECKAGE);
    }

    void BuildArea2ForestTrial(TileMap& map) {
        // 2. Forest trail and first variation puzzle / rune clearing.
        FillRect(map, 43, 12, 15, 12, TileID::DARK_GRASS);
        FillRect(map, 47, 15, 8, 6, TileID::STONE_FLOOR);
        BorderRect(map, 46, 14, 10, 8, TileID::RUIN);
        Set(map, 51, 14, TileID::PURPLE_MAGIC);
        Set(map, 48, 16, TileID::RUNE);
        Set(map, 53, 16, TileID::RUNE);
        Set(map, 48, 20, TileID::TORCH);
        Set(map, 53, 20, TileID::TORCH);
        Scatter(map, 40, 10, 60, 26, TileID::PINE_TREE, 2, 4);
    }

    void BuildArea3Mountain(TileMap& map) {
        // 3. Mountain entrance and timed first puzzle, north-central.
        FillRect(map, 58, 15, 16, 19, TileID::CLIFF);
        FillRect(map, 61, 25, 10, 7, TileID::STONE_FLOOR);
        BorderRect(map, 60, 24, 12, 9, TileID::MOUNTAIN);
        // Entrance gap in the south wall.
        Set(map, 65, 32, TileID::STONE_FLOOR);
        Set(map, 65, 31, TileID::STONE_FLOOR);
        Set(map, 63, 25, TileID::TORCH);
        Set(map, 68, 25, TileID::TORCH);
        Set(map, 65, 26, TileID::PURPLE_MAGIC);
        // Rock outcrops around the mountain base.
        Set(map, 58, 29, TileID::ROCK);
        Set(map, 73, 28, TileID::ROCK);
        Set(map, 59, 20, TileID::ROCK);
        Set(map, 72, 19, TileID::ROCK);
        Scatter(map, 55, 13, 77, 35, TileID::PINE_TREE, 3, 9);
    }

    void BuildArea4Maze(TileMap& map) {
        // 4. Ancient maze on the east side. Openings make a traversable route.
        FillRect(map, 83, 29, 18, 17, TileID::STONE_FLOOR);
        BorderRect(map, 83, 29, 18, 17, TileID::STONE_WALL);

        // Maze walls; leave deliberate gaps in each run.
        FillRect(map, 86, 32, 1, 10, TileID::STONE_WALL);
        FillRect(map, 87, 32, 8, 1, TileID::STONE_WALL);
        FillRect(map, 90, 33, 1, 7, TileID::STONE_WALL);
        FillRect(map, 90, 39, 7, 1, TileID::STONE_WALL);
        FillRect(map, 96, 34, 1, 6, TileID::STONE_WALL);
        FillRect(map, 93, 34, 1, 3, TileID::STONE_WALL);
        FillRect(map, 87, 42, 10, 1, TileID::STONE_WALL);
        // Gate gaps / puzzle exit.
        Set(map, 92, 45, TileID::STONE_FLOOR);
        Set(map, 100, 38, TileID::STONE_FLOOR);
        Set(map, 86, 36, TileID::STONE_FLOOR);
        Set(map, 90, 36, TileID::RUNE);
        Set(map, 98, 43, TileID::PURPLE_MAGIC);
        Set(map, 92, 30, TileID::TORCH);
        Set(map, 98, 30, TileID::TORCH);
    }

    void BuildArea5ForestRuins(TileMap& map) {
        // 5. Forest ruins / second major puzzle, central-south.
        FillRect(map, 55, 49, 19, 15, TileID::DARK_GRASS);
        FillRect(map, 59, 52, 11, 9, TileID::STONE_FLOOR);
        BorderRect(map, 58, 51, 13, 11, TileID::RUIN);
        // Ruined central dais.
        FillRect(map, 62, 54, 5, 4, TileID::STONE_WALL);
        Set(map, 64, 56, TileID::PURPLE_MAGIC);
        Set(map, 60, 53, TileID::TORCH);
        Set(map, 69, 53, TileID::TORCH);
        Set(map, 60, 60, TileID::RUNE);
        Set(map, 69, 60, TileID::RUNE);
        Scatter(map, 52, 47, 77, 66, TileID::TREE, 2, 11);
        // Clear path through the forest so trees do not block the route.
        Line(map, 64, 58, 64, 64, TileID::DIRT, 1);
    }

    void BuildArea6LuminousRoom(TileMap& map) {
        // 6. Luminous chamber / word puzzle, south-west.
        FillRect(map, 21, 57, 17, 15, TileID::STONE_FLOOR);
        BorderRect(map, 21, 57, 17, 15, TileID::STONE_WALL);
        // Entrance on east side.
        Set(map, 37, 64, TileID::STONE_FLOOR);
        Set(map, 37, 65, TileID::STONE_FLOOR);
        // Central glowing glyph and four marker stones.
        Set(map, 29, 63, TileID::PURPLE_MAGIC);
        Set(map, 25, 61, TileID::RUNE);
        Set(map, 33, 61, TileID::RUNE);
        Set(map, 25, 68, TileID::RUNE);
        Set(map, 33, 68, TileID::RUNE);
        Set(map, 23, 59, TileID::TORCH);
        Set(map, 35, 59, TileID::TORCH);
        Set(map, 23, 70, TileID::TORCH);
        Set(map, 35, 70, TileID::TORCH);
    }

    void BuildArea7ChronosArena(TileMap& map) {
        // 7. Chronos arena / final boss, far south-east.
        FillRect(map, 87, 59, 19, 17, TileID::DARK_GRASS);
        FillRect(map, 90, 61, 13, 12, TileID::STONE_FLOOR);
        BorderRect(map, 89, 60, 15, 14, TileID::RUIN);
        // Arena dais, central time sigil, and four pillars.
        FillRect(map, 94, 64, 5, 5, TileID::STONE_WALL);
        Set(map, 96, 66, TileID::PURPLE_MAGIC);
        Set(map, 91, 62, TileID::TORCH);
        Set(map, 101, 62, TileID::TORCH);
        Set(map, 91, 71, TileID::TORCH);
        Set(map, 101, 71, TileID::TORCH);
        Set(map, 93, 63, TileID::RUIN);
        Set(map, 99, 63, TileID::RUIN);
        Set(map, 93, 70, TileID::RUIN);
        Set(map, 99, 70, TileID::RUIN);
        // Portal approach.
        Line(map, 96, 74, 96, 78, TileID::STONE_PATH, 1);
    }

    void AddDecorations(TileMap& map) {
        // Scatter rocks and sparse trees around the open woodland.
        Scatter(map, 10, 10, 42, 47, TileID::PINE_TREE, 3, 2);
        Scatter(map, 36, 27, 56, 47, TileID::TREE, 3, 3);
        Scatter(map, 72, 46, 85, 61, TileID::PINE_TREE, 2, 7);
        Scatter(map, 39, 68, 57, 78, TileID::ROCK, 3, 13);

        // Re-open the main route after placing decorative objects.
        BuildMainPaths(map);
        // Keep area floors and entrances readable by restoring their key paths.
        Line(map, 24, 25, 39, 18, TileID::DIRT, 2);
        Line(map, 39, 18, 54, 22, TileID::DIRT, 2);
        Line(map, 54, 22, 65, 29, TileID::STONE_PATH, 2);
        Line(map, 65, 29, 79, 35, TileID::DIRT, 2);
        Line(map, 79, 35, 91, 39, TileID::STONE_PATH, 2);
        Line(map, 91, 39, 82, 52, TileID::DIRT, 2);
        Line(map, 82, 52, 64, 58, TileID::DIRT, 2);
        Line(map, 64, 58, 46, 65, TileID::STONE_PATH, 2);
        Line(map, 46, 65, 32, 66, TileID::DIRT, 2);
        Line(map, 46, 65, 64, 72, TileID::DIRT, 2);
        Line(map, 64, 72, 84, 70, TileID::STONE_PATH, 2);
        Line(map, 84, 70, 96, 66, TileID::STONE_PATH, 2);
    }
}

void BuildIsland2Map(TileMap& map) {
    BuildCoastAndGround(map);
    BuildMainPaths(map);

    BuildArea1CrashSite(map);
    BuildArea2ForestTrial(map);
    BuildArea3Mountain(map);
    BuildArea4Maze(map);
    BuildArea5ForestRuins(map);
    BuildArea6LuminousRoom(map);
    BuildArea7ChronosArena(map);

    AddDecorations(map);
}

int Island2TileAt(int x, int y) {
    if (x < 0 || x >= W || y < 0 || y >= H) return -1;
    return g_tiles[y * W + x];
}
