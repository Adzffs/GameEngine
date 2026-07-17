#include "Graphics.h"
#include <iostream>
#include "../World/Map.h"
#include <string>
#include <array>

Graphics::Graphics()
    : window(nullptr),
      renderer(nullptr),
      clickPending(false),
      hasClickedTile(false),
      clickedTileX(0),
      clickedTileY(0),
      selectedTab(SidePanelTab::INVENTORY),
      inventorySlotClickPending(false),
      clickedInventorySlotIndex(-1),
      weaponSlotClickPending(false)
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
bool Graphics::HandleSidePanelClick(
    float mouseX,
    float mouseY)
{
    constexpr float panelWidth = 192.0f;
    constexpr float panelHeight = 348.0f;

    constexpr float tabWidth = 25.0f;
    constexpr float tabHeight = 30.0f;
    constexpr float tabGap = 2.0f;

    const float panelX =
        static_cast<float>(WindowWidth) -
        panelWidth -
        20.0f;

    const float panelY =
        static_cast<float>(WindowHeight) -
        panelHeight -
        20.0f;

    const float tabY =
        panelY -
        tabHeight -
        4.0f;

    const std::array<SidePanelTab, 7> tabs{
        SidePanelTab::SKILLS,
        SidePanelTab::QUESTS,
        SidePanelTab::INVENTORY,
        SidePanelTab::EQUIPMENT,
        SidePanelTab::PRAYER,
        SidePanelTab::MAGIC,
        SidePanelTab::SETTINGS};

    // Check tab buttons first.
    for (std::size_t tabIndex = 0;
         tabIndex < tabs.size();
         tabIndex++)
    {
        const float tabX =
            panelX +
            static_cast<float>(tabIndex) *
                (tabWidth + tabGap);

        const bool insideTab =
            mouseX >= tabX &&
            mouseX < tabX + tabWidth &&
            mouseY >= tabY &&
            mouseY < tabY + tabHeight;

        if (insideTab)
        {
            selectedTab = tabs[tabIndex];
            return true;
        }
    }

    const bool insidePanel =
        mouseX >= panelX &&
        mouseX < panelX + panelWidth &&
        mouseY >= panelY &&
        mouseY < panelY + panelHeight;

    if (!insidePanel)
    {
        return false;
    }

    // Inventory slot clicks.
    if (selectedTab ==
        SidePanelTab::INVENTORY)
    {
        constexpr int columns = 4;

        constexpr float slotSize = 40.0f;
        constexpr float slotGap = 4.0f;
        constexpr float padding = 10.0f;
        constexpr float headerHeight = 24.0f;

        for (int slotIndex = 0;
             slotIndex < Inventory::SlotCount;
             slotIndex++)
        {
            const int column =
                slotIndex % columns;

            const int row =
                slotIndex / columns;

            const float slotX =
                panelX +
                padding +
                static_cast<float>(column) *
                    (slotSize + slotGap);

            const float slotY =
                panelY +
                padding +
                headerHeight +
                static_cast<float>(row) *
                    (slotSize + slotGap);

            const bool insideSlot =
                mouseX >= slotX &&
                mouseX < slotX + slotSize &&
                mouseY >= slotY &&
                mouseY < slotY + slotSize;

            if (insideSlot)
            {
                clickedInventorySlotIndex =
                    slotIndex;

                inventorySlotClickPending =
                    true;

                return true;
            }
        }
    }

    // Equipment weapon-slot click.
    if (selectedTab ==
        SidePanelTab::EQUIPMENT)
    {
        constexpr float weaponXOffset =
            16.0f;

        constexpr float weaponYOffset =
            112.0f;

        constexpr float equipmentSlotSize =
            48.0f;

        const float weaponX =
            panelX + weaponXOffset;

        const float weaponY =
            panelY + weaponYOffset;

        const bool insideWeaponSlot =
            mouseX >= weaponX &&
            mouseX < weaponX +
                         equipmentSlotSize &&
            mouseY >= weaponY &&
            mouseY < weaponY +
                         equipmentSlotSize;

        if (insideWeaponSlot)
        {
            weaponSlotClickPending = true;
            return true;
        }
    }

    // Consume other panel clicks so they do not move the player.
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
            float mouseX = event.button.x;
            float mouseY = event.button.y;

            if (HandleSidePanelClick(
                    mouseX,
                    mouseY))
            {
                continue;
            }

            clickedTileX =
                static_cast<int>(mouseX) / TileSize;

            clickedTileY =
                static_cast<int>(mouseY) / TileSize;

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
    for (const ResourceNode &resource :
         resources)
    {
        float tileX =
            static_cast<float>(
                resource.GetX() * TileSize);

        float tileY =
            static_cast<float>(
                resource.GetY() * TileSize);

        if (!resource.IsActive())
        {
            SDL_FRect stumpRectangle{
                tileX + 11.0f,
                tileY + 19.0f,
                10.0f,
                9.0f};

            SDL_SetRenderDrawColor(
                renderer,
                85,
                55,
                30,
                255);

            SDL_RenderFillRect(
                renderer,
                &stumpRectangle);

            continue;
        }

        int leafRed = 55;
        int leafGreen = 140;
        int leafBlue = 55;

        switch (resource.GetResourceType())
        {
        case ResourceType::NORMAL_TREE:
            leafRed = 55;
            leafGreen = 140;
            leafBlue = 55;
            break;

        case ResourceType::OAK_TREE:
            leafRed = 75;
            leafGreen = 110;
            leafBlue = 40;
            break;

        case ResourceType::WILLOW_TREE:
            leafRed = 65;
            leafGreen = 135;
            leafBlue = 95;
            break;
        }

        SDL_FRect trunkRectangle{
            tileX + 13.0f,
            tileY + 14.0f,
            6.0f,
            15.0f};

        SDL_SetRenderDrawColor(
            renderer,
            125,
            80,
            40,
            255);

        SDL_RenderFillRect(
            renderer,
            &trunkRectangle);

        SDL_FRect leavesRectangle{
            tileX + 5.0f,
            tileY + 3.0f,
            22.0f,
            18.0f};

        SDL_SetRenderDrawColor(
            renderer,
            leafRed,
            leafGreen,
            leafBlue,
            255);

        SDL_RenderFillRect(
            renderer,
            &leavesRectangle);
    }
}
void Graphics::DrawSkills(
    int woodcuttingLevel,
    int woodcuttingXP,
    int currentLevelXP,
    int nextLevelXP)
{
    constexpr float panelWidth = 192.0f;
    constexpr float panelHeight = 348.0f;

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
        panelX + 10.0f,
        panelY + 10.0f,
        "SKILLS");

    SDL_RenderDebugText(
        renderer,
        panelX + 10.0f,
        panelY + 42.0f,
        "WOODCUTTING");

    std::string levelText =
        "Level: " +
        std::to_string(woodcuttingLevel);

    SDL_RenderDebugText(
        renderer,
        panelX + 10.0f,
        panelY + 64.0f,
        levelText.c_str());

    std::string xpText =
        "XP: " +
        std::to_string(woodcuttingXP) +
        " / " +
        std::to_string(nextLevelXP);

    SDL_RenderDebugText(
        renderer,
        panelX + 10.0f,
        panelY + 84.0f,
        xpText.c_str());

    float progress = 1.0f;

    if (nextLevelXP > currentLevelXP)
    {
        progress =
            static_cast<float>(
                woodcuttingXP - currentLevelXP) /
            static_cast<float>(
                nextLevelXP - currentLevelXP);
    }

    if (progress < 0.0f)
    {
        progress = 0.0f;
    }

    if (progress > 1.0f)
    {
        progress = 1.0f;
    }

    SDL_FRect progressBackground{
        panelX + 10.0f,
        panelY + 110.0f,
        panelWidth - 20.0f,
        16.0f};

    SDL_SetRenderDrawColor(
        renderer,
        55,
        55,
        55,
        255);

    SDL_RenderFillRect(
        renderer,
        &progressBackground);

    SDL_FRect progressFill{
        progressBackground.x,
        progressBackground.y,
        progressBackground.w * progress,
        progressBackground.h};

    SDL_SetRenderDrawColor(
        renderer,
        70,
        160,
        70,
        255);

    SDL_RenderFillRect(
        renderer,
        &progressFill);

    SDL_SetRenderDrawColor(
        renderer,
        130,
        130,
        130,
        255);

    SDL_RenderRect(
        renderer,
        &progressBackground);
}
void Graphics::DrawPlaceholderPanel(
    const char *title)
{
    constexpr float panelWidth = 192.0f;
    constexpr float panelHeight = 348.0f;

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
        panelX + 10.0f,
        panelY + 10.0f,
        title);

    SDL_RenderDebugText(
        renderer,
        panelX + 45.0f,
        panelY + 160.0f,
        "COMING SOON");
}
void Graphics::DrawSidePanelTabs()
{
    constexpr float panelWidth = 192.0f;
    constexpr float panelHeight = 348.0f;

    constexpr float tabWidth = 25.0f;
    constexpr float tabHeight = 30.0f;
    constexpr float tabGap = 2.0f;

    const float panelX =
        static_cast<float>(WindowWidth) -
        panelWidth -
        20.0f;

    const float panelY =
        static_cast<float>(WindowHeight) -
        panelHeight -
        20.0f;

    const float tabY =
        panelY -
        tabHeight -
        4.0f;

    const std::array<SidePanelTab, 7> tabs{
        SidePanelTab::SKILLS,
        SidePanelTab::QUESTS,
        SidePanelTab::INVENTORY,
        SidePanelTab::EQUIPMENT,
        SidePanelTab::PRAYER,
        SidePanelTab::MAGIC,
        SidePanelTab::SETTINGS};

    const std::array<const char *, 7> labels{
        "SK",
        "QU",
        "IN",
        "EQ",
        "PR",
        "MA",
        "SE"};

    for (std::size_t tabIndex = 0;
         tabIndex < tabs.size();
         tabIndex++)
    {
        const float tabX =
            panelX +
            static_cast<float>(tabIndex) *
                (tabWidth + tabGap);

        SDL_FRect tabRectangle{
            tabX,
            tabY,
            tabWidth,
            tabHeight};

        if (selectedTab == tabs[tabIndex])
        {
            SDL_SetRenderDrawColor(
                renderer,
                120,
                95,
                55,
                255);
        }
        else
        {
            SDL_SetRenderDrawColor(
                renderer,
                45,
                45,
                45,
                255);
        }

        SDL_RenderFillRect(
            renderer,
            &tabRectangle);

        SDL_SetRenderDrawColor(
            renderer,
            160,
            160,
            160,
            255);

        SDL_RenderRect(
            renderer,
            &tabRectangle);

        SDL_SetRenderDrawColor(
            renderer,
            255,
            255,
            255,
            255);

        SDL_RenderDebugText(
            renderer,
            tabX + 4.0f,
            tabY + 11.0f,
            labels[tabIndex]);
    }
}

