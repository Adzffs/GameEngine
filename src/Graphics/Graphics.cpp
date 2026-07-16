#include "Graphics.h"
#include <iostream>
#include "../World/Map.h"
#include <string>

Graphics::Graphics()
    : window(nullptr),
      renderer(nullptr),
      clickPending(false),
      hasClickedTile(false),
      clickedTileX(0),
      clickedTileY(0)
{
}

Graphics::~Graphics()
{
    if (renderer != nullptr)
    {
        SDL_DestroyRenderer(renderer);
    }

    if (window != nullptr)
    {
        SDL_DestroyWindow(window);
    }

    SDL_Quit();
}

bool Graphics::Initialize()
{
    if (!SDL_Init(SDL_INIT_VIDEO))
    {
        std::cerr
            << "SDL initialization failed: "
            << SDL_GetError()
            << std::endl;

        return false;
    }

    if (!SDL_CreateWindowAndRenderer(
            "GameEngine",
            WindowWidth,
            WindowHeight,
            0,
            &window,
            &renderer))
    {
        std::cerr
            << "Window creation failed: "
            << SDL_GetError()
            << std::endl;

        return false;
    }

    return true;
}

void Graphics::ProcessEvents(bool &running)
{
    SDL_Event event;

    while (SDL_PollEvent(&event))
    {
        if (event.type == SDL_EVENT_QUIT)
        {
            running = false;
        }

        if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN &&
            event.button.button == SDL_BUTTON_LEFT)
        {
            clickedTileX =
                static_cast<int>(event.button.x) / TileSize;

            clickedTileY =
                static_cast<int>(event.button.y) / TileSize;

            hasClickedTile = true;
            clickPending = true;
        }
    }
}
bool Graphics::ConsumeClickedTile(
    int &tileX,
    int &tileY)
{
    if (!clickPending)
    {
        return false;
    }

    tileX = clickedTileX;
    tileY = clickedTileY;

    clickPending = false;

    return true;
}
void Graphics::DrawMap(Map &map)
{
    int visibleColumns =
        WindowWidth / TileSize;

    int visibleRows =
        WindowHeight / TileSize;

    for (int y = 0; y < visibleRows; y++)
    {
        for (int x = 0; x < visibleColumns; x++)
        {
            SDL_FRect tileRectangle{
                static_cast<float>(x * TileSize),
                static_cast<float>(y * TileSize),
                static_cast<float>(TileSize),
                static_cast<float>(TileSize)};

            if (map.IsValidPosition(x, y))
            {
                SDL_SetRenderDrawColor(
                    renderer,
                    45,
                    85,
                    45,
                    255);
            }
            else
            {
                SDL_SetRenderDrawColor(
                    renderer,
                    35,
                    45,
                    70,
                    255);
            }

            SDL_RenderFillRect(
                renderer,
                &tileRectangle);
        }
    }
}
void Graphics::DrawGrid()

