#include <raylib.h>
#include <cctype>
#include <cmath>
#include <string>
#include <vector>

#include "game.h"
#include "defines.h"

// The yellow shadow's riddle minigame: a tiny Wordle. You type the answer
// (letters only, exactly as long as the answer) and get 3 tries. Each wrong
// guess is coloured like Wordle: green = right letter in the right place,
// yellow = right letter in the wrong place, grey = not in the answer.

struct Riddle {
    const char* question[2];   // up to two lines of text
    const char* answer;        // UPPERCASE
};

static const Riddle RIDDLES[] = {
    { { "I run all day but never move.", "What am I?" }, "CLOCK" },
};
static const int RIDDLE_COUNT = (int)(sizeof(RIDDLES) / sizeof(RIDDLES[0]));
static const int MAX_TRIES = 3;

static const char* INTRO[][2] = {
    { "Another lost soul in the loop...", "How delicious." },
    { "Answer my riddle, mortal, and", "I may let you pass." },
    { "Fail three times and you stay", "with me. Forever." },
};
static const int INTRO_COUNT = (int)(sizeof(INTRO) / sizeof(INTRO[0]));

static const Color SHADOW_GOLD = { 255, 214, 60, 255 };

static const Riddle& currentRiddle(const GameState* state) {
    return RIDDLES[state->riddleIndex % RIDDLE_COUNT];
}

void startRiddle(GameState* state) {
    state->currentScreen = SCREEN_RIDDLE;
    state->riddlePhase = 0;
    state->riddleIntroLine = 0;
    state->riddleInput.clear();
    state->riddleGuesses.clear();
    state->riddleTriesLeft = MAX_TRIES;
    state->riddleMessage.clear();
    state->riddleTimer = 0.0f;

    // Throw away any keys typed before the riddle started.
    while (GetCharPressed() > 0) {}
}

// 0 = grey, 1 = yellow, 2 = green (standard Wordle scoring, handles repeats)
static std::vector<int> scoreGuess(const std::string& guess, const std::string& answer) {
    const size_t n = answer.size();
    std::vector<int> result(n, 0);
    std::vector<bool> used(n, false);

    for (size_t i = 0; i < n && i < guess.size(); ++i) {
        if (guess[i] == answer[i]) {
            result[i] = 2;
            used[i] = true;
        }
    }
    for (size_t i = 0; i < n && i < guess.size(); ++i) {
        if (result[i] == 2) continue;
        for (size_t j = 0; j < n; ++j) {
            if (!used[j] && guess[i] == answer[j]) {
                result[i] = 1;
                used[j] = true;
                break;
            }
        }
    }
    return result;
}

static bool advancePressed() {
    return IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE) ||
           IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
}

static void drawCentered(const char* text, int y, int size, Color colour) {
    int w = MeasureText(text, size);
    DrawText(text, (SCREEN_WIDTH - w) / 2, y, size, colour);
}