void Graphics::DrawAxeIcon(
    ItemType itemType,
    float x,
    float y,
    float size)
{
    int headRed = 0;
    int headGreen = 0;
    int headBlue = 0;

    switch (itemType)
    {
    case ItemType::BRONZE_AXE:
        headRed = 170;
        headGreen = 110;
        headBlue = 60;
        break;

    case ItemType::IRON_AXE:
        headRed = 165;
        headGreen = 165;
        headBlue = 165;
        break;

    case ItemType::STEEL_AXE:
        headRed = 90;
        headGreen = 115;
        headBlue = 140;
        break;

    default:
        return;
    }

    SDL_FRect handleRectangle{
        x + size * 0.45f,
        y + size * 0.20f,
        size * 0.14f,
        size * 0.62f};

    SDL_SetRenderDrawColor(
        renderer,
        130,
        80,
        35,
        255);

    SDL_RenderFillRect(
        renderer,
        &handleRectangle);

    SDL_FRect axeHeadRectangle{
        x + size * 0.24f,
        y + size * 0.16f,
        size * 0.54f,
        size * 0.25f};

    SDL_SetRenderDrawColor(
        renderer,
        headRed,
        headGreen,
        headBlue,
        255);

    SDL_RenderFillRect(
        renderer,
        &axeHeadRectangle);
}

