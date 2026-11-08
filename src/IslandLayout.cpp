#include "IslandLayout.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <utility>

namespace {

constexpr float kPi = 3.14159265358979f;

std::uint32_t Hash(int x, int y, std::uint32_t seed) {
    std::uint32_t h = seed;
    h ^= static_cast<std::uint32_t>(x) * 0x9E3779B1u;
    h = (h << 13) | (h >> 19);
    h ^= static_cast<std::uint32_t>(y) * 0x85EBCA77u;
    h *= 0xC2B2AE3Du;
    h ^= h >> 15;
    h *= 0x27D4EB2Fu;
    h ^= h >> 13;
    return h;
}

float Rand01(int x, int y, std::uint32_t seed) {
    return static_cast<float>(Hash(x, y, seed) & 0xFFFFFFu) / 16777216.0f;
}

float Smooth(float t) { return t * t * (3.0f - 2.0f * t); }

float ValueNoise(float x, float y, std::uint32_t seed) {
    const int x0 = static_cast<int>(std::floor(x));
    const int y0 = static_cast<int>(std::floor(y));
    const float fx = Smooth(x - x0);
    const float fy = Smooth(y - y0);
    const float a = Rand01(x0, y0, seed);
    const float b = Rand01(x0 + 1, y0, seed);
    const float c = Rand01(x0, y0 + 1, seed);
    const float d = Rand01(x0 + 1, y0 + 1, seed);
    const float top = a + (b - a) * fx;
    const float bottom = c + (d - c) * fx;
    return top + (bottom - top) * fy;
}

class Builder {
public:
    Builder(int w, int h, std::uint32_t s)
        : W(w), H(h), seed(s), rng(s * 2654435761u + 1u) {
        out.width = w;
        out.height = h;
        out.cells.assign(static_cast<std::size_t>(w) * h, Terrain::DeepWater);
        depth.assign(static_cast<std::size_t>(w) * h, -99.0f);
        angle.assign(static_cast<std::size_t>(w) * h, 0.0f);
        locked.assign(static_cast<std::size_t>(w) * h, 0);
        clear.assign(static_cast<std::size_t>(w) * h, 0);
    }

    IslandLayout Build() {
        BuildCoast();

        // Areas first, so paths and scenery can route around them.
        Mountain();
        Maze();
        LuminousRoom();
        ForestRuins();
        Arena();
        CrashSite();
        Dock();

        Paths();
        Scatter();
        return out;
    }

private:
    int W, H;
    std::uint32_t seed;
    std::uint32_t rng;
    IslandLayout out;
    std::vector<float> depth;          // tiles inland from the shore, <= 0 is water
    std::vector<float> angle;          // angle from island centre
    std::vector<std::uint8_t> locked;  // part of an area: paths and scenery keep out
    std::vector<std::uint8_t> clear;   // no trees here, but paths may cross

    TilePos mountainCentre;

    // ---- helpers -----------------------------------------------------------

    std::size_t Idx(int x, int y) const { return static_cast<std::size_t>(y) * W + x; }
    bool In(int x, int y) const { return x >= 0 && y >= 0 && x < W && y < H; }

    TilePos P(float fx, float fy) const {
        return {static_cast<int>(fx * W), static_cast<int>(fy * H)};
    }

    std::uint32_t Next() {
        rng ^= rng << 13;
        rng ^= rng >> 17;
        rng ^= rng << 5;
        return rng;
    }

    void Put(int x, int y, Terrain t, bool lock = true) {
        if (!In(x, y)) return;
        out.cells[Idx(x, y)] = t;
        if (lock) locked[Idx(x, y)] = 1;
    }

    Terrain Get(int x, int y) const {
        return In(x, y) ? out.cells[Idx(x, y)] : Terrain::DeepWater;
    }

    bool IsGround(Terrain t) const {
        return t == Terrain::Grass || t == Terrain::DarkGrass || t == Terrain::Sand;
    }

