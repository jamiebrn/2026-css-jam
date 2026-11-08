#ifdef __EMSCRIPTEN__
#include <emscripten/emscripten.h>
#endif
#include <raylib.h>
#include "chronos.h"
#include "lighting.h"
#include "Map.h"
#include "island_layout.h"
#include "animated_sprite.h"
#include "util.h"
#include "game.h"   // GameState, structs (+ defines.h: screen size, ScreenState)
#include "island2_layout.h"   // island 2 layout
#include "TilePalette.h"    // TileID::*

// Title + credits screens live in main_menu.cpp
void menuScreen(GameState* state, Vector2 mousePos);
void creditsScreen(GameState* state, Vector2 mousePos);

#include <cstdint>
#include <vector>
#include <cmath>
#include <algorithm>
#include <atomic>
#include <string>
#include <tuple>
#include <string>

Vector2 getGameMousePos() {
    Vector2 mousePos = GetMousePosition();

    int scrWidth = GetScreenWidth();
    int scrHeight = GetScreenHeight();

    int scale = std::max(1, std::min(scrWidth / SCREEN_WIDTH, scrHeight / SCREEN_HEIGHT));

    int scrBufferWidth = SCREEN_WIDTH * scale;
    int scrBufferHeight = SCREEN_HEIGHT * scale;

    const Rectangle gameRect = {
        (scrWidth - scrBufferWidth) / 2.0f,
        (scrHeight - scrBufferHeight) / 2.0f,
        (float)scrBufferWidth, (float)scrBufferHeight
    };

    if (!CheckCollisionPointRec(mousePos, gameRect)) {
        return (Vector2){-1.0f, -1.0f};
    }

    return (Vector2){
        (mousePos.x - gameRect.x) / scale,
        (mousePos.y - gameRect.y) / scale
    };
}

//UI Functions:
void drawTitle(const char* title){
	int titleFontSize=30;
	int titleWidth=MeasureText(title,titleFontSize);
	DrawText(title,(SCREEN_WIDTH-titleWidth)/2,20,titleFontSize,DARKGRAY);
}

TimeShot extract(const GameState& l){
    TimeShot t;
    t.currentLoop=l.currentLoop;
    t.timeRemaining=l.timeRemaining;
    t.player=l.player;
    t.chronos=l.chronos;
    t.testExit=l.testExit;
    t.mapOrigin=l.mapOrigin;

    return t;
}

void draw_button(const char* title, Rectangle button,bool overHover){
	DrawRectangleRec(button, overHover ? LIGHTGRAY : WHITE);
    DrawRectangleLines((int)button.x, (int)button.y, (int)button.width, (int)button.height, DARKGRAY);
    int playWidth = MeasureText(title, 20);
    DrawText(title, (int)(button.x + (button.width - playWidth) / 2), (int)(button.y + 15), 20, DARKGRAY);
}

void drawController(GameState* state,Vector2 mouse,float dt){
	Controller& c=state->controller;

	//Layout.
    const Rectangle body   = { 8, 62, 224, 132 };
    const Rectangle screen = { 18, 74, 124, 84 };
    const float cx = 188, cy = 128, s = 18, gap = 20;

    const Rectangle up    = { cx - s/2,       cy - s/2 - gap, s, s };
    const Rectangle down  = { cx - s/2,       cy - s/2 + gap, s, s };
    const Rectangle left  = { cx - s/2 - gap, cy - s/2,       s, s };
    const Rectangle right = { cx - s/2 + gap, cy - s/2,       s, s };
    const Rectangle mid   = { cx - s/2,       cy - s/2,       s, s };

    auto held = [&](int key, Rectangle r) {
        return IsKeyDown(key) ||
               (IsMouseButtonDown(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(mouse, r));
    };
    const float speed = 0.6f * dt;
    if (held(KEY_UP, up))       c.cursor.y -= speed;
    if (held(KEY_DOWN, down))   c.cursor.y += speed;
    if (held(KEY_LEFT, left))   c.cursor.x -= speed;
    if (held(KEY_RIGHT, right)) c.cursor.x += speed;
    c.cursor.x = std::clamp(c.cursor.x, 0.0f, 1.0f);
    c.cursor.y = std::clamp(c.cursor.y, 0.0f, 1.0f);


    DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, Color{ 0, 0, 0, 150 });

    DrawRectangleRounded({ body.x - 2, body.y - 2, body.width + 4, body.height + 4 },
                         0.15f, 6, BLACK);
    DrawRectangleRounded(body, 0.15f, 6, Color{ 52, 56, 60, 255 });

    // screen
    DrawRectangleRec({ screen.x - 2, screen.y - 2, screen.width + 4, screen.height + 4 }, BLACK);
    DrawRectangleRec(screen, Color{ 18, 34, 22, 255 });

	    auto P = [&](float u, float v) {
        return Vector2{ screen.x + u * screen.width, screen.y + v * screen.height };
    };
    const Color road = Color{ 0x23, 0x7b, 0x52, 255 };
    auto seg = [&](float u1, float v1, float u2, float v2) {
        DrawLineEx(P(u1, v1), P(u2, v2), 1.0f, road);
    };

    if (static_cast<int>(GetTime() * 3.0) % 2 == 0) {
        Vector2 p = P(state->player.x / SCREEN_WIDTH, state->player.y / SCREEN_HEIGHT);
        DrawCircleV(p, 2.0f, GREEN);
    }

    // cursor crosshair
    Vector2 k = P(c.cursor.x, c.cursor.y);
    DrawLineEx({ k.x - 4, k.y }, { k.x + 4, k.y }, 1.0f, RAYWHITE);
    DrawLineEx({ k.x, k.y - 4 }, { k.x, k.y + 4 }, 1.0f, RAYWHITE);

    auto button = [&](Rectangle r, const char* label) {
        bool hot = CheckCollisionPointRec(mouse, r) && IsMouseButtonDown(MOUSE_BUTTON_LEFT);
        DrawRectangleRec(r, hot ? Color{ 90, 96, 100, 255 } : Color{ 30, 33, 36, 255 });
        DrawRectangleLines((int)r.x, (int)r.y, (int)r.width, (int)r.height, LIGHTGRAY);
        int w = MeasureText(label, 8);
        DrawText(label, (int)(r.x + (r.width - w) / 2), (int)(r.y + 5), 8, RAYWHITE);
    };
    button(up, "^");  button(down, "v");
    button(left, "<"); button(right, ">");
    button(mid, "o");

    DrawText("[C] close", 20, 172, 7, Color{ 170, 220, 140, 255 });	
}

//Concurrency MVP.

using LogEntry=std::tuple<TimeShot, std::chrono::time_point<std::chrono::system_clock>>;
std::vector<LogEntry> timeLogger;
std::mutex stateMutex;
std::atomic<bool> loggerRunning{false};






//Threading function.
void loggerThread(GameState* state){
	using namespace std::chrono;
	auto next=steady_clock::now();

	while (loggerRunning){
		next+=seconds(1);
		{
			std::lock_guard<std::mutex> lock(stateMutex);
			timeLogger.emplace_back(extract(*state),system_clock::now());
			
		}
		std::this_thread::sleep_until(next);
		
	}
}





void menuLoop(void *data){
	GameState* state = static_cast<GameState*>(data);
	const Vector2 mousePos = getGameMousePos();

	Rectangle playButton = { (SCREEN_WIDTH - 200) / 2.0f, (SCREEN_HEIGHT / 2.0f) - 40.0f, 200.0f, 50.0f };
    Rectangle creditsButton = { (SCREEN_WIDTH - 200) / 2.0f, (SCREEN_HEIGHT / 2.0f) + 20.0f, 200.0f, 50.0f };

	bool playHovered = CheckCollisionPointRec(mousePos, playButton);
    bool creditsHovered = CheckCollisionPointRec(mousePos, creditsButton);

	if (playHovered && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
		state->currentScreen = SCREEN_SELECT;
    }
	if (creditsHovered && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        // Trigger transition to credits state
    }

	drawTitle("The Last Loop");
	draw_button("Play",playButton,playHovered);
	draw_button("Credits",creditsButton,creditsHovered);
}

void DrawSlantedQuad(float x, float y, float w, float h, float slant, Color color) {
    Vector2 v1 = { x + slant, y };
    Vector2 v2 = { x, y + h };
    Vector2 v3 = { x + w + slant, y };
    Vector2 v4 = { x + w, y + h };

    DrawTriangle(v1, v2, v3, color);
    DrawTriangle(v3, v2, v4, color);
}

