#ifdef __EMSCRIPTEN__
#include <emscripten/emscripten.h>
#endif
#include <raylib.h>

#include <cstdint>
#include <vector>
#include <cmath>
#include <algorithm>
#include <atomic>
#include <string>
#include <tuple>

//Concurrency and monads, what could go wrong.
#include <thread>
#include <mutex>
#include <chrono>

#include "util.h"
#include "chronos.h"
#include "lighting.h"
#include "Map.h"
#include "animated_sprite.h"
#include "game.h"

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

void draw_button(const char* title, Rectangle button,bool overHover){
	DrawRectangleRec(button, overHover ? LIGHTGRAY : WHITE);
    DrawRectangleLines((int)button.x, (int)button.y, (int)button.width, (int)button.height, DARKGRAY);
    int playWidth = MeasureText(title, 20);
    DrawText(title, (int)(button.x + (button.width - playWidth) / 2), (int)(button.y + 15), 20, DARKGRAY);
}

//Concurrency MVP.

using LogEntry=std::tuple<GameState, std::chrono::time_point<std::chrono::system_clock>>;
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
			timeLogger.emplace_back(*state,system_clock::now());
			
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

    if (IsKeyPressed(KEY_ESCAPE)) {
        state->currentScreen = SCREEN_MENU;
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

    drawMenuBackdrop(t, jitterX);

    // Header panel
    DrawSlantedQuad(12.0f + jitterX, 14.0f, 220.0f, 22.0f, -15.0f, Color{ 150, 80, 200, 90 });
    DrawSlantedQuad(10.0f, 12.0f, 220.0f, 22.0f, -15.0f, Color{ 24, 14, 36, 255 });
    DrawSlantedQuad(12.0f, 12.0f, 6.0f, 22.0f, -15.0f, Color{ 212, 175, 55, 255 });
    DrawSlantedQuad(18.0f, 12.0f, 2.0f, 22.0f, -15.0f, Color{ 150, 80, 200, 255 });
    DrawText("Select your Character", 28 + (int)jitterX, 17, 10, Color{ 240, 210, 130, 255 });

    for (int i = 0; i < 2; i++) {
        bool selected = (state->selectedCharacter == i);
        float slant = (i == 0) ? -14.0f : -8.0f;
        float x = ((i == 0) ? 22.0f : 126.0f) + (selected ? jitterX : 0.0f);
        float y = (selected ? 41.0f : 47.0f) + (selected ? jitterY : 0.0f);
        float w = 92.0f;
        float h = selected ? 109.0f : 99.0f;

        if (selected) {
            DrawSlantedQuad(x + 5.0f, y + 4.0f, w, h, slant - 2.0f, Color{ 150, 80, 200, 110 });
            DrawSlantedQuad(x - 3.0f, y - 2.0f, w, h, slant + 1.0f, Color{ 212, 175, 55, 85 });
        } else {
            DrawSlantedQuad(x + 3.0f, y + 4.0f, w, h, slant, Color{ 5, 3, 8, 210 });
        }

        Color cardColor = selected ? Color{ 40, 24, 58, 255 } : Color{ 18, 11, 26, 255 };
        DrawSlantedQuad(x, y, w, h, slant, cardColor);

        Color accentColor = selected ? Color{ 150, 80, 200, 255 } : Color{ 96, 60, 130, 120 };
        DrawSlantedQuad(x + 1.0f, y, w - 2.0f, 4.0f, slant, accentColor);
        DrawSlantedQuad(x, y + h - 2.0f, w, 2.0f, slant,
                        selected ? Color{ 212, 175, 55, 255 } : Color{ 34, 22, 46, 255 });

        const char* numTag = (i == 0) ? "01" : "02";
        DrawText(numTag, (int)(x + 14.0f), (int)(y + 8.0f), 10,
                 selected ? Color{ 212, 175, 55, 255 } : Color{ 60, 44, 80, 255 });

        float spriteSize = selected ? 38.0f : 28.0f;
        float spriteY = y + (selected ? 22.0f : 24.0f);
        float spriteX = x + (w - spriteSize) / 2.0f - 2.0f;

        if (selected) {
            DrawTexturePro(
                state->assets.players[i],
                { 0, 0, (float)PLAYER_RECT_SIZE, (float)PLAYER_RECT_SIZE },
                { spriteX + 2.0f, spriteY, spriteSize, spriteSize },
                { 0, 0 }, 0.0f, Color{ 150, 80, 200, 160 }
            );
        }

        DrawTexturePro(
            state->assets.players[i],
            { 0, 0, (float)PLAYER_RECT_SIZE, (float)PLAYER_RECT_SIZE },
            { spriteX, spriteY, spriteSize, spriteSize },
            { 0, 0 }, 0.0f, selected ? WHITE : Color{ 130, 110, 150, 190 }
        );

        const char* name = (i == 0) ? "JAMIE" : "CAROL";
        Color nameColor = selected ? Color{ 255, 230, 160, 255 } : Color{ 120, 100, 140, 255 };

        if (selected) {
            DrawSlantedQuad(x + 8.0f, y + h - 26.0f, w - 16.0f, 16.0f, -6.0f, Color{ 150, 80, 200, 255 });
            int nw = MeasureText(name, 10);
            DrawText(name, (int)(x + (w - nw) / 2.0f), (int)(y + h - 23.0f), 10, nameColor);
        } else {
            int nw = MeasureText(name, 9);
            DrawText(name, (int)(x + (w - nw) / 2.0f), (int)(y + h - 20.0f), 9, nameColor);
        }
    }

    float descY = 162.0f;
    float descH = 50.0f;

    DrawSlantedQuad(18.0f - jitterX, descY + 4.0f, 210.0f, descH, -13.0f, Color{ 150, 80, 200, 80 });
    DrawSlantedQuad(14.0f, descY, 212.0f, descH, -11.0f, Color{ 20, 12, 30, 255 });
    DrawSlantedQuad(12.0f, descY, 5.0f, descH, -11.0f, Color{ 212, 175, 55, 255 });
    DrawSlantedQuad(17.0f, descY, 2.0f, descH, -11.0f, Color{ 150, 80, 200, 255 });

    if (state->selectedCharacter == 0) {
        DrawText("2nd Year Uni Student", 26 + (int)jitterX, (int)descY + 6, 8, Color{ 240, 210, 130, 255 });
        DrawText("Ability", 26, (int)descY + 18, 8, Color{ 212, 175, 55, 255 });
        DrawText(": Radar ping detects clues", 66, (int)descY + 18, 8, Color{ 190, 170, 200, 255 });
        DrawText("Trait", 26, (int)descY + 30, 8, Color{ 150, 80, 200, 255 });
        DrawText(": Lower field of vision", 66, (int)descY + 30, 8, Color{ 150, 130, 170, 255 });
    } else {
        DrawText("2nd Year Uni Student", 26 + (int)jitterX, (int)descY + 6, 8, Color{ 240, 210, 130, 255 });
        DrawText("Ability", 26, (int)descY + 18, 8, Color{ 212, 175, 55, 255 });
        DrawText(": Able to sprint for period", 66, (int)descY + 18, 8, Color{ 190, 170, 200, 255 });
        DrawText("Trait", 26, (int)descY + 30, 8, Color{ 150, 80, 200, 255 });
        DrawText(": Lower regular speed", 66, (int)descY + 30, 8, Color{ 150, 130, 170, 255 });
    }

    DrawText("Left/Right", 12, 222, 7, Color{ 212, 175, 55, 200 });
    DrawText("[ENTER] Start", 140, 222, 7, Color{ 220, 190, 120, 255 });
}