    void ClearDisc(TilePos c, float rx, float ry) {
        for (int y = c.y - static_cast<int>(ry) - 1; y <= c.y + static_cast<int>(ry) + 1; ++y) {
            for (int x = c.x - static_cast<int>(rx) - 1; x <= c.x + static_cast<int>(rx) + 1; ++x) {
                if (!In(x, y)) continue;
                const float dx = (x - c.x) / rx;
                const float dy = (y - c.y) / ry;
                if (dx * dx + dy * dy <= 1.0f) clear[Idx(x, y)] = 1;
            }
        }
    }

    // ---- coastline ---------------------------------------------------------

    void BuildCoast() {
        const float cx = W * 0.50f;
        const float cy = H * 0.54f;
        const float rx = W * 0.40f;
        const float ry = H * 0.37f;
        const float scale = std::min(rx, ry);

        for (int y = 0; y < H; ++y) {
            for (int x = 0; x < W; ++x) {
                const float dx = (x + 0.5f - cx) / rx;
                const float dy = (y + 0.5f - cy) / ry;
                const float r = std::sqrt(dx * dx + dy * dy);
                const float a = std::atan2(dy, dx);

                float wobble = 1.0f
                    + 0.07f * std::sin(3.0f * a + 1.3f)
                    + 0.05f * std::sin(5.0f * a + 4.1f)
                    + 0.03f * std::sin(9.0f * a + 2.2f)
                    + 0.02f * std::sin(14.0f * a + 0.7f);

                // Bulge to the north for the mountain.
                const float da = a + kPi / 2.0f;
                wobble += 0.16f * std::exp(-da * da / 0.35f);

                float d = (wobble - r) * scale;
                // Keep a strip of ocean around the map edge.
                if (x < 2 || y < 2 || x >= W - 2 || y >= H - 2) d = -50.0f;

                depth[Idx(x, y)] = d;
                angle[Idx(x, y)] = a;
            }
        }

        const auto inSector = [](float a, float lo, float hi) { return a >= lo && a <= hi; };

        for (int y = 0; y < H; ++y) {
            for (int x = 0; x < W; ++x) {
                const std::size_t i = Idx(x, y);
                const float d = depth[i];
                const float a = angle[i];

                if (d <= 0.0f) {
                    out.cells[i] = (d > -4.0f) ? Terrain::ShallowWater : Terrain::DeepWater;
                    continue;
                }

                const float n = ValueNoise(x / 4.0f, y / 4.0f, seed + 3);
                const bool cliffSide =
                    inSector(a, kPi * 0.58f, kPi * 0.85f) ||   // south-west
                    inSector(a, kPi * 0.15f, kPi * 0.42f);     // south-east

                const float sandWidth = cliffSide ? 1.5f : (2.2f + 1.6f * n);

                if (d < sandWidth) {
                    out.cells[i] = Terrain::Sand;
                } else if (cliffSide && d < 4.2f + n) {
                    out.cells[i] = Terrain::Cliff;
                } else {
                    out.cells[i] = Terrain::DarkGrass;
                }
            }
        }
    }

    // ---- area 3: mountain entrance (puzzle 1) -------------------------------

    void Mountain() {
        const TilePos door = P(0.50f, 0.27f);
        const int topY = std::max(4, static_cast<int>(H * 0.08f));
        const int baseY = door.y;
        mountainCentre = {door.x, (topY + baseY) / 2};

        for (int y = topY; y <= baseY; ++y) {
            const float t = static_cast<float>(y - topY) / std::max(1, baseY - topY);
            for (int x = door.x - 14; x <= door.x + 14; ++x) {
                const float jitter = (Rand01(x / 2, y, seed + 5) - 0.5f) * 3.0f;
                const float half = 2.5f + t * 8.0f + jitter;
                if (std::fabs(static_cast<float>(x - door.x)) <= half) {
                    const bool faceWall = y >= baseY - 1 && std::abs(x - door.x) <= 6;
                    Put(x, y, faceWall ? Terrain::StoneWall : Terrain::Cliff);
                }
            }
        }

        // Stone platform in front of the cave.
        for (int y = baseY + 1; y <= baseY + 2; ++y) {
            for (int x = door.x - 6; x <= door.x + 6; ++x) {
                Put(x, y, Terrain::StoneFloor);
            }
        }

        // Cave door, torches and runes.
        for (int y = baseY - 1; y <= baseY; ++y) {
            for (int x = door.x - 1; x <= door.x + 1; ++x) {
                Put(x, y, Terrain::Door);
            }
        }
        Put(door.x - 3, baseY, Terrain::Torch);
        Put(door.x + 3, baseY, Terrain::Torch);
        Put(door.x - 5, baseY + 1, Terrain::Rune);
        Put(door.x + 5, baseY + 1, Terrain::Rune);

        // Stairs down from the platform.
        for (int y = baseY + 3; y <= baseY + 6; ++y) {
            for (int x = door.x - 1; x <= door.x + 1; ++x) {
                Put(x, y, Terrain::Stairs);
            }
        }
        out.mountainDoor = {door.x, baseY + 8};
    }

