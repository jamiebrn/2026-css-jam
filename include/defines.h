#pragma once

#define SCREEN_WIDTH 240
#define SCREEN_HEIGHT 240

#define PLAYER_RECT_SIZE 16
#define PLAYER_VELOCITY_MAX 100
#define PLAYER_ACCELERATION 15
#define PLAYER_LIGHT_SIZE 100

// World size, in tiles. 120 x 120 tiles of 16px = 1920 x 1920 world pixels,
// about 8 screens wide. Raise MAP_*_TILES for a bigger world.
#define MAP_WIDTH_TILES 120
#define MAP_HEIGHT_TILES 120
#define TILE_SIZE 16

enum ScreenState {
    SCREEN_MENU,
    SCREEN_SELECT,
    SCREEN_CREDITS,
    SCREEN_DIALOGUE,
    SCREEN_GAME,
    SCREEN_CHRONOS_CONFRONTATION
};

enum GameOverReason {
    GAME_OVER_NONE,
    GAME_OVER_TIME,
    GAME_OVER_CAUGHT
};