void selectScreen(GameState* state, Vector2 mousePos) {
    if (IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_A))  state->selectedCharacter = 0;
    if (IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_D)) state->selectedCharacter = 1;

    if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE)) {
        state->dialogueIndex = 0;
        state->currentScreen = SCREEN_DIALOGUE;
    }

    Rectangle jamieHit = { 20.0f, 44.0f, 96.0f, 106.0f };
    Rectangle carolHit = { 124.0f, 44.0f, 96.0f, 106.0f };

    if (CheckCollisionPointRec(mousePos, jamieHit)) {
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            if (state->selectedCharacter == 0) {
                state->dialogueIndex = 0;
                state->currentScreen = SCREEN_DIALOGUE;
            } else {
                state->selectedCharacter = 0;
            }
        }
    }
    if (CheckCollisionPointRec(mousePos, carolHit)) {
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            if (state->selectedCharacter == 1) {
                state->dialogueIndex = 0;
                state->currentScreen = SCREEN_DIALOGUE;
            } else {
                state->selectedCharacter = 1;
            }
        }
    }

    float t = (float)GetTime();
    float jitterX = (std::sin(t * 35.0f) > 0.92f) ? 2.0f : 0.0f;
    float jitterY = (std::cos(t * 27.0f) < -0.94f) ? -1.0f : 0.0f;

    ClearBackground(Color{ 8, 12, 9, 255 });

    DrawSlantedQuad(-35.0f, 0.0f, 55.0f, (float)SCREEN_HEIGHT, 55.0f, Color{ 14, 24, 16, 255 });
    DrawSlantedQuad(115.0f, 0.0f, 75.0f, (float)SCREEN_HEIGHT, 48.0f, Color{ 16, 28, 20, 255 });

    float scanY = std::fmod(t * 36.0f, (float)SCREEN_HEIGHT);
    DrawRectangle(0, (int)scanY, SCREEN_WIDTH, 2, Color{ 0x23, 0x7b, 0x52, 45 });

    DrawSlantedQuad(12.0f + jitterX, 14.0f, 220.0f, 22.0f, -15.0f, Color{ 0x33, 0x65, 0x02, 90 });
    DrawSlantedQuad(10.0f, 12.0f, 220.0f, 22.0f, -15.0f, Color{ 16, 24, 19, 255 });
    DrawSlantedQuad(12.0f, 12.0f, 6.0f, 22.0f, -15.0f, Color{ 0x33, 0x65, 0x02, 255 });
    DrawSlantedQuad(18.0f, 12.0f, 2.0f, 22.0f, -15.0f, Color{ 0x23, 0x7b, 0x52, 255 });
    DrawText("Select your Character", 28 + (int)jitterX, 17, 10, Color{ 190, 230, 170, 255 });
    DrawText("Fugit Hora", 180, 18, 8, Color{ 0x23, 0x7b, 0x52, 255 });

    for (int i = 0; i < 2; i++) {
        bool selected = (state->selectedCharacter == i);
        float slant = (i == 0) ? -14.0f : -8.0f;
        float x = ((i == 0) ? 22.0f : 126.0f) + (selected ? jitterX : 0.0f);
        float y = (selected ? 41.0f : 47.0f) + (selected ? jitterY : 0.0f); 
        float w = 92.0f;
        float h = selected ? 109.0f : 99.0f;

        if (selected) {
            DrawSlantedQuad(x + 5.0f, y + 4.0f, w, h, slant - 2.0f, Color{ 0x33, 0x65, 0x02, 110 });
            DrawSlantedQuad(x - 3.0f, y - 2.0f, w, h, slant + 1.0f, Color{ 0x23, 0x7b, 0x52, 85 });
        } else {
            DrawSlantedQuad(x + 3.0f, y + 4.0f, w, h, slant, Color{ 3, 6, 4, 210 });
        }

        Color cardColor = selected ? Color{ 20, 32, 24, 255 } : Color{ 12, 17, 14, 255 };
        DrawSlantedQuad(x, y, w, h, slant, cardColor);

        Color accentColor = selected ? Color{ 0x33, 0x65, 0x02, 255 } : Color{ 0x23, 0x7b, 0x52, 120 };
        DrawSlantedQuad(x + 1.0f, y, w - 2.0f, 4.0f, slant, accentColor);
        DrawSlantedQuad(x, y + h - 2.0f, w, 2.0f, slant, selected ? Color{ 0x23, 0x7b, 0x52, 255 } : Color{ 25, 40, 30, 255 });

        const char* numTag = (i == 0) ? "01" : "02";
        DrawText(numTag, (int)(x + 14.0f), (int)(y + 8.0f), 10, selected ? Color{ 0x23, 0x7b, 0x52, 255 } : Color{ 40, 60, 48, 255 });

        float spriteSize = selected ? 38.0f : 28.0f;
        float spriteY = y + (selected ? 22.0f : 24.0f);
        float spriteX = x + (w - spriteSize) / 2.0f - 2.0f;

        if (selected) {
            DrawTexturePro(
                state->assets.players[i],
                { 0, 0, (float)PLAYER_RECT_SIZE, (float)PLAYER_RECT_SIZE },
                { spriteX + 2.0f, spriteY, spriteSize, spriteSize },
                { 0, 0 }, 0.0f, Color{ 0x33, 0x65, 0x02, 160 }
            );
        }

        DrawTexturePro(
            state->assets.players[i],
            { 0, 0, (float)PLAYER_RECT_SIZE, (float)PLAYER_RECT_SIZE },
            { spriteX, spriteY, spriteSize, spriteSize },
            { 0, 0 }, 0.0f, selected ? WHITE : Color{ 110, 140, 120, 190 }
        );

        const char* name = (i == 0) ? "JAMIE" : "CAROL";
        Color nameColor = selected ? Color{ 200, 240, 180, 255 } : Color{ 85, 115, 95, 255 };
        
        if (selected) {
            DrawSlantedQuad(x + 8.0f, y + h - 26.0f, w - 16.0f, 16.0f, -6.0f, Color{ 0x33, 0x65, 0x02, 255 });
            int nw = MeasureText(name, 10);
            DrawText(name, (int)(x + (w - nw) / 2.0f), (int)(y + h - 23.0f), 10, nameColor);
        } else {
            int nw = MeasureText(name, 9);
            DrawText(name, (int)(x + (w - nw) / 2.0f), (int)(y + h - 20.0f), 9, nameColor);
        }
    }

    float descY = 162.0f;
    float descH = 50.0f;

    DrawSlantedQuad(18.0f - jitterX, descY + 4.0f, 210.0f, descH, -13.0f, Color{ 0x33, 0x65, 0x02, 80 });
    DrawSlantedQuad(14.0f, descY, 212.0f, descH, -11.0f, Color{ 13, 20, 15, 255 });
    DrawSlantedQuad(12.0f, descY, 5.0f, descH, -11.0f, Color{ 0x33, 0x65, 0x02, 255 });
    DrawSlantedQuad(17.0f, descY, 2.0f, descH, -11.0f, Color{ 0x23, 0x7b, 0x52, 255 });

    if (state->selectedCharacter == 0) {
        DrawText("2nd Year Uni Student", 26 + (int)jitterX, (int)descY + 6, 8, Color{ 150, 215, 110, 255 });
        DrawText("Ability", 26, (int)descY + 18, 8, Color{ 0x23, 0x7b, 0x52, 255 });
        DrawText(": Radar ping detects clues", 66, (int)descY + 18, 8, Color{ 165, 190, 175, 255 });
        DrawText("Trait", 26, (int)descY + 30, 8, Color{ 0x33, 0x65, 0x02, 255 });
        DrawText(": Lower field of vision", 66, (int)descY + 30, 8, Color{ 120, 145, 130, 255 });
    } else {
        DrawText("2nd Year Uni Student", 26 + (int)jitterX, (int)descY + 6, 8, Color{ 150, 215, 110, 255 });
        DrawText("Ability", 26, (int)descY + 18, 8, Color{ 0x23, 0x7b, 0x52, 255 });
        DrawText(": Able to sprint for period", 66, (int)descY + 18, 8, Color{ 165, 190, 175, 255 });
        DrawText("Trait", 26, (int)descY + 30, 8, Color{ 0x33, 0x65, 0x02, 255 });
        DrawText(": Lower regular speed", 66, (int)descY + 30, 8, Color{ 120, 145, 130, 255 });
    }

    DrawText("Left/Right", 12, 222, 7, Color{ 0x23, 0x7b, 0x52, 200 });
    DrawText("[ENTER] Start", 140, 222, 7, Color{ 170, 220, 140, 255 });
}