    // ---- area 4: maze (puzzle 3) --------------------------------------------

    void Maze() {
        const int cw = 8;
        const int ch = 6;
        const int mw = cw * 3 + 1;   // 2-tile corridors, 1-tile walls
        const int mh = ch * 3 + 1;
        const TilePos o = P(0.60f, 0.24f);

        for (int y = 0; y < mh; ++y) {
            for (int x = 0; x < mw; ++x) {
                Put(o.x + x, o.y + y, Terrain::StoneWall);
            }
        }

        const auto carveCell = [&](int i, int j, Terrain t) {
            for (int dy = 0; dy < 2; ++dy) {
                for (int dx = 0; dx < 2; ++dx) {
                    Put(o.x + 1 + 3 * i + dx, o.y + 1 + 3 * j + dy, t);
                }
            }
        };

        std::vector<std::uint8_t> visited(static_cast<std::size_t>(cw) * ch, 0);
        std::vector<std::pair<int, int>> stack;
        int bestDepth = -1;
        int goalI = 0, goalJ = 0;

        const int startI = cw / 2;
        const int startJ = ch - 1;
        visited[static_cast<std::size_t>(startJ) * cw + startI] = 1;
        carveCell(startI, startJ, Terrain::StoneFloor);
        stack.push_back({startI, startJ});

        const int di[4] = {1, -1, 0, 0};
        const int dj[4] = {0, 0, 1, -1};

        while (!stack.empty()) {
            const auto [i, j] = stack.back();
            int options[4];
            int count = 0;
            for (int k = 0; k < 4; ++k) {
                const int ni = i + di[k];
                const int nj = j + dj[k];
                if (ni < 0 || nj < 0 || ni >= cw || nj >= ch) continue;
                if (visited[static_cast<std::size_t>(nj) * cw + ni]) continue;
                options[count++] = k;
            }
            if (count == 0) {
                stack.pop_back();
                continue;
            }

            const int k = options[Next() % count];
            const int ni = i + di[k];
            const int nj = j + dj[k];
            visited[static_cast<std::size_t>(nj) * cw + ni] = 1;
            carveCell(ni, nj, Terrain::StoneFloor);

            // Knock out the wall between the two cells.
            if (k < 2) {
                const int wallX = 3 * (std::max(i, ni));
                Put(o.x + wallX, o.y + 1 + 3 * j, Terrain::StoneFloor);
                Put(o.x + wallX, o.y + 2 + 3 * j, Terrain::StoneFloor);
            } else {
                const int wallY = 3 * (std::max(j, nj));
                Put(o.x + 1 + 3 * i, o.y + wallY, Terrain::StoneFloor);
                Put(o.x + 2 + 3 * i, o.y + wallY, Terrain::StoneFloor);
            }

            stack.push_back({ni, nj});
            if (static_cast<int>(stack.size()) > bestDepth) {
                bestDepth = static_cast<int>(stack.size());
                goalI = ni;
                goalJ = nj;
            }
        }

        // Wall torches: only on wall tiles that have floor directly below.
        for (int y = 1; y < mh - 1; ++y) {
            for (int x = 1; x < mw - 1; ++x) {
                if (Get(o.x + x, o.y + y) == Terrain::StoneWall &&
                    Get(o.x + x, o.y + y + 1) == Terrain::StoneFloor &&
                    Rand01(x, y, seed + 17) < 0.07f) {
                    Put(o.x + x, o.y + y, Terrain::Torch);
                }
            }
        }

        // Goal: runes, with an altar tile at the far end.
        carveCell(goalI, goalJ, Terrain::Rune);
        Put(o.x + 1 + 3 * goalI, o.y + 1 + 3 * goalJ, Terrain::Altar);
        out.mazeGoal = {o.x + 1 + 3 * goalI, o.y + 1 + 3 * goalJ};

        // Entrance in the south wall, with torches and a short stair.
        const int doorX = o.x + 1 + 3 * startI;
        Put(doorX, o.y + mh - 1, Terrain::Door);
        Put(doorX + 1, o.y + mh - 1, Terrain::Door);
        Put(doorX - 1, o.y + mh - 1, Terrain::Torch);
        Put(doorX + 2, o.y + mh - 1, Terrain::Torch);
        for (int y = o.y + mh; y < o.y + mh + 3; ++y) {
            Put(doorX, y, Terrain::Stairs);
            Put(doorX + 1, y, Terrain::Stairs);
        }
        out.mazeDoor = {doorX, o.y + mh + 4};
    }