void dialogueScreen(GameState* state) {
    const char* lines[][2] = {
        {
            "I awoke on a strange island",
            "after a plane crash."
        },
        {
            "I should figure out where I am,",
            "this place looks creepy..."
        }
    };
    const int totalLines = 2;

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

// Builds a big irregular island in the middle of the map. The coastline
// radius wobbles with the angle so it isn't a perfect circle.
void generateIsland(TileMap& map) {
    const int centreX = map.GetWidth() / 2;
    const int centreY = map.GetHeight() / 2;
    const float baseRadius = std::min(map.GetWidth(), map.GetHeight()) * 0.32f;

    map.Fill(14); // shallow water

    for (int y = 0; y < map.GetHeight(); ++y) {
        for (int x = 0; x < map.GetWidth(); ++x) {
            const float dx = (float)(x - centreX);
            const float dy = (float)(y - centreY);
            const float distance = std::sqrt(dx * dx + dy * dy);
            const float angle = std::atan2(dy, dx);

            const float wobble = 1.0f
                + 0.10f * std::sin(angle * 3.0f + 1.3f)
                + 0.06f * std::sin(angle * 5.0f + 4.1f)
                + 0.04f * std::sin(angle * 9.0f + 2.2f);
            const float shoreRadius = baseRadius * wobble;

            if (distance <= shoreRadius) {
                map.SetTile(x, y, 5 + (x + y) % 3);        // grass variants
            } else if (distance <= shoreRadius + 2.5f) {
                map.SetTile(x, y, (x + y) % 3);            // sand variants
            }
        }
    }
}

void gameScreen(GameState* state, Vector2 mousePos, float deltaTime) {
    // Temporary wall used for collision testing until map obstacle data is wired in.
    const Rectangle temporaryWall = { 140.0f, 58.0f, 4.0f, 108.0f };
    if (state->gameOver && IsKeyPressed(KEY_R)) {
        state->currentLoop = 1;
        state->timeRemaining = 60.0f;
        state->gameOver = false;
        state->victory = false;
        state->gameOverReason = GAME_OVER_NONE;
        state->chronosDangerous = false;
        state->chronosHitCooldown = 0.0f;
        state->playerHitFlash = 0.0f;
        state->playerShakeTimer = 0.0f;
        state->testExit.unlocked = false;
        state->player.x = SCREEN_WIDTH / 2.0f;
        state->player.y = SCREEN_HEIGHT / 2.0f;
        state->player.velx = 0.0f;
        state->player.vely = 0.0f;
        state->chronos.position = state->chronos.waypointA;
        state->chronos.movingToB = true;
    }

    if (!state->gameOver && !state->victory && state->timeRemaining > 0.0f) {
        state->timeRemaining -= deltaTime;

        if (state->timeRemaining < 0.0f) {
            state->timeRemaining = 0.0f;
        }

        if (state->timeRemaining <= 0.0f) {
            if (state->currentLoop < 3) {
                state->currentLoop++;
                state->timeRemaining = 60.0f;

                state->player.x = SCREEN_WIDTH / 2.0f;
                state->player.y = SCREEN_HEIGHT / 2.0f;
                state->player.velx = 0.0f;
                state->player.vely = 0.0f;
                state->chronos.position = state->chronos.waypointA;
                state->chronos.movingToB = true;
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
        updateChronos(state->chronos, deltaTime, state->chronosDangerous);
    }

    state->testExit.unlocked = state->collectiblesCollected >= state->collectibles.size();

    // The camera keeps the player at the centre of the screen, and mousePos is
    // in screen space, so the aim angle is measured from the screen centre.
    float playerMouseAngle = std::atan2(
        mousePos.y - SCREEN_HEIGHT / 2.0f,
        mousePos.x - SCREEN_WIDTH / 2.0f
    );

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

        const float halfPlayer = PLAYER_RECT_SIZE * 0.5f;
        const Rectangle worldBounds = getWorldBounds(state);
        const float nextX = std::clamp(
            state->player.x + state->player.velx * deltaTime,
            worldBounds.x + halfPlayer,
            worldBounds.x + worldBounds.width - halfPlayer
        );
        const float nextY = std::clamp(
            state->player.y + state->player.vely * deltaTime,
            worldBounds.y + halfPlayer,
            worldBounds.y + worldBounds.height - halfPlayer
        );

        const Rectangle nextXBounds = {
            nextX - halfPlayer, state->player.y - halfPlayer,
            PLAYER_RECT_SIZE, PLAYER_RECT_SIZE
        };
        if (!CheckCollisionRecs(nextXBounds, temporaryWall)) {
            state->player.x = nextX;
        } else {
            state->player.velx = 0.0f;
        }

        const Rectangle nextYBounds = {
            state->player.x - halfPlayer, nextY - halfPlayer,
            PLAYER_RECT_SIZE, PLAYER_RECT_SIZE
        };
        if (!CheckCollisionRecs(nextYBounds, temporaryWall)) {
            state->player.y = nextY;
        } else {
            state->player.vely = 0.0f;
        }
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
                const float halfPlayer = PLAYER_RECT_SIZE * 0.5f;
                const float knockbackX = pushX * 30.0f;
                const float knockbackY = pushY * 30.0f;
                const auto makesPlayerBounds = [halfPlayer](float x, float y) {
                    return Rectangle{
                        x - halfPlayer,
                        y - halfPlayer,
                        PLAYER_RECT_SIZE,
                        PLAYER_RECT_SIZE
                    };
                };

                const Rectangle worldBounds = getWorldBounds(state);
                const float minX = worldBounds.x + halfPlayer;
                const float maxX = worldBounds.x + worldBounds.width - halfPlayer;
                const float minY = worldBounds.y + halfPlayer;
                const float maxY = worldBounds.y + worldBounds.height - halfPlayer;
                const float fullX = std::clamp(state->player.x + knockbackX, minX, maxX);
                const float fullY = std::clamp(state->player.y + knockbackY, minY, maxY);

                if (!CheckCollisionRecs(makesPlayerBounds(fullX, fullY), temporaryWall)) {
                    state->player.x = fullX;
                    state->player.y = fullY;
                    state->player.velx = pushX * 80.0f;
                    state->player.vely = pushY * 80.0f;
                } else {
                    const float xOnly = std::clamp(state->player.x + knockbackX, minX, maxX);
                    const float yOnly = std::clamp(state->player.y + knockbackY, minY, maxY);
                    const bool canMoveX = !CheckCollisionRecs(
                        makesPlayerBounds(xOnly, state->player.y), temporaryWall
                    );
                    const bool canMoveY = !CheckCollisionRecs(
                        makesPlayerBounds(state->player.x, yOnly), temporaryWall
                    );

                    if (canMoveX) {
                        state->player.x = xOnly;
                        state->player.velx = pushX * 80.0f;
                    } else {
                        state->player.velx = 0.0f;
                    }

                    if (canMoveY) {
                        state->player.y = yOnly;
                        state->player.vely = pushY * 80.0f;
                    } else {
                        state->player.vely = 0.0f;
                    }
                }

                state->playerHitFlash = 0.25f;
                state->playerShakeTimer = 0.35f;
            }
        }
    }

    if (state->testExit.unlocked && IsKeyPressed(KEY_E)) {
        const float dx = state->player.x - state->testExit.position.x;
        const float dy = state->player.y - state->testExit.position.y;
        const float distance = std::sqrt(dx * dx + dy * dy);

        if (distance <= 18.0f) {
            StopMusicStream(state->assets.bgm);

            state->currentScreen = SCREEN_CHRONOS_CONFRONTATION;
            state->confrontationLineIndex = 0;
            state->displayedChars = 0;
            state->typewriterTimer = 0.0f;
            state->voicePlayed = false;
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
    DrawRectangle(140, 58, 4, 108, DARKGRAY);

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

    state->assets.cogSprite.updateFrame(deltaTime);

    for (const Collectible& collectible : state->collectibles) {
        if (collectible.collected) {
            continue;
        }

        const Texture* texture = NULL;
        switch (collectible.type) {
            case COLLECTIBLE_COG: texture = &state->assets.cogSprite.getCurrentTexture();

            default: break;
        }

        if (!texture) {
            continue;
        }

        DrawTexturePro(
            state->assets.cogSprite.getCurrentTexture(),
            {0, 0, (float)texture->width, (float)texture->height},
            {
                collectible.position.x, collectible.position.y,
                (float)texture->width, (float)texture->height
            },
            {
                (float)texture->width / 2,
                (float)texture->height / 2
            },
            0.0f,
            {180, 180, 180, 255}
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
        ("Items: " +
        std::to_string(state->collectiblesCollected) + "/" +
        std::to_string(state->collectibles.size())).c_str(),
        8,
        8,
        10,
        RAYWHITE
    );

    // test for item pickups
    if (!state->gameOver && !state->victory) {
        bool drawnCollectTooltip = false;

        for (Collectible& collectible : state->collectibles) {
            if (collectible.collected) {
                continue;
            }

            const float itemDx = state->player.x - collectible.position.x;
            const float itemDy = state->player.y - collectible.position.y;
            const float itemDistance = std::sqrt(itemDx * itemDx + itemDy * itemDy);

            if (itemDistance <= 28.0f) {
                DrawText("Press E to collect", 62, 190, 10, GOLD);

                if (IsKeyPressed(KEY_E)) {
                    collectible.collected = true;
                    state->collectiblesCollected++;
                }

                drawnCollectTooltip = true;
                break;
            }
        }

        if (!drawnCollectTooltip) {
            const float exitDx = state->player.x - state->testExit.position.x;
            const float exitDy = state->player.y - state->testExit.position.y;
            const float exitDistance = std::sqrt(exitDx * exitDx + exitDy * exitDy);

            if (state->collectiblesCollected >= state->collectibles.size() && exitDistance <= 30.0f) {
                DrawText("Press E to escape", 68, 190, 10, GREEN);
            } else if (!state->testExit.unlocked && exitDistance <= 30.0f) {
                DrawText("Find the item first", 64, 190, 10, LIGHTGRAY);
            }
        }
    }

    if (state->victory) {
        DrawRectangle(30, 82, SCREEN_WIDTH - 60, 76, Color{10, 30, 18, 235});
        DrawRectangleLines(30, 82, SCREEN_WIDTH - 60, 76, GREEN);
        DrawText("ESCAPED!", 78, 96, 22, GREEN);
        DrawText("You escaped the island.", 55, 126, 10, RAYWHITE);
    } else if (state->gameOver) {
        DrawRectangle(30, 82, SCREEN_WIDTH - 60, 76, Color{35, 10, 12, 235});
        DrawRectangleLines(30, 82, SCREEN_WIDTH - 60, 76, RED);
        if (state->gameOverReason == GAME_OVER_CAUGHT) {
            DrawText("CAUGHT!", 80, 96, 22, RED);
            DrawText("Chronos caught you.", 63, 124, 10, RAYWHITE);
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
}

void chronosConfrontationScreen(GameState* state, float deltaTime) {
    const char* lines[] = {
        "How dare you... How dare a fleeting, microscopic speck defy the grand design of the ages!",
        " I am the beginning and the end, yet you treat my absolute authority like a trivial hurdle?!",
        "I will age your very soul to ash!"
    };
    const int totalLines = 3;

    if (!state->voicePlayed && IsAudioDeviceReady()) {
        if (state->confrontationLineIndex >= 0 && state->confrontationLineIndex < totalLines) {
            PlaySound(state->assets.chronosVoices[state->confrontationLineIndex]);
        }
        state->voicePlayed = true;
    }

    const float charsPerSecond = 35.0f;
    if (state->confrontationLineIndex < totalLines) {
        std::string currentLine = lines[state->confrontationLineIndex];
        if (state->displayedChars < (int)currentLine.length()) {
            state->typewriterTimer += deltaTime;
            if (state->typewriterTimer >= (1.0f / charsPerSecond)) {
                state->displayedChars++;
                state->typewriterTimer = 0.0f;
            }
        }
    }

    if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE) || IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        if (state->confrontationLineIndex < totalLines) {
            std::string currentLine = lines[state->confrontationLineIndex];
            if (state->displayedChars < (int)currentLine.length()) {
                state->displayedChars = currentLine.length();
            } else {
                if (IsSoundPlaying(state->assets.chronosVoices[state->confrontationLineIndex])) {
                    StopSound(state->assets.chronosVoices[state->confrontationLineIndex]);
                }

                state->confrontationLineIndex++;
                state->displayedChars = 0;
                state->typewriterTimer = 0.0f;
                state->voicePlayed = false;

                if (state->confrontationLineIndex >= totalLines) {
                    state->victory = true;
                    state->currentScreen = SCREEN_GAME;
                }
            }
        }
    }

    ClearBackground(Color{ 10, 10, 15, 255 });

    if (state->assets.chronosPortrait.id != 0) {
        DrawTexturePro(
            state->assets.chronosPortrait,
            { 0, 0, (float)state->assets.chronosPortrait.width, (float)state->assets.chronosPortrait.height },
            { -10.0f, SCREEN_HEIGHT - 175.0f, 110.0f, 130.0f },
            { 0, 0 }, 0.0f, WHITE
        );
    }

    DrawRectangle(10, 145, SCREEN_WIDTH - 20, 85, Color{ 15, 15, 20, 240 });
    DrawRectangleLines(10, 145, SCREEN_WIDTH - 20, 85, RED);

    DrawRectangle(20, 135, 60, 16, BLACK);
    DrawRectangleLines(20, 135, 60, 16, RED);
    DrawText("CHRONOS", 24, 139, 8, RED);

    if (state->confrontationLineIndex < totalLines) {
        std::string fullLine = lines[state->confrontationLineIndex];
        std::string subText = fullLine.substr(0, state->displayedChars);

        int maxCharsPerLine = 34;
        std::string line1 = "";
        std::string line2 = "";
        std::string line3 = "";

        if (subText.length() > maxCharsPerLine) {
            line1 = subText.substr(0, maxCharsPerLine);
            if (subText.length() > maxCharsPerLine * 2) {
                line2 = subText.substr(maxCharsPerLine, maxCharsPerLine);
                line3 = subText.substr(maxCharsPerLine * 2);
            } else {
                line2 = subText.substr(maxCharsPerLine);
            }
        } else {
            line1 = subText;
        }
        
        DrawText(line1.c_str(), 20, 155, 8, RAYWHITE);
        DrawText(line2.c_str(), 20, 167, 8, RAYWHITE);
        DrawText(line3.c_str(), 20, 179, 8, RAYWHITE);
    }

    if (static_cast<int>(GetTime() * 3.0) % 2 == 0) {
        DrawText("[CLICK / ENTER]", SCREEN_WIDTH - 90, 220, 6, RED);
    }
}

void mainLoop(void* data) {
    GameState* state = (GameState*)data;
    if (IsAudioDeviceReady() && !IsMusicStreamPlaying(state->assets.bgm)) {
        if (state->currentScreen != SCREEN_CHRONOS_CONFRONTATION) {
            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) || GetKeyPressed() != 0) {
                PlayMusicStream(state->assets.bgm);
            }
        }
    }

    UpdateMusicStream(state->assets.bgm);

    float deltaTime = GetFrameTime();
    Vector2 gameMousePos = getGameMousePos();

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
        } else if (state->currentScreen == SCREEN_CHRONOS_CONFRONTATION) {
            chronosConfrontationScreen(state, deltaTime);
        }
    EndTextureMode();

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
    assets.chronosPortrait = LoadTexture("assets/chronos-anim1.png"); 
    assets.chronosVoices[0] = LoadSound("assets/PartOne.mp3");
    assets.chronosVoices[1] = LoadSound("assets/PartTwo.mp3");
    assets.chronosVoices[2] = LoadSound("assets/PartThree.mp3");

    assets.bgm = LoadMusicStream("assets/oooscary.mp3");
    assets.bgm.looping = true;
    SetMusicVolume(assets.bgm, 0.7f);
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

    assets.cogSprite = AnimatedSprite(
        {
            "assets/cog-anim1.png",
            "assets/cog-anim2.png",
            "assets/cog-anim3.png",
            "assets/cog-anim4.png",
            "assets/cog-anim5.png",
            "assets/cog-anim6.png"
        },
        0.1f
    );

    return assets;
}

