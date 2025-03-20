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
SDL_Texture *skeletonTexture = nullptr;       // Skeleton texture

const int LIGHTNING_TOWER_WIDTH = 2;  // Width in tiles
const int LIGHTNING_TOWER_HEIGHT = 2; // Height in tiles
const int SKELETON_WIDTH = 4;         // Skeleton sprite width in tiles
const int SKELETON_HEIGHT = 4;        // Skeleton sprite height in tiles

struct TextureSingular
{
    SDL_Texture *texture;
    SDL_Rect position;
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
    "build/build-client/Debug/assets/Simple Tower Defense/Enemies/spr_skeleton.png",
    // Add more file paths as needed
};

// Pretty similar to a typedef in C if I remember correctly
using namespace std;

// Helper structure for A* pathfinding
struct Node
{
    int x, y;
    float cost, heuristic;
    Node *parent;

    Node(int x, int y, float cost, float heuristic, Node *parent = nullptr)
        : x(x), y(y), cost(cost), heuristic(heuristic), parent(parent) {}

    float totalCost() const { return cost + heuristic; }

    bool operator>(const Node &other) const { return totalCost() > other.totalCost(); }
};

// Function to calculate the heuristic (Manhattan distance)
float Heuristic(int x1, int y1, int x2, int y2)
{
    return std::abs(x1 - x2) + std::abs(y1 - y2);
}

// Function to find the path using A* algorithm
std::vector<std::pair<int, int>> FindPath(const std::vector<std::vector<int>> &mapGrid, int startX, int startY, int targetX, int targetY)
{
    const int gridWidth = mapGrid[0].size();
    const int gridHeight = mapGrid.size();

    std::priority_queue<Node, std::vector<Node>, std::greater<Node>> openList;
    std::vector<std::vector<bool>> closedList(gridHeight, std::vector<bool>(gridWidth, false));

    openList.emplace(startX, startY, 0, Heuristic(startX, startY, targetX, targetY));

    while (!openList.empty())
    {
        Node current = openList.top();
        openList.pop();

        if (current.x == targetX && current.y == targetY)
        {
            // Reconstruct the path
            std::vector<std::pair<int, int>> path;
            for (Node *node = &current; node != nullptr; node = node->parent)
            {
                path.emplace_back(node->x, node->y);
            }
            std::reverse(path.begin(), path.end());
            return path;
        }

        if (closedList[current.y][current.x])
            continue;

        closedList[current.y][current.x] = true;

        // Check all neighbors
        const int dx[] = {0, 1, 0, -1};
        const int dy[] = {-1, 0, 1, 0};

        for (int i = 0; i < 4; ++i)
        {
            int nx = current.x + dx[i];
            int ny = current.y + dy[i];

            if (nx >= 0 && ny >= 0 && nx < gridWidth && ny < gridHeight && !closedList[ny][nx] && mapGrid[ny][nx] != 4 && mapGrid[ny][nx] != 5 && mapGrid[ny][nx] != 6)
            {
                float newCost = current.cost + 1;
                float heuristic = Heuristic(nx, ny, targetX, targetY);
                openList.emplace(nx, ny, newCost, heuristic, new Node(current));
            }
        }
    }

    return {}; // Return an empty path if no path is found
}

// Global variables for skeleton movement
std::vector<std::pair<int, int>> skeletonPath;
size_t skeletonPathIndex = 0;

// Global variable for the map grid
std::vector<std::vector<int>> mapGrid;

