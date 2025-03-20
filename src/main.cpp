/*
  Copyright (C) 1997-2025 Sam Lantinga <slouken@libsdl.org>

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely.
*/
#define SDL_MAIN_USE_CALLBACKS 1 /* use the callbacks instead of main() for web compatibility */
#include "common.h"

static SDL_Window *window = NULL;
static SDL_Renderer *renderer = NULL;

const int tileSize = 16;    // Assuming each texture is 16x16 pixels
const float padding = 2.0f; // Padding between tiles
const int mapWidth = tileSize * 50;
const int mapHeight = tileSize * 38;

SDL_Texture *lightningTowerTexture = nullptr; // Declare a global or static variable

struct TextureSingular
{
    SDL_Texture *texture;
    const SDL_Rect position;
};

std::vector<TextureSingular> texturesPlural;

/* List of hard-coded asset file paths */
std::vector<const char *> textureFiles = {
    "build/build-client/Debug/assets/Simple Tower Defense/Environment/Grass/spr_grass_01.png",
    "build/build-client/Debug/assets/Simple Tower Defense/Environment/Grass/spr_grass_02.png",
    "build/build-client/Debug/assets/Simple Tower Defense/Environment/Tile Set/spr_tile_set_stone.png",
    "build/build-client/Debug/assets/Simple Tower Defense/Towers/Combat Towers/spr_tower_lightning_tower.png",
    "build/build-client/Debug/assets/Simple Tower Defense/Environment/Decoration/spr_rock_01.png",
    "build/build-client/Debug/assets/Simple Tower Defense/Environment/Decoration/spr_rock_02.png",
    "build/build-client/Debug/assets/Simple Tower Defense/Environment/Decoration/spr_rock_03.png",
    // Add more file paths as needed
};

// Pretty similar to a typedef in C if I remember correctly
using namespace std;

// Function prototype for GenerateEnvironment
void GenerateEnvironment()
{
    const int gridWidth = mapWidth / tileSize;
    const int gridHeight = mapHeight / tileSize;

    // 2D grid to represent the map
    std::vector<std::vector<int>> mapGrid(gridHeight, std::vector<int>(gridWidth, 0));

    // Define tile types
    const int GRASS_1 = 0;
    const int GRASS_2 = 1;
    const int STONE_PATH = 2;
    const int LIGHTNING_TOWER = 3;
    const int ROCK_1 = 4;
    const int ROCK_2 = 5;
    const int ROCK_3 = 6;

    // Hard-code the lightning tower placement at the center of the map
    const int towerX = gridWidth / 2; // Centered horizontally
    const int towerY = gridHeight / 2; // Centered vertically
    mapGrid[towerY][towerX] = LIGHTNING_TOWER;

    // Generate a stone path leading to the tower from the left
    for (int x = 0; x <= towerX; ++x)
    {
        mapGrid[towerY][x] = STONE_PATH;
    }

    // Generate a stone path leading to the tower from the bottom
    for (int y = gridHeight - 1; y >= towerY; --y)
    {
        mapGrid[y][towerX] = STONE_PATH;
    }

    // Procedurally place rocks at random positions
    for (int i = 0; i < 10; ++i) // Place 10 rocks
    {
        int rockX = std::rand() % gridWidth;
        int rockY = std::rand() % gridHeight;

        // Ensure rocks do not overwrite the lightning tower or paths
        if (mapGrid[rockY][rockX] == 0)
        {
            int rockType = ROCK_1 + (std::rand() % 3); // Randomly select ROCK_1, ROCK_2, or ROCK_3
            mapGrid[rockY][rockX] = rockType;
        }
    }

    // Populate the rest of the map with grass textures
    for (int y = 0; y < gridHeight; ++y)
    {
        for (int x = 0; x < gridWidth; ++x)
        {
            if (mapGrid[y][x] == 0)
            {
                mapGrid[y][x] = (std::rand() % 2 == 0) ? GRASS_1 : GRASS_2;
            }
        }
    }

    // Convert the map grid to textures
    for (int y = 0; y < gridHeight; ++y)
    {
        for (int x = 0; x < gridWidth; ++x)
        {
            SDL_Texture *texture = nullptr;
            switch (mapGrid[y][x])
            {
            case GRASS_1:
                texture = IMG_LoadTexture(renderer, textureFiles[0]);
                break;
            case GRASS_2:
                texture = IMG_LoadTexture(renderer, textureFiles[1]);
                break;
            case STONE_PATH:
                texture = IMG_LoadTexture(renderer, textureFiles[2]);
                break;
            case LIGHTNING_TOWER:
                texture = IMG_LoadTexture(renderer, textureFiles[3]);
                if (texture)
                {
                    lightningTowerTexture = texture; // Store the lightning tower texture
                }
                break;
            case ROCK_1:
                texture = IMG_LoadTexture(renderer, textureFiles[4]);
                break;
            case ROCK_2:
                texture = IMG_LoadTexture(renderer, textureFiles[5]);
                break;
            case ROCK_3:
                texture = IMG_LoadTexture(renderer, textureFiles[6]);
                break;
            }

            if (!texture)
            {
                SDL_Log("Failed to load texture: %s\n", SDL_GetError());
                continue; // Skip this tile if the texture couldn't be loaded
            }

            SDL_Rect position = {x * tileSize, y * tileSize, tileSize, tileSize};
            texturesPlural.push_back({texture, position});
        }
    }
}