void dialogueScreen(GameState* state) {
 const char* lines[][2] = {
    { "GBP 1,000 to sit on a research",   "flight. Rent's due. I said yes." },
    { "Three other students. Strangers.", "Then the plane hit the ground." },
    { "No broken bones, just a dark",     "island under a full moon..." },
    { "Something is watching us.",        "I need to find a way off." },
};
const int totalLines = 4;

    if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE) || IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        state->dialogueIndex++;
        if (state->dialogueIndex >= totalLines) {
            state->currentScreen = SCREEN_GAME;
            return;
        }
    }

    ClearBackground(BLACK);

    Rectangle box = { 12.0f, 164.0f, SCREEN_WIDTH - 24.0f, 64.0f };
    DrawRectangleRec(box, Color{ 10, 14, 11, 255 });
    DrawRectangleLinesEx(box, 1.0f, Color{ 0x23, 0x7b, 0x52, 255 });

    const char* speaker = (state->selectedCharacter == 0) ? "JAMIE" : "CAROL";
    DrawText(speaker, (int)box.x + 8, (int)box.y + 6, 8, Color{ 0x33, 0x65, 0x02, 255 });

    DrawText(lines[state->dialogueIndex][0], (int)box.x + 8, (int)box.y + 20, 8, RAYWHITE);
    DrawText(lines[state->dialogueIndex][1], (int)box.x + 8, (int)box.y + 32, 8, RAYWHITE);

    if (static_cast<int>(GetTime() * 2.0) % 2 == 0) {
        DrawText("[ENTER]", (int)(box.x + box.width - 42), (int)(box.y + box.height - 12), 7, Color{ 0x23, 0x7b, 0x52, 255 });
    }
}

// The playable area (the whole tile map) in world coordinates.
Rectangle getWorldBounds(const GameState* state) {
    return {
        state->mapOrigin.x,
        state->mapOrigin.y,
        (float)(state->islandMap.GetWidth() * state->islandMap.GetWorldTileSize()),
        (float)(state->islandMap.GetHeight() * state->islandMap.GetWorldTileSize())
    };
}

// Keeps the player inside the map and stops them dead against the edge.
void clampPlayerToWorld(GameState* state) {
    const Rectangle bounds = getWorldBounds(state);
    const float half = PLAYER_RECT_SIZE * 0.5f;
    Player& p = state->player;

    if (p.x < bounds.x + half)                { p.x = bounds.x + half;                p.velx = 0.0f; }
    if (p.x > bounds.x + bounds.width - half) { p.x = bounds.x + bounds.width - half; p.velx = 0.0f; }
    if (p.y < bounds.y + half)                { p.y = bounds.y + half;                p.vely = 0.0f; }
    if (p.y > bounds.y + bounds.height - half){ p.y = bounds.y + bounds.height - half;p.vely = 0.0f; }
}

// Placeholder art: each terrain character in island_layout.cpp is drawn as a
// flat colour. A character's tile ID is its position in this list, so when
// the real spritesheet arrives, this becomes the table of symbol -> tile ID.
struct TerrainColour {
    char symbol;
    Color colour;
};

static const TerrainColour TERRAIN[] = {
    {'~', {8, 14, 40, 255}},     // deep water
    {',', {26, 56, 104, 255}},   // shallow water
    {'s', {196, 170, 120, 255}}, // sand
    {'c', {96, 72, 116, 255}},   // cliff
    {'g', {62, 104, 56, 255}},   // grass
    {'f', {30, 64, 44, 255}},    // dark grass
    {'T', {14, 44, 30, 255}},    // tree
    {'p', {156, 120, 78, 255}},  // dirt path
    {'k', {120, 120, 130, 255}}, // rock
    {'M', {92, 70, 126, 255}},   // mountain
    {'o', {112, 112, 128, 255}}, // stone floor
    {'q', {84, 104, 82, 255}},   // mossy cobble
    {'W', {54, 54, 76, 255}},    // stone wall
    {'S', {176, 166, 150, 255}}, // stairs
    {'E', {255, 190, 80, 255}},  // door / cave mouth
    {'l', {255, 214, 60, 255}},  // torch / candle
    {'r', {176, 96, 255, 255}},  // rune / portal
    {'i', {160, 160, 180, 255}}, // pillar
    {'b', {124, 88, 52, 255}},   // wooden dock
    {'B', {170, 116, 72, 255}},  // boat
    {'x', {214, 214, 220, 255}}, // plane wreck
    {'d', {132, 112, 92, 255}},  // debris
};

static int terrainTileId(char symbol) {
    for (size_t i = 0; i < sizeof(TERRAIN) / sizeof(TERRAIN[0]); ++i) {
        if (TERRAIN[i].symbol == symbol) {
            return static_cast<int>(i);
        }
    }
    return -1; // unknown character: leave the cell empty
}

// Fills the map from ISLAND_LAYOUT and gives it the placeholder tileset.
void loadIslandMap(TileMap& map) {
    std::vector<Color> colours;
    for (const TerrainColour& terrain : TERRAIN) {
        colours.push_back(terrain.colour);
    }

    map.LoadPlaceholderTileset(colours);
    map.SetTint({175, 180, 185, 255});

    for (int y = 0; y < ISLAND_HEIGHT; ++y) {
        for (int x = 0; x < ISLAND_WIDTH; ++x) {
            map.SetTile(x, y, terrainTileId(ISLAND_LAYOUT[y][x]));
        }
    }
}
// ---------------------------------------------------------------------------
// Island 2 (reached through the portal)
// ---------------------------------------------------------------------------
// The layout comes from IslandLayout.cpp. Its map origin is (0, 0), so tile
// (tx, ty) has its centre at world (tx * 16 + 8, ty * 16 + 8).
static const int ISLAND2_W = 120;
static const int ISLAND2_H = 90;
static const float ISLAND2_LOOP_TIME = 90.0f;   // seconds per loop on island 2

static const float CHRONOS_CHASE_SPEED = 28.0f;            // px/s
static const float CHRONOS_CHASE_SPEED_DANGEROUS = 46.0f;  // last 15 s of a loop
static const float YELLOW_SHADOW_SPEED = 30.0f;

// Where things are on island 2 (tile coordinates, checked to be reachable).
static const int I2_SPAWN_X = 22,  I2_SPAWN_Y = 25;   // crash site
static const int I2_ITEM_X  = 29,  I2_ITEM_Y  = 63;   // luminous chamber
static const int I2_EXIT_X  = 96,  I2_EXIT_Y  = 71;   // Chronos arena
static const int I2_CHRONOS_X = 14, I2_CHRONOS_Y = 19;
static const int I2_SHADOW_X  = 58, I2_SHADOW_Y  = 40;

static Vector2 islandTileCentre(int tx, int ty) {
    return { tx * (float)TILE_SIZE + TILE_SIZE / 2.0f,
             ty * (float)TILE_SIZE + TILE_SIZE / 2.0f };
}

// Colour for each TileID in TilePalette.h (placeholder art, same idea as TERRAIN).
static const Color ISLAND2_COLOURS[] = {
    {62, 104, 56, 255},    // GRASS
    {30, 64, 44, 255},     // DARK_GRASS
    {90, 124, 70, 255},    // FLOWER_GRASS
    {156, 120, 78, 255},   // DIRT
    {132, 130, 136, 255},  // STONE_PATH
    {196, 170, 120, 255},  // SAND
    {26, 56, 104, 255},    // WATER
    {96, 72, 116, 255},    // CLIFF
    {112, 112, 128, 255},  // STONE_FLOOR
    {54, 54, 76, 255},     // STONE_WALL
    {92, 70, 126, 255},    // MOUNTAIN
    {14, 44, 30, 255},     // TREE
    {18, 52, 40, 255},     // PINE_TREE
    {120, 120, 130, 255},  // ROCK
    {84, 84, 108, 255},    // RUIN
    {176, 96, 255, 255},   // RUNE
    {255, 214, 60, 255},   // TORCH
    {124, 88, 52, 255},    // BRIDGE
    {214, 214, 220, 255},  // WRECKAGE
    {190, 110, 255, 255},  // PURPLE_MAGIC
};

static bool isSolidTileId(int id) {
    switch (id) {
        case -1:                // outside the map
        case TileID::WATER:
        case TileID::CLIFF:
        case TileID::STONE_WALL:
        case TileID::MOUNTAIN:
        case TileID::TREE:
        case TileID::PINE_TREE:
        case TileID::ROCK:
        case TileID::RUIN:
        case TileID::WRECKAGE:
            return true;
        default:
            return false;
    }
}

