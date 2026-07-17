#pragma once

#include <SDL3/SDL.h>
#include <memory>
#include <vector>

#include "../Entity/Entity.h"
#include "../World/Object/Resource/ResourceNode.h"
#include "../World/Object/Station/CraftingStation.h"
#include "../World/Object/Station/StationType.h"

#include "../Inventory/Inventory.h"
#include "../Equipment/Equipment.h"
#include "../Recipe/RecipeType.h"

class Map;
class Player;

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
    bool ConsumeRecipeRequest(
        RecipeType &recipeType);
    bool ConsumeStationMenuClose();

    void OpenStationMenu(
        StationType stationType);
    void DrawStationMenu();

    void Render(
        Map &map,
        const std::vector<std::unique_ptr<Entity>> &entities,
        const std::vector<ResourceNode> &resources,
        const std::vector<CraftingStation> &stations,
        int playerX,
        int playerY,
        const Inventory &inventory,
        const Equipment &equipment,
        const Player &player);

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

    void HandleStationMenuClick(
        float mouseX,
        float mouseY);

    bool IsPointInsideRectangle(
        float pointX,
        float pointY,
        const SDL_FRect &rectangle) const;

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
    void DrawStations(
        const std::vector<CraftingStation> &stations);
    void DrawInventory(
        const Inventory &inventory);
    void DrawSkills(const Player &player);
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
    void DrawBarIcon(
        ItemType itemType,
        float x,
        float y,
        float size);
    void DrawLogIcon(
        ItemType itemType,
        float x,
        float y,
        float size);
    void DrawOreIcon(
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
    bool recipeRequestPending;
    RecipeType requestedRecipeType;

    bool stationMenuOpen;
    StationType openStationType;
    bool stationMenuClosePending;
};
