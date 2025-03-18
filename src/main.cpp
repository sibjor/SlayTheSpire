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
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <SDL3_image/SDL_image.h>
#include <iostream>
#include <vector>
#include <string>
#include <cstdlib>
#include <ctime>
#include <random>

static SDL_Window *window = NULL;
static SDL_Renderer *renderer = NULL;

const int tileSize = 16;    // Assuming each texture is 16x16 pixels
const float padding = 2.0f; // Padding between tiles
const int mapWidth = tileSize * 50;
const int mapHeight = tileSize * 38;

struct TextureSingular
{
    SDL_Texture *texture;
    const SDL_Rect position;
};

std::vector<TextureSingular> texturesPlural;

/* List of hard-coded asset file paths */
std::vector<const char *> grassFiles = {
    "Debug/assets/Simple Tower Defense/Environment/Grass/spr_grass_01.png",
    "Debug/assets/Simple Tower Defense/Environment/Grass/spr_grass_02.png",
    // Add more file paths as needed
};

// Pretty similar to a typedef in C if I remember correctly
using namespace std;

/* This function runs once at startup. */
SDL_AppResult SDL_AppInit(void **appstate, int argc, char *argv[])
{

    /* Create the window */
    if (!SDL_CreateWindowAndRenderer("EndlessDungeon", mapWidth, mapHeight, SDL_WINDOW_FULLSCREEN, &window, &renderer))
    {
        SDL_Log("Couldn't create window and renderer: %s\n", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    /* Load the SDL_Textures to the vector */
    std::srand(std::time(nullptr)); // Seed the random number generator

    for (const char *path : grassFiles)
    {
        SDL_Surface *surface = IMG_Load(path);
        if (!surface)
        {
            SDL_Log("Couldn't load image: %s\n", SDL_GetError());
            return SDL_APP_FAILURE;
        }

        SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);
        SDL_DestroySurface(surface); // Free the surface after creating the texture

        if (!texture)
        {
            SDL_Log("Couldn't create texture: %s\n", SDL_GetError());
            return SDL_APP_FAILURE;
        }

        // Generate random positions within the map bounds
        int x = std::rand() % (mapWidth - tileSize);
        int y = std::rand() % (mapHeight - tileSize);

        // Assign the texture and position to the vector
        SDL_Rect position = {x, y, tileSize, tileSize};
        texturesPlural.push_back({texture, position});
    }
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

    /* Render each texture */
    for (TextureSingular &texInfo : texturesPlural)
    {
        SDL_SetRenderScale(renderer, scale, scale);
        SDL_FRect fPosition = {static_cast<float>(texInfo.position.x), static_cast<float>(texInfo.position.y), 
                               static_cast<float>(texInfo.position.w), static_cast<float>(texInfo.position.h)};
        SDL_RenderTexture(renderer, texInfo.texture, NULL, &fPosition);
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

/* Function to generate the envorinment */
void GenerateEnvironment();
void DrawGrass();
void DrawStonePath();