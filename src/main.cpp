#ifdef __EMSCRIPTEN__
#include <emscripten/emscripten.h>
#endif
#include <raylib.h>
#include "chronos.h"
#include "lighting.h"
#include "Map.h"
#include "animated_sprite.h"

#include <cstdint>
#include <vector>
#include <cmath>
#include <algorithm>

#define SCREEN_WIDTH 240
#define SCREEN_HEIGHT 240

#define PLAYER_RECT_SIZE 16
#define PLAYER_VELOCITY_MAX 100
#define PLAYER_ACCELERATION 15
#define PLAYER_LIGHT_SIZE 100

enum ScreenState {
    SCREEN_MENU,
    SCREEN_SELECT,
    SCREEN_DIALOGUE,
    SCREEN_GAME
};

enum GameOverReason {
    GAME_OVER_NONE,
    GAME_OVER_TIME,
    GAME_OVER_CAUGHT
};

struct Player {
    float x, y;
    float velx, vely;
};

struct Collectible {
    Vector2 position;
    bool collected;
};

struct Exit {
    Vector2 position;
    bool unlocked;
};

struct GameAssets {
    Shader crtShader;

    Texture players[2];
    Music bgm;

    AnimatedSprite chronosSprite;
    AnimatedSprite cogSprite;
};

struct GameState {
    ScreenState currentScreen;
    int selectedCharacter;
    float selectAnim;
    int dialogueIndex = 0;

    Player player;
    Chronos chronos;
    TileMap islandMap;
    LightingMap lightingMap;
    Collectible testItem;
    Exit testExit;

    int currentLoop = 1;
    float timeRemaining = 60.0f;
    bool gameOver = false;
    bool victory = false;
    GameOverReason gameOverReason = GAME_OVER_NONE;
    bool chronosDangerous = false;
    float chronosHitCooldown = 0.0f;
    float playerHitFlash = 0.0f;
    float playerShakeTimer = 0.0f;

    GameAssets assets;

    RenderTexture2D screenTexture;
};

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
	int titleFontSize=40;
	int titleWidth=MeasureText(title,titleFontSize);
	DrawText(title,(SCREEN_WIDTH-titleWidth)/2,20,titleFontSize,DARKGRAY);

}

void draw_button(const char* title, Rectangle button,bool overHover){
	DrawRectangleRec(button, overHover ? LIGHTGRAY : WHITE);
    DrawRectangleLines((int)button.x, (int)button.y, (int)button.width, (int)button.height, DARKGRAY);
    int playWidth = MeasureText(title, 20);
    DrawText(title, (int)(button.x + (button.width - playWidth) / 2), (int)(button.y + 15), 20, DARKGRAY);

}