void Graphics::DrawLogIcon(
    ItemType itemType,
    float x,
    float y,
    float size)
{
    int red = 0;
    int green = 0;
    int blue = 0;

    switch (itemType)
    {
    case ItemType::LOG:
        red = 140;
        green = 90;
        blue = 40;
        break;

    case ItemType::OAK_LOG:
        red = 105;
        green = 70;
        blue = 35;
        break;

    case ItemType::WILLOW_LOG:
        red = 90;
        green = 115;
        blue = 70;
        break;

    default:
        return;
    }

    SDL_FRect logRectangle{
        x + size * 0.20f,
        y + size * 0.28f,
        size * 0.60f,
        size * 0.42f};

    SDL_SetRenderDrawColor(
        renderer,
        red,
        green,
        blue,
        255);

    SDL_RenderFillRect(
        renderer,
        &logRectangle);

    SDL_FRect logEndRectangle{
        x + size * 0.64f,
        y + size * 0.28f,
        size * 0.16f,
        size * 0.42f};

    SDL_SetRenderDrawColor(
        renderer,
        red + 25,
        green + 20,
        blue + 10,
        255);

    SDL_RenderFillRect(
        renderer,
        &logEndRectangle);
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

        DrawLogIcon(
            slot.GetItemType(),
            slotX,
            slotY,
            slotSize);

        DrawAxeIcon(
            slot.GetItemType(),
            slotX,
            slotY,
            slotSize);

        if (slot.GetAmount() > 1)
        {
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
}

void Graphics::DrawEquipment(
    const Equipment &equipment)
{
    constexpr float panelWidth = 192.0f;
    constexpr float panelHeight = 348.0f;

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
        panelX + 10.0f,
        panelY + 10.0f,
        "EQUIPMENT");

    struct EquipmentSlotDisplay
    {
        EquipmentSlotType slotType;
        const char *label;
        float x;
        float y;
    };

    const std::array<
        EquipmentSlotDisplay,
        5>
        equipmentSlots{
            EquipmentSlotDisplay{
                EquipmentSlotType::HEAD,
                "HEAD",
                panelX + 76.0f,
                panelY + 48.0f},

            EquipmentSlotDisplay{
                EquipmentSlotType::BODY,
                "BODY",
                panelX + 76.0f,
                panelY + 112.0f},

            EquipmentSlotDisplay{
                EquipmentSlotType::LEGS,
                "LEGS",
                panelX + 76.0f,
                panelY + 176.0f},

            EquipmentSlotDisplay{
                EquipmentSlotType::WEAPON,
                "WEAPON",
                panelX + 16.0f,
                panelY + 112.0f},

            EquipmentSlotDisplay{
                EquipmentSlotType::SHIELD,
                "SHIELD",
                panelX + 136.0f,
                panelY + 112.0f}};

    constexpr float slotSize = 48.0f;

    for (const EquipmentSlotDisplay &slot :
         equipmentSlots)
    {
        SDL_FRect slotRectangle{
            slot.x,
            slot.y,
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
            130,
            130,
            130,
            255);

        SDL_RenderRect(
            renderer,
            &slotRectangle);

        SDL_SetRenderDrawColor(
            renderer,
            210,
            210,
            210,
            255);

        SDL_RenderDebugText(
            renderer,
            slot.x,
            slot.y + 52.0f,
            slot.label);

        ItemType equippedItem =
            equipment.GetEquippedItem(
                slot.slotType);

        DrawAxeIcon(
            equippedItem,
            slot.x,
            slot.y,
            slotSize);
    }
}

