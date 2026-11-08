#pragma once

#include <raylib.h>

#include "defines.h"
#include "chronos.h"
#include "animated_sprite.h"
#include "Map.h"
#include "lighting.h"

#include <algorithm>
#include <vector>
#include <raylib.h>

struct Player {
    float x, y;
    float velx, vely;
};

enum CollectibleType {
    COLLECTIBLE_COG,
};

struct Collectible {
    CollectibleType type;
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

    int menuSelection;

    int selectedCharacter;
    float selectAnim;
    int dialogueIndex = 0;

    Player player;
    Chronos chronos;
    TileMap islandMap;
    Vector2 mapOrigin = {0.0f, 0.0f}; // world position of the map's top-left corner
    LightingMap lightingMap;
    Controller controller;

    std::vector<Collectible> collectibles;
    int collectiblesCollected;

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

    // Chronos confrontation variables
    int confrontationLineIndex = 0;
    float typewriterTimer = 0.0f;
    int displayedChars = 0;
    bool voicePlayed = false;

    GameAssets assets;

    RenderTexture2D screenTexture;
};



//Core features of GameState.
struct TimeShot{
    int currentLoop;
    float timeRemaining;

    //Physical entities.
    Player player;
    Chronos chronos;
    Collectible testItem;
    Exit testExit;
    Vector2 mapOrigin;
};

//Converts a GameState to a timeShot.


void menuScreen(GameState* state, Vector2 mousePos);
void creditsScreen(GameState* state, Vector2 mousePos);

void drawMenuBackdrop(float t, float jitterX);