// True if a small box centred on (cx, cy) overlaps a solid island-2 tile.
// The box is smaller than a tile (10 px) so 1-tile gaps in the maze are passable.
static bool island2Blocked(float cx, float cy) {
    const float half = 5.0f;
    for (int sy = -1; sy <= 1; sy += 2) {
        for (int sx = -1; sx <= 1; sx += 2) {
            const int tx = (int)std::floor((cx + sx * half) / TILE_SIZE);
            const int ty = (int)std::floor((cy + sy * half) / TILE_SIZE);
            if (isSolidTileId(Island2TileAt(tx, ty))) return true;
        }
    }
    return false;
}

static void buildIsland2Map(TileMap& map) {
    std::vector<Color> colours(ISLAND2_COLOURS,
                               ISLAND2_COLOURS + sizeof(ISLAND2_COLOURS) / sizeof(ISLAND2_COLOURS[0]));
    map.LoadPlaceholderTileset(colours);
    map.SetTint({175, 180, 185, 255});
    BuildIsland2Map(map);
}

static const char* LOOP_GOAL[] = {
    "Learn Chronos's route", "Find the clue", "Escape before he hunts"
};

// Puts the player, Chronos, the shadows and the item back at their starting
// spots for the current island. Used on restart and at the start of each loop.
static void respawnForLoop(GameState* state) {
    state->player.velx = 0.0f;
    state->player.vely = 0.0f;
    state->testItem.collected = false;
    state->testExit.unlocked = false;

    if (state->currentIsland == 1) {
        state->player.x = SCREEN_WIDTH / 2.0f;
        state->player.y = SCREEN_HEIGHT / 2.0f;
        state->chronos.position = state->chronos.waypointA;
        state->chronos.movingToB = true;
        if (!state->shadowDefeated) {
            state->patroller.position = {40.0f, 40.0f};
        }
    } else {
        const Vector2 spawn = islandTileCentre(I2_SPAWN_X, I2_SPAWN_Y);
        state->player.x = spawn.x;
        state->player.y = spawn.y;
        state->chronos.position = islandTileCentre(I2_CHRONOS_X, I2_CHRONOS_Y);
        if (!state->yellowShadowDefeated) {
            state->yellowShadow.position = islandTileCentre(I2_SHADOW_X, I2_SHADOW_Y);
        }
    }
}

// Swaps the world over to island 2 and starts a fresh run of 3 loops there.
static void enterIsland2(GameState* state) {
    TraceLog(LOG_WARNING, "I2 item tile=%d solid=%d | exit tile=%d solid=%d",
    Island2TileAt(I2_ITEM_X, I2_ITEM_Y), isSolidTileId(Island2TileAt(I2_ITEM_X, I2_ITEM_Y)),
    Island2TileAt(I2_EXIT_X, I2_EXIT_Y), isSolidTileId(Island2TileAt(I2_EXIT_X, I2_EXIT_Y)));
    state->islandMap.Unload();
    state->islandMap = TileMap(ISLAND2_W, ISLAND2_H, TILE_SIZE, TILE_SIZE);
    buildIsland2Map(state->islandMap);

    state->currentIsland = 2;
    state->mapOrigin = {0.0f, 0.0f};
    state->lightingMap = LightingMap(ISLAND2_W * TILE_SIZE, ISLAND2_H * TILE_SIZE);

    state->loopDuration = ISLAND2_LOOP_TIME;
    state->currentLoop = 1;
    state->timeRemaining = state->loopDuration;
    state->gameOver = false;
    state->victory = false;
    state->gameOverReason = GAME_OVER_NONE;
    state->chronosDangerous = false;
    state->chronosHitCooldown = 0.0f;
    state->playerHitFlash = 0.0f;
    state->playerShakeTimer = 0.0f;

    state->testItem.position = islandTileCentre(I2_ITEM_X, I2_ITEM_Y);
    state->testExit.position = islandTileCentre(I2_EXIT_X, I2_EXIT_Y);

    state->yellowShadow.speed = YELLOW_SHADOW_SPEED;
    state->yellowShadowDefeated = false;

    respawnForLoop(state);
}

void portalScreen(GameState* state, float deltaTime) {
    const float duration = 3.0f;
    state->portalTimer += deltaTime;
    const float t = state->portalTimer;
    const float grow = std::min(1.0f, t / duration);

    ClearBackground(Color{ 6, 2, 14, 255 });
    const Vector2 c = { SCREEN_WIDTH / 2.0f, SCREEN_HEIGHT / 2.0f };

    // Rings rushing inwards
    for (int i = 0; i < 9; ++i) {
        float r = 112.0f - std::fmod(i * 13.0f + t * 45.0f, 112.0f);
        unsigned char a = (unsigned char)(40 + 150 * (1.0f - r / 112.0f));
        DrawRing(c, r, r + 2.0f, 0.0f, 360.0f, 48, Color{ 176, 96, 255, a });
    }

    // Spinning arms
    for (int k = 0; k < 6; ++k) {
        float ang = t * 3.5f + k * (2.0f * PI / 6.0f);
        float len = 30.0f + 70.0f * grow;
        Vector2 tip = { c.x + std::cos(ang) * len, c.y + std::sin(ang) * len };
        DrawLineEx(c, tip, 2.0f, Color{ 230, 140, 255, 200 });
    }

    // Bright core swallowing the screen
    const float core = 6.0f + grow * grow * 150.0f;
    DrawCircleV(c, core, Color{ 235, 215, 255, (unsigned char)(60 + 195 * grow * grow) });

    if (t < duration * 0.7f) {
        const char* msg = "The portal pulls you in...";
        int w = MeasureText(msg, 8);
        DrawText(msg, (SCREEN_WIDTH - w) / 2, 214, 8, Color{ 220, 190, 255, 255 });
    }

    if (t >= duration) {
        state->portalTimer = 0.0f;
        enterIsland2(state);
        state->currentScreen = SCREEN_GAME;
    }
}

static Vector2 chronosPatrolPos(float t) {
    static const int WP[][2] = { {14,19}, {30,19}, {30,35}, {14,35} };
    const int n = 4;
    const float speed = CHRONOS_CHASE_SPEED;
    float len[4], total = 0.0f;
    for (int i = 0; i < n; ++i) {
        Vector2 a = islandTileCentre(WP[i][0], WP[i][1]);
        Vector2 b = islandTileCentre(WP[(i+1)%n][0], WP[(i+1)%n][1]);
        len[i] = std::hypot(b.x - a.x, b.y - a.y);
        total += len[i];
    }
    float d = std::fmod(t * speed, total);
    for (int i = 0; i < n; ++i) {
        if (d <= len[i]) {
            Vector2 a = islandTileCentre(WP[i][0], WP[i][1]);
            Vector2 b = islandTileCentre(WP[(i+1)%n][0], WP[(i+1)%n][1]);
            float k = d / len[i];
            return { a.x + (b.x - a.x) * k, a.y + (b.y - a.y) * k };
        }
        d -= len[i];
    }
    return islandTileCentre(WP[0][0], WP[0][1]);
}

