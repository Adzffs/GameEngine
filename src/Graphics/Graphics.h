#pragma once

#include <SDL3/SDL.h>

class Graphics
{
public:
    Graphics();
    ~Graphics();

    bool Initialize();
    void ProcessEvents(bool &running);
    bool ConsumeClickedTile(int &tileX, int &tileY);
    void Render(int playerX, int playerY);

private:
    void DrawGrid();
    void DrawClickedTile();
    void DrawPlayer(int playerX, int playerY);

    SDL_Window *window;
    SDL_Renderer *renderer;

    bool clickPending;
    bool hasClickedTile;

    int clickedTileX;
    int clickedTileY;

    static constexpr int WindowWidth = 1280;
    static constexpr int WindowHeight = 720;
    static constexpr int TileSize = 32;
};