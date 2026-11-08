#include <raylib.h>
#include <cmath>

#include "game.h"

#include "defines.h"
#include "util.h"

static const float PI_F = 3.14159265f;

static void drawGoldClock(float t) {
    const float CLOCK_SPEED = 20.0f;   // simulated seconds per real second
    float ct = t * CLOCK_SPEED;

    Vector2 c = { SCREEN_WIDTH / 2.0f, 118.0f };
    const float R = 78.0f;

    // Dark disc and gold bezel
    DrawCircleV(c, R, Color{ 14, 8, 22, 150 });
    DrawRing(c, R - 3.0f, R, 0.0f, 360.0f, 72, Color{ 212, 175, 55, 70 });
    DrawRing(c, R - 14.0f, R - 13.0f, 0.0f, 360.0f, 72, Color{ 212, 175, 55, 40 });

    // Hour ticks
    for (int i = 0; i < 12; i++) {
        float a = ((float)i * 30.0f - 90.0f) * PI_F / 180.0f;
        float inner = (i % 3 == 0) ? R - 12.0f : R - 8.0f;
        Vector2 p1 = { c.x + std::cos(a) * inner,      c.y + std::sin(a) * inner };
        Vector2 p2 = { c.x + std::cos(a) * (R - 4.0f), c.y + std::sin(a) * (R - 4.0f) };
        DrawLineEx(p1, p2, (i % 3 == 0) ? 2.0f : 1.0f, Color{ 212, 175, 55, 150 });
    }

    // Hands, driven by simulated clock time
    float secDeg  = std::fmod(std::floor(ct), 60.0f) * 6.0f;
    float minDeg  = std::fmod(ct / 60.0f, 60.0f) * 6.0f;
    float hourDeg = std::fmod(ct / 720.0f, 12.0f) * 30.0f;

    auto handEnd = [&](float deg, float len) {
        float a = (deg - 90.0f) * PI_F / 180.0f;
        return Vector2{ c.x + std::cos(a) * len, c.y + std::sin(a) * len };
    };

    DrawLineEx(c, handEnd(hourDeg, R * 0.45f), 3.0f, Color{ 212, 175, 55, 170 });
    DrawLineEx(c, handEnd(minDeg, R * 0.68f), 2.0f, Color{ 212, 175, 55, 200 });

    Vector2 secTip  = handEnd(secDeg, R * 0.78f);
    Vector2 secTail = handEnd(secDeg + 180.0f, R * 0.18f);
    DrawLineEx(secTail, secTip, 1.0f, Color{ 230, 110, 200, 220 });

    // Centre pin flashes briefly on each simulated tick
    bool flash = std::fmod(ct, 1.0f) < 0.25f;
    DrawCircleV(c, 3.0f, Color{ 240, 210, 120, (unsigned char)(flash ? 255 : 140) });
}

static float hash01(float s) {
    return std::fabs(std::fmod(std::sin(s) * 43758.5453f, 1.0f));
}

static void drawFlyingClocks(float t) {
    const int N = 32;
    const float span = 640.0f;   // distance travelled along the line before wrapping

    for (int i = 0; i < N; i++) {
        float seed = (float)i * 12.9898f;
        float r1 = hash01(seed);
        float r2 = hash01(seed + 1.0f);
        float r3 = hash01(seed + 2.0f);
        float r4 = hash01(seed + 3.0f);

        // Each clock sits on a line y = x + c. All lines are parallel, with different c values.
        float c = -(float)SCREEN_WIDTH + r1 * (float)(SCREEN_WIDTH + SCREEN_HEIGHT);

        // Travel along the direction (1, 1), which reads as y = -x on screen
        float speed = 18.0f + r2 * 22.0f;
        float p = std::fmod(r3 * span + t * speed, span) - 200.0f;
        Vector2 pos = { p, p + c };

        float radius = 5.0f + r4 * 7.0f;
        float dir = (i % 2 == 0) ? 1.0f : -1.0f;
        float rot = t * (1.0f + r4 * 2.0f) * dir;
        unsigned char alpha = (unsigned char)(50 + r3 * 60.0f);

        // Face and rim
        DrawCircleV(pos, radius, Color{ 14, 8, 22, (unsigned char)(alpha / 2) });
        DrawRing(pos, radius - 1.0f, radius, 0.0f, 360.0f, 20, Color{ 212, 175, 55, alpha });

        // Spinning hands
        Vector2 hourTip = { pos.x + std::cos(rot) * radius * 0.5f, pos.y + std::sin(rot) * radius * 0.5f };
        float minAngle = rot * 12.0f;
        Vector2 minTip  = { pos.x + std::cos(minAngle) * radius * 0.8f, pos.y + std::sin(minAngle) * radius * 0.8f };
        DrawLineEx(pos, hourTip, 1.0f, Color{ 212, 175, 55, alpha });
        DrawLineEx(pos, minTip, 1.0f, Color{ 150, 80, 200, alpha });
    }
}