void gameScreen(GameState* state, Vector2 mousePos, float deltaTime) {
    if (state->gameOver && IsKeyPressed(KEY_R)) {
        state->currentLoop = 1;
        state->timeRemaining = state->loopDuration;
        state->gameOver = false;
        state->victory = false;
        state->gameOverReason = GAME_OVER_NONE;
        state->chronosDangerous = false;
        state->chronosHitCooldown = 0.0f;
        state->playerHitFlash = 0.0f;
        state->playerShakeTimer = 0.0f;
        respawnForLoop(state);
    }

    if (!state->gameOver && !state->victory && state->timeRemaining > 0.0f) {
        state->timeRemaining -= deltaTime;

        if (state->timeRemaining < 0.0f) {
            state->timeRemaining = 0.0f;
        }

        if (state->timeRemaining <= 0.0f) {
            if (state->currentLoop < 3) {
                state->currentLoop++;
                state->timeRemaining = state->loopDuration;
                respawnForLoop(state);
                state->chronosHitCooldown = 0.0f;
                state->playerHitFlash = 0.0f;
                state->playerShakeTimer = 0.0f;
            } else {
                state->gameOver = true;
                state->gameOverReason = GAME_OVER_TIME;
            }
        }
    }

    state->chronosDangerous =
        !state->gameOver &&
        !state->victory &&
        state->timeRemaining > 0.0f &&
        state->timeRemaining <= 15.0f;

    if (!state->victory) {
        if (state->currentIsland == 1) {
            updateChronos(state->chronos, deltaTime, state->chronosDangerous);
        } else if (!state->gameOver) {
            if (state->chronosDangerous) {
                const float cdx = state->player.x - state->chronos.position.x;
                const float cdy = state->player.y - state->chronos.position.y;
                const float cdist = std::sqrt(cdx * cdx + cdy * cdy);
                if (cdist > 1.0f) {
                    state->chronos.position.x += (cdx / cdist) * CHRONOS_CHASE_SPEED_DANGEROUS * deltaTime;
                    state->chronos.position.y += (cdy / cdist) * CHRONOS_CHASE_SPEED_DANGEROUS * deltaTime;
                }
            } else {
                state->chronos.position =
                    chronosPatrolPos(state->loopDuration - state->timeRemaining);
            }
        }
    }

    // The camera keeps the player at the centre of the screen, and mousePos is
    // in screen space, so the aim angle is measured from the screen centre.
    float playerMouseAngle = std::atan2(
        mousePos.y - SCREEN_HEIGHT / 2.0f,
        mousePos.x - SCREEN_WIDTH / 2.0f
    );

    if (state->currentIsland == 1 && !state->gameOver && !state->victory && !state->shadowDefeated) {
        float edx = state->player.x - state->patroller.position.x;
        float edy = state->player.y - state->patroller.position.y;
        float edist = std::sqrt(edx * edx + edy * edy);
        if (edist > 1.0f) {
            state->patroller.position.x += (edx / edist) * state->patroller.speed * deltaTime;
            state->patroller.position.y += (edy / edist) * state->patroller.speed * deltaTime;
        }

        Rectangle playerBounds = {
            state->player.x - PLAYER_RECT_SIZE * 0.5f,
            state->player.y - PLAYER_RECT_SIZE * 0.5f,
            PLAYER_RECT_SIZE,
            PLAYER_RECT_SIZE
        };
        Rectangle enemyBounds = {
            state->patroller.position.x - 6.0f,
            state->patroller.position.y - 6.0f,
            12.0f,
            12.0f
        };

        if (CheckCollisionRecs(playerBounds, enemyBounds)) {
            state->currentScreen = SCREEN_BATTLE;
            state->battleCursorPos = 0.0f;
            state->battleEnded = false;
            state->battleWon = false;
            state->battleResultTimer = 0.0f;
        }
    }

    // Island 2: the yellow shadow chases you; when it reaches you it asks a riddle.
    if (state->currentIsland == 2 && !state->gameOver && !state->victory &&
        !state->yellowShadowDefeated) {
        const float ydx = state->player.x - state->yellowShadow.position.x;
        const float ydy = state->player.y - state->yellowShadow.position.y;
        const float ydist = std::sqrt(ydx * ydx + ydy * ydy);
        if (ydist > 1.0f) {
            state->yellowShadow.position.x += (ydx / ydist) * state->yellowShadow.speed * deltaTime;
            state->yellowShadow.position.y += (ydy / ydist) * state->yellowShadow.speed * deltaTime;
        }

        const Rectangle yPlayer = {
            state->player.x - PLAYER_RECT_SIZE * 0.5f,
            state->player.y - PLAYER_RECT_SIZE * 0.5f,
            PLAYER_RECT_SIZE, PLAYER_RECT_SIZE
        };
        const Rectangle yEnemy = {
            state->yellowShadow.position.x - 6.0f,
            state->yellowShadow.position.y - 6.0f,
            12.0f, 12.0f
        };
        if (CheckCollisionRecs(yPlayer, yEnemy)) {
            startRiddle(state);
        }
    }

    if (!state->gameOver && !state->victory) {
        state->chronosHitCooldown = std::max(0.0f, state->chronosHitCooldown - deltaTime);
        state->playerHitFlash = std::max(0.0f, state->playerHitFlash - deltaTime);
        state->playerShakeTimer = std::max(0.0f, state->playerShakeTimer - deltaTime);

        float accelerateX = (float)(IsKeyDown(KEY_D) - IsKeyDown(KEY_A));
        float accelerateY = (float)(IsKeyDown(KEY_S) - IsKeyDown(KEY_W));

        state->player.velx = std::lerp(
            state->player.velx, PLAYER_VELOCITY_MAX * accelerateX,
            PLAYER_ACCELERATION * deltaTime
        );
        state->player.vely = std::lerp(
            state->player.vely, PLAYER_VELOCITY_MAX * accelerateY,
            PLAYER_ACCELERATION * deltaTime
        );

        const float nextX = state->player.x + state->player.velx * deltaTime;
        if (state->currentIsland == 2 && island2Blocked(nextX, state->player.y)) {
            state->player.velx = 0.0f;
        } else {
            state->player.x = nextX;
        }

        const float nextY = state->player.y + state->player.vely * deltaTime;
        if (state->currentIsland == 2 && island2Blocked(state->player.x, nextY)) {
            state->player.vely = 0.0f;
        } else {
            state->player.y = nextY;
        }

        clampPlayerToWorld(state);
    }

    if (!state->gameOver && !state->victory && state->chronosHitCooldown <= 0.0f) {
        const float dx = state->player.x - state->chronos.position.x;
        const float dy = state->player.y - state->chronos.position.y;
        const float distance = std::sqrt(dx * dx + dy * dy);
        const Texture2D chronosTexture = state->assets.chronosSprite.getCurrentTexture();
        const float chronosRadius =
            std::max(chronosTexture.width, chronosTexture.height) * 0.5f;
        const Rectangle playerBounds = {
            state->player.x - PLAYER_RECT_SIZE * 0.5f,
            state->player.y - PLAYER_RECT_SIZE * 0.5f,
            PLAYER_RECT_SIZE,
            PLAYER_RECT_SIZE
        };

        if (CheckCollisionCircleRec(state->chronos.position, chronosRadius, playerBounds)) {
            state->chronosHitCooldown = 0.5f;

            if (state->chronosDangerous) {
                state->gameOver = true;
                state->gameOverReason = GAME_OVER_CAUGHT;
            } else {
                const float safeDistance = std::max(distance, 0.001f);
                const float pushX = dx / safeDistance;
                const float pushY = dy / safeDistance;
                const float oldX = state->player.x;
                const float oldY = state->player.y;
                state->player.x += pushX * 30.0f;
                state->player.y += pushY * 30.0f;
                if (state->currentIsland == 2 &&
                    island2Blocked(state->player.x, state->player.y)) {
                    state->player.x = oldX;   // wall in the way: no knockback
                    state->player.y = oldY;
                }
                state->player.velx = pushX * 80.0f;
                state->player.vely = pushY * 80.0f;
                clampPlayerToWorld(state);
                state->playerHitFlash = 0.25f;
                state->playerShakeTimer = 0.35f;
            }
        }
    }

    if (!state->gameOver && !state->victory &&
        !state->testItem.collected && IsKeyPressed(KEY_E)) {
        const float dx = state->player.x - state->testItem.position.x;
        const float dy = state->player.y - state->testItem.position.y;
        const float distance = std::sqrt(dx * dx + dy * dy);

        if (distance <= 16.0f) {
            state->testItem.collected = true;
        }
    }

    state->testExit.unlocked = state->testItem.collected;

    if (state->testExit.unlocked && IsKeyPressed(KEY_E)) {
        const float dx = state->player.x - state->testExit.position.x;
        const float dy = state->player.y - state->testExit.position.y;
        const float distance = std::sqrt(dx * dx + dy * dy);

        if (distance <= 18.0f) {
            if (state->currentIsland == 1) {
                state->portalTimer = 0.0f;
                state->currentScreen = SCREEN_PORTAL;   // step through to island 2
            } else {
                state->victory = true;                  // escaped for real
            }
        }
    }

    // Camera: its target is the player and its offset is the screen centre, so
    // the player never moves on screen, the world scrolls past instead.
    // Rounding the target keeps pixel art on whole pixels (no shimmering).
    Camera2D camera = {};
    camera.offset = { SCREEN_WIDTH / 2.0f, SCREEN_HEIGHT / 2.0f };
    camera.target = { std::round(state->player.x), std::round(state->player.y) };
    camera.rotation = 0.0f;
    camera.zoom = 1.0f;

    // The part of the world currently on screen, used to skip off-screen tiles.
    const Rectangle view = {
        camera.target.x - camera.offset.x,
        camera.target.y - camera.offset.y,
        (float)SCREEN_WIDTH,
        (float)SCREEN_HEIGHT
    };

    ClearBackground(Color{20, 20, 24, 255});

    BeginMode2D(camera);

    state->islandMap.DrawView(view, state->mapOrigin, false);

    // Temporary occlusion-test wall; replace with the map's obstacle data later.
    if (state->currentIsland == 1) {
        DrawRectangle(140, 58, 4, 108, DARKGRAY);
    }

    if (state->currentIsland == 1 && !state->shadowDefeated) {
        DrawRectangle(
            (int)state->patroller.position.x - 6,
            (int)state->patroller.position.y - 6,
            12,
            12,
            MAROON
        );
    }

    if (state->currentIsland == 2 && !state->yellowShadowDefeated) {
        const bool flicker = static_cast<int>(GetTime() * 6.0) % 2 == 0;
        DrawRectangle(
            (int)state->yellowShadow.position.x - 6,
            (int)state->yellowShadow.position.y - 6,
            12, 12,
            flicker ? Color{255, 214, 60, 255} : Color{210, 170, 30, 255}
        );
    }

    Color chronosColor = DARKGRAY;
    if (state->chronosDangerous &&
        static_cast<int>(GetTime() * 4.0) % 2 == 0) {
        chronosColor = RED;
    }

    state->assets.chronosSprite.updateFrame(deltaTime);
    DrawTexturePro(
        state->assets.chronosSprite.getCurrentTexture(),
        {
            0, 0,
            (float)state->assets.chronosSprite.getCurrentTexture().width,
            (float)state->assets.chronosSprite.getCurrentTexture().height
        },
        {
            state->chronos.position.x,
            state->chronos.position.y,
            (float)state->assets.chronosSprite.getCurrentTexture().width,
            (float)state->assets.chronosSprite.getCurrentTexture().height
        },
        {
            (float)state->assets.chronosSprite.getCurrentTexture().width / 2,
            (float)state->assets.chronosSprite.getCurrentTexture().height / 2
        },
        0.0f,
        {180, 180, 180, 255}
    );

    const float shakeX = state->playerShakeTimer > 0.0f
        ? std::sin(GetTime() * 70.0f) * 3.0f
        : 0.0f;
    const float shakeY = state->playerShakeTimer > 0.0f
        ? std::cos(GetTime() * 85.0f) * 2.0f
        : 0.0f;

    DrawTexturePro(
        state->assets.players[state->selectedCharacter],
        {0, 0, (float)PLAYER_RECT_SIZE, (float)PLAYER_RECT_SIZE},
        {camera.target.x + shakeX, camera.target.y + shakeY,
            (float)PLAYER_RECT_SIZE, (float)PLAYER_RECT_SIZE},
        {PLAYER_RECT_SIZE / 2.0f, PLAYER_RECT_SIZE / 2.0f},
        playerMouseAngle * RAD2DEG,
        state->playerHitFlash > 0.0f ? ORANGE : WHITE
    );

    EndMode2D();



    // LightingMap works in map-local coordinates (0,0 = the map's top-left
    // corner), so subtract mapOrigin from every world position we give it.
    // It draws in screen space, so this call stays outside BeginMode2D.
    const int originX = (int)state->mapOrigin.x;
    const int originY = (int)state->mapOrigin.y;

    const std::vector<LightSource> lightSources{
        LightSource{
            (int)camera.target.x - originX,
            (int)camera.target.y - originY,
            PLAYER_LIGHT_SIZE / 2
        }
    };
    state->lightingMap.calcAndDrawLights(
        lightSources,
        (int)view.x - originX, (int)view.y - originY,   // top-left of the screen, map-local
        SCREEN_WIDTH, SCREEN_HEIGHT
    );


    // Item and exit live in the world, so they go through the camera too.
    BeginMode2D(camera);

    if (!state->testItem.collected) {
        DrawRectangle(
            (int)state->testItem.position.x - 4,
            (int)state->testItem.position.y - 4,
            8,
            8,
            GOLD
        );
    }

    DrawRectangle(
        (int)state->testExit.position.x - 6,
        (int)state->testExit.position.y - 6,
        12,
        12,
        state->testExit.unlocked ? GREEN : DARKGRAY
    );

    EndMode2D();

    // HUD (screen space)
    DrawText(
        state->testItem.collected ? "Item: 1/1" : "Item: 0/1",
        8,
        8,
        10,
        RAYWHITE
    );

    if (!state->gameOver && !state->victory) {
        const float pdx = state->player.x - state->testItem.position.x;
        const float pdy = state->player.y - state->testItem.position.y;
        const float edx2 = state->player.x - state->testExit.position.x;
        const float edy2 = state->player.y - state->testExit.position.y;
        if (!state->testItem.collected && std::sqrt(pdx * pdx + pdy * pdy) <= 28.0f) {
            DrawText("Press E to take", 70, 190, 10, GOLD);
        } else if (std::sqrt(edx2 * edx2 + edy2 * edy2) <= 30.0f) {
            if (state->testExit.unlocked) {
                DrawText(state->currentIsland == 1 ? "Press E to enter portal" : "Press E to escape",
                         state->currentIsland == 1 ? 46 : 68, 190, 10, GREEN);
            } else {
                DrawText("Find the item first", 64, 190, 10, LIGHTGRAY);
            }
        }
    }

    if (state->victory) {
        static int page = 0;
        if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE)) page = std::min(page + 1, 2);
        DrawRectangle(20, 70, SCREEN_WIDTH - 40, 100, Color{10, 30, 18, 235});
        DrawRectangleLines(20, 70, SCREEN_WIDTH - 40, 100, GOLD);
        const char* a[3] = { "Time is not infinite, Chronos.", "He drops a golden watch and fades.", "Pawned: 4000 GBP. Four ways." };
        const char* b[3] = { "You wasted yours. Let us go.", "A portal opens. We step through.", "1000 each. Exactly what we signed for." };
        DrawText(a[page], 30, 90, 8, RAYWHITE);
        DrawText(b[page], 30, 106, 8, RAYWHITE);
        DrawText("[ENTER]", SCREEN_WIDTH - 60, 154, 7, GOLD);
    } else if (state->gameOver) {
    } else if (state->gameOver) {
        DrawRectangle(30, 82, SCREEN_WIDTH - 60, 76, Color{35, 10, 12, 235});
        DrawRectangleLines(30, 82, SCREEN_WIDTH - 60, 76, RED);
        if (state->gameOverReason == GAME_OVER_CAUGHT) {
            DrawText("CAUGHT!", 80, 96, 22, RED);
            DrawText("Chronos caught you.", 63, 124, 10, RAYWHITE);
        } else if (state->gameOverReason == GAME_OVER_SHADOW) {
            DrawText("CAUGHT!", 80, 96, 22, RED);
            DrawText("The shadow took you.", 60, 124, 10, RAYWHITE);
        } else {
            DrawText("TIME OVER", 76, 96, 22, RED);
            DrawText("You failed to escape.", 58, 124, 10, RAYWHITE);
        }
        DrawText("Press R to restart", 66, 140, 9, LIGHTGRAY);
    }

    int secondsLeft = static_cast<int>(std::ceil(state->timeRemaining));
    std::string timerText = "Time: " + std::to_string(secondsLeft);
    
    bool isUrgent = (state->timeRemaining <= 5.0f && state->timeRemaining > 0.0f);
    
    int fontSize = isUrgent ? 14 : 10;          
    Color textColor = isUrgent ? RED : RAYWHITE; 
    
    int textWidth = MeasureText(timerText.c_str(), fontSize);
    int posX = (SCREEN_WIDTH - textWidth) / 2;
    int posY = 6;
    
    if (isUrgent) {
        float t = (float)GetTime();
        posX += (int)(std::sin(t * 50.0f) * 3.0f);
        posY += (int)(std::cos(t * 45.0f) * 2.0f);

        DrawText(timerText.c_str(), posX - 2, posY, fontSize, SKYBLUE);
        DrawText(timerText.c_str(), posX + 2, posY, fontSize, Color{255, 50, 50, 180});
    }
    
    DrawText(timerText.c_str(), posX, posY, fontSize, textColor);

    std::string loopText = "Loop: " + std::to_string(state->currentLoop) + "/3";
    DrawText(loopText.c_str(), 8, 22, 10, RAYWHITE);
    if (IsKeyPressed(KEY_C)) state->controller.open = !state->controller.open;
    if (state->controller.open) drawController(state, mousePos, deltaTime);
    DrawText(LOOP_GOAL[state->currentLoop - 1], 8, 36, 8, GOLD);
}