    // ---- area 5: luminous room (word puzzle) --------------------------------

    void LuminousRoom() {
        const TilePos c = P(0.27f, 0.58f);
        const int hw = 9;
        const int hh = 6;
        out.luminousRoom = c;

        for (int dy = -hh - 1; dy <= hh + 1; ++dy) {
            for (int dx = -hw - 1; dx <= hw + 1; ++dx) {
                const bool border = std::abs(dx) == hw + 1 || std::abs(dy) == hh + 1;
                Put(c.x + dx, c.y + dy, border ? Terrain::StoneWall : Terrain::DarkStoneFloor);
            }
        }

        // Doorway on the east side.
        for (int dy = -1; dy <= 1; ++dy) {
            Put(c.x + hw + 1, c.y + dy, Terrain::Door);
        }

        // Purple rune circle in the middle, crystals around it.
        for (int dy = -2; dy <= 2; ++dy) {
            for (int dx = -2; dx <= 2; ++dx) {
                if (dx * dx + dy * dy <= 5) Put(c.x + dx, c.y + dy, Terrain::Rune);
            }
        }
        const int crystalX[6] = {-5, 5, -4, 4, -4, 4};
        const int crystalY[6] = {0, 0, -3, -3, 3, 3};
        for (int k = 0; k < 6; ++k) {
            Put(c.x + crystalX[k], c.y + crystalY[k], Terrain::Crystal);
        }

        // Stone pillars and wall torches.
        for (int sx = -1; sx <= 1; sx += 2) {
            for (int sy = -1; sy <= 1; sy += 2) {
                Put(c.x + sx * 7, c.y + sy * 4, Terrain::Pillar);
            }
        }
        for (int dx = -6; dx <= 6; dx += 4) {
            Put(c.x + dx, c.y - hh - 1, Terrain::Torch);
            Put(c.x + dx, c.y + hh + 1, Terrain::Torch);
        }
    }

    // ---- area 6: forest ruins (puzzle 2) ------------------------------------

