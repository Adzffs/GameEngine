#pragma once

#include <SDL3/SDL.h>

class Graphics
{
public:
    Graphics();
    ~Graphics();

    bool Initialize();
    void ProcessEvents(bool &running);
    void Render(int playerX, int playerY);

private:
    void DrawGrid();
    void DrawPlayer(int playerX, int playerY);
    SDL_Window *window;
    SDL_Renderer *renderer;

    static constexpr int WindowWidth = 1280;
    static constexpr int WindowHeight = 720;
    static constexpr int TileSize = 32;
};