SDL_Texture *LoadTextureWithTransparency(SDL_Renderer *renderer, const char *filePath)
{
    SDL_Texture *texture = IMG_LoadTexture(renderer, filePath);
    if (!texture)
    {
        SDL_Log("Failed to load texture: %s\n", SDL_GetError());
        return nullptr;
    }

    // Enable blending for transparency
    SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);
    return texture;
}

/* This function runs once at startup. */
SDL_AppResult SDL_AppInit(void **appstate, int argc, char *argv[])
{

    /* Create the window */
    if (!SDL_CreateWindowAndRenderer("EndlessDungeon", mapWidth, mapHeight, SDL_WINDOW_FULLSCREEN, &window, &renderer))
    {
        SDL_Log("Couldn't create window and renderer: %s\n", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    /* Load the textures */
    GenerateEnvironment();
    return SDL_APP_CONTINUE;
}

/* This function runs when a new event (mouse input, keypresses, etc) occurs. */
SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event)
{
    if (event->type == SDL_EVENT_KEY_DOWN ||
        event->type == SDL_EVENT_QUIT)
    {
        return SDL_APP_SUCCESS; /* end the program, reporting success to the OS. */
    }
    return SDL_APP_CONTINUE;
}

/* This function runs once per frame, and is the heart of the program. */
SDL_AppResult SDL_AppIterate(void *appstate)
{
    int w = 0, h = 0;
    const float scale = 1.0f;

    /* Clear the screen */
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);

    /* Render all textures except the lightning tower */
    for (TextureSingular &texInfo : texturesPlural)
    {
        if (texInfo.texture == nullptr)
            continue;

        // Skip rendering the lightning tower for now
        if (texInfo.texture == lightningTowerTexture)
            continue;

        SDL_SetRenderScale(renderer, scale, scale);
        SDL_FRect fPosition = {static_cast<float>(texInfo.position.x), static_cast<float>(texInfo.position.y), 
                               static_cast<float>(texInfo.position.w), static_cast<float>(texInfo.position.h)};
        SDL_RenderTexture(renderer, texInfo.texture, NULL, &fPosition);
    }

    /* Render the lightning tower last */
    for (TextureSingular &texInfo : texturesPlural)
    {
        if (texInfo.texture == lightningTowerTexture)
        {
            SDL_FRect fPosition = {static_cast<float>(texInfo.position.x), static_cast<float>(texInfo.position.y), 
                                   static_cast<float>(texInfo.position.w), static_cast<float>(texInfo.position.h)};
            SDL_RenderTexture(renderer, texInfo.texture, NULL, &fPosition);
        }
    }

    /* Present the renderer */
    SDL_RenderPresent(renderer);

    return SDL_APP_CONTINUE;
}

/* This function runs once at shutdown. */
void SDL_AppQuit(void *appstate, SDL_AppResult result)
{
    for (TextureSingular &texInfo : texturesPlural)
    {
        SDL_DestroyTexture(texInfo.texture);
    }
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
}