    void ForestRuins() {
        const TilePos c = P(0.52f, 0.66f);
        out.forestRuins = c;

        for (int dy = -11; dy <= 11; ++dy) {
            for (int dx = -11; dx <= 11; ++dx) {
                const float d = std::sqrt(static_cast<float>(dx * dx + dy * dy));
                const float edge = 9.5f + (Rand01(dx, dy, seed + 23) - 0.5f) * 2.0f;
                if (d <= edge && Get(c.x + dx, c.y + dy) != Terrain::DeepWater) {
                    Put(c.x + dx, c.y + dy, d <= 6.5f ? Terrain::Cobble : Terrain::Grass, d <= 6.5f);
                }
            }
        }
        ClearDisc(c, 10.5f, 10.5f);

        // Broken pillars in a ring, with a few chunks of fallen wall.
        for (int k = 0; k < 6; ++k) {
            const float a = k * kPi / 3.0f + 0.26f;
            Put(c.x + static_cast<int>(std::round(std::cos(a) * 6.0f)),
                c.y + static_cast<int>(std::round(std::sin(a) * 6.0f)),
                Terrain::BrokenPillar);
        }
        for (int k = 0; k < 4; ++k) {
            const float a = k * kPi / 2.0f + 0.9f;
            const int wx = c.x + static_cast<int>(std::round(std::cos(a) * 8.0f));
            const int wy = c.y + static_cast<int>(std::round(std::sin(a) * 8.0f));
            Put(wx, wy, Terrain::StoneWall);
            Put(wx + 1, wy, Terrain::StoneWall);
        }

        // Puzzle objects: central altar with four plates around it.
        Put(c.x, c.y, Terrain::Altar);
        Put(c.x - 3, c.y, Terrain::Rune);
        Put(c.x + 3, c.y, Terrain::Rune);
        Put(c.x, c.y - 3, Terrain::Rune);
        Put(c.x, c.y + 3, Terrain::Rune);
    }

    // ---- area 7: Chronos's arena (final boss) --------------------------------

    void Arena() {
        const TilePos c = P(0.75f, 0.64f);
        const float R = 10.5f;
        out.arena = c;

        // Openings: west (towards the hub) and towards the maze door.
        const float gapA = kPi;
        const float gapB = std::atan2(static_cast<float>(out.mazeDoor.y - c.y),
                                      static_cast<float>(out.mazeDoor.x - c.x));
        const auto inGap = [&](float a) {
            const float wrapA = std::fabs(std::fabs(a) - gapA);
            return wrapA < 0.30f || std::fabs(a - gapB) < 0.28f;
        };

        for (int dy = -12; dy <= 12; ++dy) {
            for (int dx = -12; dx <= 12; ++dx) {
                const float d = std::sqrt(static_cast<float>(dx * dx + dy * dy));
                const float a = std::atan2(static_cast<float>(dy), static_cast<float>(dx));
                const int x = c.x + dx;
                const int y = c.y + dy;

                if (d <= R - 1.0f) {
                    Terrain t = Terrain::StoneFloor;
                    if (d <= 1.6f) t = Terrain::Portal;
                    else if (d >= 4.0f && d <= 6.0f) t = Terrain::DarkStoneFloor;
                    Put(x, y, t);
                } else if (d <= R) {
                    Put(x, y, inGap(a) ? Terrain::StoneFloor : Terrain::StoneWall);
                }
            }
        }

        // Crystals around the portal, pillars near the wall, torches on it.
        for (int k = 0; k < 6; ++k) {
            const float a = k * kPi / 3.0f;
            Put(c.x + static_cast<int>(std::round(std::cos(a) * 3.2f)),
                c.y + static_cast<int>(std::round(std::sin(a) * 3.2f)),
                Terrain::Crystal);
        }
        for (int k = 0; k < 8; ++k) {
            const float a = k * kPi / 4.0f + kPi / 8.0f;
            if (inGap(std::atan2(std::sin(a), std::cos(a)))) continue;
            Put(c.x + static_cast<int>(std::round(std::cos(a) * 7.8f)),
                c.y + static_cast<int>(std::round(std::sin(a) * 7.8f)),
                Terrain::Pillar);
        }
        for (int k = 0; k < 8; ++k) {
            const float a = k * kPi / 4.0f;
            const float wrapped = std::atan2(std::sin(a), std::cos(a));
            if (inGap(wrapped)) continue;
            Put(c.x + static_cast<int>(std::round(std::cos(a) * (R - 0.4f))),
                c.y + static_cast<int>(std::round(std::sin(a) * (R - 0.4f))),
                Terrain::Torch);
        }
        ClearDisc(c, R + 2.0f, R + 2.0f);
    }

    // ---- area 1: plane crash site (spawn) ------------------------------------