void menuLoop(void *data){
	GameState* state = static_cast<GameState*>(data);
	const Vector2 mousePos = getGameMousePos();

	//Bounding boxes.
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

	drawTitle("Game Name");
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

void gameScreen(GameState* state, Vector2 mousePos, float deltaTime) {
    const Rectangle temporaryWall = {140.0f, 58.0f, 4.0f, 108.0f};

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
        state->testItem.collected = false;
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
                state->testItem.collected = false;
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

    float playerMouseAngle = std::atan2(
        mousePos.y - state->player.y,
        mousePos.x - state->player.x
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
        const float nextX = std::clamp(
            state->player.x + state->player.velx * deltaTime,
            halfPlayer, SCREEN_WIDTH - halfPlayer
        );
        const float nextY = std::clamp(
            state->player.y + state->player.vely * deltaTime,
            halfPlayer, SCREEN_HEIGHT - halfPlayer
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
                state->player.x += pushX * 30.0f;
                state->player.y += pushY * 30.0f;
                state->player.velx = pushX * 80.0f;
                state->player.vely = pushY * 80.0f;
                state->player.x = std::clamp(state->player.x, 8.0f, SCREEN_WIDTH - 8.0f);
                state->player.y = std::clamp(state->player.y, 8.0f, SCREEN_HEIGHT - 8.0f);
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
            state->victory = true;
        }
    }

    ClearBackground(Color{20, 20, 24, 255});
    state->islandMap.Draw({0.0f, 0.0f}, false);

    // Temporary occlusion-test wall; replace with the map's obstacle data later.
    DrawRectangleRec(temporaryWall, DARKGRAY);

    Color chronosColor = DARKGRAY;

    if (state->chronosDangerous &&
        static_cast<int>(GetTime() * 4.0) % 2 == 0) {
        chronosColor = RED;
    }

    // DrawCircleV(
    //     state->chronos.position,
    //     8.0f,
    //     chronosColor
    // );

    // draw chronos

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

    if (!state->testItem.collected) {
        state->assets.cogSprite.updateFrame(deltaTime);

        DrawTexturePro(
            state->assets.cogSprite.getCurrentTexture(),
            {
                0, 0,
                (float)state->assets.cogSprite.getCurrentTexture().width,
                (float)state->assets.cogSprite.getCurrentTexture().height
            },
            {
                state->testItem.position.x,
                state->testItem.position.y,
                (float)state->assets.cogSprite.getCurrentTexture().width,
                (float)state->assets.cogSprite.getCurrentTexture().height
            },
            {
                (float)state->assets.cogSprite.getCurrentTexture().width / 2,
                (float)state->assets.cogSprite.getCurrentTexture().height / 2
            },
            0.0f,
            {180, 180, 180, 255}
        );
    }

    const float shakeX = state->playerShakeTimer > 0.0f
        ? std::sin(GetTime() * 70.0f) * 3.0f
        : 0.0f;
    const float shakeY = state->playerShakeTimer > 0.0f
        ? std::cos(GetTime() * 85.0f) * 2.0f
        : 0.0f;

    DrawTexturePro(
        state->assets.players[state->selectedCharacter],
        {0, 0, (float)PLAYER_RECT_SIZE, (float)PLAYER_RECT_SIZE},
        {state->player.x + shakeX, state->player.y + shakeY,
            (float)PLAYER_RECT_SIZE, (float)PLAYER_RECT_SIZE},
        {PLAYER_RECT_SIZE / 2.0f, PLAYER_RECT_SIZE / 2.0f},
        playerMouseAngle * RAD2DEG,
        state->playerHitFlash > 0.0f ? ORANGE : WHITE
    );

    const std::vector<LightSource> lightSources{
        LightSource{
            static_cast<int>(state->player.x),
            static_cast<int>(state->player.y),
            PLAYER_LIGHT_SIZE / 2
        }
    };

    state->lightingMap.calcAndDrawLights(
        lightSources, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT
    );

    DrawText(
        state->testItem.collected ? "Item: 1/1" : "Item: 0/1",
        8,
        8,
        10,
        RAYWHITE
    );

    DrawRectangle(
        (int)state->testExit.position.x - 6,
        (int)state->testExit.position.y - 6,
        12,
        12,
        state->testExit.unlocked ? GREEN : DARKGRAY
    );

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

void mainLoop(void* data) {
    GameState* state = (GameState*)data;
    if (IsAudioDeviceReady() && !IsMusicStreamPlaying(state->assets.bgm)) {
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) || GetKeyPressed() != 0) {
            PlayMusicStream(state->assets.bgm);
        }
    }

    UpdateMusicStream(state->assets.bgm);

    float deltaTime = GetFrameTime();
    Vector2 gameMousePos = getGameMousePos();

    BeginTextureMode(state->screenTexture);
        if (state->currentScreen == SCREEN_MENU) {
			ClearBackground(WHITE);
			menuLoop(state);
		} else if (state->currentScreen == SCREEN_SELECT) {
            selectScreen(state, gameMousePos);
        } else if (state->currentScreen == SCREEN_DIALOGUE) {
            dialogueScreen(state);
        } else if (state->currentScreen == SCREEN_GAME) {
            gameScreen(state, gameMousePos, deltaTime);
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

    UnloadMusicStream(assets.bgm);

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
    state.currentScreen = SCREEN_SELECT;
    state.selectedCharacter = 0;
    state.selectAnim = 0.0f;
    state.assets = loadAssets();
    state.currentScreen = SCREEN_MENU;
    state.testExit.position = {200.0f, 80.0f};
    state.testExit.unlocked = false;

    state.islandMap = TileMap(15, 15, 16, 16);
    state.islandMap.LoadTileset(
        "assets/TIles/Pixel_Beach_Island_Tileset_v1/spritesheet.png"
    );
    state.islandMap.Fill(14); // shallow water
    for (int y = 0; y < state.islandMap.GetHeight(); ++y) {
        for (int x = 0; x < state.islandMap.GetWidth(); ++x) {
            const int dx = x - state.islandMap.GetWidth() / 2;
            const int dy = y - state.islandMap.GetHeight() / 2;
            const int distanceSquared = dx * dx + dy * dy;

            if (distanceSquared <= 25) {
                state.islandMap.SetTile(x, y, 5 + (x + y) % 3); // grass variants
            } else if (distanceSquared <= 36) {
                state.islandMap.SetTile(x, y, (x + y) % 3); // sand variants
            }
        }
    }

    state.lightingMap = LightingMap(SCREEN_WIDTH, SCREEN_HEIGHT);
    for (int y = 58; y < 166; ++y) {
        for (int x = 140; x < 144; ++x) {
            state.lightingMap.setObstacle(x, y, 1);
        }
    }

    state.screenTexture = LoadRenderTexture(SCREEN_WIDTH, SCREEN_HEIGHT);
    SetTextureWrap(state.screenTexture.texture, TEXTURE_WRAP_CLAMP);

    #ifdef __EMSCRIPTEN__
    emscripten_set_main_loop_arg(mainLoop, &state, 0, true);
    #else
    while (!WindowShouldClose()) {
        mainLoop(&state);
    }
    #endif

    UnloadRenderTexture(state.screenTexture);

    state.islandMap.Unload();
    unloadAssets(state.assets);

    CloseAudioDevice();
    CloseWindow();

    return 0;
}