bool Graphics::ConsumeInventorySlotClick(
    int &slotIndex)
{
    if (!inventorySlotClickPending)
    {
        return false;
    }

    slotIndex = clickedInventorySlotIndex;

    inventorySlotClickPending = false;
    clickedInventorySlotIndex = -1;

    return true;
}

bool Graphics::ConsumeWeaponSlotClick()
{
    if (!weaponSlotClickPending)
    {
        return false;
    }

    weaponSlotClickPending = false;
    return true;
}

void Graphics::Render(
    Map &map,
    const std::vector<std::unique_ptr<Entity>> &entities,
    const std::vector<ResourceNode> &resources,
    int playerX,
    int playerY,
    const Inventory &inventory,
    const Equipment &equipment,
    int woodcuttingLevel,
    int woodcuttingXP,
    int currentLevelXP,
    int nextLevelXP)
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

    DrawSidePanelTabs();

    switch (selectedTab)
    {
    case SidePanelTab::INVENTORY:
        DrawInventory(inventory);
        break;

    case SidePanelTab::EQUIPMENT:
        DrawEquipment(equipment);
        break;

    case SidePanelTab::SKILLS:
        DrawSkills(
            woodcuttingLevel,
            woodcuttingXP,
            currentLevelXP,
            nextLevelXP);
        break;

    case SidePanelTab::QUESTS:
        DrawPlaceholderPanel("QUESTS");
        break;

    case SidePanelTab::PRAYER:
        DrawPlaceholderPanel("PRAYER");
        break;

    case SidePanelTab::MAGIC:
        DrawPlaceholderPanel("MAGIC");
        break;

    case SidePanelTab::SETTINGS:
        DrawPlaceholderPanel("SETTINGS");
        break;
    }

    SDL_RenderPresent(renderer);
}