    void CrashSite() {
        const TilePos c = P(0.25f, 0.37f);
        out.crashSite = c;

        // Bright grass patch with dirt scars, fading into the dark forest.
        for (int dy = -14; dy <= 14; ++dy) {
            for (int dx = -18; dx <= 18; ++dx) {
                const int x = c.x + dx;
                const int y = c.y + dy;
                if (!In(x, y) || locked[Idx(x, y)]) continue;
                const float nx = dx / 17.0f;
                const float ny = dy / 13.0f;
                const float edge = 1.0f + (ValueNoise(x / 3.0f, y / 3.0f, seed + 41) - 0.5f) * 0.4f;
                if (nx * nx + ny * ny > edge) continue;
                if (Get(x, y) == Terrain::DarkGrass) {
                    Put(x, y, Terrain::Grass, false);
                    if (ValueNoise(x / 2.5f, y / 2.5f, seed + 43) > 0.74f) {
                        Put(x, y, Terrain::Dirt, false);
                    }
                }
            }
        }
        ClearDisc(c, 11.0f, 9.0f);

        // Fuselage, wing and tail lying across the clearing.
        for (int dx = -8; dx <= -2; ++dx) {
            for (int dy = -1; dy <= 0; ++dy) Put(c.x + dx, c.y + dy, Terrain::Wreckage);
        }
        for (int dx = -6; dx <= -4; ++dx) {
            for (int dy = 1; dy <= 2; ++dy) Put(c.x + dx, c.y + dy, Terrain::Wreckage);
        }
        Put(c.x - 9, c.y - 2, Terrain::Wreckage);
        Put(c.x - 9, c.y - 1, Terrain::Wreckage);
        Put(c.x - 10, c.y - 3, Terrain::Wreckage);

        // Small rocks and broken objects scattered around the wreck.
        for (int dy = -8; dy <= 8; ++dy) {
            for (int dx = -11; dx <= 8; ++dx) {
                const int x = c.x + dx;
                const int y = c.y + dy;
                if (!In(x, y) || locked[Idx(x, y)] || !IsGround(Get(x, y))) continue;
                if (Get(x, y) == Terrain::Sand) continue;
                const float d = std::sqrt(static_cast<float>(dx * dx + dy * dy));
                if (d > 10.0f) continue;
                const float r = Rand01(x, y, seed + 47);
                if (r < 0.05f) Put(x, y, Terrain::Debris, true);
                else if (r < 0.065f) Put(x, y, Terrain::Rock, true);
            }
        }

        // Player starts just east of the fuselage.
        out.spawn = {c.x + 2, c.y + 2};
        Put(out.spawn.x, out.spawn.y, Terrain::Grass, true);
    }

    // ---- pier ----------------------------------------------------------------

    void Dock() {
        const int x = P(0.50f, 0.5f).x;
        int shoreY = static_cast<int>(H * 0.70f);
        while (shoreY < H - 2 && depth[Idx(x, shoreY)] > 0.0f) ++shoreY;

        for (int y = shoreY - 1; y <= std::min(H - 3, shoreY + 6); ++y) {
            Put(x, y, Terrain::Dock);
            Put(x + 1, y, Terrain::Dock);
        }
        out.dock = {x, std::max(0, shoreY - 4)};
    }

    // ---- paths ---------------------------------------------------------------

    void Path(TilePos a, TilePos b, float salt) {
        const float dx = static_cast<float>(b.x - a.x);
        const float dy = static_cast<float>(b.y - a.y);
        const float len = std::sqrt(dx * dx + dy * dy);
        if (len < 1.0f) return;
        const float px = -dy / len;
        const float py = dx / len;
        const int steps = static_cast<int>(len * 2.0f);

        for (int s = 0; s <= steps; ++s) {
            const float t = static_cast<float>(s) / steps;
            const float wobble = (std::sin(t * len * 0.17f + salt) * 1.6f
                               + std::sin(t * len * 0.06f + salt * 2.0f) * 2.2f)
                               * std::sin(kPi * t);
            const float fx = a.x + dx * t + px * wobble;
            const float fy = a.y + dy * t + py * wobble;
            const int cx = static_cast<int>(std::round(fx));
            const int cy = static_cast<int>(std::round(fy));

            for (int oy = -2; oy <= 2; ++oy) {
                for (int ox = -2; ox <= 2; ++ox) {
                    if (ox * ox + oy * oy > 2) continue;   // ~3 tiles wide
                    const int x = cx + ox;
                    const int y = cy + oy;
                    if (!In(x, y) || locked[Idx(x, y)]) continue;
                    if (IsGround(Get(x, y))) Put(x, y, Terrain::Dirt, false);
                }
            }
        }
    }