void drawMenuBackdrop(float t, float jitterX) {
    ClearBackground(Color{ 10, 6, 16, 255 });

    DrawSlantedQuad(-35.0f, 0.0f, 55.0f, (float)SCREEN_HEIGHT, 55.0f, Color{ 22, 12, 34, 255 });
    DrawSlantedQuad(115.0f, 0.0f, 75.0f, (float)SCREEN_HEIGHT, 48.0f, Color{ 28, 16, 44, 255 });

    drawFlyingClocks(t);
    drawGoldClock(t);

    float scanY = std::fmod(t * 36.0f, (float)SCREEN_HEIGHT);
    DrawRectangle(0, (int)scanY, SCREEN_WIDTH, 2, Color{ 212, 175, 55, 35 });
}

static void drawMenuButton(const char* label, float x, float y, float w, float h,
                           bool selected, float jitterX, float jitterY) {
    float slant = -10.0f;
    float bx = x + (selected ? jitterX : 0.0f);
    float by = y + (selected ? jitterY : 0.0f);

    if (selected) {
        DrawSlantedQuad(bx + 5.0f, by + 4.0f, w, h, slant - 2.0f, Color{ 150, 80, 200, 110 });
        DrawSlantedQuad(bx - 3.0f, by - 2.0f, w, h, slant + 1.0f, Color{ 212, 175, 55, 75 });
    } else {
        DrawSlantedQuad(bx + 3.0f, by + 4.0f, w, h, slant, Color{ 5, 3, 8, 210 });
    }

    Color cardColor = selected ? Color{ 40, 24, 58, 255 } : Color{ 18, 11, 26, 255 };
    DrawSlantedQuad(bx, by, w, h, slant, cardColor);

    // Accent strips lean in proportion to their own height, matching the card's slope.
    float topSlant = slant * 4.0f / h;
    float botSlant = slant * 2.0f / h;

    Color accentColor = selected ? Color{ 150, 80, 200, 255 } : Color{ 96, 60, 130, 120 };
    DrawSlantedQuad(bx + 1.0f, by, w - 2.0f, 4.0f, topSlant, accentColor);
    DrawSlantedQuad(bx, by + h - 2.0f, w, 2.0f, botSlant,
                    selected ? Color{ 212, 175, 55, 255 } : Color{ 34, 22, 46, 255 });

    int fontSize = selected ? 12 : 10;
    if (selected) {
        DrawText(">", (int)(bx + 12.0f), (int)(by + h / 2.0f - 6.0f), 10, Color{ 212, 175, 55, 255 });
    }
    int nw = MeasureText(label, fontSize);
    DrawText(label, (int)(bx + (w - nw) / 2.0f), (int)(by + h / 2.0f - fontSize / 2.0f), fontSize,
             selected ? Color{ 255, 230, 160, 255 } : Color{ 120, 100, 140, 255 });
}

static const float BTN_W = 200.0f;
static const float BTN_H = 36.0f;
static const float BTN_X = (SCREEN_WIDTH - BTN_W) / 2.0f;
static const float PLAY_Y = 110.0f;
static const float CREDITS_Y = 158.0f;

static void activateMenuItem(GameState* state, int item) {
    if (item == 0) {
        state->selectedCharacter = 0;
        state->currentScreen = SCREEN_SELECT;
    } else {
        state->currentScreen = SCREEN_CREDITS;
    }
}