// Function prototype for GenerateEnvironment
void GenerateEnvironment()
{
    const int gridWidth = mapWidth / tileSize;
    const int gridHeight = mapHeight / tileSize;

    // Resize the global map grid
    mapGrid.resize(gridHeight, std::vector<int>(gridWidth, 0));

    // Define tile types
    const int GRASS_1 = 0;
    const int GRASS_2 = 1;
    const int STONE_PATH = 2;
    const int LIGHTNING_TOWER = 3;
    const int ROCK_1 = 4;
    const int ROCK_2 = 5;
    const int ROCK_3 = 6;

    // Hard-code the lightning tower placement at the center of the map
    const int towerX = gridWidth / 2 - LIGHTNING_TOWER_WIDTH / 2; // Centered horizontally
    const int towerY = gridHeight / 2 - LIGHTNING_TOWER_HEIGHT / 2; // Centered vertically

    for (int y = 0; y < LIGHTNING_TOWER_HEIGHT; ++y)
    {
        for (int x = 0; x < LIGHTNING_TOWER_WIDTH; ++x)
        {
            mapGrid[towerY + y][towerX + x] = LIGHTNING_TOWER;
        }
    }

    SDL_Log("Lightning tower placed at (%d, %d) with size (%d x %d tiles)", towerX, towerY, LIGHTNING_TOWER_WIDTH, LIGHTNING_TOWER_HEIGHT);

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
        if (mapGrid[rockY][rockX] == 0) // Check for empty tiles only
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

            // Skip adding individual tiles for the lightning tower
            if (mapGrid[y][x] == LIGHTNING_TOWER)
            {
                continue; // Skip this tile
            }

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

    // Add the lightning tower as a single large texture
    lightningTowerTexture = IMG_LoadTexture(renderer, textureFiles[3]);
    if (!lightningTowerTexture)
    {
        SDL_Log("Failed to load lightning tower texture: %s\n", SDL_GetError());
    }
    else
    {
        SDL_Rect towerPosition = {towerX * tileSize, towerY * tileSize, LIGHTNING_TOWER_WIDTH * tileSize, LIGHTNING_TOWER_HEIGHT * tileSize};
        texturesPlural.push_back({lightningTowerTexture, towerPosition});
    }

    // Load the skeleton texture
    skeletonTexture = IMG_LoadTexture(renderer, textureFiles[7]);
    if (!skeletonTexture)
    {
        SDL_Log("Failed to load skeleton texture: %s\n", SDL_GetError());
    }
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

    // Calculate the path for the skeleton
    const int gridWidth = mapWidth / tileSize;
    const int gridHeight = mapHeight / tileSize;
    const int startX = 0; // Starting position of the skeleton
    const int startY = 0;
    const int targetX = gridWidth / 2; // Center of the map (lightning tower)
    const int targetY = gridHeight / 2;

    skeletonPath = FindPath(mapGrid, startX, startY, targetX, targetY);

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
    static SDL_FRect skeletonPosition = {0, 0, SKELETON_WIDTH * tileSize, SKELETON_HEIGHT * tileSize};
    static float skeletonAngle = 0.0f;

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

        SDL_FRect positionF = {static_cast<float>(texInfo.position.x), static_cast<float>(texInfo.position.y), static_cast<float>(texInfo.position.w), static_cast<float>(texInfo.position.h)};
        SDL_RenderTexture(renderer, texInfo.texture, NULL, &positionF);
    }

    /* Render the lightning tower last */
    for (TextureSingular &texInfo : texturesPlural)
    {
        if (texInfo.texture == lightningTowerTexture)
        {
            SDL_FRect positionF = {static_cast<float>(texInfo.position.x), static_cast<float>(texInfo.position.y), static_cast<float>(texInfo.position.w), static_cast<float>(texInfo.position.h)};
            SDL_RenderTexture(renderer, texInfo.texture, NULL, &positionF);
        }
    }

    /* Move the skeleton along the path */
    if (!skeletonPath.empty() && skeletonPathIndex < skeletonPath.size())
    {
        int targetX = skeletonPath[skeletonPathIndex].first * tileSize;
        int targetY = skeletonPath[skeletonPathIndex].second * tileSize;

        float dx = targetX - skeletonPosition.x;
        float dy = targetY - skeletonPosition.y;
        float distance = std::sqrt(dx * dx + dy * dy);

        if (distance < 1.0f)
        {
            // Move to the next point in the path
            skeletonPathIndex++;
        }
        else
        {
            // Move towards the target
            skeletonPosition.x += dx / distance;
            skeletonPosition.y += dy / distance;

            // Update the skeleton's angle
            skeletonAngle = std::atan2(dy, dx) * 180.0f / M_PI;
        }
    }

    /* Render the skeleton */
    if (skeletonTexture)
    {
        SDL_RenderTextureRotated(renderer, skeletonTexture, NULL, &skeletonPosition, skeletonAngle, NULL, SDL_FLIP_NONE);
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
    SDL_DestroyTexture(skeletonTexture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
}