    void Paths() {
        const TilePos spawn = out.spawn;
        const TilePos hub1 = P(0.35f, 0.46f);
        const TilePos centre = P(0.50f, 0.50f);
        const TilePos ruinsNorth = {out.forestRuins.x, out.forestRuins.y - 9};
        const TilePos ruinsWest = {out.forestRuins.x - 9, out.forestRuins.y};
        const TilePos ruinsSouth = {out.forestRuins.x, out.forestRuins.y + 9};
        const TilePos roomDoor = {out.luminousRoom.x + 11, out.luminousRoom.y};
        const TilePos arenaWest = {out.arena.x - 13, out.arena.y};

        const float gapB = std::atan2(static_cast<float>(out.mazeDoor.y - out.arena.y),
                                      static_cast<float>(out.mazeDoor.x - out.arena.x));
        const TilePos arenaNW = {
            out.arena.x + static_cast<int>(std::round(std::cos(gapB) * 13.0f)),
            out.arena.y + static_cast<int>(std::round(std::sin(gapB) * 13.0f))
        };

        Path({spawn.x + 1, spawn.y}, hub1, 1.0f);
        Path(hub1, centre, 2.0f);
        Path(centre, out.mountainDoor, 3.0f);
        Path(centre, out.mazeDoor, 4.0f);
        Path(centre, ruinsNorth, 5.0f);
        Path(centre, arenaWest, 6.0f);
        Path(out.mazeDoor, arenaNW, 7.0f);
        Path(hub1, roomDoor, 8.0f);
        Path(roomDoor, ruinsWest, 9.0f);
        Path(ruinsSouth, out.dock, 10.0f);
    }

    // ---- trees, rocks, bushes -------------------------------------------------

    bool Near(TilePos c, float radius, int x, int y) const {
        const float dx = static_cast<float>(x - c.x);
        const float dy = static_cast<float>(y - c.y);
        return dx * dx + dy * dy <= radius * radius;
    }

    void Scatter() {
        for (int y = 0; y < H; ++y) {
            for (int x = 0; x < W; ++x) {
                const std::size_t i = Idx(x, y);
                const Terrain t = out.cells[i];
                if (locked[i] || clear[i]) continue;
                if (!IsGround(t)) continue;

                const float r = Rand01(x, y, seed + 21);
                const float pick = Rand01(x, y, seed + 31);

                if (t == Terrain::Sand) {
                    if (r < 0.012f) Put(x, y, Terrain::Rock, false);
                    continue;
                }

                const float n = ValueNoise(x / 5.0f, y / 5.0f, seed + 11);
                const bool bright = (t == Terrain::Grass);

                float density = bright ? 0.07f : 0.14f;
                if (!bright && Near(mountainCentre, 22.0f, x, y)) density = 0.22f;
                if (!bright && Near(out.forestRuins, 19.0f, x, y)) density = 0.22f;
                density *= 0.3f + 1.4f * n;

                if (r >= density) continue;

                Terrain pickT;
                if (bright) {
                    pickT = pick < 0.40f ? Terrain::Tree
                          : pick < 0.70f ? Terrain::Bush
                          : pick < 0.90f ? Terrain::Rock
                                         : Terrain::Debris;
                } else {
                    pickT = pick < 0.70f ? Terrain::PineTree
                          : pick < 0.78f ? Terrain::DeadTree
                          : pick < 0.90f ? Terrain::Bush
                          : pick < 0.96f ? Terrain::Rock
                                         : Terrain::Log;
                }
                Put(x, y, pickT, false);
            }
        }
    }
};

}  // namespace

IslandLayout GenerateIslandLayout(int width, int height, std::uint32_t seed) {
    Builder builder(width, height, seed);
    return builder.Build();
}