void unloadAssets(GameAssets& assets) {
    UnloadShader(assets.crtShader);

    UnloadTexture(assets.players[0]);
    UnloadTexture(assets.players[1]);
    UnloadTexture(assets.chronosPortrait);
    UnloadSound(assets.chronosVoices[0]);
    UnloadSound(assets.chronosVoices[1]);
    UnloadSound(assets.chronosVoices[2]);

    UnloadMusicStream(assets.bgm);

    assets.chronosSprite.unloadTextures();
    assets.cogSprite.unloadTextures();

    assets = {};
}

int main(int argc, char** argv) {
    SetTraceLogLevel(LOG_WARNING);
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);

    InitWindow(1280, 720, "game");
    InitAudioDevice();

    GameState state = {};
    state.player = {SCREEN_WIDTH / 2.0f, SCREEN_HEIGHT / 2.0f};

    state.collectibles.push_back(Collectible{COLLECTIBLE_COG, {80.0f, 80.0f}});

    state.currentScreen = SCREEN_SELECT;
    
    state.menuSelection = 0;

    state.selectedCharacter = 0;
    state.selectAnim = 0.0f;
    state.assets = loadAssets();
    state.currentScreen = SCREEN_MENU;
    state.testExit.position = {200.0f, 80.0f};
    state.testExit.unlocked = false;

    state.islandMap = TileMap(MAP_WIDTH_TILES, MAP_HEIGHT_TILES, TILE_SIZE, TILE_SIZE);
    state.islandMap.LoadTileset(
        "assets/TIles/Pixel_Beach_Island_Tileset_v1/spritesheet.png"
    );
    generateIsland(state.islandMap);

    // Place the map so its centre tile sits where the old 15x15 island's centre
    // was, (120, 120). The spawn point, item, exit, Chronos waypoints and test
    // wall all keep their coordinates. The map just extends into negative
    // coordinates, which is fine in a camera-driven world.
    state.mapOrigin = {
        SCREEN_WIDTH / 2.0f - (MAP_WIDTH_TILES / 2) * TILE_SIZE - TILE_SIZE / 2.0f,
        SCREEN_HEIGHT / 2.0f - (MAP_HEIGHT_TILES / 2) * TILE_SIZE - TILE_SIZE / 2.0f
    };

    // One lighting cell per world pixel, covering the whole map. Obstacles use
    // map-local coordinates, so the test wall (world 140,58 to 143,165) is
    // shifted by mapOrigin.
    state.lightingMap = LightingMap(
        MAP_WIDTH_TILES * TILE_SIZE, MAP_HEIGHT_TILES * TILE_SIZE
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
