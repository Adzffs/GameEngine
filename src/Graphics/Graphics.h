#pragma once

#include <SDL3/SDL.h>
#include <memory>
#include <vector>

#include "../Entity/Entity.h"
#include "../World/Object/Resource/ResourceNode.h"

#include "../Inventory/Inventory.h"
#include "../Equipment/Equipment.h"

class Map;

class Graphics
{
public:
    Graphics();
    ~Graphics();

    bool Initialize();
    void ProcessEvents(bool &running);
    bool ConsumeClickedTile(int &tileX, int &tileY);
    bool ConsumeInventorySlotClick(
        int &slotIndex);

    bool ConsumeWeaponSlotClick();

    void Render(
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
        int nextLevelXP);

    enum class SidePanelTab
    {
        SKILLS,
        QUESTS,
        INVENTORY,
        EQUIPMENT,
        PRAYER,
        MAGIC,
        SETTINGS
    };
    bool HandleSidePanelClick(
        float mouseX,
        float mouseY);

    void DrawSidePanelTabs();

    void DrawPlaceholderPanel(
        const char *title);

    void DrawMap(Map &map);
    void DrawGrid();
    void DrawClickedTile();
    void DrawPlayer(int playerX, int playerY);
    void DrawNPCs(
        const std::vector<std::unique_ptr<Entity>> &entities);
    void DrawResources(
        const std::vector<ResourceNode> &resources);
    void DrawInventory(
        const Inventory &inventory);
    void DrawSkills(
        int woodcuttingLevel,
        int woodcuttingXP,
        int currentLevelXP,
        int nextLevelXP);
    void DrawEquipment(
        const Equipment &equipment);
    void DrawAxeIcon(
        ItemType itemType,
        float x,
        float y,
        float size);
    void DrawPickaxeIcon(
        ItemType itemType,
        float x,
        float y,
        float size);
    void DrawLogIcon(
        ItemType itemType,
        float x,
        float y,
        float size);
    SDL_Window *window;
    SDL_Renderer *renderer;

    bool clickPending;
    bool hasClickedTile;

    int clickedTileX;
    int clickedTileY;

    static constexpr int WindowWidth = 1280;
    static constexpr int WindowHeight = 720;
    static constexpr int TileSize = 32;
    SidePanelTab selectedTab;

private:
    bool inventorySlotClickPending;
    int clickedInventorySlotIndex;

    bool weaponSlotClickPending;
};