void battleScreen(GameState* state, float deltaTime) {
    if (!state->battleEnded) {
        state->battleCursorPos = (std::sin((float)GetTime() * 6.0f * 0.9f) + 1.0f) * 0.5f;

        if (IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_ENTER) || IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            state->battleEnded = true;
            if (state->battleCursorPos >= 0.32f && state->battleCursorPos <= 0.68f) {
                state->battleWon = true;
            } else {
                state->battleWon = false;
            }
        }
    } else {
        state->battleResultTimer += deltaTime;
        if (state->battleResultTimer >= 1.5f) {
            if (state->battleWon) {
                state->currentScreen = SCREEN_SAMARTH_DIALOGUE;
                state->samarthLineIndex = 0;
                state->samarthChoiceActive = false;
                state->samarthResponded = false;
                state->samarthResponseTimer = 0.0f;
            } else {
                state->gameOver = true;
                state->gameOverReason = GAME_OVER_CAUGHT;
                state->currentScreen = SCREEN_GAME;
            }
            state->battleResultTimer = 0.0f;
        }
    }

    ClearBackground(Color{ 15, 10, 25, 255 });
    DrawTriangle((Vector2){0, 0}, (Vector2){240, 0}, (Vector2){0, 240}, Color{ 180, 15, 15, 25 });

    DrawRectangle(130, 12, 100, 24, BLACK);
    DrawRectangleLines(130, 12, 100, 24, RED);
    DrawText("TIME SHADOW", 136, 18, 8, RAYWHITE);
    
    DrawRectangle(180, 38, 48, 48, Color{ 40, 10, 30, 255 });
    DrawRectangleLines(180, 38, 48, 48, RED);
    DrawText("SHADOW", 185, 58, 6, RED);

    DrawRectangle(10, SCREEN_HEIGHT - 65, 90, 22, BLACK);
    DrawRectangleLines(10, SCREEN_HEIGHT - 65, 90, 22, RED);
    const char* charName = (state->selectedCharacter == 0) ? "JAMIE" : "CAROL";
    DrawText(charName, 16, SCREEN_HEIGHT - 58, 8, RAYWHITE);
    
    if (state->assets.players[state->selectedCharacter].id != 0) {
        DrawTexturePro(
            state->assets.players[state->selectedCharacter],
            {0, 0, (float)PLAYER_RECT_SIZE, (float)PLAYER_RECT_SIZE},
            {26.0f, (float)SCREEN_HEIGHT - 118.0f, 48.0f, 48.0f},
            {0, 0}, 0.0f, WHITE
        );
    }

    int barX = 30;
    int barY = 110;
    int barW = SCREEN_WIDTH - 60;
    int barH = 22;

    DrawRectangle(barX, barY, barW, barH, BLACK);
    DrawRectangleLines(barX, barY, barW, barH, RED);

    int sweetZoneW = (int)(barW * 0.36f);
    int sweetZoneX = barX + (barW - sweetZoneW) / 2;
    DrawRectangle(sweetZoneX, barY, sweetZoneW, barH, Color{ 50, 220, 50, 190 });

    int cursorX = barX + (int)(state->battleCursorPos * barW);
    DrawRectangle(cursorX - 2, barY - 5, 4, barH + 10, YELLOW);

    if (!state->battleEnded) {
        DrawText("Press [SPACE] in green zone!", 42, 85, 8, RAYWHITE);
    } else {
        if (state->battleWon) {
            DrawRectangle(30, 145, SCREEN_WIDTH - 60, 42, Color{ 10, 40, 20, 245 });
            DrawRectangleLines(30, 145, SCREEN_WIDTH - 60, 42, GREEN);
            DrawText("Shadow Defeated!", 70, 161, 8, GREEN);
        } else {
            DrawRectangle(30, 145, SCREEN_WIDTH - 60, 42, Color{ 40, 10, 10, 245 });
            DrawRectangleLines(30, 145, SCREEN_WIDTH - 60, 42, RED);
            DrawText("Defeated...", 90, 161, 10, RED);
        }
    }
}