void menuScreen(GameState* state, Vector2 mousePos) {
    if (IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W))   state->menuSelection = 0;
    if (IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S)) state->menuSelection = 1;

    if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE)) {
        activateMenuItem(state, state->menuSelection);
    }

    Rectangle playHit    = { BTN_X, PLAY_Y, BTN_W, BTN_H };
    Rectangle creditsHit = { BTN_X, CREDITS_Y, BTN_W, BTN_H };

    if (CheckCollisionPointRec(mousePos, playHit)) {
        state->menuSelection = 0;
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) activateMenuItem(state, 0);
    }
    if (CheckCollisionPointRec(mousePos, creditsHit)) {
        state->menuSelection = 1;
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) activateMenuItem(state, 1);
    }

    float t = (float)GetTime();
    float jitterX = (std::sin(t * 35.0f) > 0.92f) ? 2.0f : 0.0f;
    float jitterY = (std::cos(t * 27.0f) < -0.94f) ? -1.0f : 0.0f;

    drawMenuBackdrop(t, jitterX);

    // Title panel
    DrawSlantedQuad(12.0f + jitterX, 26.0f, 296.0f, 52.0f, -8.0f, Color{ 150, 80, 200, 90 });
    DrawSlantedQuad(10.0f, 24.0f, 296.0f, 52.0f, -8.0f, Color{ 24, 14, 36, 255 });
    DrawSlantedQuad(12.0f, 24.0f, 6.0f, 52.0f, -8.0f, Color{ 212, 175, 55, 255 });
    DrawSlantedQuad(18.0f, 24.0f, 2.0f, 52.0f, -8.0f, Color{ 150, 80, 200, 255 });

    const char* title = "THE LAST LOOP";
    int tw = MeasureText(title, 20);
    int tx = (int)((SCREEN_WIDTH - tw) / 2.0f) + (int)jitterX;
    DrawText(title, tx + 2, 43, 20, Color{ 150, 80, 200, 200 });
    DrawText(title, tx, 41, 20, Color{ 240, 210, 130, 255 });

    drawMenuButton("PLAY", BTN_X, PLAY_Y, BTN_W, BTN_H,
                   state->menuSelection == 0, jitterX, jitterY);
    drawMenuButton("CREDITS", BTN_X, CREDITS_Y, BTN_W, BTN_H,
                   state->menuSelection == 1, jitterX, jitterY);

    DrawText("Up/Down", 12, 222, 7, Color{ 212, 175, 55, 200 });
    DrawText("[ENTER] Select", 140, 222, 7, Color{ 220, 190, 120, 255 });
}

void creditsScreen(GameState* state, Vector2 mousePos) {
    if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_ESCAPE)) {
        state->currentScreen = SCREEN_MENU;
    }

    Rectangle backHit = { BTN_X, 180.0f, BTN_W, BTN_H };
    if (CheckCollisionPointRec(mousePos, backHit) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        state->currentScreen = SCREEN_MENU;
    }

    float t = (float)GetTime();
    float jitterX = (std::sin(t * 35.0f) > 0.92f) ? 2.0f : 0.0f;

    drawMenuBackdrop(t, jitterX);

    // Header
    DrawSlantedQuad(12.0f + jitterX, 14.0f, 220.0f, 22.0f, -15.0f, Color{ 150, 80, 200, 90 });
    DrawSlantedQuad(10.0f, 12.0f, 220.0f, 22.0f, -15.0f, Color{ 24, 14, 36, 255 });
    DrawSlantedQuad(12.0f, 12.0f, 6.0f, 22.0f, -15.0f, Color{ 212, 175, 55, 255 });
    DrawSlantedQuad(18.0f, 12.0f, 2.0f, 22.0f, -15.0f, Color{ 150, 80, 200, 255 });
    DrawText("Credits", 28 + (int)jitterX, 17, 10, Color{ 240, 210, 130, 255 });

    // Credits panel
    DrawSlantedQuad(18.0f, 50.0f + 4.0f, 280.0f, 110.0f, -6.0f, Color{ 150, 80, 200, 80 });
    DrawSlantedQuad(14.0f, 50.0f, 282.0f, 110.0f, -6.0f, Color{ 20, 12, 30, 255 });
    DrawSlantedQuad(12.0f, 50.0f, 5.0f, 110.0f, -6.0f, Color{ 212, 175, 55, 255 });
    DrawSlantedQuad(17.0f, 50.0f, 2.0f, 110.0f, -6.0f, Color{ 150, 80, 200, 255 });

    DrawText("Game Design", 26, 62, 8, Color{ 212, 175, 55, 255 });
    DrawText("Your Name", 26, 74, 8, Color{ 190, 170, 200, 255 });

    DrawText("Programming", 26, 92, 8, Color{ 212, 175, 55, 255 });
    DrawText("Your Name", 26, 104, 8, Color{ 190, 170, 200, 255 });

    DrawText("Art and Audio", 26, 122, 8, Color{ 212, 175, 55, 255 });
    DrawText("Your Name", 26, 134, 8, Color{ 190, 170, 200, 255 });

    drawMenuButton("BACK", BTN_X, 180.0f, BTN_W, BTN_H, true, jitterX, 0.0f);

    DrawText("[ENTER] Back", 140, 222, 7, Color{ 220, 190, 120, 255 });
}
