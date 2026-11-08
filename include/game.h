#pragma once

#include <raylib.h>
#include <vector>
#include <string>

#include "defines.h"
#include "chronos.h"
#include "animated_sprite.h"
#include "Map.h"
#include "lighting.h"

#include <algorithm>
#include <vector>
#include <raylib.h>

#include <mutex>

struct Player {
    float x, y;
    float velx, vely;
};

struct Enemy {
    Vector2 position;
    float speed;
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
    Music bgm2;
    Sound chronosVoices[3];
    Texture chronosPortrait;

    AnimatedSprite chronosSprite;
    AnimatedSprite cogSprite;
};
struct Controller{
	bool open=false;
	Vector2 cursor={0.5f,0.5f};
};



struct GameState {
    ScreenState currentScreen;
    int selectedCharacter;
    float selectAnim;
    int dialogueIndex = 0;
    int menuSelection = 0;   // 0 = Play, 1 = Credits

    Player player;
    Chronos chronos;
    TileMap islandMap;
    Vector2 mapOrigin = {0.0f, 0.0f}; // world position of the map's top-left corner
    LightingMap lightingMap;
    Collectible testItem;
    Exit testExit;
    Enemy patroller;

    Controller controller;

    // Island 2: portal trip, loop length, and the yellow riddle shadow
    int currentIsland = 1;
    float portalTimer = 0.0f;
    float loopDuration = 60.0f;
    Enemy yellowShadow;
    bool yellowShadowDefeated = false;

    // Riddle (wordle-style) minigame
    int riddleIndex = 0;
    int riddlePhase = 0;          // 0 intro, 1 typing, 2 solved, 3 failed
    int riddleIntroLine = 0;
    int riddleTriesLeft = 3;
    float riddleTimer = 0.0f;
    std::string riddleInput;
    std::string riddleMessage;
    std::vector<std::string> riddleGuesses;

    int currentLoop = 1;
    float timeRemaining = 60.0f;
    bool gameOver = false;
    bool victory = false;
    GameOverReason gameOverReason = GAME_OVER_NONE;
    bool chronosDangerous = false;
    float chronosHitCooldown = 0.0f;
    float playerHitFlash = 0.0f;
    float playerShakeTimer = 0.0f;

    // Chronos confrontation variables
    int confrontationLineIndex = 0;
    float typewriterTimer = 0.0f;
    int displayedChars = 0;
    bool voicePlayed = false;

    // Battle minigame variables
    float battleCursorPos = 0.0f;
    bool battleEnded = false;
    bool battleWon = false;
    float battleResultTimer = 0.0f;

    // Samarth dialogue & choice variables
    int samarthLineIndex = 0;
    int samarthChoiceSelected = 0;
    bool samarthChoiceActive = false;
    int luckPoints = 0;
    bool samarthResponded = false;
    float samarthResponseTimer = 0.0f;
    bool acceptedChallenge = false;
    bool shadowDefeated = false;

    GameAssets assets;

    RenderTexture2D screenTexture;
};
// riddle.cpp
void startRiddle(GameState* state);
void riddleScreen(GameState* state, float deltaTime);