void samarthDialogueScreen(GameState* state, float deltaTime) {
    const char* lines[] = {
        "Hey there, I was trapped inside that shadow, thanks for freeing me",
        "Chronos has trapped me and several other humans on this island",
        "Chronos has witnessed humans wasting time and resources",
        "Thus he has decided that humans shall spend all of eternity in a loop",
        state->selectedCharacter == 0 
            ? "You're the only hope Jamie, are you in or out" 
            : "You're the only hope Carol, are you in or out"
    };
    const int totalLines = 5;

    if (!state->samarthChoiceActive && !state->samarthResponded) {
        if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE) || IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            state->samarthLineIndex++;
            if (state->samarthLineIndex >= totalLines) {
                state->samarthChoiceActive = true;
            }
        }
    } else if (state->samarthChoiceActive && !state->samarthResponded) {
        if (IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W)) {
            state->samarthChoiceSelected = 0;
        }
        if (IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S)) {
            state->samarthChoiceSelected = 1;
        }

        if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE) || IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            if (state->samarthChoiceSelected == 0) {
                state->luckPoints += 1;
                state->acceptedChallenge = true;
            } else {
                state->acceptedChallenge = false;
            }
            state->samarthChoiceActive = false;
            state->samarthResponded = true;
            state->samarthResponseTimer = 0.0f;
        }
    } else {
        state->samarthResponseTimer += deltaTime;
        if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE) || IsMouseButtonPressed(MOUSE_BUTTON_LEFT) || state->samarthResponseTimer >= 3.5f) {
            state->currentScreen = SCREEN_GAME;
            state->samarthResponded = false;
            state->samarthLineIndex = 0;
            state->shadowDefeated = true; // Permanently mark shadow as defeated
            state->patroller.position = { -999.0f, -999.0f }; // Send shadow off-screen
            state->samarthResponseTimer = 0.0f;
        }
    }

    ClearBackground(Color{ 10, 10, 15, 255 });

    // 1. Dialogue Box Background
    DrawRectangle(10, 145, SCREEN_WIDTH - 20, 85, Color{ 15, 15, 25, 240 });
    DrawRectangleLines(10, 145, SCREEN_WIDTH - 20, 85, BLUE);

    // 2. Speaker Nameplate floating properly above the dialogue box
    DrawRectangle(16, 127, 72, 18, BLACK);
    DrawRectangleLines(16, 127, 72, 18, BLUE);
    DrawText("SAMARTH", 20, 132, 8, BLUE);

    if (!state->samarthChoiceActive && !state->samarthResponded) {
        std::string currentText = lines[state->samarthLineIndex];
        int maxCharsPerLine = 34;
        std::string line1 = (currentText.length() > maxCharsPerLine) ? currentText.substr(0, maxCharsPerLine) : currentText;
        std::string line2 = (currentText.length() > maxCharsPerLine) ? currentText.substr(maxCharsPerLine) : "";

        DrawText(line1.c_str(), 20, 160, 8, RAYWHITE);
        DrawText(line2.c_str(), 20, 175, 8, RAYWHITE);

        if (static_cast<int>(GetTime() * 3.0) % 2 == 0) {
            DrawText("[CLICK / ENTER]", SCREEN_WIDTH - 90, 215, 6, BLUE);
        }
    } else if (state->samarthChoiceActive && !state->samarthResponded) {
        DrawText("Make your choice:", 20, 152, 8, SKYBLUE);

        Rectangle opt1Rect = { 35.0f, 168.0f, 170.0f, 20.0f };
        DrawRectangleRec(opt1Rect, state->samarthChoiceSelected == 0 ? Color{ 20, 80, 160, 230 } : Color{ 20, 20, 30, 230 });
        DrawRectangleLines((int)opt1Rect.x, (int)opt1Rect.y, (int)opt1Rect.width, (int)opt1Rect.height, BLUE);
        DrawText("Yes I'm in", (int)opt1Rect.x + 10, (int)opt1Rect.y + 6, 8, RAYWHITE);

        Rectangle opt2Rect = { 35.0f, 192.0f, 170.0f, 20.0f };
        DrawRectangleRec(opt2Rect, state->samarthChoiceSelected == 1 ? Color{ 160, 40, 40, 230 } : Color{ 20, 20, 30, 230 });
        DrawRectangleLines((int)opt2Rect.x, (int)opt2Rect.y, (int)opt2Rect.width, (int)opt2Rect.height, RED);
        DrawText("I don't care", (int)opt2Rect.x + 10, (int)opt2Rect.y + 6, 8, RAYWHITE);
    } else {
        if (state->acceptedChallenge) {
            DrawText("Thats the spirit, I will be cheering", 20, 156, 8, SKYBLUE);
            DrawText("you on all the way!", 20, 168, 8, SKYBLUE);
            DrawText("-> I feel Samarth's resolve motivating me", 20, 190, 7, GOLD);
        } else {
            DrawText("That's too bad, I thought you'd be", 20, 160, 8, LIGHTGRAY);
            DrawText("the one to save humanity...", 20, 174, 8, LIGHTGRAY);
        }

        if (static_cast<int>(GetTime() * 3.0) % 2 == 0) {
            DrawText("[CLICK / ENTER]", SCREEN_WIDTH - 90, 215, 6, BLUE);
        }
    }
}