{
    SDL_SetRenderDrawColor(
        renderer,
        50,
        50,
        50,
        255);

    for (int x = 0; x <= WindowWidth; x += TileSize)
    {
        SDL_RenderLine(
            renderer,
            static_cast<float>(x),
            0.0f,
            static_cast<float>(x),
            static_cast<float>(WindowHeight));
    }

    for (int y = 0; y <= WindowHeight; y += TileSize)
    {
        SDL_RenderLine(
            renderer,
            0.0f,
            static_cast<float>(y),
            static_cast<float>(WindowWidth),
            static_cast<float>(y));
    }
}
void Graphics::DrawClickedTile()
{
    if (!hasClickedTile)
    {
        return;
    }

    SDL_FRect clickedRectangle{
        static_cast<float>(
            clickedTileX * TileSize + 2),
        static_cast<float>(
            clickedTileY * TileSize + 2),
        static_cast<float>(
            TileSize - 4),
        static_cast<float>(
            TileSize - 4)};

    SDL_SetRenderDrawColor(
        renderer,
        255,
        215,
        0,
        255);

    SDL_RenderRect(
        renderer,
        &clickedRectangle);
}
void Graphics::DrawPlayer(
    int playerX,
    int playerY)
{
    SDL_FRect playerRectangle{
        static_cast<float>(
            playerX * TileSize + 4),
        static_cast<float>(
            playerY * TileSize + 4),
        static_cast<float>(
            TileSize - 8),
        static_cast<float>(
            TileSize - 8)};

    SDL_SetRenderDrawColor(
        renderer,
        50,
        150,
        255,
        255);

    SDL_RenderFillRect(
        renderer,
        &playerRectangle);
}
void Graphics::DrawNPCs(
    const std::vector<std::unique_ptr<Entity>> &entities)
{
    for (const auto &entity : entities)
    {
        if (entity->GetType() != EntityType::NPC)
        {
            continue;
        }

        SDL_FRect npcRectangle{
            static_cast<float>(
                entity->GetPosition().GetX() * TileSize + 5),
            static_cast<float>(
                entity->GetPosition().GetY() * TileSize + 5),
            static_cast<float>(TileSize - 10),
            static_cast<float>(TileSize - 10)};

        SDL_SetRenderDrawColor(
            renderer,
            255,
            150,
            50,
            255);

        SDL_RenderFillRect(
            renderer,
            &npcRectangle);
    }
}
void Graphics::DrawResources(
    const std::vector<ResourceNode> &resources)
{
    for (const ResourceNode &resource : resources)
    {
        if (resource.IsActive())
        {
            SDL_FRect resourceRectangle{
                static_cast<float>(
                    resource.GetX() * TileSize + 7),
                static_cast<float>(
                    resource.GetY() * TileSize + 7),
                static_cast<float>(TileSize - 14),
                static_cast<float>(TileSize - 14)};

            SDL_SetRenderDrawColor(
                renderer,
                140,
                90,
                40,
                255);

            SDL_RenderFillRect(
                renderer,
                &resourceRectangle);
        }
        else
        {
            SDL_FRect stumpRectangle{
                static_cast<float>(
                    resource.GetX() * TileSize + 9),
                static_cast<float>(
                    resource.GetY() * TileSize + 18),
                static_cast<float>(TileSize - 18),
                static_cast<float>(TileSize - 21)};

            SDL_SetRenderDrawColor(
                renderer,
                85,
                55,
                30,
                255);

            SDL_RenderFillRect(
                renderer,
                &stumpRectangle);
        }
    }
}
void Graphics::DrawInventory(
    const Inventory &inventory)
{
    constexpr int columns = 4;
    constexpr int rows = 7;

    constexpr float slotSize = 40.0f;
    constexpr float slotGap = 4.0f;
    constexpr float padding = 10.0f;
    constexpr float headerHeight = 24.0f;

    constexpr float panelWidth =
        padding * 2.0f +
        columns * slotSize +
        (columns - 1) * slotGap;

    constexpr float panelHeight =
        padding * 2.0f +
        headerHeight +
        rows * slotSize +
        (rows - 1) * slotGap;

    const float panelX =
        static_cast<float>(WindowWidth) -
        panelWidth -
        20.0f;

    const float panelY =
        static_cast<float>(WindowHeight) -
        panelHeight -
        20.0f;

    SDL_FRect panelRectangle{
        panelX,
        panelY,
        panelWidth,
        panelHeight};

    SDL_SetRenderDrawColor(
        renderer,
        30,
        30,
        30,
        240);

    SDL_RenderFillRect(
        renderer,
        &panelRectangle);

    SDL_SetRenderDrawColor(
        renderer,
        180,
        180,
        180,
        255);

    SDL_RenderRect(
        renderer,
        &panelRectangle);

    SDL_SetRenderDrawColor(
        renderer,
        255,
        255,
        255,
        255);

    SDL_RenderDebugText(
        renderer,
        panelX + padding,
        panelY + 8.0f,
        "INVENTORY");

    const auto &slots =
        inventory.GetSlots();

    for (int slotIndex = 0;
         slotIndex < Inventory::SlotCount;
         slotIndex++)
    {
        int column =
            slotIndex % columns;

        int row =
            slotIndex / columns;

        float slotX =
            panelX +
            padding +
            column * (slotSize + slotGap);

        float slotY =
            panelY +
            padding +
            headerHeight +
            row * (slotSize + slotGap);

        SDL_FRect slotRectangle{
            slotX,
            slotY,
            slotSize,
            slotSize};

        SDL_SetRenderDrawColor(
            renderer,
            55,
            55,
            55,
            255);

        SDL_RenderFillRect(
            renderer,
            &slotRectangle);

        SDL_SetRenderDrawColor(
            renderer,
            120,
            120,
            120,
            255);

        SDL_RenderRect(
            renderer,
            &slotRectangle);

        const InventorySlot &slot =
            slots[slotIndex];

        if (slot.IsEmpty())
        {
            continue;
        }

        if (slot.GetItemType() ==
            ItemType::LOG)
        {
            SDL_FRect logRectangle{
                slotX + 9.0f,
                slotY + 8.0f,
                slotSize - 18.0f,
                slotSize - 18.0f};

            SDL_SetRenderDrawColor(
                renderer,
                140,
                90,
                40,
                255);

            SDL_RenderFillRect(
                renderer,
                &logRectangle);
        }

        std::string amountText =
            std::to_string(
                slot.GetAmount());

        SDL_SetRenderDrawColor(
            renderer,
            255,
            255,
            255,
            255);

        SDL_RenderDebugText(
            renderer,
            slotX + 4.0f,
            slotY + 27.0f,
            amountText.c_str());
    }
}

void Graphics::Render(
    Map &map,
    const std::vector<std::unique_ptr<Entity>> &entities,
    const std::vector<ResourceNode> &resources,
    int playerX,
    int playerY,
    const Inventory &inventory)
{
    SDL_SetRenderDrawColor(
        renderer,
        20,
        20,
        20,
        255);

    SDL_RenderClear(renderer);

    DrawMap(map);
    DrawGrid();
    DrawClickedTile();
    DrawResources(resources);
    DrawNPCs(entities);
    DrawPlayer(playerX, playerY);
    DrawInventory(inventory);

    SDL_RenderPresent(renderer);
}