void riddleScreen(GameState* state, float deltaTime) {
    state->riddleTimer += deltaTime;
    const Riddle& riddle = currentRiddle(state);
    const std::string answer = riddle.answer;
    const int cols = (int)answer.size();

    // ---- update ----
    if (state->riddlePhase == 0) {
        while (GetCharPressed() > 0) {}   // ignore typing during the intro
        if (advancePressed()) {
            state->riddleIntroLine++;
            if (state->riddleIntroLine >= INTRO_COUNT) {
                state->riddlePhase = 1;
                state->riddleMessage = "Type your answer.";
            }
        }
    } else if (state->riddlePhase == 1) {
        int key = GetCharPressed();
        while (key > 0) {
            if (key < 128 && std::isalpha(key) && (int)state->riddleInput.size() < cols) {
                state->riddleInput.push_back((char)std::toupper(key));
            }
            key = GetCharPressed();
        }
        if (IsKeyPressed(KEY_BACKSPACE) && !state->riddleInput.empty()) {
            state->riddleInput.pop_back();
        }
        if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_KP_ENTER)) {
            if ((int)state->riddleInput.size() != cols) {
                state->riddleMessage = "Need " + std::to_string(cols) + " letters!";
            } else {
                state->riddleGuesses.push_back(state->riddleInput);
                const bool correct = (state->riddleInput == answer);
                state->riddleInput.clear();

                if (correct) {
                    state->riddlePhase = 2;
                    state->riddleMessage = "Correct!";
                } else {
                    state->riddleTriesLeft--;
                    if (state->riddleTriesLeft <= 0) {
                        state->riddlePhase = 3;
                        state->riddleMessage = "No tries left...";
                    } else if (state->riddleTriesLeft == 1) {
                        state->riddleMessage = "Wrong! 1 try left.";
                    } else {
                        state->riddleMessage =
                            "Wrong! " + std::to_string(state->riddleTriesLeft) + " tries left.";
                    }
                }
            }
        }
    } else if (state->riddlePhase == 2) {
        if (advancePressed()) {
            state->yellowShadowDefeated = true;
            state->yellowShadow.position = { -9999.0f, -9999.0f };
            state->currentScreen = SCREEN_GAME;
        }
    } else {
        if (advancePressed()) {
            state->gameOver = true;
            state->gameOverReason = GAME_OVER_SHADOW;
            state->currentScreen = SCREEN_GAME;
        }
    }

    // ---- draw ----
    ClearBackground(Color{ 14, 11, 4, 255 });
    DrawTriangle(Vector2{ 0, 0 }, Vector2{ 240, 0 }, Vector2{ 240, 240 }, Color{ 80, 64, 10, 22 });

    // Pulsing yellow shadow at the top.
    const float pulse = 0.5f + 0.5f * std::sin(state->riddleTimer * 4.0f);
    const int figSize = 26 + (int)(pulse * 4.0f);
    DrawRectangle(120 - figSize / 2, 14 - 4 + (30 - figSize) / 2, figSize, figSize, Color{ 255, 214, 60, 230 });
    DrawRectangleLines(120 - figSize / 2, 14 - 4 + (30 - figSize) / 2, figSize, figSize, Color{ 120, 90, 10, 255 });

    if (state->riddlePhase == 0) {
        Rectangle box = { 10.0f, 145.0f, SCREEN_WIDTH - 20.0f, 85.0f };
        DrawRectangleRec(box, Color{ 20, 16, 6, 245 });
        DrawRectangleLinesEx(box, 1.0f, SHADOW_GOLD);

        DrawRectangle(16, 127, 96, 18, BLACK);
        DrawRectangleLines(16, 127, 96, 18, SHADOW_GOLD);
        DrawText("YELLOW SHADOW", 20, 132, 8, SHADOW_GOLD);

        int line = state->riddleIntroLine < INTRO_COUNT ? state->riddleIntroLine : INTRO_COUNT - 1;
        DrawText(INTRO[line][0], 20, 160, 8, RAYWHITE);
        DrawText(INTRO[line][1], 20, 175, 8, RAYWHITE);

        if (static_cast<int>(GetTime() * 3.0) % 2 == 0) {
            DrawText("[CLICK / ENTER]", SCREEN_WIDTH - 90, 215, 6, SHADOW_GOLD);
        }
        return;
    }

    // Riddle text
    drawCentered(riddle.question[0], 52, 8, RAYWHITE);
    drawCentered(riddle.question[1], 64, 8, SHADOW_GOLD);

    // Wordle grid: MAX_TRIES rows x cols boxes
    const int box = 20;
    const int gap = 4;
    const int gridW = cols * box + (cols - 1) * gap;
    const int gridX = (SCREEN_WIDTH - gridW) / 2;
    const int gridY = 84;

    for (int row = 0; row < MAX_TRIES; ++row) {
        const bool submitted = row < (int)state->riddleGuesses.size();
        const bool typing = (row == (int)state->riddleGuesses.size()) && state->riddlePhase == 1;

        std::vector<int> score;
        std::string letters;
        if (submitted) {
            letters = state->riddleGuesses[row];
            score = scoreGuess(letters, answer);
        } else if (typing) {
            letters = state->riddleInput;
        }

        for (int col = 0; col < cols; ++col) {
            const int x = gridX + col * (box + gap);
            const int y = gridY + row * (box + gap);

            Color fill = Color{ 28, 22, 8, 255 };
            Color edge = Color{ 90, 72, 20, 255 };
            if (submitted) {
                const int s = score[col];
                fill = (s == 2) ? Color{ 40, 150, 70, 255 }
                     : (s == 1) ? Color{ 200, 160, 30, 255 }
                                : Color{ 60, 56, 52, 255 };
                edge = fill;
            } else if (typing && col == (int)letters.size()) {
                edge = SHADOW_GOLD;   // cursor box
            }

            DrawRectangle(x, y, box, box, fill);
            DrawRectangleLines(x, y, box, box, edge);

            if (col < (int)letters.size()) {
                const char ch[2] = { letters[col], '\0' };
                const int w = MeasureText(ch, 10);
                DrawText(ch, x + (box - w) / 2, y + 5, 10, RAYWHITE);
            }
        }
    }

    // Status
    Color msgColour = RAYWHITE;
    if (state->riddlePhase == 2) msgColour = GREEN;
    else if (state->riddlePhase == 3) msgColour = RED;
    else if (state->riddleMessage.rfind("Wrong", 0) == 0 || state->riddleMessage.rfind("Need", 0) == 0) msgColour = ORANGE;
    drawCentered(state->riddleMessage.c_str(), 172, 10, msgColour);

    if (state->riddlePhase == 1) {
        const std::string tries = "Tries left: " + std::to_string(state->riddleTriesLeft);
        drawCentered(tries.c_str(), 190, 8, SHADOW_GOLD);
        drawCentered("Type  |  [ENTER] submit  |  [BACKSPACE] delete", 222, 6, Color{ 150, 130, 70, 255 });
    } else if (state->riddlePhase == 2) {
        drawCentered("Hmph... clever. Go, mortal.", 192, 8, SHADOW_GOLD);
        if (static_cast<int>(GetTime() * 3.0) % 2 == 0) {
            drawCentered("[CLICK / ENTER]", 218, 6, SHADOW_GOLD);
        }
    } else {
        const std::string reveal = "It was: " + answer;
        drawCentered(reveal.c_str(), 190, 8, SHADOW_GOLD);
        drawCentered("The shadow swallows you.", 202, 8, RED);
        if (static_cast<int>(GetTime() * 3.0) % 2 == 0) {
            drawCentered("[CLICK / ENTER]", 218, 6, SHADOW_GOLD);
        }
    }
}