void mainLoop(void* data) {
    GameState* state = (GameState*)data;
    Music& bgm = (state->currentIsland == 2) ? state->assets.bgm2 : state->assets.bgm;
    if (IsAudioDeviceReady() && !IsMusicStreamPlaying(bgm)) {
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) || GetKeyPressed() != 0) {
            PlayMusicStream(bgm);
        }
    }
    UpdateMusicStream(bgm);

    float deltaTime = GetFrameTime();
    Vector2 gameMousePos = getGameMousePos();

	{
	std::lock_guard<std::mutex> lock(stateMutex);
    BeginTextureMode(state->screenTexture);
        if (state->currentScreen == SCREEN_MENU) {
            menuScreen(state, gameMousePos);
        } else if (state->currentScreen == SCREEN_CREDITS) {
            creditsScreen(state, gameMousePos);
        } else if (state->currentScreen == SCREEN_SELECT) {
            selectScreen(state, gameMousePos);
        } else if (state->currentScreen == SCREEN_DIALOGUE) {
            dialogueScreen(state);
        } else if (state->currentScreen == SCREEN_GAME) {
            gameScreen(state, gameMousePos, deltaTime);
        } else if (state->currentScreen == SCREEN_BATTLE) {
            battleScreen(state, deltaTime);
        } else if (state->currentScreen == SCREEN_SAMARTH_DIALOGUE) {
            samarthDialogueScreen(state, deltaTime);
        } else if (state->currentScreen == SCREEN_PORTAL) {
            portalScreen(state, deltaTime);
        } else if (state->currentScreen == SCREEN_RIDDLE) {
            riddleScreen(state, deltaTime);
        }
    EndTextureMode();
	}

    BeginDrawing();
        ClearBackground(BLACK);

        int scrWidth = GetScreenWidth();
        int scrHeight = GetScreenHeight();

        int scale = std::max(1, std::min(scrWidth / SCREEN_WIDTH, scrHeight / SCREEN_HEIGHT));

        int scrBufferWidth = SCREEN_WIDTH * scale;
        int scrBufferHeight = SCREEN_HEIGHT * scale;

        float crtRes[2] = {(float)scrBufferWidth, (float)scrBufferHeight};
        SetShaderValue(state->assets.crtShader, GetShaderLocation(state->assets.crtShader, "resolution"), crtRes, SHADER_UNIFORM_VEC2);

        BeginShaderMode(state->assets.crtShader); {
            DrawTexturePro(
                state->screenTexture.texture,
                (Rectangle){0, 0, (float)state->screenTexture.texture.width, (float)-state->screenTexture.texture.height},
                (Rectangle){
                    (float)(int)((scrWidth - scrBufferWidth) / 2.0f),
                    (float)(int)((scrHeight - scrBufferHeight) / 2.0f),
                    (float)scrBufferWidth, (float)scrBufferHeight
                },
                (Vector2){0.0f, 0.0f}, 0.0f, WHITE
            );
        } EndShaderMode();

        DrawFPS(10, 10);
    EndDrawing();
}

GameAssets loadAssets() {
    GameAssets assets = {};

    #ifdef __EMSCRIPTEN__
    assets.crtShader = LoadShader(0, "assets/crt.frag");
    #else
    assets.crtShader = LoadShader(0, "assets/crt_desktop.frag");
    #endif

    float crtCurve = 1.2f;
    float crtVignette = 0.4f;

    SetShaderValue(
        assets.crtShader, GetShaderLocation(assets.crtShader, "curvature"),
        &crtCurve, SHADER_UNIFORM_FLOAT
    );
    SetShaderValue(
        assets.crtShader, GetShaderLocation(assets.crtShader, "vignette"),
        &crtVignette, SHADER_UNIFORM_FLOAT
    );

    assets.players[0] = LoadTexture("assets/player_jamie.png");
    assets.players[1] = LoadTexture("assets/player_carol.png");

    assets.bgm = LoadMusicStream("assets/AreaOne.mp3");
    assets.bgm.looping = true;
    SetMusicVolume(assets.bgm, 0.7f);
    assets.bgm2 = LoadMusicStream("assets/AreaTwo.mp3");
    assets.bgm2.looping = true;
    SetMusicVolume(assets.bgm2, 0.7f);
    PlayMusicStream(assets.bgm);

    assets.chronosSprite = AnimatedSprite(
        {
            "assets/chronos-anim1.png",
            "assets/chronos-anim2.png",
            "assets/chronos-anim3.png",
            "assets/chronos-anim4.png",
            "assets/chronos-anim5.png"
        },
        0.1f
    );

    return assets;
}

void unloadAssets(GameAssets& assets) {
    UnloadShader(assets.crtShader);

    UnloadTexture(assets.players[0]);
    UnloadTexture(assets.players[1]);

    UnloadMusicStream(assets.bgm);
    UnloadMusicStream(assets.bgm2);

    assets.chronosSprite.unloadTextures();

    assets = {};
}

int main(int argc, char** argv) {
    SetTraceLogLevel(LOG_WARNING);
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);

    InitWindow(1280, 720, "game");
    InitAudioDevice();

    GameState state = {};
    state.player = {SCREEN_WIDTH / 2.0f, SCREEN_HEIGHT / 2.0f};
    state.testItem.position = {80.0f, 80.0f};
    state.testItem.collected = false;
    
    // Initialize patroller shadow position & speed
    state.patroller.position = {40.0f, 40.0f};
    state.patroller.speed = 25.0f;
    state.yellowShadow.speed = YELLOW_SHADOW_SPEED;
    state.shadowDefeated = false;

    state.currentScreen = SCREEN_SELECT;
    state.selectedCharacter = 0;
    state.selectAnim = 0.0f;
    state.assets = loadAssets();
    state.currentScreen = SCREEN_MENU;
    state.testExit.position = {200.0f, 80.0f};
    state.testExit.unlocked = false;

    state.islandMap = TileMap(ISLAND_WIDTH, ISLAND_HEIGHT, TILE_SIZE, TILE_SIZE);
    loadIslandMap(state.islandMap);

    // Place the map so the spawn tile (the plane crash site) is centred on
    // world (120, 120), where the player starts. The item, exit, Chronos
    // waypoints and test wall all keep their coordinates, so they now sit at
    // the crash site. The map extends into negative coordinates, which is
    // fine in a camera-driven world.
    state.mapOrigin = {
        SCREEN_WIDTH / 2.0f - ISLAND_SPAWN_X * TILE_SIZE - TILE_SIZE / 2.0f,
        SCREEN_HEIGHT / 2.0f - ISLAND_SPAWN_Y * TILE_SIZE - TILE_SIZE / 2.0f
    };

    // One lighting cell per world pixel, covering the whole map. Obstacles use
    // map-local coordinates, so the test wall (world 140,58 to 143,165) is
    // shifted by mapOrigin.
    state.lightingMap = LightingMap(
        ISLAND_WIDTH * TILE_SIZE, ISLAND_HEIGHT * TILE_SIZE
    );
    for (int y = 58; y < 166; ++y) {
        for (int x = 140; x < 144; ++x) {
            state.lightingMap.setObstacle(
                x - (int)state.mapOrigin.x, y - (int)state.mapOrigin.y, 1
            );
        }
    }

    state.screenTexture = LoadRenderTexture(SCREEN_WIDTH, SCREEN_HEIGHT);
    SetTextureWrap(state.screenTexture.texture, TEXTURE_WRAP_CLAMP);

	#ifndef __EMSCRIPTEN__
	loggerRunning=true;
	std::thread logger(loggerThread,&state);
	#endif

    #ifdef __EMSCRIPTEN__
    emscripten_set_main_loop_arg(mainLoop, &state, 0, true);
    #else
    while (!WindowShouldClose()) {
        mainLoop(&state);
    }
    #endif

	#ifndef __EMSCRIPTEN__
	loggerRunning=false;
	logger.join();
	#endif

    UnloadRenderTexture(state.screenTexture);

    state.islandMap.Unload();
    unloadAssets(state.assets);

    CloseAudioDevice();
    CloseWindow();

    return 0;
}