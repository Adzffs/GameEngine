#include "Graphics.h"
#include <iostream>
#include "../World/Map.h"
#include "../Player/Player.h"
#include "../Entity/Monster/Monster.h"
#include "../NPC/NPC.h"
#include "../NPC/NpcDefinitionDatabase.h"
#include "../Item/ItemDatabase.h"
#include "../Recipe/RecipeDatabase.h"
#include "../Quest/QuestDefinitionDatabase.h"
#include "../Combat/Combatant.h"
#include "../World/Object/Resource/ResourceDatabase.h"
#include "../World/Object/Resource/DepletedVisualType.h"
#include "../Entity/EntityType.h"
#include <string>
#include <array>
#include <algorithm>
#include <map>
#include <cmath>
#include <cctype>

namespace
{
    const char *GetSkillDisplayName(SkillType skillType)
    {
        switch (skillType)
        {
        case SkillType::ATTACK: return "Attack";
        case SkillType::DEFENCE: return "Defence";
        case SkillType::WOODCUTTING: return "Woodcutting";
        case SkillType::MINING: return "Mining";
        case SkillType::SMITHING: return "Smithing";
        default: return "Skill";
        }
    }

    Entity *GetEntityByID(
        const std::vector<std::unique_ptr<Entity>> &entities,
        int entityID)
    {
        for (const auto &entity : entities)
        {
            if (entity->GetID() == entityID)
            {
                return entity.get();
            }
        }

        return nullptr;
    }

    SDL_FRect MakeEntityTileRectangle(
        int tileX,
        int tileY,
        float inset,
        int cameraTileX,
        int cameraTileY)
    {
        return SDL_FRect{
            static_cast<float>(
                (tileX - cameraTileX) * Graphics::TileSize) +
                inset,
            static_cast<float>(
                (tileY - cameraTileY) * Graphics::TileSize) +
                inset,
            static_cast<float>(Graphics::TileSize) - inset * 2.0f,
            static_cast<float>(Graphics::TileSize) - inset * 2.0f};
    }
}

void Graphics::UpdateProgressionFeedback(
    const Player &player,
    const Inventory &inventory)
{
    for (ActiveProgressionNotice &notice : progressionNotices)
    {
        notice.remainingSeconds -= presentationDeltaSeconds;
    }
    std::erase_if(progressionNotices,
        [](const ActiveProgressionNotice &notice)
        {
            return notice.remainingSeconds <= 0.0f;
        });

    const std::vector<ProgressionEvent> events =
        progressionPresentationState.Synchronize(
            player.GetSkills(), inventory);
    for (const ProgressionEvent &event : events)
    {
        std::string text;
        float lifetime = 2.4f;
        if (event.type == ProgressionEventType::XP_GAINED)
        {
            text = "+" + std::to_string(event.amount) + " " +
                GetSkillDisplayName(event.skillType) + " XP";
        }
        else if (event.type == ProgressionEventType::LEVEL_GAINED)
        {
            text = std::string(GetSkillDisplayName(event.skillType)) +
                " reached level " +
                std::to_string(event.level) + "!";
            lifetime = 4.0f;
        }
        else
        {
            text = "+" + std::to_string(event.amount) + " " +
                ItemDatabase::Get(event.itemType).GetName();
        }
        progressionNotices.push_back({event.type, std::move(text), lifetime});
    }

    constexpr std::size_t MaximumNotices = 6;
    if (progressionNotices.size() > MaximumNotices)
    {
        progressionNotices.erase(
            progressionNotices.begin(),
            progressionNotices.end() - MaximumNotices);
    }
}

void Graphics::DrawProgressionFeedback()
{
    float y = 22.0f;
    for (const ActiveProgressionNotice &notice : progressionNotices)
    {
        const bool levelUp =
            notice.type == ProgressionEventType::LEVEL_GAINED;
        const SDL_FRect panel{
            levelUp ? 430.0f : 680.0f,
            y,
            levelUp ? 400.0f : 260.0f,
            levelUp ? 42.0f : 32.0f};
        SDL_SetRenderDrawColor(renderer, 20, 24, 28, 235);
        SDL_RenderFillRect(renderer, &panel);
        if (levelUp)
        {
            SDL_SetRenderDrawColor(renderer, 245, 190, 74, 255);
        }
        else if (notice.type == ProgressionEventType::XP_GAINED)
        {
            SDL_SetRenderDrawColor(renderer, 105, 190, 235, 255);
        }
        else
        {
            SDL_SetRenderDrawColor(renderer, 115, 205, 130, 255);
        }
        SDL_RenderRect(renderer, &panel);
        SDL_RenderDebugText(renderer,
            panel.x + (levelUp ? 22.0f : 12.0f),
            panel.y + (levelUp ? 16.0f : 12.0f),
            notice.text.c_str());
        y += panel.h + 7.0f;
    }
}

Graphics::Graphics()
    : window(nullptr),
      renderer(nullptr),
      clickPending(false),
      hasClickedTile(false),
      clickedTileX(0),
      clickedTileY(0),
      clickedMouseX(0),
      clickedMouseY(0),
      selectedTab(SidePanelTab::INVENTORY),
      inventorySlotClickPending(false),
      clickedInventorySlotIndex(-1),
      weaponSlotClickPending(false),
      recipeRequestPending(false),
      requestedRecipeType(RecipeType::NONE),
      stationMenuOpen(false),
      openStationType(StationType::NONE),
      stationMenuClosePending(false),
      pendingDialogueCommand(std::nullopt),
      pendingShopCommand(std::nullopt)
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

    if (!SDL_SetRenderLogicalPresentation(
            renderer,
            WindowWidth,
            WindowHeight,
            SDL_LOGICAL_PRESENTATION_LETTERBOX))
    {
        std::cerr << "Logical presentation setup failed: "
                  << SDL_GetError() << std::endl;
        return false;
    }

    SDL_SetWindowTitle(window, "Embervale - Adventure Prototype");

    return true;
}
void Graphics::OpenStationMenu(
    StationType stationType)
{
    DismissStationMenu(false);
    const bool supportedStation =
        stationType == StationType::FURNACE ||
        stationType == StationType::CRAFTING_TABLE ||
        stationType == StationType::BANK;
    openStationType = supportedStation
        ? stationType
        : StationType::NONE;
    stationMenuOpen = supportedStation;
    stationFeedback.clear();
    if (stationType == StationType::BANK && window != nullptr)
        SDL_StartTextInput(window);
}

bool Graphics::IsStationMenuOpen() const
{
    return stationMenuOpen;
}

StationType Graphics::GetOpenStationType() const
{
    return openStationType;
}

void Graphics::DismissStationMenu(
    bool queueCloseCommand)
{
    const bool wasOpen = stationMenuOpen;
    const StationType closingType = openStationType;
    stationMenuOpen = false;
    openStationType = StationType::NONE;
    recipeRequestPending = false;
    requestedRecipeType = RecipeType::NONE;
    craftingGridRequestPending = false;
    requestedCraftingGrid.fill(ItemType::NONE);
    craftingGrid.fill(ItemType::NONE);
    selectedCraftingItem = ItemType::NONE;
    pendingCraftingInteraction.reset();
    craftingDragActive = false;
    craftingDragSlotIndices.clear();
    stationFeedback.clear();
    stationMenuClosePending = false;
    if (closingType == StationType::BANK && window != nullptr)
        SDL_StopTextInput(window);

    if (queueCloseCommand && wasOpen && closingType == StationType::BANK &&
        bankSessionId != InvalidBankSessionId)
    {
        pendingBankCommand = BankCloseCommand{
            bankActorEntityID, bankSessionId};
    }
    else if (queueCloseCommand && wasOpen)
    {
        stationMenuClosePending = true;
    }
}

Graphics::FurnaceMenuLayout Graphics::GetFurnaceMenuLayout() const
{
    constexpr float menuWidth = 360.0f;
    constexpr float menuHeight = 280.0f;
    const float menuX =
        (WindowWidth - menuWidth) / 2.0f;
    const float menuY =
        (WindowHeight - menuHeight) / 2.0f;

    FurnaceMenuLayout layout;
    layout.panelBounds = {
        menuX, menuY, menuWidth, menuHeight};
    layout.closeButton = {
        menuX + menuWidth - 86.0f,
        menuY + 12.0f,
        68.0f,
        28.0f};
    layout.recipeButtons = {{
        {menuX + 18.0f, menuY + 52.0f, menuWidth - 36.0f, 42.0f},
        {menuX + 18.0f, menuY + 104.0f, menuWidth - 36.0f, 42.0f},
        {menuX + 18.0f, menuY + 156.0f, menuWidth - 36.0f, 42.0f},
    }};
    return layout;
}

Graphics::CraftingTableLayout Graphics::GetCraftingTableLayout() const
{
    constexpr float menuWidth = 820.0f;
    constexpr float menuHeight = 470.0f;
    constexpr float slotSize = 64.0f;
    constexpr float slotGap = 8.0f;
    const float menuX =
        (WindowWidth - menuWidth) / 2.0f;
    const float menuY =
        (WindowHeight - menuHeight) / 2.0f;

    CraftingTableLayout layout;
    layout.panelBounds = {
        menuX, menuY, menuWidth, menuHeight};
    layout.closeButton = {
        menuX + menuWidth - 98.0f,
        menuY + 16.0f,
        80.0f,
        32.0f};

    const float gridX = menuX + 40.0f;
    const float gridY = menuY + 92.0f;
    for (std::size_t index = 0;
         index < layout.gridSlots.size();
         ++index)
    {
        const float column = static_cast<float>(index % 3);
        const float row = static_cast<float>(index / 3);
        layout.gridSlots[index] = {
            gridX + column * (slotSize + slotGap),
            gridY + row * (slotSize + slotGap),
            slotSize,
            slotSize};
    }

    const float inventoryX = menuX + 300.0f;
    const float inventoryY = menuY + 92.0f;
    constexpr float inventorySlotSize = 36.0f;
    constexpr float inventoryGap = 4.0f;
    for (std::size_t index = 0; index < layout.inventorySlots.size(); ++index)
    {
        const float column = static_cast<float>(index % 4);
        const float row = static_cast<float>(index / 4);
        layout.inventorySlots[index] = {
            inventoryX + column * (inventorySlotSize + inventoryGap),
            inventoryY + row * (inventorySlotSize + inventoryGap),
            inventorySlotSize, inventorySlotSize};
    }

    layout.ingredientButtons = {{
        {menuX + 490.0f, menuY + 92.0f, 280.0f, 48.0f},
        {menuX + 490.0f, menuY + 150.0f, 280.0f, 48.0f},
    }};
    layout.previewBounds = {
        menuX + 490.0f, menuY + 218.0f, 280.0f, 88.0f};
    layout.clearButton = {
        menuX + 40.0f, menuY + 338.0f, 150.0f, 42.0f};
    layout.craftButton = {
        menuX + 620.0f, menuY + 338.0f, 150.0f, 42.0f};
    return layout;
}

const CraftingRecipeMatcher::Grid &Graphics::GetCraftingGrid() const
{
    return craftingGrid;
}

int Graphics::GetDisplayedCraftingItemCount(
    ItemType itemType) const
{
    if (itemType == ItemType::LOG)
    {
        return displayedLogCount;
    }

    if (itemType == ItemType::WOODEN_PLANKS)
    {
        return displayedPlankCount;
    }

    return 0;
}

int Graphics::GetCraftingGridItemCount(
    ItemType itemType) const
{
    return static_cast<int>(std::count(
        craftingGrid.begin(),
        craftingGrid.end(),
        itemType));
}

bool Graphics::IsPointInsideRectangle(
    float pointX,
    float pointY,
    const SDL_FRect &rectangle) const
{
    return pointX >= rectangle.x &&
           pointX <= rectangle.x + rectangle.w &&
           pointY >= rectangle.y &&
           pointY <= rectangle.y + rectangle.h;
}

void Graphics::HandleStationMenuClick(
    float mouseX,
    float mouseY,
    bool rightClick,
    bool shiftHeld,
    bool doubleClick)
{
    if (!stationMenuOpen || recipeCommandInFlight || craftingInteractionInFlight)
    {
        return;
    }

    if (openStationType == StationType::BANK)
    {
        HandleBankPanelClick(mouseX, mouseY);
        return;
    }

    if (openStationType == StationType::FURNACE)
    {
        const FurnaceMenuLayout layout =
            GetFurnaceMenuLayout();

        if (IsPointInsideRectangle(
                mouseX, mouseY, layout.closeButton))
        {
            DismissStationMenu(true);
            return;
        }

        if (recipeActionActive)
        {
            return;
        }

        const std::array<RecipeType, 3> recipes{
            RecipeType::BRONZE_BAR,
            RecipeType::IRON_BAR,
            RecipeType::STEEL_BAR};

        for (std::size_t index = 0;
             index < recipes.size();
             ++index)
        {
            if (IsPointInsideRectangle(
                    mouseX,
                    mouseY,
                    layout.recipeButtons[index]))
            {
                requestedRecipeType = recipes[index];
                recipeRequestPending = true;
                stationFeedback.clear();
                return;
            }
        }

        return;
    }

    if (openStationType != StationType::CRAFTING_TABLE)
    {
        return;
    }

    const CraftingTableLayout layout =
        GetCraftingTableLayout();

    if (IsPointInsideRectangle(
            mouseX, mouseY, layout.closeButton))
    {
        DismissStationMenu(true);
        return;
    }

    for (std::size_t index = 0; index < layout.inventorySlots.size(); ++index)
    {
        if (!IsPointInsideRectangle(mouseX, mouseY, layout.inventorySlots[index]))
            continue;
        const int unifiedIndex = static_cast<int>(index);
        if (craftingSessionSnapshot.has_value() &&
            !craftingSessionSnapshot->cursor.IsEmpty() && !shiftHeld && !doubleClick)
        {
            craftingDragActive = true;
            craftingDragRightClick = rightClick;
            craftingDragSlotIndices = {unifiedIndex};
        }
        else
            pendingCraftingInteraction = CraftingInteractionRequest{
                unifiedIndex, ItemType::NONE, rightClick, shiftHeld,
                doubleClick, {}, false};
        stationFeedback.clear();
        return;
    }

    const std::array<ItemType, 2> ingredients{
        ItemType::LOG,
        ItemType::WOODEN_PLANKS};

    for (std::size_t index = 0;
         index < ingredients.size();
         ++index)
    {
        if (IsPointInsideRectangle(
                mouseX,
                mouseY,
                layout.ingredientButtons[index]) &&
            GetDisplayedCraftingItemCount(ingredients[index]) > 0)
        {
            pendingCraftingInteraction = CraftingInteractionRequest{
                -1, ingredients[index], rightClick, shiftHeld, doubleClick, {}, false};
            selectedCraftingItem = ingredients[index];
            stationFeedback.clear();
            return;
        }
    }

    for (std::size_t index = 0;
         index < layout.gridSlots.size();
         ++index)
    {
        if (!IsPointInsideRectangle(
                mouseX,
                mouseY,
                layout.gridSlots[index]))
        {
            continue;
        }

        const int unifiedIndex = Inventory::SlotCount + static_cast<int>(index);
        if (craftingSessionSnapshot.has_value() &&
            !craftingSessionSnapshot->cursor.IsEmpty() && !shiftHeld && !doubleClick)
        {
            craftingDragActive = true;
            craftingDragRightClick = rightClick;
            craftingDragSlotIndices = {unifiedIndex};
        }
        else
        {
            pendingCraftingInteraction = CraftingInteractionRequest{
                unifiedIndex, ItemType::NONE, rightClick, shiftHeld, doubleClick, {}, false};
        }
        if (craftingGrid[index] != ItemType::NONE)
            craftingGrid[index] = ItemType::NONE;
        else if (selectedCraftingItem != ItemType::NONE &&
                 GetCraftingGridItemCount(selectedCraftingItem) <
                     GetDisplayedCraftingItemCount(selectedCraftingItem))
            craftingGrid[index] = selectedCraftingItem;
        stationFeedback.clear();
        return;
    }

    if (IsPointInsideRectangle(
            mouseX, mouseY, layout.clearButton))
    {
        pendingCraftingInteraction = CraftingInteractionRequest{
            -1, ItemType::NONE, false, false, false, {}, true};
        craftingGrid.fill(ItemType::NONE);
        stationFeedback.clear();
        return;
    }

    if (IsPointInsideRectangle(
            mouseX, mouseY, layout.craftButton) &&
        !recipeActionActive &&
        CraftingRecipeMatcher::FindMatch(craftingGrid) != nullptr)
    {
        requestedCraftingGrid = craftingGrid;
        craftingGridRequestPending = true;
        stationFeedback.clear();
    }
}

void Graphics::DrawStationMenu()
{
    if (!stationMenuOpen)
    {
        return;
    }

    if (openStationType == StationType::BANK)
    {
        DrawBankPanel();
        return;
    }

    const bool furnace =
        openStationType == StationType::FURNACE;
    const SDL_FRect menuRectangle = furnace
        ? GetFurnaceMenuLayout().panelBounds
        : GetCraftingTableLayout().panelBounds;

    SDL_SetRenderDrawColor(
        renderer,
        28,
        28,
        28,
        245);

    SDL_RenderFillRect(
        renderer,
        &menuRectangle);

    SDL_SetRenderDrawColor(
        renderer,
        190,
        190,
        190,
        255);

    SDL_RenderRect(
        renderer,
        &menuRectangle);

    if (furnace)
    {
        const FurnaceMenuLayout layout =
            GetFurnaceMenuLayout();
        SDL_SetRenderDrawColor(
            renderer, 60, 60, 65, 255);
        for (const SDL_FRect &button : layout.recipeButtons)
        {
            SDL_RenderFillRect(renderer, &button);
        }
        SDL_SetRenderDrawColor(
            renderer, 175, 175, 180, 255);
        for (const SDL_FRect &button : layout.recipeButtons)
        {
            SDL_RenderRect(renderer, &button);
        }
        SDL_RenderRect(renderer, &layout.closeButton);
        SDL_SetRenderDrawColor(
            renderer, 255, 255, 255, 255);
        SDL_RenderDebugText(
            renderer,
            layout.panelBounds.x + 18.0f,
            layout.panelBounds.y + 18.0f,
            "FURNACE");
        SDL_RenderDebugText(renderer,
            layout.closeButton.x + 12.0f,
            layout.closeButton.y + 10.0f,
            "CLOSE");
        SDL_RenderDebugText(renderer,
            layout.recipeButtons[0].x + 12.0f,
            layout.recipeButtons[0].y + 14.0f,
            "BRONZE BAR - 1 COPPER + 1 TIN");
        SDL_RenderDebugText(renderer,
            layout.recipeButtons[1].x + 12.0f,
            layout.recipeButtons[1].y + 14.0f,
            "IRON BAR - 1 IRON ORE");
        SDL_RenderDebugText(renderer,
            layout.recipeButtons[2].x + 12.0f,
            layout.recipeButtons[2].y + 14.0f,
            "STEEL BAR - 1 IRON + 2 COAL");
        if (!stationFeedback.empty())
        {
            SDL_RenderDebugText(renderer,
                layout.panelBounds.x + 18.0f,
                layout.panelBounds.y + 224.0f,
                stationFeedback.c_str());
        }
        return;
    }

    if (openStationType != StationType::CRAFTING_TABLE)
    {
        return;
    }

    const CraftingTableLayout layout =
        GetCraftingTableLayout();
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderDebugText(renderer,
        layout.panelBounds.x + 24.0f,
        layout.panelBounds.y + 24.0f,
        "CRAFTING TABLE");
    SDL_RenderDebugText(renderer,
        layout.panelBounds.x + 40.0f,
        layout.panelBounds.y + 68.0f,
        "3 x 3 GRID");
    SDL_RenderDebugText(renderer,
        layout.panelBounds.x + 300.0f,
        layout.panelBounds.y + 68.0f,
        "INVENTORY");

    SDL_SetRenderDrawColor(renderer, 65, 58, 50, 255);
    for (const SDL_FRect &slot : layout.gridSlots)
    {
        SDL_RenderFillRect(renderer, &slot);
    }
    SDL_SetRenderDrawColor(renderer, 165, 145, 115, 255);
    for (const SDL_FRect &slot : layout.gridSlots)
    {
        SDL_RenderRect(renderer, &slot);
    }
    for (std::size_t index = 0;
         index < craftingGrid.size();
         ++index)
    {
        DrawCraftingItemIcon(
            craftingGrid[index],
            layout.gridSlots[index]);
    }

    for (std::size_t index = 0; index < layout.inventorySlots.size(); ++index)
    {
        const SDL_FRect &bounds = layout.inventorySlots[index];
        SDL_SetRenderDrawColor(renderer, 55, 55, 55, 255);
        SDL_RenderFillRect(renderer, &bounds);
        SDL_SetRenderDrawColor(renderer, 120, 120, 120, 255);
        SDL_RenderRect(renderer, &bounds);
        const InventorySlot &slot = displayedInventorySlots[index];
        if (slot.IsEmpty()) continue;
        DrawCraftingItemIcon(slot.GetItemType(), bounds);
        DrawLogIcon(slot.GetItemType(), bounds.x, bounds.y, bounds.w);
        DrawOreIcon(slot.GetItemType(), bounds.x, bounds.y, bounds.w);
        DrawAxeIcon(slot.GetItemType(), bounds.x, bounds.y, bounds.w);
        DrawSwordIcon(slot.GetItemType(), bounds.x, bounds.y, bounds.w);
        DrawPickaxeIcon(slot.GetItemType(), bounds.x, bounds.y, bounds.w);
        DrawBarIcon(slot.GetItemType(), bounds.x, bounds.y, bounds.w);
        DrawFoodIcon(slot.GetItemType(), bounds.x, bounds.y, bounds.w);
        if (slot.GetAmount() > 1)
        {
            const std::string amount = std::to_string(slot.GetAmount());
            SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
            SDL_RenderDebugText(renderer, bounds.x + 3.0f, bounds.y + 23.0f,
                                amount.c_str());
        }
    }

    const std::array<ItemType, 2> ingredientTypes{
        ItemType::LOG,
        ItemType::WOODEN_PLANKS};
    for (std::size_t index = 0;
         index < ingredientTypes.size();
         ++index)
    {
        const bool available =
            GetDisplayedCraftingItemCount(ingredientTypes[index]) > 0;
        const bool selected =
            selectedCraftingItem == ingredientTypes[index];
        SDL_SetRenderDrawColor(
            renderer,
            selected ? 92 : (available ? 65 : 42),
            selected ? 105 : (available ? 72 : 42),
            selected ? 72 : (available ? 58 : 42),
            255);
        SDL_RenderFillRect(
            renderer,
            &layout.ingredientButtons[index]);
        SDL_SetRenderDrawColor(
            renderer, 170, 155, 125, 255);
        SDL_RenderRect(
            renderer,
            &layout.ingredientButtons[index]);
        DrawCraftingItemIcon(
            ingredientTypes[index],
            layout.ingredientButtons[index]);
    }

    const std::string logText =
        "Log x" + std::to_string(displayedLogCount);
    const std::string plankText =
        "Wooden planks x" + std::to_string(displayedPlankCount);
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderDebugText(renderer,
        layout.ingredientButtons[0].x + 58.0f,
        layout.ingredientButtons[0].y + 17.0f,
        logText.c_str());
    SDL_RenderDebugText(renderer,
        layout.ingredientButtons[1].x + 58.0f,
        layout.ingredientButtons[1].y + 17.0f,
        plankText.c_str());

    SDL_SetRenderDrawColor(renderer, 45, 45, 48, 255);
    SDL_RenderFillRect(renderer, &layout.previewBounds);
    SDL_SetRenderDrawColor(renderer, 135, 135, 140, 255);
    SDL_RenderRect(renderer, &layout.previewBounds);
    const RecipeDefinition *match =
        CraftingRecipeMatcher::FindMatch(craftingGrid);
    if (match == nullptr)
    {
        SDL_RenderDebugText(renderer,
            layout.previewBounds.x + 14.0f,
            layout.previewBounds.y + 18.0f,
            "Arrange materials to discover a recipe");
    }
    else
    {
        SDL_RenderDebugText(renderer,
            layout.previewBounds.x + 14.0f,
            layout.previewBounds.y + 14.0f,
            match->GetName().c_str());
        const std::string output =
            "OUTPUT: " +
            std::to_string(match->GetOutputAmount()) +
            " " +
            ItemDatabase::Get(match->GetOutputItem()).GetName();
        SDL_RenderDebugText(renderer,
            layout.previewBounds.x + 14.0f,
            layout.previewBounds.y + 46.0f,
            output.c_str());
    }

    SDL_SetRenderDrawColor(renderer, 70, 62, 52, 255);
    SDL_RenderFillRect(renderer, &layout.clearButton);
    const bool canCraft =
        match != nullptr &&
        !recipeCommandInFlight &&
        !recipeActionActive;
    SDL_SetRenderDrawColor(
        renderer,
        canCraft ? 64 : 45,
        canCraft ? 92 : 45,
        canCraft ? 60 : 45,
        255);
    SDL_RenderFillRect(renderer, &layout.craftButton);
    SDL_SetRenderDrawColor(renderer, 180, 165, 135, 255);
    SDL_RenderRect(renderer, &layout.clearButton);
    SDL_RenderRect(renderer, &layout.craftButton);
    SDL_RenderRect(renderer, &layout.closeButton);
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderDebugText(renderer,
        layout.clearButton.x + 42.0f,
        layout.clearButton.y + 15.0f,
        "CLEAR");
    SDL_RenderDebugText(renderer,
        layout.craftButton.x + 46.0f,
        layout.craftButton.y + 15.0f,
        "CRAFT");
    SDL_RenderDebugText(renderer,
        layout.closeButton.x + 18.0f,
        layout.closeButton.y + 12.0f,
        "CLOSE");
    if (!stationFeedback.empty())
    {
        SDL_RenderDebugText(renderer,
            layout.panelBounds.x + 40.0f,
            layout.panelBounds.y + 410.0f,
            stationFeedback.c_str());
    }

    if (craftingSessionSnapshot.has_value() &&
        !craftingSessionSnapshot->cursor.IsEmpty())
    {
        float mouseX = 0.0f;
        float mouseY = 0.0f;
        SDL_GetMouseState(&mouseX, &mouseY);
        const SDL_FRect cursorBounds{mouseX - 24.0f, mouseY - 24.0f, 48.0f, 48.0f};
        DrawCraftingItemIcon(
            craftingSessionSnapshot->cursor.GetItemType(), cursorBounds);
        DrawLogIcon(craftingSessionSnapshot->cursor.GetItemType(),
                    cursorBounds.x, cursorBounds.y, cursorBounds.w);
        DrawOreIcon(craftingSessionSnapshot->cursor.GetItemType(),
                    cursorBounds.x, cursorBounds.y, cursorBounds.w);
        DrawAxeIcon(craftingSessionSnapshot->cursor.GetItemType(),
                    cursorBounds.x, cursorBounds.y, cursorBounds.w);
        DrawSwordIcon(craftingSessionSnapshot->cursor.GetItemType(),
                      cursorBounds.x, cursorBounds.y, cursorBounds.w);
        DrawPickaxeIcon(craftingSessionSnapshot->cursor.GetItemType(),
                        cursorBounds.x, cursorBounds.y, cursorBounds.w);
        DrawBarIcon(craftingSessionSnapshot->cursor.GetItemType(),
                    cursorBounds.x, cursorBounds.y, cursorBounds.w);
        DrawFoodIcon(craftingSessionSnapshot->cursor.GetItemType(),
                     cursorBounds.x, cursorBounds.y, cursorBounds.w);
        if (craftingSessionSnapshot->cursor.GetAmount() > 1)
        {
            const std::string amount =
                std::to_string(craftingSessionSnapshot->cursor.GetAmount());
            SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
            SDL_RenderDebugText(renderer, mouseX + 10.0f, mouseY + 10.0f,
                                amount.c_str());
        }
    }
}

void Graphics::SynchronizeBank(
    int actorEntityID, const ActiveBankSession *session, const Bank &bank)
{
    bankActorEntityID = actorEntityID;
    if (session == nullptr)
    {
        bankSessionId = InvalidBankSessionId;
        displayedBankEntries.clear();
        if (openStationType == StationType::BANK) DismissStationMenu(false);
        return;
    }
    bankSessionId = session->sessionId;
    bankRevision = bank.GetRevision();
    displayedBankEntries = bank.GetEntries();
}

std::optional<ServerCommandData> Graphics::ConsumeBankCommand()
{
    auto command = pendingBankCommand;
    pendingBankCommand.reset();
    return command;
}

void Graphics::HandleBankPanelClick(float screenX, float screenY)
{
    if (pendingBankCommand || bankSessionId == InvalidBankSessionId) return;
    const SDL_FRect panel{220.0f, 54.0f, 840.0f, 612.0f};
    const SDL_FRect close{panel.x + panel.w - 82.0f, panel.y + 14.0f, 64.0f, 28.0f};
    if (IsPointInsideRectangle(screenX, screenY, close))
    {
        DismissStationMenu(true);
        return;
    }
    const std::array<int, 4> quantities{1, 5, 10, -1};
    for (std::size_t index = 0; index < quantities.size(); ++index)
    {
        const SDL_FRect button{panel.x + 20.0f + index * 58.0f,
            panel.y + 64.0f, 50.0f, 28.0f};
        if (IsPointInsideRectangle(screenX, screenY, button))
        {
            bankQuantityMode = quantities[index];
            return;
        }
    }
    const SDL_FRect depositAll{panel.x + 610.0f, panel.y + 64.0f, 190.0f, 28.0f};
    if (IsPointInsideRectangle(screenX, screenY, depositAll))
    {
        pendingBankCommand = BankDepositInventoryCommand{
            bankActorEntityID, bankSessionId, bankRevision};
        return;
    }

    std::vector<BankEntry> filtered;
    for (const BankEntry &entry : displayedBankEntries)
    {
        std::string name = ItemDatabase::Get(entry.itemType).GetName();
        std::string lowered = name;
        std::transform(lowered.begin(), lowered.end(), lowered.begin(),
            [](unsigned char value){return static_cast<char>(std::tolower(value));});
        std::string query = bankSearch;
        std::transform(query.begin(), query.end(), query.begin(),
            [](unsigned char value){return static_cast<char>(std::tolower(value));});
        if (query.empty() || lowered.find(query) != std::string::npos)
            filtered.push_back(entry);
    }
    for (std::size_t index = 0; index < filtered.size(); ++index)
    {
        const SDL_FRect slot{panel.x + 20.0f + (index % 8) * 50.0f,
            panel.y + 142.0f + (index / 8) * 50.0f, 44.0f, 44.0f};
        if (IsPointInsideRectangle(screenX, screenY, slot))
        {
            const int quantity = bankQuantityMode < 0
                ? filtered[index].quantity
                : std::min(bankQuantityMode, filtered[index].quantity);
            pendingBankCommand = BankWithdrawCommand{bankActorEntityID,
                bankSessionId, bankRevision, filtered[index].itemType, quantity};
            return;
        }
    }
    for (int index = 0; index < Inventory::SlotCount; ++index)
    {
        const SDL_FRect slot{panel.x + 470.0f + (index % 7) * 46.0f,
            panel.y + 142.0f + (index / 7) * 50.0f, 40.0f, 44.0f};
        if (!displayedInventorySlots[index].IsEmpty() &&
            IsPointInsideRectangle(screenX, screenY, slot))
        {
            const int available = displayedInventorySlots[index].GetAmount();
            const int quantity = bankQuantityMode < 0
                ? available : std::min(bankQuantityMode, available);
            pendingBankCommand = BankDepositCommand{bankActorEntityID,
                bankSessionId, bankRevision, index, quantity};
            return;
        }
    }
}

void Graphics::DrawBankPanel()
{
    const SDL_FRect panel{220.0f, 54.0f, 840.0f, 612.0f};
    SDL_SetRenderDrawColor(renderer, 28, 25, 20, 250);
    SDL_RenderFillRect(renderer, &panel);
    SDL_SetRenderDrawColor(renderer, 196, 166, 92, 255);
    SDL_RenderRect(renderer, &panel);
    SDL_SetRenderDrawColor(renderer, 255, 242, 205, 255);
    SDL_RenderDebugText(renderer, panel.x + 20.0f, panel.y + 20.0f, "BANK");
    SDL_RenderDebugText(renderer, panel.x + 470.0f, panel.y + 112.0f, "INVENTORY");
    const std::string search = "SEARCH: " + (bankSearch.empty() ? std::string("type to filter") : bankSearch);
    SDL_RenderDebugText(renderer, panel.x + 280.0f, panel.y + 72.0f, search.c_str());
    const std::array<const char *, 4> labels{"1", "5", "10", "ALL"};
    const std::array<int, 4> quantities{1, 5, 10, -1};
    for (std::size_t index = 0; index < labels.size(); ++index)
    {
        const SDL_FRect button{panel.x + 20.0f + index * 58.0f,
            panel.y + 64.0f, 50.0f, 28.0f};
        SDL_SetRenderDrawColor(renderer,
            bankQuantityMode == quantities[index] ? 105 : 58,
            bankQuantityMode == quantities[index] ? 88 : 55, 45, 255);
        SDL_RenderFillRect(renderer, &button);
        SDL_SetRenderDrawColor(renderer, 200, 174, 110, 255);
        SDL_RenderRect(renderer, &button);
        SDL_RenderDebugText(renderer, button.x + 16.0f, button.y + 10.0f, labels[index]);
    }
    const SDL_FRect depositAll{panel.x + 610.0f, panel.y + 64.0f, 190.0f, 28.0f};
    const SDL_FRect close{panel.x + panel.w - 82.0f, panel.y + 14.0f, 64.0f, 28.0f};
    SDL_SetRenderDrawColor(renderer, 64, 86, 58, 255);
    SDL_RenderFillRect(renderer, &depositAll);
    SDL_SetRenderDrawColor(renderer, 200, 174, 110, 255);
    SDL_RenderRect(renderer, &depositAll); SDL_RenderRect(renderer, &close);
    SDL_RenderDebugText(renderer, depositAll.x + 35.0f, depositAll.y + 10.0f, "DEPOSIT INVENTORY");
    SDL_RenderDebugText(renderer, close.x + 12.0f, close.y + 10.0f, "CLOSE");

    std::vector<BankEntry> filtered;
    std::string query = bankSearch;
    std::transform(query.begin(), query.end(), query.begin(),
        [](unsigned char value){return static_cast<char>(std::tolower(value));});
    for (const BankEntry &entry : displayedBankEntries)
    {
        std::string name = ItemDatabase::Get(entry.itemType).GetName();
        std::transform(name.begin(), name.end(), name.begin(),
            [](unsigned char value){return static_cast<char>(std::tolower(value));});
        if (query.empty() || name.find(query) != std::string::npos) filtered.push_back(entry);
    }
    auto drawItem = [&](ItemType type, const SDL_FRect &slot, int amount)
    {
        SDL_SetRenderDrawColor(renderer, 54, 50, 43, 255); SDL_RenderFillRect(renderer, &slot);
        SDL_SetRenderDrawColor(renderer, 135, 117, 78, 255); SDL_RenderRect(renderer, &slot);
        DrawCraftingItemIcon(type, slot); DrawLogIcon(type, slot.x, slot.y, slot.w);
        DrawOreIcon(type, slot.x, slot.y, slot.w); DrawAxeIcon(type, slot.x, slot.y, slot.w);
        DrawSwordIcon(type, slot.x, slot.y, slot.w); DrawPickaxeIcon(type, slot.x, slot.y, slot.w);
        DrawBarIcon(type, slot.x, slot.y, slot.w); DrawFoodIcon(type, slot.x, slot.y, slot.w);
        if (amount > 1) { const std::string text = std::to_string(amount);
            SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
            SDL_RenderDebugText(renderer, slot.x + 3.0f, slot.y + 29.0f, text.c_str()); }
    };
    for (std::size_t index = 0; index < filtered.size(); ++index)
        drawItem(filtered[index].itemType,
            SDL_FRect{panel.x + 20.0f + (index % 8) * 50.0f,
                panel.y + 142.0f + (index / 8) * 50.0f, 44.0f, 44.0f},
            filtered[index].quantity);
    for (int index = 0; index < Inventory::SlotCount; ++index)
    {
        const SDL_FRect slot{panel.x + 470.0f + (index % 7) * 46.0f,
            panel.y + 142.0f + (index / 7) * 50.0f, 40.0f, 44.0f};
        const InventorySlot &item = displayedInventorySlots[index];
        if (item.IsEmpty()) { SDL_SetRenderDrawColor(renderer, 45, 43, 39, 255);
            SDL_RenderFillRect(renderer, &slot); SDL_SetRenderDrawColor(renderer, 90, 82, 65, 255); SDL_RenderRect(renderer, &slot); }
        else drawItem(item.GetItemType(), slot, item.GetAmount());
    }
}

void Graphics::DrawCraftingItemIcon(
    ItemType itemType,
    const SDL_FRect &bounds)
{
    if (itemType == ItemType::LOG)
    {
        DrawLogIcon(
            itemType,
            bounds.x + 8.0f,
            bounds.y + 8.0f,
            std::min(bounds.w, bounds.h) - 16.0f);
        return;
    }

    if (itemType != ItemType::WOODEN_PLANKS)
    {
        return;
    }

    const float width = std::min(bounds.w - 16.0f, 44.0f);
    const float height = 8.0f;
    const float x = bounds.x + 8.0f;
    const float y = bounds.y + 9.0f;
    SDL_SetRenderDrawColor(renderer, 166, 111, 58, 255);
    for (int index = 0; index < 3; ++index)
    {
        SDL_FRect plank{
            x,
            y + static_cast<float>(index) * 11.0f,
            width,
            height};
        SDL_RenderFillRect(renderer, &plank);
    }
}

void Graphics::DrawActionProgress(
    const Action *activeAction)
{
    if (activeAction == nullptr)
    {
        return;
    }

    constexpr float panelWidth = 320.0f;
    constexpr float panelHeight = 42.0f;
    constexpr float barPadding = 10.0f;
    constexpr float barHeight = 10.0f;

    const float panelX =
        (static_cast<float>(WindowWidth) -
         panelWidth) /
        2.0f;

    const float panelY =
        static_cast<float>(WindowHeight) -
        panelHeight -
        18.0f;

    SDL_FRect panel{panelX, panelY, panelWidth, panelHeight};

    SDL_SetRenderDrawColor(
        renderer,
        25,
        25,
        25,
        235);

    SDL_RenderFillRect(renderer, &panel);

    SDL_SetRenderDrawColor(
        renderer,
        180,
        180,
        180,
        255);

    SDL_RenderRect(renderer, &panel);

    SDL_SetRenderDrawColor(
        renderer,
        255,
        255,
        255,
        255);

    SDL_RenderDebugText(
        renderer,
        panelX + barPadding,
        panelY + 7.0f,
        activeAction->GetName().c_str());

    SDL_FRect barBackground{
        panelX + barPadding,
        panelY + 25.0f,
        panelWidth - barPadding * 2.0f,
        barHeight};

    SDL_SetRenderDrawColor(
        renderer,
        65,
        65,
        65,
        255);

    SDL_RenderFillRect(renderer, &barBackground);

    SDL_FRect barFill{
        barBackground.x,
        barBackground.y,
        barBackground.w *
            activeAction->GetProgress(),
        barBackground.h};

    SDL_SetRenderDrawColor(
        renderer,
        80,
        190,
        90,
        255);

    SDL_RenderFillRect(renderer, &barFill);

    SDL_SetRenderDrawColor(
        renderer,
        210,
        210,
        210,
        255);

    SDL_RenderRect(renderer, &barBackground);
}

bool Graphics::HandleSidePanelClick(
    float mouseX,
    float mouseY,
    bool rightClick)
{
    const SidePanelLayout layout = GetSidePanelLayout();
    const float panelX = layout.panelBounds.x;
    const float panelY = layout.panelBounds.y;

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
        const SDL_FRect &tab = layout.tabBounds[tabIndex];
        const bool insideTab =
            mouseX >= tab.x && mouseX < tab.x + tab.w &&
            mouseY >= tab.y && mouseY < tab.y + tab.h;

        if (insideTab)
        {
            selectedTab = tabs[tabIndex];
            return true;
        }
    }

    const bool insidePanel =
        mouseX >= layout.panelBounds.x &&
        mouseX < layout.panelBounds.x + layout.panelBounds.w &&
        mouseY >= layout.panelBounds.y &&
        mouseY < layout.panelBounds.y + layout.panelBounds.h;

    if (!insidePanel)
    {
        return false;
    }

    // Inventory slot clicks.
    if (selectedTab ==
        SidePanelTab::INVENTORY)
    {
        constexpr int columns = 7;

        constexpr float slotSize = 40.0f;
        constexpr float slotGap = 4.0f;
        constexpr float padding = 13.0f;
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
                if (rightClick && !displayedInventorySlots[slotIndex].IsEmpty())
                {
                    dropInventorySlotIndex = slotIndex;
                    dropInventoryAmount = 1;
                    inventoryDropPending = true;
                    return true;
                }
                clickedInventorySlotIndex =
                    slotIndex;

                inventorySlotClickPending =
                    true;

                return true;
            }
        }
    }

    if (selectedTab == SidePanelTab::SETTINGS)
    {
        const SDL_FRect gridToggle{
            layout.contentBounds.x,
            layout.contentBounds.y + 54.0f,
            layout.contentBounds.w,
            38.0f};
        const SDL_FRect guideButton{
            layout.contentBounds.x,
            layout.contentBounds.y + 104.0f,
            layout.contentBounds.w,
            38.0f};
        const SDL_FRect skipTutorialButton{
            layout.contentBounds.x,
            layout.contentBounds.y + 154.0f,
            layout.contentBounds.w,
            38.0f};
        if (IsPointInsideRectangle(mouseX, mouseY, gridToggle))
        {
            showGrid = !showGrid;
            return true;
        }
        if (IsPointInsideRectangle(mouseX, mouseY, guideButton))
        {
            gameGuideVisible = true;
            return true;
        }
        if (IsPointInsideRectangle(mouseX, mouseY, skipTutorialButton))
        {
            tutorialSkipPending = true;
            return true;
        }
    }

    // Equipment weapon-slot click.
    if (selectedTab ==
        SidePanelTab::EQUIPMENT)
    {
        const std::array<CombatStyle, 3> styles{
            CombatStyle::ACCURATE,
            CombatStyle::AGGRESSIVE,
            CombatStyle::DEFENSIVE};
        for (std::size_t index = 0; index < styles.size(); ++index)
        {
            const SDL_FRect styleButton{
                panelX + 12.0f + static_cast<float>(index) * 108.0f,
                panelY + 266.0f,
                100.0f,
                38.0f};
            if (IsPointInsideRectangle(mouseX, mouseY, styleButton))
            {
                combatStyleRequest = styles[index];
                return true;
            }
        }

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
        if (event.type == SDL_EVENT_MOUSE_MOTION)
        {
            if (renderer != nullptr)
                SDL_ConvertEventToRenderCoordinates(renderer, &event);
            mouseX = event.motion.x;
            mouseY = event.motion.y;
        }
        if (craftingDragActive && event.type == SDL_EVENT_MOUSE_MOTION &&
            stationMenuOpen && openStationType == StationType::CRAFTING_TABLE)
        {
            const CraftingTableLayout layout = GetCraftingTableLayout();
            for (std::size_t index = 0; index < layout.inventorySlots.size(); ++index)
                if (IsPointInsideRectangle(event.motion.x, event.motion.y,
                                           layout.inventorySlots[index]))
                {
                    const int unified = static_cast<int>(index);
                    if (std::find(craftingDragSlotIndices.begin(),
                                  craftingDragSlotIndices.end(), unified) ==
                        craftingDragSlotIndices.end())
                        craftingDragSlotIndices.push_back(unified);
                }
            for (std::size_t index = 0; index < layout.gridSlots.size(); ++index)
                if (IsPointInsideRectangle(event.motion.x, event.motion.y,
                                           layout.gridSlots[index]))
                {
                    const int unified = Inventory::SlotCount + static_cast<int>(index);
                    if (std::find(craftingDragSlotIndices.begin(),
                                  craftingDragSlotIndices.end(), unified) ==
                        craftingDragSlotIndices.end())
                        craftingDragSlotIndices.push_back(unified);
                }
            continue;
        }
        if (craftingDragActive && event.type == SDL_EVENT_MOUSE_BUTTON_UP)
        {
            pendingCraftingInteraction = CraftingInteractionRequest{
                craftingDragSlotIndices.empty() ? -1 : craftingDragSlotIndices.front(),
                ItemType::NONE, craftingDragRightClick, false, false,
                craftingDragSlotIndices, false};
            craftingDragActive = false;
            craftingDragSlotIndices.clear();
            continue;
        }
        if (event.type == SDL_EVENT_QUIT)
        {
            running = false;
        }

        if (event.type == SDL_EVENT_KEY_DOWN &&
            !event.key.repeat)
        {
            if (stationMenuOpen && openStationType == StationType::BANK &&
                event.key.scancode == SDL_SCANCODE_BACKSPACE &&
                !bankSearch.empty())
            {
                bankSearch.pop_back();
                continue;
            }
            if (event.key.scancode == SDL_SCANCODE_H)
            {
                gameGuideVisible = !gameGuideVisible;
                continue;
            }
            if (event.key.scancode ==
                SDL_SCANCODE_ESCAPE)
            {
                if (worldContextMenu.open)
                {
                    worldContextMenu.open = false;
                    continue;
                }
                if (shopPresentationState.IsOpen())
                {
                    if (!pendingShopCommand)
                        pendingShopCommand = shopPresentationState.MakeClose();
                    continue;
                }
                if (stationMenuOpen)
                {
                    if (!recipeCommandInFlight)
                    {
                        DismissStationMenu(true);
                    }
                }

                continue;
            }
        }

        if (event.type == SDL_EVENT_TEXT_INPUT && stationMenuOpen &&
            openStationType == StationType::BANK && bankSearch.size() < 24)
        {
            for (const char *cursor = event.text.text;
                 *cursor != '\0' && bankSearch.size() < 24; ++cursor)
                if (std::isalnum(static_cast<unsigned char>(*cursor)) || *cursor == ' ')
                    bankSearch.push_back(*cursor);
            continue;
        }

        if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN &&
            (event.button.button == SDL_BUTTON_LEFT ||
             event.button.button == SDL_BUTTON_RIGHT))
        {
            if (renderer != nullptr &&
                !SDL_ConvertEventToRenderCoordinates(renderer, &event))
            {
                continue;
            }
            float mouseX = event.button.x;
            float mouseY = event.button.y;
            this->mouseX = mouseX;
            this->mouseY = mouseY;

            if (gameGuideVisible)
            {
                gameGuideVisible = false;
                continue;
            }

            if (shopPresentationState.IsOpen())
            {
                HandleShopClick(mouseX, mouseY);
                continue;
            }

            if (dialoguePresentationState.IsOpen())
            {
                HandleDialogueClick(mouseX, mouseY);
                continue;
            }

            if (stationMenuOpen)
            {
                HandleStationMenuClick(
                    event.button.x,
                    event.button.y,
                    event.button.button == SDL_BUTTON_RIGHT,
                    (SDL_GetModState() & SDL_KMOD_SHIFT) != 0,
                    event.button.clicks > 1);

                continue;
            }

            if (worldContextMenu.open)
            {
                if (event.button.button == SDL_BUTTON_LEFT)
                {
                    HandleWorldContextMenuClick(mouseX, mouseY);
                }
                else
                {
                    worldContextMenu.open = false;
                }
                continue;
            }

            if (event.button.button == SDL_BUTTON_LEFT &&
                HandleMinimapClick(mouseX, mouseY))
            {
                continue;
            }

            if (HandleSidePanelClick(
                    mouseX,
                    mouseY,
                    event.button.button == SDL_BUTTON_RIGHT))
            {
                continue;
            }

            if (event.button.button == SDL_BUTTON_RIGHT)
            {
                OpenWorldContextMenu(mouseX, mouseY);
                continue;
            }

            clickedTileX =
                static_cast<int>(mouseX + cameraPixelX) / TileSize;

            clickedTileY =
                static_cast<int>(mouseY + cameraPixelY) / TileSize;

            clickedMouseX =
                static_cast<int>(mouseX + cameraPixelX);

            clickedMouseY =
                static_cast<int>(mouseY + cameraPixelY);

            hasClickedTile = true;
            clickPending = true;
        }
    }
}

void Graphics::SynchronizeDialogue(
    int localActorEntityID,
    const std::vector<NpcTalkEvent> &publishedEvents,
    const ActiveDialogueSession *activeSession)
{
    const DialogueSessionId previousSession =
        dialoguePresentationState.GetEvent() == nullptr
            ? InvalidDialogueSessionId
            : dialoguePresentationState.GetEvent()->sessionId;

    dialoguePresentationState.Synchronize(
        localActorEntityID,
        publishedEvents,
        activeSession);

    const NpcTalkEvent *current = dialoguePresentationState.GetEvent();
    if (current != nullptr)
    {
        DismissStationMenu(false);
    }

    if (pendingDialogueCommand.has_value() &&
        (current == nullptr || current->sessionId != previousSession))
    {
        pendingDialogueCommand.reset();
    }
}

void Graphics::SynchronizeShop(int actor,const std::vector<ShopOpenedEvent>& events,const ActiveShopSession* active)
{
    const ShopSessionId old = shopPresentationState.GetEvent() ? shopPresentationState.GetEvent()->sessionId : InvalidShopSessionId;
    shopPresentationState.Synchronize(actor, events, active);
    if (shopPresentationState.IsOpen())
    {
        dialoguePresentationState.Dismiss();
        DismissStationMenu(false);
    }
    const auto* current=shopPresentationState.GetEvent();
    if(pendingShopCommand && (!current || current->sessionId!=old)) pendingShopCommand.reset();
}

std::optional<ServerCommandData> Graphics::ConsumeDialogueCommand()
{
    if (!pendingDialogueCommand.has_value()) return std::nullopt;
    std::optional<ServerCommandData> command = pendingDialogueCommand;
    pendingDialogueCommand.reset();
    return command;
}

std::optional<ServerCommandData> Graphics::ConsumeShopCommand(){ if(!pendingShopCommand)return std::nullopt; auto c=pendingShopCommand; pendingShopCommand.reset(); return c; }
void Graphics::SetShopCommandInFlight(bool inFlight)
{
    shopCommandInFlight = inFlight;
    if (inFlight)
    {
        pendingShopCommand.reset();
    }
}

bool Graphics::IsShopCommandInFlight() const
{
    return shopCommandInFlight;
}

void Graphics::ReconcileShopCommandResult(CommandResultCode result, bool closeCommand){ if(result==CommandResultCode::ACCEPTED){shopPresentationState.ClearRejection();return;} shopPresentationState.ReconcileRejectedCommand(); if(closeCommand) shopPresentationState.Dismiss(); }

SDL_FRect Graphics::GetDialogueCloseButtonRectangle() const
{
    for (const DialogueControlLayout &control : GetDialoguePanelLayout().controls)
        if (control.type == DialogueControlType::GENERAL_CLOSE)
            return control.bounds;
    return {};
}

SDL_FRect Graphics::GetDialogueContinueButtonRectangle() const
{
    for (const DialogueControlLayout &control : GetDialoguePanelLayout().controls)
        if (control.type == DialogueControlType::PRIMARY)
            return control.bounds;
    return {};
}

SDL_FRect Graphics::GetDialogueChoiceButtonRectangle(
    std::size_t choiceIndex) const
{
    for (const DialogueControlLayout &control : GetDialoguePanelLayout().controls)
        if (control.type == DialogueControlType::AUTHORED_CHOICE &&
            control.index == choiceIndex)
            return control.bounds;
    return {};
}

SDL_FRect Graphics::GetDialogueTradeButtonRectangle() const
{
    for (const DialogueControlLayout &control : GetDialoguePanelLayout().controls)
        if (control.type == DialogueControlType::TRADE)
            return control.bounds;
    return {};
}
SDL_FRect Graphics::GetDialogueQuestButtonRectangle(
    std::size_t actionIndex) const
{
    for (const DialogueControlLayout &control : GetDialoguePanelLayout().controls)
        if (control.type == DialogueControlType::QUEST_ACTION &&
            control.index == actionIndex)
            return control.bounds;
    return {};
}

Graphics::DialoguePanelLayout Graphics::GetDialoguePanelLayout() const
{
    DialoguePanelLayout layout{{220.0f, 450.0f, 840.0f, 230.0f}, {}};
    const NpcTalkEvent *event = dialoguePresentationState.GetEvent();
    if (event == nullptr) return layout;

    layout.controls.push_back({DialogueControlType::GENERAL_CLOSE, 0,
        {960.0f, 462.0f, 80.0f, 30.0f}});

    std::vector<std::pair<DialogueControlType, std::size_t>> actions;
    if (event->nodeKind == DialogueNodeKind::CHOICE)
        for (std::size_t index = 0; index < event->choices.size(); ++index)
            actions.push_back({DialogueControlType::AUTHORED_CHOICE, index});
    if (dialoguePresentationState.CanTrade())
        actions.push_back({DialogueControlType::TRADE, 0});
    for (std::size_t index = 0; index < event->questActions.size(); ++index)
        actions.push_back({DialogueControlType::QUEST_ACTION, index});
    if (event->nodeKind == DialogueNodeKind::CONTINUE || event->isTerminal)
        actions.push_back({DialogueControlType::PRIMARY, 0});

    if (actions.empty()) return layout;
    constexpr float left = 240.0f;
    constexpr float width = 800.0f;
    constexpr float top = 558.0f;
    constexpr float bottom = 668.0f;
    constexpr float gap = 4.0f;
    constexpr float preferredHeight = 38.0f;
    const float available = bottom - top -
        gap * static_cast<float>(actions.size() - 1);
    const float height = std::min(preferredHeight,
        available / static_cast<float>(actions.size()));
    const float totalHeight = height * static_cast<float>(actions.size()) +
        gap * static_cast<float>(actions.size() - 1);
    const float firstY = bottom - totalHeight;
    for (std::size_t index = 0; index < actions.size(); ++index)
        layout.controls.push_back({actions[index].first, actions[index].second,
            {left, firstY + static_cast<float>(index) * (height + gap),
             width, height}});
    return layout;
}
SDL_FRect Graphics::GetShopRowRectangle(std::size_t i) const { return SDL_FRect{160.0f,150.0f+static_cast<float>(i)*48.0f,960.0f,42.0f}; }
SDL_FRect Graphics::GetShopBuyButtonRectangle(std::size_t i) const { auto r=GetShopRowRectangle(i); return SDL_FRect{r.x+r.w-300.0f,r.y+4.0f,120.0f,r.h-8.0f}; }
SDL_FRect Graphics::GetShopSellButtonRectangle(std::size_t i) const { auto r=GetShopRowRectangle(i); return SDL_FRect{r.x+r.w-160.0f,r.y+4.0f,120.0f,r.h-8.0f}; }
SDL_FRect Graphics::GetShopCloseButtonRectangle() const { return SDL_FRect{1020.0f,100.0f,90.0f,32.0f}; }

bool Graphics::HandleShopClick(float x,float y)
{
    if(!shopPresentationState.IsOpen() || pendingShopCommand ||
       shopCommandInFlight) return true;
    if(IsPointInsideRectangle(x,y,GetShopCloseButtonRectangle())) { pendingShopCommand=shopPresentationState.MakeClose(); return true; }
    const auto* e=shopPresentationState.GetEvent();
    for(std::size_t i=0;i<e->entries.size();++i){ const auto& row=e->entries[i]; if(row.buyPrice && IsPointInsideRectangle(x,y,GetShopBuyButtonRectangle(i))) { pendingShopCommand=shopPresentationState.MakeBuy(i); return true; } if(row.sellPrice && IsPointInsideRectangle(x,y,GetShopSellButtonRectangle(i))) { pendingShopCommand=shopPresentationState.MakeSell(i); return true; } }
    return true;
}

bool Graphics::HandleDialogueClick(float mouseX, float mouseY)
{
    if (!dialoguePresentationState.IsOpen()) return false;
    if (pendingDialogueCommand.has_value()) return true;
    const NpcTalkEvent *event = dialoguePresentationState.GetEvent();
    const DialoguePanelLayout layout = GetDialoguePanelLayout();
    for (const DialogueControlLayout &control : layout.controls)
    {
        if (!IsPointInsideRectangle(mouseX, mouseY, control.bounds)) continue;
        switch (control.type)
        {
        case DialogueControlType::GENERAL_CLOSE:
            if (event->isTerminal)
                dialoguePresentationState.DismissTerminal();
            else
                pendingDialogueCommand =
                    dialoguePresentationState.MakeCloseCommand();
            break;
        case DialogueControlType::AUTHORED_CHOICE:
            pendingDialogueCommand =
                dialoguePresentationState.MakeChoiceCommand(control.index);
            break;
        case DialogueControlType::TRADE:
            pendingDialogueCommand =
                dialoguePresentationState.MakeTradeCommand();
            break;
        case DialogueControlType::QUEST_ACTION:
            pendingDialogueCommand =
                dialoguePresentationState.MakeQuestCommand(control.index);
            break;
        case DialogueControlType::PRIMARY:
            if (event->isTerminal)
                dialoguePresentationState.DismissTerminal();
            else
                pendingDialogueCommand =
                    dialoguePresentationState.MakeContinueCommand();
            break;
        }
        return true;
    }
    return true;
}

float Graphics::CalculateHealthRatio(
    int currentHealth,
    int maximumHealth) const
{
    if (maximumHealth <= 0)
    {
        return 0.0f;
    }

    float ratio = static_cast<float>(currentHealth) /
                  static_cast<float>(maximumHealth);

    return std::clamp(ratio, 0.0f, 1.0f);
}

SDL_FRect Graphics::GetPlayerScreenRectangle(
    const Player &player,
    int cameraTileX,
    int cameraTileY) const
{
    return MakeEntityTileRectangle(
        player.GetPosition().GetX(),
        player.GetPosition().GetY(),
        4.0f,
        cameraTileX,
        cameraTileY);
}

SDL_FRect Graphics::GetPlayerHealthBarBackgroundRectangle(
    const Player &player,
    int cameraTileX,
    int cameraTileY) const
{
    SDL_FRect playerRectangle = GetPlayerScreenRectangle(
        player,
        cameraTileX,
        cameraTileY);

    return SDL_FRect{
        playerRectangle.x,
        playerRectangle.y - 9.0f,
        playerRectangle.w,
        5.0f};
}

SDL_FRect Graphics::GetPlayerHealthBarFillRectangle(
    const Player &player,
    int cameraTileX,
    int cameraTileY) const
{
    const Combatant &combatant = player;

    SDL_FRect background = GetPlayerHealthBarBackgroundRectangle(
        player,
        cameraTileX,
        cameraTileY);

    return SDL_FRect{
        background.x,
        background.y,
        background.w * CalculateHealthRatio(
                           combatant.GetCurrentHealth(),
                           combatant.GetMaximumHealth()),
        background.h};
}

SDL_FPoint Graphics::GetPlayerCombatFeedbackPosition(
    const Player &player,
    int cameraTileX,
    int cameraTileY) const
{
    SDL_FRect playerRectangle = GetPlayerScreenRectangle(
        player,
        cameraTileX,
        cameraTileY);

    return SDL_FPoint{
        playerRectangle.x + 3.0f,
        playerRectangle.y - 18.0f};
}

SDL_FRect Graphics::GetMonsterHealthBarBackgroundRectangle(
    const Monster &monster,
    int cameraTileX,
    int cameraTileY) const
{
    SDL_FRect monsterRectangle = GetMonsterScreenRectangle(
        monster,
        cameraTileX,
        cameraTileY);

    return SDL_FRect{
        monsterRectangle.x,
        monsterRectangle.y - 7.0f,
        monsterRectangle.w,
        4.0f};
}

SDL_FRect Graphics::GetMonsterHealthBarFillRectangle(
    const Monster &monster,
    int cameraTileX,
    int cameraTileY) const
{
    const Combatant &combatant = monster;

    SDL_FRect background = GetMonsterHealthBarBackgroundRectangle(
        monster,
        cameraTileX,
        cameraTileY);

    return SDL_FRect{
        background.x,
        background.y,
        background.w * CalculateHealthRatio(
                           combatant.GetCurrentHealth(),
                           combatant.GetMaximumHealth()),
        background.h};
}

SDL_FRect Graphics::GetMonsterCorpseRectangle(
    const Monster &monster,
    int cameraTileX,
    int cameraTileY) const
{
    SDL_FRect monsterRectangle = GetMonsterScreenRectangle(
        monster,
        cameraTileX,
        cameraTileY);

    return SDL_FRect{
        monsterRectangle.x + 2.0f,
        monsterRectangle.y + monsterRectangle.h * 0.46f,
        monsterRectangle.w - 4.0f,
        monsterRectangle.h * 0.34f};
}

SDL_FPoint Graphics::GetMonsterCombatFeedbackPosition(
    const Monster &monster,
    int cameraTileX,
    int cameraTileY) const
{
    SDL_FRect monsterRectangle = GetMonsterScreenRectangle(
        monster,
        cameraTileX,
        cameraTileY);

    return SDL_FPoint{
        monsterRectangle.x + 4.0f,
        monsterRectangle.y - 15.0f};
}
bool Graphics::ConsumeClickedTile(
    int &tileX,
    int &tileY)
{
    int mouseX;
    int mouseY;

    return ConsumeClickedTile(
        tileX,
        tileY,
        mouseX,
        mouseY);
}

bool Graphics::ConsumeClickedTile(
    int &tileX,
    int &tileY,
    int &mouseX,
    int &mouseY)
{
    if (!clickPending)
    {
        return false;
    }

    tileX = clickedTileX;
    tileY = clickedTileY;
    mouseX = clickedMouseX;
    mouseY = clickedMouseY;

    clickPending = false;

    return true;
}

bool Graphics::ConsumeWalkTile(int &tileX, int &tileY)
{
    if (!walkClickPending)
    {
        return false;
    }

    tileX = walkTileX;
    tileY = walkTileY;
    walkClickPending = false;
    return true;
}

bool Graphics::ConsumeRecipeRequest(
    RecipeType &recipeType)
{
    if (!recipeRequestPending)
    {
        return false;
    }

    recipeType = requestedRecipeType;

    requestedRecipeType =
        RecipeType::NONE;

    recipeRequestPending = false;

    return true;
}

bool Graphics::ConsumeCraftingGridRequest(
    CraftingRecipeMatcher::Grid &grid)
{
    if (!craftingGridRequestPending)
    {
        return false;
    }

    grid = requestedCraftingGrid;
    requestedCraftingGrid.fill(ItemType::NONE);
    craftingGridRequestPending = false;
    return true;
}

void Graphics::SetRecipeCommandInFlight(
    bool inFlight)
{
    recipeCommandInFlight = inFlight;

    if (inFlight)
    {
        recipeRequestPending = false;
        craftingGridRequestPending = false;
    }
}

bool Graphics::IsRecipeCommandInFlight() const
{
    return recipeCommandInFlight;
}

void Graphics::ReconcileRecipeCommandResult(
    CommandResultCode result)
{
    if (result == CommandResultCode::ACCEPTED)
    {
        stationFeedback = "Crafting started";

        if (openStationType == StationType::CRAFTING_TABLE)
        {
            craftingGrid.fill(ItemType::NONE);
            selectedCraftingItem = ItemType::NONE;
        }
    }
    else
    {
        stationFeedback = "Crafting rejected by server";
    }
}

void Graphics::SynchronizeCraftingSession(
    const CraftingSessionSnapshot *session)
{
    if (session == nullptr)
    {
        craftingSessionSnapshot.reset();
        return;
    }
    craftingSessionSnapshot = *session;
    for (std::size_t index = 0; index < craftingGrid.size(); ++index)
        craftingGrid[index] = session->inputSlots[index].GetItemType();
}

const std::optional<CraftingSessionSnapshot> &
Graphics::GetCraftingSessionSnapshot() const
{
    return craftingSessionSnapshot;
}

std::optional<Graphics::CraftingInteractionRequest>
Graphics::ConsumeCraftingInteractionRequest()
{
    std::optional<CraftingInteractionRequest> request = pendingCraftingInteraction;
    pendingCraftingInteraction.reset();
    return request;
}

void Graphics::SetCraftingInteractionInFlight(bool inFlight)
{
    craftingInteractionInFlight = inFlight;
}

void Graphics::ReconcileCraftingInteractionResult(CommandResultCode result)
{
    stationFeedback = result == CommandResultCode::ACCEPTED
        ? "Inventory updated"
        : "Inventory interaction rejected by server";
}

bool Graphics::ConsumeStationMenuClose()
{
    if (!stationMenuClosePending)
    {
        return false;
    }

    stationMenuClosePending = false;

    return true;
}

void Graphics::UpdateCamera(
    int playerX,
    int playerY,
    const Map &map)
{
    constexpr float usefulWorldWidth = 930.0f;
    constexpr float preferredPlayerScreenX = 450.0f;
    constexpr float preferredPlayerScreenY = 350.0f;
    const float maximumX = std::max(
        0.0f,
        static_cast<float>(map.GetWidth() * TileSize) - usefulWorldWidth);
    const float maximumY = std::max(
        0.0f,
        static_cast<float>(map.GetHeight() * TileSize - WindowHeight));
    const float targetX = std::clamp(
        static_cast<float>(playerX * TileSize) + TileSize * 0.5f -
            preferredPlayerScreenX,
        0.0f,
        maximumX);
    const float targetY = std::clamp(
        static_cast<float>(playerY * TileSize) + TileSize * 0.5f -
            preferredPlayerScreenY,
        0.0f,
        maximumY);

    if (!cameraInitialized)
    {
        cameraPixelX = targetX;
        cameraPixelY = targetY;
        cameraInitialized = true;
        return;
    }

    const float smoothing = 1.0f - std::exp(
        -8.0f * presentationDeltaSeconds);
    cameraPixelX += (targetX - cameraPixelX) * smoothing;
    cameraPixelY += (targetY - cameraPixelY) * smoothing;
    if (std::abs(targetX - cameraPixelX) < 0.25f) cameraPixelX = targetX;
    if (std::abs(targetY - cameraPixelY) < 0.25f) cameraPixelY = targetY;
}

SDL_FPoint Graphics::ScreenToWorldPoint(
    float screenX,
    float screenY) const
{
    return SDL_FPoint{
        screenX + cameraPixelX,
        screenY + cameraPixelY};
}

bool Graphics::HandleMinimapClick(float screenX, float screenY)
{
    constexpr SDL_FRect mapBounds{1040.0f, 42.0f, 210.0f, 154.0f};
    constexpr float minimapTileSize = 7.0f;
    if (!IsPointInsideRectangle(screenX, screenY, mapBounds))
    {
        return false;
    }

    walkTileX = minimapOriginX + static_cast<int>(
        (screenX - mapBounds.x) / minimapTileSize);
    walkTileY = minimapOriginY + static_cast<int>(
        (screenY - mapBounds.y) / minimapTileSize);
    walkClickPending = true;
    return true;
}

void Graphics::DrawMinimap(
    Map &map,
    const std::vector<std::unique_ptr<Entity>> &entities,
    const std::vector<ResourceNode> &resources,
    const std::vector<CraftingStation> &stations,
    int playerX,
    int playerY)
{
    constexpr int columns = 30;
    constexpr int rows = 22;
    constexpr float minimapTileSize = 7.0f;
    constexpr SDL_FRect panel{1030.0f, 18.0f, 230.0f, 188.0f};
    constexpr SDL_FRect mapBounds{1040.0f, 42.0f, 210.0f, 154.0f};
    minimapOriginX = std::clamp(
        playerX - columns / 2,
        0,
        std::max(0, map.GetWidth() - columns));
    minimapOriginY = std::clamp(
        playerY - rows / 2,
        0,
        std::max(0, map.GetHeight() - rows));
    minimapOriginX = std::clamp(playerX - columns / 2, 0,
        std::max(0, map.GetWidth() - columns));
    minimapOriginY = std::clamp(playerY - rows / 2, 0,
        std::max(0, map.GetHeight() - rows));

    SDL_SetRenderDrawColor(renderer, 17, 21, 23, 238);
    SDL_RenderFillRect(renderer, &panel);
    SDL_SetRenderDrawColor(renderer, 164, 139, 87, 255);
    SDL_RenderRect(renderer, &panel);
    SDL_SetRenderDrawColor(renderer, 235, 225, 202, 255);
    SDL_RenderDebugText(renderer, panel.x + 10.0f, panel.y + 10.0f,
        "AREA MAP — CLICK TO WALK");

    for (int localY = 0; localY < rows; ++localY)
    {
        for (int localX = 0; localX < columns; ++localX)
        {
            const TileType tileType = map.GetTileType(
                minimapOriginX + localX,
                minimapOriginY + localY);
            switch (tileType)
            {
            case TileType::DIRT:
                SDL_SetRenderDrawColor(renderer, 127, 94, 57, 255);
                break;
            case TileType::STONE:
                SDL_SetRenderDrawColor(renderer, 105, 108, 106, 255);
                break;
            case TileType::WATER:
                SDL_SetRenderDrawColor(renderer, 43, 91, 139, 255);
                break;
            case TileType::TREE:
                SDL_SetRenderDrawColor(renderer, 24, 66, 31, 255);
                break;
            case TileType::STATION:
                SDL_SetRenderDrawColor(renderer, 120, 104, 72, 255);
                break;
            case TileType::WALL:
                SDL_SetRenderDrawColor(renderer, 64, 64, 68, 255);
                break;
            case TileType::GRASS:
            default:
                SDL_SetRenderDrawColor(renderer, 47, 83, 48, 255);
                break;
            }
            const SDL_FRect tile{
                mapBounds.x + localX * minimapTileSize,
                mapBounds.y + localY * minimapTileSize,
                minimapTileSize,
                minimapTileSize};
            SDL_RenderFillRect(renderer, &tile);
        }
    }

    const auto drawMarker = [this](
        int worldX, int worldY, Uint8 red, Uint8 green, Uint8 blue,
        float size)
    {
        constexpr float mapX = 1040.0f;
        constexpr float mapY = 42.0f;
        constexpr float scale = 7.0f;
        const int localX = worldX - minimapOriginX;
        const int localY = worldY - minimapOriginY;
        if (localX < 0 || localY < 0 || localX >= 30 || localY >= 22)
        {
            return;
        }
        const SDL_FRect marker{
            mapX + localX * scale + (scale - size) * 0.5f,
            mapY + localY * scale + (scale - size) * 0.5f,
            size,
            size};
        SDL_SetRenderDrawColor(renderer, red, green, blue, 255);
        SDL_RenderFillRect(renderer, &marker);
    };

    for (const ResourceNode &resource : resources)
        if (resource.IsActive())
            drawMarker(resource.GetX(), resource.GetY(), 88, 190, 92, 3.0f);
    for (const CraftingStation &station : stations)
        drawMarker(station.GetX(), station.GetY(), 245, 190, 70, 5.0f);
    for (const auto &entity : entities)
    {
        if (const Monster *monster = dynamic_cast<const Monster *>(entity.get()))
        {
            if (monster->IsAlive())
                drawMarker(monster->GetPosition().GetX(),
                    monster->GetPosition().GetY(), 225, 70, 70, 4.0f);
        }
        else if (dynamic_cast<const NPC *>(entity.get()) != nullptr)
        {
            drawMarker(entity->GetPosition().GetX(),
                entity->GetPosition().GetY(), 240, 150, 55, 4.0f);
        }
    }
    drawMarker(playerX, playerY, 110, 215, 255, 6.0f);
}

void Graphics::DrawMap(Map &map)
{
    for (int y = 0; y < map.GetHeight(); y++)
    {
        for (int x = 0; x < map.GetWidth(); x++)
        {
            SDL_FRect tileRectangle{
                static_cast<float>(x * TileSize),
                static_cast<float>(y * TileSize),
                static_cast<float>(TileSize),
                static_cast<float>(TileSize)};

            switch (map.GetTileType(x, y))
            {
            case TileType::GRASS:
                SDL_SetRenderDrawColor(
                    renderer,
                    static_cast<Uint8>(42 + ((x + y) % 3) * 3),
                    static_cast<Uint8>(76 + ((x * 3 + y) % 4) * 3),
                    static_cast<Uint8>(43 + ((x + y * 2) % 3) * 2),
                    255);
                break;
            case TileType::DIRT:
                SDL_SetRenderDrawColor(renderer,
                    static_cast<Uint8>(111 + (x + y) % 3 * 4),
                    static_cast<Uint8>(86 + (x * 2 + y) % 3 * 3),
                    55, 255);
                break;
            case TileType::STONE:
                SDL_SetRenderDrawColor(renderer,
                    static_cast<Uint8>(91 + (x + y) % 3 * 4),
                    static_cast<Uint8>(94 + (x * 2 + y) % 3 * 3),
                    92, 255);
                break;
            case TileType::WATER:
                SDL_SetRenderDrawColor(renderer, 35, 70, 105, 255);
                break;
            case TileType::TREE:
                SDL_SetRenderDrawColor(renderer, 31, 62, 35, 255);
                break;
            case TileType::STATION:
                SDL_SetRenderDrawColor(renderer, 73, 68, 60, 255);
                break;
            case TileType::WALL:
                SDL_SetRenderDrawColor(renderer, 58, 58, 62, 255);
                break;
            }

            SDL_RenderFillRect(
                renderer,
                &tileRectangle);
        }
    }

    SDL_SetRenderDrawColor(renderer, 225, 214, 176, 180);
    SDL_RenderDebugText(renderer, 34.0f, 154.0f, "VILLAGE SQUARE");
    SDL_RenderDebugText(renderer, 112.0f, 226.0f, "WOODLAND");
    SDL_RenderDebugText(renderer, 112.0f, 414.0f, "COPPER MINE");
    SDL_RenderDebugText(renderer, 424.0f, 414.0f, "WORKSHOP");
    SDL_RenderDebugText(renderer, 168.0f, 122.0f, "TRAINING FIELD");
}
void Graphics::DrawGrid(const Map &map)

{
    SDL_SetRenderDrawColor(
        renderer,
        35,
        55,
        38,
        110);

    for (int x = 0; x <= map.GetWidth() * TileSize; x += TileSize)
    {
        SDL_RenderLine(
            renderer,
            static_cast<float>(x),
            0.0f,
            static_cast<float>(x),
            static_cast<float>(WindowHeight));
    }

    for (int y = 0; y <= map.GetHeight() * TileSize; y += TileSize)
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
    if (!visualPlayerInitialized)
    {
        visualPlayerX = static_cast<float>(playerX);
        visualPlayerY = static_cast<float>(playerY);
        visualPlayerInitialized = true;
    }
    const float targetX = static_cast<float>(playerX);
    const float targetY = static_cast<float>(playerY);
    const float smoothing = 1.0f - std::exp(
        -14.0f * presentationDeltaSeconds);
    visualPlayerX += (targetX - visualPlayerX) * smoothing;
    visualPlayerY += (targetY - visualPlayerY) * smoothing;
    if (std::abs(targetX - visualPlayerX) < 0.01f) visualPlayerX = targetX;
    if (std::abs(targetY - visualPlayerY) < 0.01f) visualPlayerY = targetY;

    SDL_FRect playerRectangle =
        SDL_FRect{visualPlayerX * TileSize + 4.0f,
                  visualPlayerY * TileSize + 4.0f,
                  TileSize - 8.0f, TileSize - 8.0f};

    const float pulse = 0.5f + 0.5f * std::sin(
        static_cast<float>(SDL_GetTicks()) * 0.008f);

    SDL_FRect shadow{playerRectangle.x + 3.0f,
                     playerRectangle.y + playerRectangle.h - 3.0f,
                     playerRectangle.w - 6.0f, 6.0f};
    SDL_SetRenderDrawColor(renderer, 12, 20, 25, 120);
    SDL_RenderFillRect(renderer, &shadow);

    SDL_SetRenderDrawColor(
        renderer,
        static_cast<Uint8>(45 + pulse * 18),
        static_cast<Uint8>(135 + pulse * 20),
        225,
        255);

    SDL_RenderFillRect(
        renderer,
        &playerRectangle);

    SDL_SetRenderDrawColor(renderer, 165, 225, 255, 255);
    SDL_RenderRect(renderer, &playerRectangle);

    SDL_FRect facingMark{playerRectangle.x + playerRectangle.w * 0.35f,
                         playerRectangle.y + 3.0f,
                         playerRectangle.w * 0.30f, 3.0f};
    SDL_RenderFillRect(renderer, &facingMark);
}

void Graphics::DrawPlayerHealthBar(
    const Player &player)
{
    SDL_FRect background{visualPlayerX * TileSize + 4.0f,
                         visualPlayerY * TileSize - 7.0f,
                         TileSize - 8.0f, 5.0f};
    SDL_FRect fill{background.x, background.y,
                   background.w * CalculateHealthRatio(
                       player.GetCurrentHealth(), player.GetMaximumHealth()),
                   background.h};

    SDL_SetRenderDrawColor(
        renderer,
        55,
        55,
        55,
        255);

    SDL_RenderFillRect(
        renderer,
        &background);

    SDL_SetRenderDrawColor(
        renderer,
        70,
        170,
        255,
        255);

    SDL_RenderFillRect(
        renderer,
        &fill);

    SDL_SetRenderDrawColor(
        renderer,
        player.IsAlive() ? 20 : 150,
        player.IsAlive() ? 20 : 35,
        player.IsAlive() ? 20 : 35,
        255);

    SDL_RenderRect(
        renderer,
        &background);

    std::string healthText =
        std::to_string(player.GetCurrentHealth()) +
        "/" +
        std::to_string(player.GetMaximumHealth());

    SDL_SetRenderDrawColor(
        renderer,
        230,
        230,
        230,
        255);

    SDL_RenderDebugText(
        renderer,
        background.x,
        background.y - 12.0f,
        healthText.c_str());
}

SDL_FRect Graphics::GetMonsterScreenRectangle(
    const Monster &monster,
    int cameraTileX,
    int cameraTileY) const
{
    return MakeEntityTileRectangle(
        monster.GetPosition().GetX(),
        monster.GetPosition().GetY(),
        5.0f,
        cameraTileX,
        cameraTileY);
}

std::optional<int> Graphics::GetMonsterAtScreenPosition(
    int mouseX,
    int mouseY,
    const std::vector<std::unique_ptr<Entity>> &entities,
    int cameraTileX,
    int cameraTileY) const
{
    for (const auto &entity : entities)
    {
        Monster *monster =
            dynamic_cast<Monster *>(entity.get());

        if (monster == nullptr || !monster->IsAlive())
        {
            continue;
        }

        SDL_FRect monsterRectangle =
            GetMonsterScreenRectangle(
                *monster,
                cameraTileX,
                cameraTileY);

        if (IsPointInsideRectangle(
                static_cast<float>(mouseX),
                static_cast<float>(mouseY),
                monsterRectangle))
        {
            return monster->GetID();
        }
    }

    return std::nullopt;
}

SDL_FRect Graphics::GetFriendlyNpcScreenRectangle(
    const NPC &npc,
    int cameraTileX,
    int cameraTileY) const
{
    return MakeEntityTileRectangle(
        npc.GetPosition().GetX(),
        npc.GetPosition().GetY(),
        5.0f,
        cameraTileX,
        cameraTileY);
}

std::optional<int> Graphics::GetFriendlyNpcAtScreenPosition(
    int mouseX,
    int mouseY,
    const std::vector<std::unique_ptr<Entity>> &entities,
    int cameraTileX,
    int cameraTileY) const
{
    for (const auto &entity : entities)
    {
        NPC *npc = dynamic_cast<NPC *>(entity.get());
        const NpcDefinition *definition = npc == nullptr
            ? nullptr
            : NpcDefinitionDatabase::TryGet(npc->GetNpcType());
        if (npc == nullptr || definition == nullptr ||
            definition->kind != NpcKind::FRIENDLY)
        {
            continue;
        }

        const SDL_FRect rectangle = GetFriendlyNpcScreenRectangle(
            *npc, cameraTileX, cameraTileY);
        if (IsPointInsideRectangle(
                static_cast<float>(mouseX),
                static_cast<float>(mouseY),
                rectangle))
        {
            return npc->GetID();
        }
    }
    return std::nullopt;
}

Graphics::WorldHoverTarget Graphics::ResolveWorldHoverTarget(
    float screenX,
    float screenY,
    const std::vector<std::unique_ptr<Entity>> &entities,
    const std::vector<ResourceNode> &resources,
    const std::vector<CraftingStation> &stations,
    const std::vector<GroundItem> &groundItems) const
{
    for (const GroundItem &item : groundItems)
    {
        const SDL_FRect bounds{
            static_cast<float>(item.position.GetX() * TileSize + 5),
            static_cast<float>(item.position.GetY() * TileSize + 5),
            static_cast<float>(TileSize - 10),
            static_cast<float>(TileSize - 10)};
        if (IsPointInsideRectangle(screenX, screenY, bounds))
        {
            return {WorldHoverTargetType::GROUND_ITEM, item.id, bounds,
                ItemDatabase::Get(item.itemType).GetName(), "Take"};
        }
    }

    // Mirror EngineWorldClickDecision's established object-first priority.
    for (const ResourceNode &resource : resources)
    {
        if (!resource.IsActive()) continue;
        const SDL_FRect bounds{
            static_cast<float>(resource.GetX() * TileSize),
            static_cast<float>(resource.GetY() * TileSize),
            static_cast<float>(TileSize), static_cast<float>(TileSize)};
        if (IsPointInsideRectangle(screenX, screenY, bounds))
        {
            const ResourceDefinition &definition =
                ResourceDatabase::Get(resource.GetResourceType());
            return {WorldHoverTargetType::RESOURCE, resource.GetID(), bounds,
                definition.GetName(), "Gather"};
        }
    }

    for (const CraftingStation &station : stations)
    {
        const SDL_FRect bounds{
            static_cast<float>(station.GetX() * TileSize),
            static_cast<float>(station.GetY() * TileSize),
            static_cast<float>(TileSize), static_cast<float>(TileSize)};
        if (IsPointInsideRectangle(screenX, screenY, bounds))
        {
            const bool furnace = station.GetStationType() == StationType::FURNACE;
            const bool bank = station.GetStationType() == StationType::BANK;
            return {WorldHoverTargetType::STATION, station.GetID(), bounds,
                furnace ? "Furnace" : (bank ? "Bank chest" : "Crafting table"),
                furnace ? "Smelt" : (bank ? "Bank" : "Craft")};
        }
    }

    for (const auto &entity : entities)
    {
        const Monster *monster = dynamic_cast<const Monster *>(entity.get());
        if (monster == nullptr || !monster->IsAlive()) continue;
        const SDL_FRect bounds = GetMonsterScreenRectangle(*monster);
        if (IsPointInsideRectangle(screenX, screenY, bounds))
        {
            const NpcDefinition *definition =
                NpcDefinitionDatabase::TryGet(monster->GetNpcType());
            return {WorldHoverTargetType::MONSTER, monster->GetID(), bounds,
                definition == nullptr ? "Enemy" : definition->name, "Attack"};
        }
    }

    for (const auto &entity : entities)
    {
        const NPC *npc = dynamic_cast<const NPC *>(entity.get());
        const NpcDefinition *definition = npc == nullptr
            ? nullptr
            : NpcDefinitionDatabase::TryGet(npc->GetNpcType());
        if (npc == nullptr || definition == nullptr ||
            definition->kind != NpcKind::FRIENDLY)
        {
            continue;
        }

        const SDL_FRect bounds = GetFriendlyNpcScreenRectangle(*npc);
        if (IsPointInsideRectangle(screenX, screenY, bounds))
        {
            return {WorldHoverTargetType::FRIENDLY_NPC, npc->GetID(),
                bounds, definition->name, "Talk-to"};
        }
    }

    return {};
}

void Graphics::DrawWorldHoverFeedback()
{
    if (!worldHoverTarget.HasTarget())
    {
        return;
    }

    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, 255, 220, 90, 52);
    SDL_RenderFillRect(renderer, &worldHoverTarget.bounds);
    SDL_SetRenderDrawColor(renderer, 255, 226, 105, 255);
    SDL_RenderRect(renderer, &worldHoverTarget.bounds);

    const std::string label = worldHoverTarget.primaryAction + " " +
        worldHoverTarget.name;
    const float labelWidth = std::max(150.0f,
        static_cast<float>(label.size() * 8 + 20));
    const SDL_FRect labelBounds{
        cameraPixelX + 12.0f,
        cameraPixelY + 12.0f,
        labelWidth,
        28.0f};
    SDL_SetRenderDrawColor(renderer, 20, 18, 15, 225);
    SDL_RenderFillRect(renderer, &labelBounds);
    SDL_SetRenderDrawColor(renderer, 230, 197, 102, 255);
    SDL_RenderRect(renderer, &labelBounds);
    SDL_SetRenderDrawColor(renderer, 255, 241, 190, 255);
    SDL_RenderDebugText(renderer,
        cameraPixelX + 22.0f,
        cameraPixelY + 22.0f,
        label.c_str());
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
}

void Graphics::OpenWorldContextMenu(float screenX, float screenY)
{
    constexpr float rowHeight = 24.0f;
    constexpr float menuWidth = 210.0f;
    const int rowCount = worldHoverTarget.HasTarget() ? 4 : 2;
    const float menuHeight = rowHeight * static_cast<float>(rowCount);
    const SDL_FRect worldBounds = GetSidePanelRectangle();
    const float maximumX = std::max(0.0f, worldBounds.x - menuWidth - 4.0f);

    worldContextMenu.open = true;
    worldContextMenu.target = worldHoverTarget;
    const SDL_FPoint worldPoint = ScreenToWorldPoint(screenX, screenY);
    worldContextMenu.tileX = static_cast<int>(worldPoint.x) / TileSize;
    worldContextMenu.tileY = static_cast<int>(worldPoint.y) / TileSize;
    worldContextMenu.screenX = static_cast<int>(worldPoint.x);
    worldContextMenu.screenY = static_cast<int>(worldPoint.y);
    worldContextMenu.bounds = SDL_FRect{
        std::clamp(screenX, 4.0f, maximumX),
        std::clamp(screenY, 4.0f,
            static_cast<float>(WindowHeight) - menuHeight - 4.0f),
        menuWidth,
        menuHeight};
}

bool Graphics::HandleWorldContextMenuClick(float screenX, float screenY)
{
    if (!worldContextMenu.open)
    {
        return false;
    }

    if (!IsPointInsideRectangle(screenX, screenY, worldContextMenu.bounds))
    {
        worldContextMenu.open = false;
        return true;
    }

    constexpr float rowHeight = 24.0f;
    const int selectedRow = static_cast<int>(
        (screenY - worldContextMenu.bounds.y) / rowHeight);
    const bool hasTarget = worldContextMenu.target.HasTarget();

    if (hasTarget && selectedRow == 0)
    {
        clickedTileX = worldContextMenu.tileX;
        clickedTileY = worldContextMenu.tileY;
        clickedMouseX = worldContextMenu.screenX;
        clickedMouseY = worldContextMenu.screenY;
        hasClickedTile = true;
        clickPending = true;
    }
    else if ((hasTarget && selectedRow == 1) ||
             (!hasTarget && selectedRow == 0))
    {
        walkTileX = worldContextMenu.tileX;
        walkTileY = worldContextMenu.tileY;
        walkClickPending = true;
    }
    else if (hasTarget && selectedRow == 2)
    {
        examineMessage = worldContextMenu.target.name +
            " — " + worldContextMenu.target.primaryAction +
            " is its main interaction.";
        examineMessageUntil = SDL_GetTicks() + 4500;
    }

    worldContextMenu.open = false;
    return true;
}

void Graphics::DrawWorldContextMenu()
{
    if (!examineMessage.empty() && SDL_GetTicks() < examineMessageUntil)
    {
        const float width = std::min(620.0f,
            std::max(260.0f, static_cast<float>(examineMessage.size() * 8 + 24)));
        const SDL_FRect messageBounds{
            12.0f, static_cast<float>(WindowHeight) - 48.0f, width, 32.0f};
        SDL_SetRenderDrawColor(renderer, 22, 19, 15, 235);
        SDL_RenderFillRect(renderer, &messageBounds);
        SDL_SetRenderDrawColor(renderer, 205, 178, 105, 255);
        SDL_RenderRect(renderer, &messageBounds);
        SDL_SetRenderDrawColor(renderer, 250, 238, 205, 255);
        SDL_RenderDebugText(renderer, messageBounds.x + 10.0f,
            messageBounds.y + 11.0f, examineMessage.c_str());
    }

    if (!worldContextMenu.open)
    {
        return;
    }

    SDL_SetRenderDrawColor(renderer, 25, 22, 18, 248);
    SDL_RenderFillRect(renderer, &worldContextMenu.bounds);
    SDL_SetRenderDrawColor(renderer, 218, 188, 106, 255);
    SDL_RenderRect(renderer, &worldContextMenu.bounds);

    std::vector<std::string> rows;
    if (worldContextMenu.target.HasTarget())
    {
        rows.push_back(worldContextMenu.target.primaryAction + " " +
            worldContextMenu.target.name);
        rows.push_back("Walk here");
        rows.push_back("Examine " + worldContextMenu.target.name);
        rows.push_back("Cancel");
    }
    else
    {
        rows.push_back("Walk here");
        rows.push_back("Cancel");
    }

    constexpr float rowHeight = 24.0f;
    for (std::size_t index = 0; index < rows.size(); ++index)
    {
        const SDL_FRect row{worldContextMenu.bounds.x,
            worldContextMenu.bounds.y + static_cast<float>(index) * rowHeight,
            worldContextMenu.bounds.w, rowHeight};
        if (IsPointInsideRectangle(mouseX, mouseY, row))
        {
            SDL_SetRenderDrawColor(renderer, 80, 67, 43, 255);
            SDL_RenderFillRect(renderer, &row);
        }
        SDL_SetRenderDrawColor(renderer,
            index == 0 ? 255 : 232,
            index == 0 ? 218 : 226,
            index == 0 ? 112 : 205, 255);
        SDL_RenderDebugText(renderer, row.x + 10.0f, row.y + 8.0f,
            rows[index].c_str());
    }
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

void Graphics::DrawMonsters(
    const std::vector<std::unique_ptr<Entity>> &entities,
    int selectedCombatTargetID,
    int localPlayerID)
{
    for (const auto &entity : entities)
    {
        Monster *monster =
            dynamic_cast<Monster *>(entity.get());

        if (monster == nullptr)
        {
            continue;
        }

        if (monster->IsAlive())
        {
            SDL_FRect monsterRectangle =
                GetMonsterScreenRectangle(*monster);
            const bool selected = monster->GetID() == selectedCombatTargetID;
            const bool aggressive = monster->HasAggressionDefinition();
            const bool attackingPlayer =
                monster->GetAggressionTargetEntityID() == localPlayerID;

            if (selected)
            {
                const SDL_FRect targetRing{
                    monsterRectangle.x - 5.0f,
                    monsterRectangle.y + monsterRectangle.h - 6.0f,
                    monsterRectangle.w + 10.0f,
                    10.0f};
                SDL_SetRenderDrawColor(renderer, 255, 211, 70, 255);
                SDL_RenderRect(renderer, &targetRing);
            }

            SDL_FRect shadow{
                monsterRectangle.x + 2.0f,
                monsterRectangle.y + monsterRectangle.h - 3.0f,
                monsterRectangle.w - 4.0f, 6.0f};
            SDL_SetRenderDrawColor(renderer, 25, 12, 14, 150);
            SDL_RenderFillRect(renderer, &shadow);

            SDL_SetRenderDrawColor(renderer,
                aggressive ? 164 : 70,
                aggressive ? 48 : 135,
                aggressive ? 48 : 78,
                255);

            SDL_RenderFillRect(
                renderer,
                &monsterRectangle);

            SDL_SetRenderDrawColor(renderer,
                aggressive ? 235 : 135,
                aggressive ? 105 : 205,
                aggressive ? 95 : 145,
                255);

            SDL_RenderRect(
                renderer,
                &monsterRectangle);

            SDL_FRect eyeLeft{monsterRectangle.x + 5.0f,
                              monsterRectangle.y + 7.0f, 3.0f, 3.0f};
            SDL_FRect eyeRight{monsterRectangle.x + monsterRectangle.w - 8.0f,
                               monsterRectangle.y + 7.0f, 3.0f, 3.0f};
            SDL_SetRenderDrawColor(renderer, 255, 210, 115, 255);
            SDL_RenderFillRect(renderer, &eyeLeft);
            SDL_RenderFillRect(renderer, &eyeRight);
            if (attackingPlayer)
            {
                SDL_SetRenderDrawColor(renderer, 255, 95, 75, 255);
                SDL_RenderDebugText(renderer,
                    monsterRectangle.x + 8.0f,
                    monsterRectangle.y - 20.0f,
                    "!");
            }
            continue;
        }

        SDL_FRect corpseRectangle =
            GetMonsterCorpseRectangle(*monster);

        SDL_SetRenderDrawColor(
            renderer,
            55,
            35,
            35,
            255);

        SDL_RenderFillRect(
            renderer,
            &corpseRectangle);

        SDL_SetRenderDrawColor(
            renderer,
            25,
            18,
            18,
            255);

        SDL_RenderRect(
            renderer,
            &corpseRectangle);

        SDL_SetRenderDrawColor(
            renderer,
            25,
            10,
            10,
            255);

        SDL_RenderLine(
            renderer,
            corpseRectangle.x + 1.0f,
            corpseRectangle.y + 1.0f,
            corpseRectangle.x + corpseRectangle.w - 1.0f,
            corpseRectangle.y + corpseRectangle.h - 1.0f);

        SDL_RenderLine(
            renderer,
            corpseRectangle.x + corpseRectangle.w - 1.0f,
            corpseRectangle.y + 1.0f,
            corpseRectangle.x + 1.0f,
            corpseRectangle.y + corpseRectangle.h - 1.0f);
    }
}

void Graphics::DrawCombatTargetPanel(
    const std::vector<std::unique_ptr<Entity>> &entities,
    int selectedCombatTargetID,
    int localPlayerID)
{
    const Monster *selected = nullptr;
    const Monster *attacker = nullptr;
    for (const auto &entity : entities)
    {
        const Monster *monster = dynamic_cast<const Monster *>(entity.get());
        if (monster == nullptr || !monster->IsAlive()) continue;
        if (monster->GetID() == selectedCombatTargetID) selected = monster;
        if (monster->GetAggressionTargetEntityID() == localPlayerID)
            attacker = monster;
    }
    const Monster *displayed = selected != nullptr ? selected : attacker;
    if (displayed == nullptr) return;

    const NpcDefinition *definition =
        NpcDefinitionDatabase::TryGet(displayed->GetNpcType());
    const CombatRatings ratings = displayed->GetCombatRatings();
    const int level = std::max(1,
        (ratings.attackAccuracy + ratings.meleeStrength + ratings.defence) / 3);
    const bool aggressive = displayed->HasAggressionDefinition();
    const bool incoming =
        displayed->GetAggressionTargetEntityID() == localPlayerID;
    const SDL_FRect panel{300.0f, 18.0f, 360.0f, 60.0f};
    SDL_SetRenderDrawColor(renderer, 20, 24, 28, 235);
    SDL_RenderFillRect(renderer, &panel);
    SDL_SetRenderDrawColor(renderer,
        incoming ? 235 : 220,
        incoming ? 80 : 178,
        incoming ? 65 : 75,
        255);
    SDL_RenderRect(renderer, &panel);
    const std::string name = definition == nullptr ? "Enemy" : definition->name;
    const std::string heading = name + "  Lv " + std::to_string(level) +
        "  " + (aggressive ? "AGGRESSIVE" : "PASSIVE");
    SDL_RenderDebugText(renderer, panel.x + 12.0f, panel.y + 12.0f,
        heading.c_str());
    const SDL_FRect healthBackground{
        panel.x + 12.0f, panel.y + 36.0f, panel.w - 24.0f, 12.0f};
    SDL_SetRenderDrawColor(renderer, 53, 35, 38, 255);
    SDL_RenderFillRect(renderer, &healthBackground);
    SDL_FRect healthFill = healthBackground;
    healthFill.w *= CalculateHealthRatio(
        displayed->GetCurrentHealth(), displayed->GetMaximumHealth());
    SDL_SetRenderDrawColor(renderer, 190, 58, 65, 255);
    SDL_RenderFillRect(renderer, &healthFill);
    if (incoming)
    {
        SDL_SetRenderDrawColor(renderer, 255, 225, 185, 255);
        SDL_RenderDebugText(renderer, panel.x + panel.w - 96.0f,
            panel.y + 12.0f, "INCOMING!");
    }
}

void Graphics::DrawMonsterHealthBars(
    const std::vector<std::unique_ptr<Entity>> &entities)
{
    for (const auto &entity : entities)
    {
        Monster *monster = dynamic_cast<Monster *>(entity.get());

        if (monster == nullptr || !monster->IsAlive())
        {
            continue;
        }

        SDL_FRect background =
            GetMonsterHealthBarBackgroundRectangle(*monster);

        SDL_FRect fill =
            GetMonsterHealthBarFillRectangle(*monster);

        SDL_SetRenderDrawColor(
            renderer,
            55,
            55,
            55,
            255);

        SDL_RenderFillRect(
            renderer,
            &background);

        SDL_SetRenderDrawColor(
            renderer,
            75,
            185,
            85,
            255);

        SDL_RenderFillRect(
            renderer,
            &fill);

        SDL_SetRenderDrawColor(
            renderer,
            20,
            20,
            20,
            255);

        SDL_RenderRect(
            renderer,
            &background);
    }
}

void Graphics::DrawMonsterCombatFeedback(
    const std::vector<std::unique_ptr<Entity>> &entities,
    const std::map<int, MeleeCombatFeedback> &combatFeedbacks)
{
    for (const auto &[defenderEntityID, feedback] : combatFeedbacks)
    {
        (void)defenderEntityID;

        Entity *entity = GetEntityByID(
            entities,
            feedback.defenderEntityID);

        if (entity == nullptr)
        {
            continue;
        }

        SDL_FPoint textPosition{};

        if (Monster *monster = dynamic_cast<Monster *>(entity))
        {
            textPosition = GetMonsterCombatFeedbackPosition(
                *monster);
        }
        else if (Player *player = dynamic_cast<Player *>(entity))
        {
            textPosition = GetPlayerCombatFeedbackPosition(
                *player);
        }
        else
        {
            continue;
        }

        SDL_SetRenderDrawColor(
            renderer,
            feedback.hit ? 255 : 205,
            feedback.hit ? 92 : 220,
            feedback.hit ? 70 : 235,
            255);

        std::string feedbackText = feedback.hit
                                       ? std::to_string(feedback.actualDamageApplied)
                                       : "MISS";

        SDL_RenderDebugText(
            renderer,
            textPosition.x - 2.0f,
            textPosition.y - 7.0f,
            feedbackText.c_str());

        SDL_FRect impact{textPosition.x - 6.0f, textPosition.y + 4.0f,
                         22.0f, 2.0f};
        SDL_RenderFillRect(renderer, &impact);
    }
}
void Graphics::DrawStations(
    const std::vector<CraftingStation> &stations)
{
    for (const CraftingStation &station :
         stations)
    {
        float tileX =
            static_cast<float>(
                station.GetX() * TileSize);

        float tileY =
            static_cast<float>(
                station.GetY() * TileSize);

        switch (station.GetStationType())
        {
        case StationType::FURNACE:
        {
            SDL_SetRenderDrawColor(
                renderer,
                80,
                80,
                85,
                255);

            SDL_FRect furnaceBody{
                tileX + TileSize * 0.15f,
                tileY + TileSize * 0.18f,
                TileSize * 0.70f,
                TileSize * 0.68f};

            SDL_RenderFillRect(
                renderer,
                &furnaceBody);

            SDL_SetRenderDrawColor(
                renderer,
                40,
                40,
                45,
                255);

            SDL_FRect opening{
                tileX + TileSize * 0.30f,
                tileY + TileSize * 0.52f,
                TileSize * 0.40f,
                TileSize * 0.25f};

            SDL_RenderFillRect(
                renderer,
                &opening);

            SDL_SetRenderDrawColor(
                renderer,
                240,
                105,
                25,
                255);

            SDL_FRect fire{
                tileX + TileSize * 0.38f,
                tileY + TileSize * 0.60f,
                TileSize * 0.24f,
                TileSize * 0.12f};

            SDL_RenderFillRect(
                renderer,
                &fire);

            SDL_SetRenderDrawColor(
                renderer,
                65,
                65,
                70,
                255);

            SDL_FRect chimney{
                tileX + TileSize * 0.58f,
                tileY + TileSize * 0.05f,
                TileSize * 0.20f,
                TileSize * 0.25f};

            SDL_RenderFillRect(
                renderer,
                &chimney);

            break;
        }

        case StationType::CRAFTING_TABLE:
        {
            SDL_SetRenderDrawColor(
                renderer, 126, 78, 38, 255);
            SDL_FRect tableBody{
                tileX + TileSize * 0.10f,
                tileY + TileSize * 0.18f,
                TileSize * 0.80f,
                TileSize * 0.68f};
            SDL_RenderFillRect(renderer, &tableBody);

            SDL_SetRenderDrawColor(
                renderer, 205, 151, 78, 255);
            SDL_FRect tableTop{
                tileX + TileSize * 0.06f,
                tileY + TileSize * 0.12f,
                TileSize * 0.88f,
                TileSize * 0.18f};
            SDL_RenderFillRect(renderer, &tableTop);

            SDL_SetRenderDrawColor(
                renderer, 75, 47, 26, 255);
            for (int index = 1; index < 3; ++index)
            {
                const float offset =
                    static_cast<float>(index) * TileSize * 0.28f;
                SDL_FRect vertical{
                    tileX + TileSize * 0.08f + offset,
                    tileY + TileSize * 0.12f,
                    1.0f,
                    TileSize * 0.18f};
                SDL_RenderFillRect(renderer, &vertical);
            }
            break;
        }

        case StationType::BANK:
        {
            SDL_SetRenderDrawColor(renderer, 94, 62, 32, 255);
            SDL_FRect chest{tileX + 3.0f, tileY + 10.0f,
                TileSize - 6.0f, TileSize - 13.0f};
            SDL_RenderFillRect(renderer, &chest);
            SDL_SetRenderDrawColor(renderer, 210, 171, 67, 255);
            SDL_RenderRect(renderer, &chest);
            SDL_FRect band{tileX + 3.0f, tileY + 16.0f,
                TileSize - 6.0f, 4.0f};
            SDL_RenderFillRect(renderer, &band);
            SDL_FRect lock{tileX + 14.0f, tileY + 17.0f, 5.0f, 7.0f};
            SDL_RenderFillRect(renderer, &lock);
            break;
        }

        case StationType::NONE:
        default:
            break;
        }
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
            const ResourceDefinition &definition =
                ResourceDatabase::Get(
                    resource.GetResourceType());

            switch (definition.GetDepletedVisualType())
            {
            case DepletedVisualType::STUMP:
            {
                SDL_SetRenderDrawColor(
                    renderer,
                    105,
                    65,
                    30,
                    255);

                SDL_FRect stump{
                    tileX + TileSize * 0.30f,
                    tileY + TileSize * 0.58f,
                    TileSize * 0.40f,
                    TileSize * 0.25f};

                SDL_RenderFillRect(
                    renderer,
                    &stump);

                break;
            }

            case DepletedVisualType::ROCK_RUBBLE:
            {
                SDL_SetRenderDrawColor(
                    renderer,
                    85,
                    85,
                    90,
                    255);

                SDL_FRect leftRock{
                    tileX + TileSize * 0.18f,
                    tileY + TileSize * 0.68f,
                    TileSize * 0.25f,
                    TileSize * 0.16f};

                SDL_FRect middleRock{
                    tileX + TileSize * 0.40f,
                    tileY + TileSize * 0.61f,
                    TileSize * 0.28f,
                    TileSize * 0.22f};

                SDL_FRect rightRock{
                    tileX + TileSize * 0.66f,
                    tileY + TileSize * 0.70f,
                    TileSize * 0.18f,
                    TileSize * 0.13f};

                SDL_RenderFillRect(
                    renderer,
                    &leftRock);

                SDL_RenderFillRect(
                    renderer,
                    &middleRock);

                SDL_RenderFillRect(
                    renderer,
                    &rightRock);

                break;
            }

            case DepletedVisualType::NONE:
            default:
                break;
            }

            continue;
        }

        int leafRed = 55;
        int leafGreen = 140;
        int leafBlue = 55;
        bool drawDefaultTree = true;

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

        case ResourceType::COPPER_ROCK:
        {
            SDL_SetRenderDrawColor(
                renderer,
                105,
                105,
                115,
                255);

            SDL_FRect rockBody{
                tileX + TileSize * 0.15f,
                tileY + TileSize * 0.35f,
                TileSize * 0.70f,
                TileSize * 0.50f};

            SDL_RenderFillRect(
                renderer,
                &rockBody);

            SDL_SetRenderDrawColor(
                renderer,
                190,
                105,
                55,
                255);

            SDL_FRect copperDeposit{
                tileX + TileSize * 0.32f,
                tileY + TileSize * 0.43f,
                TileSize * 0.20f,
                TileSize * 0.15f};

            SDL_RenderFillRect(
                renderer,
                &copperDeposit);

            drawDefaultTree = false;
            break;
        }

        case ResourceType::TIN_ROCK:
        {
            SDL_SetRenderDrawColor(
                renderer,
                105,
                105,
                115,
                255);

            SDL_FRect rockBody{
                tileX + TileSize * 0.15f,
                tileY + TileSize * 0.35f,
                TileSize * 0.70f,
                TileSize * 0.50f};

            SDL_RenderFillRect(
                renderer,
                &rockBody);

            SDL_SetRenderDrawColor(
                renderer,
                190,
                190,
                200,
                255);

            SDL_FRect tinDeposit{
                tileX + TileSize * 0.32f,
                tileY + TileSize * 0.43f,
                TileSize * 0.20f,
                TileSize * 0.15f};

            SDL_RenderFillRect(
                renderer,
                &tinDeposit);

            drawDefaultTree = false;
            break;
        }

        case ResourceType::IRON_ROCK:
        {
            SDL_SetRenderDrawColor(
                renderer,
                105,
                105,
                115,
                255);

            SDL_FRect rockBody{
                tileX + TileSize * 0.15f,
                tileY + TileSize * 0.35f,
                TileSize * 0.70f,
                TileSize * 0.50f};

            SDL_RenderFillRect(
                renderer,
                &rockBody);

            SDL_SetRenderDrawColor(
                renderer,
                135,
                85,
                65,
                255);

            SDL_FRect ironDeposit{
                tileX + TileSize * 0.32f,
                tileY + TileSize * 0.43f,
                TileSize * 0.20f,
                TileSize * 0.15f};

            SDL_RenderFillRect(
                renderer,
                &ironDeposit);

            drawDefaultTree = false;
            break;
        }

        case ResourceType::COAL_ROCK:
        {
            SDL_SetRenderDrawColor(
                renderer,
                45,
                45,
                50,
                255);

            SDL_FRect rockBody{
                tileX + TileSize * 0.15f,
                tileY + TileSize * 0.35f,
                TileSize * 0.70f,
                TileSize * 0.50f};

            SDL_RenderFillRect(
                renderer,
                &rockBody);

            SDL_SetRenderDrawColor(
                renderer,
                35,
                35,
                40,
                255);

            SDL_FRect coalDeposit{
                tileX + TileSize * 0.32f,
                tileY + TileSize * 0.43f,
                TileSize * 0.20f,
                TileSize * 0.15f};

            SDL_RenderFillRect(
                renderer,
                &coalDeposit);

            drawDefaultTree = false;
            break;
        }
        }

        if (!drawDefaultTree)
        {
            continue;
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

void Graphics::DrawGroundItems(const std::vector<GroundItem> &groundItems)
{
    for (const GroundItem &item : groundItems)
    {
        const float x = static_cast<float>(item.position.GetX() * TileSize);
        const float y = static_cast<float>(item.position.GetY() * TileSize);
        const SDL_FRect shadow{x + 8.0f, y + 21.0f, 16.0f, 6.0f};
        SDL_SetRenderDrawColor(renderer, 25, 22, 18, 130);
        SDL_RenderFillRect(renderer, &shadow);
        const SDL_FRect icon{x + 9.0f, y + 8.0f, 14.0f, 14.0f};
        SDL_SetRenderDrawColor(renderer, 224, 190, 78, 255);
        SDL_RenderFillRect(renderer, &icon);
        SDL_SetRenderDrawColor(renderer, 255, 231, 142, 255);
        SDL_RenderRect(renderer, &icon);
        if (item.amount > 1)
        {
            const std::string amount = std::to_string(item.amount);
            SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
            SDL_RenderDebugText(renderer, x + 20.0f, y + 5.0f, amount.c_str());
        }
    }
}
Graphics::SidePanelLayout Graphics::GetSidePanelLayout() const
{
    constexpr float panelWidth = 330.0f;
    constexpr float panelHeight = 348.0f;
    constexpr float panelMargin = 20.0f;
    constexpr float contentInset = 10.0f;
    constexpr float tabWidth = 45.0f;
    constexpr float tabHeight = 30.0f;
    constexpr float tabGap = 2.0f;
    constexpr float tabOffset = 4.0f;

    SidePanelLayout layout;
    layout.panelBounds = SDL_FRect{
        static_cast<float>(WindowWidth) - panelWidth - panelMargin,
        static_cast<float>(WindowHeight) - panelHeight - panelMargin,
        panelWidth,
        panelHeight};
    layout.contentBounds = SDL_FRect{
        layout.panelBounds.x + contentInset,
        layout.panelBounds.y + contentInset,
        layout.panelBounds.w - contentInset * 2.0f,
        layout.panelBounds.h - contentInset * 2.0f};
    for (std::size_t index = 0; index < layout.tabBounds.size(); ++index)
    {
        layout.tabBounds[index] = SDL_FRect{
            layout.panelBounds.x + static_cast<float>(index) * (tabWidth + tabGap),
            layout.panelBounds.y - tabHeight - tabOffset,
            tabWidth,
            tabHeight};
    }
    return layout;
}

SDL_FRect Graphics::GetSidePanelRectangle() const
{
    return GetSidePanelLayout().panelBounds;
}

void Graphics::DrawSidePanelFrame(const char *title, float headingYOffset)
{
    const SidePanelLayout layout = GetSidePanelLayout();
    SDL_SetRenderDrawColor(renderer, 24, 27, 31, 245);
    SDL_RenderFillRect(renderer, &layout.panelBounds);
    SDL_SetRenderDrawColor(renderer, 164, 139, 87, 255);
    SDL_RenderRect(renderer, &layout.panelBounds);
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderDebugText(
        renderer,
        layout.contentBounds.x,
        layout.panelBounds.y + headingYOffset,
        title);
}

void Graphics::DrawSkills(const Player &player)
{
    struct SkillDisplayEntry
    {
        SkillType type;
        const char *name;
    };

    static constexpr std::array<SkillDisplayEntry, 5>
        displayedSkills{{
            {SkillType::ATTACK, "ATTACK"},
            {SkillType::DEFENCE, "DEFENCE"},
            {SkillType::WOODCUTTING, "WOODCUTTING"},
            {SkillType::MINING, "MINING"},
            {SkillType::SMITHING, "SMITHING"},
        }};

    const SidePanelLayout layout = GetSidePanelLayout();
    const float panelX = layout.panelBounds.x;
    const float panelY = layout.panelBounds.y;
    DrawSidePanelFrame("SKILLS");

    std::string healthText =
        "HEALTH " +
        std::to_string(
            player.GetCurrentHealth()) +
        " / " +
        std::to_string(
            player.GetMaximumHealth());

    SDL_RenderDebugText(
        renderer,
        panelX + 10.0f,
        panelY + 30.0f,
        healthText.c_str());

    float skillX =
        panelX + 14.0f;

    float skillY =
        panelY + 74.0f;

    constexpr float skillRowHeight =
        48.0f;

    for (const SkillDisplayEntry &entry :
         displayedSkills)
    {
        const Skill &skill =
            player.GetSkills()
                .GetSkill(entry.type);

        SDL_RenderDebugText(
            renderer,
            skillX,
            skillY,
            entry.name);

        std::string detailsText =
            "Level " +
            std::to_string(
                skill.GetLevel()) +
            "    " +
            std::to_string(
                skill.GetXP()) +
            " XP";

        SDL_RenderDebugText(
            renderer,
            skillX,
            skillY + 20.0f,
            detailsText.c_str());

        skillY += skillRowHeight;
    }
}
void Graphics::DrawPlaceholderPanel(
    const char *title)
{
    const SidePanelLayout layout = GetSidePanelLayout();
    const float panelX = layout.panelBounds.x;
    const float panelY = layout.panelBounds.y;
    DrawSidePanelFrame(title);

    SDL_RenderDebugText(
        renderer,
        panelX + 45.0f,
        panelY + 160.0f,
        "COMING SOON");
}

void Graphics::DrawSettings()
{
    const SidePanelLayout layout = GetSidePanelLayout();
    DrawSidePanelFrame("SETTINGS");

    SDL_SetRenderDrawColor(renderer, 190, 195, 195, 255);
    SDL_RenderDebugText(renderer, layout.contentBounds.x,
                        layout.contentBounds.y + 28.0f,
                        "DISPLAY & HELP");

    const std::array<SDL_FRect, 3> buttons{{
        {layout.contentBounds.x, layout.contentBounds.y + 54.0f,
         layout.contentBounds.w, 38.0f},
        {layout.contentBounds.x, layout.contentBounds.y + 104.0f,
         layout.contentBounds.w, 38.0f},
        {layout.contentBounds.x, layout.contentBounds.y + 154.0f,
         layout.contentBounds.w, 38.0f}}};
    for (const SDL_FRect &button : buttons)
    {
        SDL_SetRenderDrawColor(renderer, 42, 47, 52, 255);
        SDL_RenderFillRect(renderer, &button);
        SDL_SetRenderDrawColor(renderer, 164, 139, 87, 255);
        SDL_RenderRect(renderer, &button);
    }

    SDL_SetRenderDrawColor(renderer, 235, 225, 202, 255);
    const std::string gridLabel = std::string("WORLD GRID: ") +
        (showGrid ? "ON" : "OFF");
    SDL_RenderDebugText(renderer, buttons[0].x + 12.0f,
                        buttons[0].y + 15.0f, gridLabel.c_str());
    SDL_RenderDebugText(renderer, buttons[1].x + 12.0f,
                        buttons[1].y + 15.0f, "OPEN GAME GUIDE");
    SDL_RenderDebugText(renderer, buttons[2].x + 12.0f,
                        buttons[2].y + 15.0f, "SKIP TUTORIAL");

    SDL_SetRenderDrawColor(renderer, 190, 195, 195, 255);
    SDL_RenderDebugText(renderer, layout.contentBounds.x,
                        layout.contentBounds.y + 214.0f, "CONTROLS");
    SDL_RenderDebugText(renderer, layout.contentBounds.x,
                        layout.contentBounds.y + 239.0f,
                        "Left click  Move / interact");
    SDL_RenderDebugText(renderer, layout.contentBounds.x,
                        layout.contentBounds.y + 261.0f,
                        "Right click Split item stacks");
    SDL_RenderDebugText(renderer, layout.contentBounds.x,
                        layout.contentBounds.y + 283.0f,
                        "Shift       Quick transfer");
    SDL_RenderDebugText(renderer, layout.contentBounds.x,
                        layout.contentBounds.y + 305.0f,
                        "H           Toggle guide");
    SDL_RenderDebugText(renderer, layout.contentBounds.x,
                        layout.contentBounds.y + 327.0f,
                        "Esc         Close window");
}
void Graphics::DrawSidePanelTabs()
{
    const SidePanelLayout layout = GetSidePanelLayout();

    const std::array<SidePanelTab, 7> tabs{
        SidePanelTab::SKILLS,
        SidePanelTab::QUESTS,
        SidePanelTab::INVENTORY,
        SidePanelTab::EQUIPMENT,
        SidePanelTab::PRAYER,
        SidePanelTab::MAGIC,
        SidePanelTab::SETTINGS};

    const std::array<const char *, 7> labels{
        "SKILL",
        "QUEST",
        "BAG",
        "GEAR",
        "FAITH",
        "MAGIC",
        "SET"};

    for (std::size_t tabIndex = 0;
         tabIndex < tabs.size();
         tabIndex++)
    {
        const SDL_FRect &tabRectangle = layout.tabBounds[tabIndex];

        if (selectedTab == tabs[tabIndex])
        {
            SDL_SetRenderDrawColor(
                renderer,
                146,
                104,
                48,
                255);
        }
        else
        {
            SDL_SetRenderDrawColor(
                renderer,
                35,
                39,
                44,
                255);
        }

        SDL_RenderFillRect(
            renderer,
            &tabRectangle);

        SDL_SetRenderDrawColor(
            renderer,
            164,
            139,
            87,
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
            tabRectangle.x + 3.0f,
            tabRectangle.y + 11.0f,
            labels[tabIndex]);
    }
}

void Graphics::DrawWorldHud(const Player &player)
{
    const float healthRatio = CalculateHealthRatio(
        player.GetCurrentHealth(), player.GetMaximumHealth());
    const SDL_FRect panel{18.0f, 18.0f, 245.0f, 60.0f};
    SDL_SetRenderDrawColor(renderer, 20, 24, 28, 225);
    SDL_RenderFillRect(renderer, &panel);
    SDL_SetRenderDrawColor(renderer, 164, 139, 87, 255);
    SDL_RenderRect(renderer, &panel);

    SDL_SetRenderDrawColor(renderer, 235, 225, 202, 255);
    SDL_RenderDebugText(renderer, 32.0f, 29.0f, "ADVENTURER");
    const SDL_FRect barBackground{32.0f, 50.0f, 180.0f, 12.0f};
    SDL_SetRenderDrawColor(renderer, 45, 31, 34, 255);
    SDL_RenderFillRect(renderer, &barBackground);
    SDL_FRect barFill = barBackground;
    barFill.w *= healthRatio;
    SDL_SetRenderDrawColor(renderer,
        healthRatio < 0.30f ? 220 : 174,
        healthRatio < 0.30f ? 55 : 57,
        healthRatio < 0.30f ? 45 : 63, 255);
    SDL_RenderFillRect(renderer, &barFill);
    SDL_SetRenderDrawColor(renderer, 235, 225, 202, 255);
    const std::string hp = std::to_string(player.GetCurrentHealth()) + " / " +
        std::to_string(player.GetMaximumHealth()) + " HP";
    SDL_RenderDebugText(renderer, 217.0f, 52.0f, hp.c_str());

    if (!player.GetTutorialState().IsCompleted())
    {
        const std::array<const char *, 10> objectives{
            "Talk to the guide at the crossroads",
            "Gather a log from a tree",
            "Make wooden planks at the crafting table",
            "Mine both copper and tin ore",
            "Smelt a bronze bar at the furnace",
            "Craft a bronze sword",
            "Equip a weapon from your bag",
            "Defeat a training enemy",
            "Take loot and deposit an item in the bank",
            "Return to the guide and finish Gathering Basics"};
        const int step = static_cast<int>(player.GetTutorialState().GetStep());
        const SDL_FRect objectivePanel{18.0f, 86.0f, 430.0f, 48.0f};
        SDL_SetRenderDrawColor(renderer, 20, 24, 28, 220);
        SDL_RenderFillRect(renderer, &objectivePanel);
        SDL_SetRenderDrawColor(renderer, 190, 151, 78, 255);
        SDL_RenderRect(renderer, &objectivePanel);
        SDL_SetRenderDrawColor(renderer, 250, 226, 170, 255);
        const std::string heading = "NEXT STEP " + std::to_string(step + 1) + "/10";
        SDL_RenderDebugText(renderer, 30.0f, 98.0f, heading.c_str());
        SDL_SetRenderDrawColor(renderer, 235, 225, 202, 255);
        SDL_RenderDebugText(renderer, 30.0f, 118.0f, objectives[step]);
    }

    SDL_SetRenderDrawColor(renderer, 235, 225, 202, 220);
    SDL_RenderDebugText(renderer, 18.0f, WindowHeight - 20.0f,
                        "Click to move/interact   H: Guide   Esc: Close");
}

void Graphics::DrawGameGuide(const Player &player)
{
    (void)player;
    if (!gameGuideVisible) return;

    const SDL_FRect shade{0.0f, 0.0f,
        static_cast<float>(WindowWidth), static_cast<float>(WindowHeight)};
    SDL_SetRenderDrawColor(renderer, 8, 10, 12, 190);
    SDL_RenderFillRect(renderer, &shade);

    const SDL_FRect panel{310.0f, 126.0f, 660.0f, 458.0f};
    SDL_SetRenderDrawColor(renderer, 25, 29, 32, 252);
    SDL_RenderFillRect(renderer, &panel);
    SDL_SetRenderDrawColor(renderer, 190, 151, 78, 255);
    SDL_RenderRect(renderer, &panel);

    SDL_SetRenderDrawColor(renderer, 250, 226, 170, 255);
    SDL_RenderDebugText(renderer, 350.0f, 160.0f,
                        "WELCOME TO EMBERVALE");
    SDL_SetRenderDrawColor(renderer, 225, 225, 215, 255);
    SDL_RenderDebugText(renderer, 350.0f, 198.0f,
                        "Build your strength, craft better gear, and tame the wilds.");

    SDL_SetRenderDrawColor(renderer, 115, 185, 235, 255);
    SDL_RenderDebugText(renderer, 350.0f, 242.0f, "1  MOVE & EXPLORE");
    SDL_SetRenderDrawColor(renderer, 220, 220, 210, 255);
    SDL_RenderDebugText(renderer, 350.0f, 264.0f,
                        "Click a ground tile. Your destination glows gold.");

    SDL_SetRenderDrawColor(renderer, 224, 143, 82, 255);
    SDL_RenderDebugText(renderer, 350.0f, 306.0f, "2  GATHER & CRAFT");
    SDL_SetRenderDrawColor(renderer, 220, 220, 210, 255);
    SDL_RenderDebugText(renderer, 350.0f, 328.0f,
                        "Click trees, rocks, the furnace, or the crafting table.");

    SDL_SetRenderDrawColor(renderer, 216, 85, 85, 255);
    SDL_RenderDebugText(renderer, 350.0f, 370.0f, "3  FIGHT SMART");
    SDL_SetRenderDrawColor(renderer, 220, 220, 210, 255);
    SDL_RenderDebugText(renderer, 350.0f, 392.0f,
                        "Click a creature to engage. Watch your health and gear.");

    SDL_SetRenderDrawColor(renderer, 198, 174, 102, 255);
    SDL_RenderDebugText(renderer, 350.0f, 434.0f, "YOUR FIRST GOAL");
    SDL_SetRenderDrawColor(renderer, 240, 232, 206, 255);
    SDL_RenderDebugText(renderer, 350.0f, 456.0f,
                        "Talk to the nearby villager and begin your first quest.");

    const SDL_FRect button{470.0f, 514.0f, 340.0f, 42.0f};
    SDL_SetRenderDrawColor(renderer, 132, 92, 41, 255);
    SDL_RenderFillRect(renderer, &button);
    SDL_SetRenderDrawColor(renderer, 230, 192, 112, 255);
    SDL_RenderRect(renderer, &button);
    SDL_SetRenderDrawColor(renderer, 255, 244, 214, 255);
    SDL_RenderDebugText(renderer, 547.0f, 531.0f,
                        "CLICK ANYWHERE TO BEGIN");
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

void Graphics::DrawSwordIcon(
    ItemType itemType,
    float x,
    float y,
    float size)
{
    int bladeRed = 0;
    int bladeGreen = 0;
    int bladeBlue = 0;

    bool isDeveloperSword = false;

    switch (itemType)
    {
    case ItemType::BRONZE_SWORD:
        bladeRed = 185;
        bladeGreen = 120;
        bladeBlue = 70;
        break;

    case ItemType::DEVELOPER_GODSWORD:
        bladeRed = 240;
        bladeGreen = 210;
        bladeBlue = 75;
        isDeveloperSword = true;
        break;

    default:
        return;
    }

    SDL_FRect guardRectangle{
        x + size * 0.22f,
        y + size * 0.52f,
        size * 0.56f,
        size * 0.08f};

    SDL_SetRenderDrawColor(
        renderer,
        100,
        75,
        35,
        255);

    SDL_RenderFillRect(
        renderer,
        &guardRectangle);

    SDL_FRect gripRectangle{
        x + size * 0.43f,
        y + size * 0.60f,
        size * 0.14f,
        size * 0.22f};

    SDL_SetRenderDrawColor(
        renderer,
        65,
        45,
        25,
        255);

    SDL_RenderFillRect(
        renderer,
        &gripRectangle);

    SDL_FRect bladeRectangle{
        x + size * 0.45f,
        y + size * 0.10f,
        size * 0.10f,
        size * 0.42f};

    SDL_SetRenderDrawColor(
        renderer,
        bladeRed,
        bladeGreen,
        bladeBlue,
        255);

    SDL_RenderFillRect(
        renderer,
        &bladeRectangle);

    SDL_SetRenderDrawColor(
        renderer,
        std::min(bladeRed + 20, 255),
        std::min(bladeGreen + 20, 255),
        std::min(bladeBlue + 20, 255),
        255);

    SDL_FRect edgeHighlight{
        x + size * 0.48f,
        y + size * 0.12f,
        size * 0.04f,
        size * 0.36f};

    SDL_RenderFillRect(
        renderer,
        &edgeHighlight);

    if (isDeveloperSword)
    {
        SDL_SetRenderDrawColor(
            renderer,
            255,
            255,
            255,
            255);

        SDL_RenderDebugText(
            renderer,
            x + 1.0f,
            y + size - 10.0f,
            "DEV");
    }
}

void Graphics::DrawPickaxeIcon(
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
    case ItemType::BRONZE_PICKAXE:
        headRed = 170;
        headGreen = 110;
        headBlue = 60;
        break;

    case ItemType::IRON_PICKAXE:
        headRed = 145;
        headGreen = 150;
        headBlue = 155;
        break;

    case ItemType::STEEL_PICKAXE:
        headRed = 195;
        headGreen = 205;
        headBlue = 215;
        break;

    default:
        return;
    }

    SDL_FRect handleRectangle{
        x + size * 0.46f,
        y + size * 0.24f,
        size * 0.12f,
        size * 0.60f};

    SDL_SetRenderDrawColor(
        renderer,
        130,
        80,
        35,
        255);

    SDL_RenderFillRect(
        renderer,
        &handleRectangle);

    SDL_FRect pickaxeHeadRectangle{
        x + size * 0.16f,
        y + size * 0.16f,
        size * 0.68f,
        size * 0.18f};

    SDL_SetRenderDrawColor(
        renderer,
        headRed,
        headGreen,
        headBlue,
        255);

    SDL_RenderFillRect(
        renderer,
        &pickaxeHeadRectangle);
}

void Graphics::DrawBarIcon(
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
    case ItemType::BRONZE_BAR:
        red = 175;
        green = 105;
        blue = 55;
        break;

    case ItemType::IRON_BAR:
        red = 125;
        green = 125;
        blue = 130;
        break;

    case ItemType::STEEL_BAR:
        red = 185;
        green = 195;
        blue = 205;
        break;

    default:
        return;
    }

    SDL_FRect bar{
        x + size * 0.16f,
        y + size * 0.34f,
        size * 0.68f,
        size * 0.34f};

    SDL_SetRenderDrawColor(
        renderer,
        red,
        green,
        blue,
        255);

    SDL_RenderFillRect(
        renderer,
        &bar);

    SDL_SetRenderDrawColor(
        renderer,
        std::min(red + 25, 255),
        std::min(green + 25, 255),
        std::min(blue + 25, 255),
        255);

    SDL_FRect highlight{
        x + size * 0.22f,
        y + size * 0.38f,
        size * 0.48f,
        size * 0.08f};

    SDL_RenderFillRect(
        renderer,
        &highlight);
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

void Graphics::DrawOreIcon(
    ItemType itemType,
    float x,
    float y,
    float size)
{
    int oreRed = 0;
    int oreGreen = 0;
    int oreBlue = 0;

    switch (itemType)
    {
    case ItemType::COPPER_ORE:
        oreRed = 190;
        oreGreen = 105;
        oreBlue = 55;
        break;

    case ItemType::TIN_ORE:
        oreRed = 190;
        oreGreen = 190;
        oreBlue = 200;
        break;

    case ItemType::IRON_ORE:
        oreRed = 135;
        oreGreen = 85;
        oreBlue = 65;
        break;

    case ItemType::COAL:
        oreRed = 35;
        oreGreen = 35;
        oreBlue = 40;
        break;

    default:
        return;
    }

    SDL_SetRenderDrawColor(
        renderer,
        105,
        105,
        115,
        255);

    SDL_FRect rock{
        x + size * 0.18f,
        y + size * 0.28f,
        size * 0.64f,
        size * 0.50f};

    SDL_RenderFillRect(
        renderer,
        &rock);

    SDL_SetRenderDrawColor(
        renderer,
        oreRed,
        oreGreen,
        oreBlue,
        255);

    SDL_FRect oreDeposit{
        x + size * 0.34f,
        y + size * 0.40f,
        size * 0.22f,
        size * 0.18f};

    SDL_RenderFillRect(
        renderer,
        &oreDeposit);
}

void Graphics::DrawFoodIcon(
    ItemType itemType,
    float x,
    float y,
    float size)
{
    if (itemType !=
        ItemType::COOKED_MEAT)
    {
        return;
    }

    SDL_SetRenderDrawColor(
        renderer,
        145,
        70,
        45,
        255);

    SDL_FRect meatRectangle{
        x + size * 0.20f,
        y + size * 0.30f,
        size * 0.60f,
        size * 0.38f};

    SDL_RenderFillRect(
        renderer,
        &meatRectangle);

    SDL_SetRenderDrawColor(
        renderer,
        205,
        125,
        90,
        255);

    SDL_FRect highlightRectangle{
        x + size * 0.26f,
        y + size * 0.36f,
        size * 0.32f,
        size * 0.12f};

    SDL_RenderFillRect(
        renderer,
        &highlightRectangle);
}

void Graphics::DrawInventory(
    const Inventory &inventory,
    const Equipment *equipment,
    const Player *player)
{
    constexpr int columns = 7;
    constexpr int rows = 4;

    constexpr float slotSize = 40.0f;
    constexpr float slotGap = 4.0f;
    constexpr float padding = 13.0f;
    constexpr float headerHeight = 24.0f;

    static_assert(
        padding * 2.0f + columns * slotSize + (columns - 1) * slotGap == 330.0f);
    const SidePanelLayout layout = GetSidePanelLayout();
    const float panelX = layout.panelBounds.x;
    const float panelY = layout.panelBounds.y;
    DrawSidePanelFrame("INVENTORY", 8.0f);

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

        if (slot.GetItemType() == ItemType::WOODEN_PLANKS)
        {
            DrawCraftingItemIcon(
                slot.GetItemType(),
                SDL_FRect{slotX, slotY, slotSize, slotSize});
        }

        DrawOreIcon(
            slot.GetItemType(),
            slotX,
            slotY,
            slotSize);

        DrawAxeIcon(
            slot.GetItemType(),
            slotX,
            slotY,
            slotSize);

        DrawSwordIcon(
            slot.GetItemType(),
            slotX,
            slotY,
            slotSize);

        DrawPickaxeIcon(
            slot.GetItemType(),
            slotX,
            slotY,
            slotSize);

        DrawBarIcon(
            slot.GetItemType(),
            slotX,
            slotY,
            slotSize);

        DrawFoodIcon(
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

    int hoveredSlot = -1;
    for (int slotIndex = 0; slotIndex < Inventory::SlotCount; ++slotIndex)
    {
        const int column = slotIndex % columns;
        const int row = slotIndex / columns;
        const SDL_FRect slot{
            panelX + padding + column * (slotSize + slotGap),
            panelY + padding + headerHeight + row * (slotSize + slotGap),
            slotSize, slotSize};
        if (IsPointInsideRectangle(mouseX, mouseY, slot))
        {
            hoveredSlot = slotIndex;
            break;
        }
    }

    const float helpY = panelY + 238.0f;
    SDL_SetRenderDrawColor(renderer, 164, 139, 87, 255);
    SDL_RenderLine(renderer, panelX + 13.0f, helpY - 12.0f,
                   panelX + 317.0f, helpY - 12.0f);
    SDL_SetRenderDrawColor(renderer, 235, 225, 202, 255);
    if (hoveredSlot >= 0 && !inventory.GetSlots()[hoveredSlot].IsEmpty())
    {
        const InventorySlot &slot = inventory.GetSlots()[hoveredSlot];
        const ItemDefinition &definition = ItemDatabase::Get(slot.GetItemType());
        const std::string itemName = definition.GetName();
        SDL_RenderDebugText(renderer, panelX + 13.0f, helpY, itemName.c_str());
        SDL_SetRenderDrawColor(renderer, 185, 190, 190, 255);
        if (definition.IsFood())
        {
            const std::string healing = "Heals " + std::to_string(
                definition.GetFoodDefinition()->healAmount) + " HP";
            SDL_RenderDebugText(renderer, panelX + 13.0f, helpY + 22.0f,
                healing.c_str());
        }
        else if (definition.IsEquippable() && equipment != nullptr)
        {
            StatBlock previous;
            const ItemType equipped = equipment->GetEquippedItem(
                definition.GetEquipmentSlot());
            if (equipped != ItemType::NONE)
                previous = ItemDatabase::Get(equipped).GetEquipmentStatBonuses();
            const StatBlock &next = definition.GetEquipmentStatBonuses();
            auto signedValue = [](int value)
            {
                return std::string(value >= 0 ? "+" : "") +
                    std::to_string(value);
            };
            const std::string comparison =
                "ATK " + signedValue(next.Get(StatType::ATTACK_ACCURACY) -
                    previous.Get(StatType::ATTACK_ACCURACY)) +
                "  STR " + signedValue(next.Get(StatType::MELEE_STRENGTH) -
                    previous.Get(StatType::MELEE_STRENGTH)) +
                "  DEF " + signedValue(next.Get(StatType::DEFENCE) -
                    previous.Get(StatType::DEFENCE));
            SDL_RenderDebugText(renderer, panelX + 13.0f, helpY + 22.0f,
                comparison.c_str());
            if (definition.HasSkillRequirement() && player != nullptr)
            {
                const int level = player->GetSkills().GetSkill(
                    definition.GetRequiredSkill()).GetLevel();
                const std::string requirement = "Requires level " +
                    std::to_string(definition.GetRequiredSkillLevel()) +
                    (level >= definition.GetRequiredSkillLevel()
                        ? " - READY" : " - NOT MET");
                SDL_RenderDebugText(renderer, panelX + 13.0f, helpY + 42.0f,
                    requirement.c_str());
            }
        }
        else
            SDL_RenderDebugText(renderer, panelX + 13.0f, helpY + 22.0f,
                                "Left: use/equip   Right: drop 1");
    }
    else
    {
        SDL_RenderDebugText(renderer, panelX + 13.0f, helpY,
                            "Hover an item for details");
        SDL_SetRenderDrawColor(renderer, 185, 190, 190, 255);
        SDL_RenderDebugText(renderer, panelX + 13.0f, helpY + 22.0f,
                            "28 slots - items stack automatically");
    }
}

void Graphics::DrawEquipment(
    const Equipment &equipment,
    const Inventory *inventory,
    const Player *player)
{
    const SidePanelLayout layout = GetSidePanelLayout();
    const float panelX = layout.panelBounds.x;
    const float panelY = layout.panelBounds.y;
    DrawSidePanelFrame("EQUIPMENT");

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

        DrawSwordIcon(
            equippedItem,
            slot.x,
            slot.y,
            slotSize);

        DrawPickaxeIcon(
            equippedItem,
            slot.x,
            slot.y,
            slotSize);
    }

    const StatBlock totals = equipment.GetTotalStatBonuses();
    const float summaryX = panelX + 210.0f;
    SDL_SetRenderDrawColor(renderer, 235, 215, 165, 255);
    SDL_RenderDebugText(renderer, summaryX, panelY + 50.0f, "PREPARATION");
    SDL_SetRenderDrawColor(renderer, 220, 220, 215, 255);
    const std::string attack = "Attack  " + std::to_string(
        totals.Get(StatType::ATTACK_ACCURACY));
    const std::string strength = "Strength " + std::to_string(
        totals.Get(StatType::MELEE_STRENGTH));
    const std::string defence = "Defence " + std::to_string(
        totals.Get(StatType::DEFENCE));
    SDL_RenderDebugText(renderer, summaryX, panelY + 78.0f, attack.c_str());
    SDL_RenderDebugText(renderer, summaryX, panelY + 98.0f, strength.c_str());
    SDL_RenderDebugText(renderer, summaryX, panelY + 118.0f, defence.c_str());
    const bool hasWeapon = equipment.GetEquippedItem(
        EquipmentSlotType::WEAPON) != ItemType::NONE;
    int foodCount = 0;
    if (inventory != nullptr)
        for (const InventorySlot &slot : inventory->GetSlots())
            if (!slot.IsEmpty() && ItemDatabase::Get(slot.GetItemType()).IsFood())
                foodCount += slot.GetAmount();
    SDL_SetRenderDrawColor(renderer,
        hasWeapon ? 120 : 225, hasWeapon ? 210 : 105, 90, 255);
    SDL_RenderDebugText(renderer, summaryX, panelY + 154.0f,
        hasWeapon ? "Weapon: READY" : "Weapon: MISSING");
    SDL_SetRenderDrawColor(renderer,
        foodCount > 0 ? 120 : 225, foodCount > 0 ? 210 : 150, 90, 255);
    const std::string food = "Food: " + std::to_string(foodCount);
    SDL_RenderDebugText(renderer, summaryX, panelY + 176.0f, food.c_str());

    const std::array<const char *, 3> styleNames{
        "ACCURATE", "AGGRESSIVE", "DEFENSIVE"};
    const std::array<const char *, 3> styleEffects{
        "+3 accuracy", "+10 strength", "+3 defence"};
    const CombatStyle selectedStyle = player == nullptr
        ? CombatStyle::ACCURATE
        : player->GetCombatStyle();
    SDL_SetRenderDrawColor(renderer, 235, 215, 165, 255);
    SDL_RenderDebugText(renderer, panelX + 12.0f, panelY + 240.0f,
        "COMBAT STYLE");
    for (int index = 0; index < 3; ++index)
    {
        const bool selected = static_cast<int>(selectedStyle) == index;
        const SDL_FRect button{
            panelX + 12.0f + index * 108.0f,
            panelY + 266.0f,
            100.0f,
            38.0f};
        SDL_SetRenderDrawColor(renderer,
            selected ? 118 : 48,
            selected ? 91 : 52,
            selected ? 42 : 56,
            255);
        SDL_RenderFillRect(renderer, &button);
        SDL_SetRenderDrawColor(renderer,
            selected ? 245 : 145,
            selected ? 196 : 145,
            selected ? 80 : 145,
            255);
        SDL_RenderRect(renderer, &button);
        SDL_RenderDebugText(renderer, button.x + 7.0f, button.y + 14.0f,
            styleNames[index]);
        SDL_SetRenderDrawColor(renderer, 190, 195, 195, 255);
        SDL_RenderDebugText(renderer, button.x + 4.0f, button.y + 48.0f,
            styleEffects[index]);
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

bool Graphics::ConsumeInventoryDropRequest(int &slotIndex, int &amount)
{
    if (!inventoryDropPending) return false;
    slotIndex = dropInventorySlotIndex;
    amount = dropInventoryAmount;
    inventoryDropPending = false;
    dropInventorySlotIndex = -1;
    dropInventoryAmount = 0;
    return true;
}

bool Graphics::ConsumeTutorialSkipRequest()
{
    const bool pending = tutorialSkipPending;
    tutorialSkipPending = false;
    return pending;
}

std::optional<CombatStyle> Graphics::ConsumeCombatStyleRequest()
{
    std::optional<CombatStyle> request = combatStyleRequest;
    combatStyleRequest.reset();
    return request;
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

void Graphics::DrawDialoguePanel()
{
    const NpcTalkEvent *event = dialoguePresentationState.GetEvent();
    if (event == nullptr) return;

    const DialoguePanelLayout layout = GetDialoguePanelLayout();
    const SDL_FRect panel = layout.panelBounds;
    SDL_SetRenderDrawColor(renderer, 24, 22, 20, 248);
    SDL_RenderFillRect(renderer, &panel);
    SDL_SetRenderDrawColor(renderer, 205, 180, 120, 255);
    SDL_RenderRect(renderer, &panel);

    SDL_SetRenderDrawColor(renderer, 255, 244, 210, 255);
    SDL_RenderDebugText(renderer, 240.0f, 468.0f,
                        dialoguePresentationState.GetNpcName().c_str());

    std::string firstLine = event->text.substr(0, 92);
    SDL_RenderDebugText(renderer, 240.0f, 510.0f, firstLine.c_str());
    if (event->text.size() > firstLine.size())
    {
        std::string secondLine = event->text.substr(firstLine.size(), 92);
        SDL_RenderDebugText(renderer, 240.0f, 528.0f, secondLine.c_str());
    }

    for (const DialogueControlLayout &control : layout.controls)
    {
        std::string label;
        switch (control.type)
        {
        case DialogueControlType::GENERAL_CLOSE:
            label = "CLOSE";
            break;
        case DialogueControlType::AUTHORED_CHOICE:
            label = std::to_string(control.index + 1) + ". " +
                event->choices[control.index].text;
            break;
        case DialogueControlType::TRADE:
            label = "Trade";
            break;
        case DialogueControlType::QUEST_ACTION:
        {
            const QuestDialogueAction &action =
                event->questActions[control.index];
            const QuestDefinition *definition =
                QuestDefinitionDatabase::TryGet(action.questId);
            if (definition == nullptr) continue;
            label = std::string(
                action.kind == QuestDialogueActionKind::ACCEPT
                    ? "Accept " : "Complete ") + definition->name;
            break;
        }
        case DialogueControlType::PRIMARY:
            label = event->isTerminal ? "CLOSE" : "CONTINUE";
            break;
        }
        const SDL_FRect button = control.bounds;
        SDL_SetRenderDrawColor(renderer, 55, 50, 45, 255);
        SDL_RenderFillRect(renderer, &button);
        SDL_SetRenderDrawColor(renderer, 190, 170, 130, 255);
        SDL_RenderRect(renderer, &button);
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        const float labelX = button.x +
            (control.type == DialogueControlType::GENERAL_CLOSE ? 20.0f : 12.0f);
        SDL_RenderDebugText(renderer, labelX,
            button.y + (button.h - 8.0f) * 0.5f, label.c_str());
    }
}

void Graphics::DrawShopPanel()
{
    const auto* e=shopPresentationState.GetEvent(); if(!e)return;
    const SDL_FRect panel{120,70,1040,620}; SDL_SetRenderDrawColor(renderer,24,22,20,248); SDL_RenderFillRect(renderer,&panel); SDL_SetRenderDrawColor(renderer,205,180,120,255); SDL_RenderRect(renderer,&panel);
    SDL_SetRenderDrawColor(renderer,255,244,210,255); SDL_RenderDebugText(renderer,160,95,e->shopName.c_str()); SDL_RenderDebugText(renderer,160,115,("Currency: "+std::to_string(displayedCurrency)+"    BUY (left)        SELL (right)     Quantity: 1").c_str());
    if (shopPresentationState.HasRejection()) SDL_RenderDebugText(renderer,160,135,"Transaction rejected by server");
    for(std::size_t i=0;i<e->entries.size();++i){ const auto&r=GetShopRowRectangle(i); const auto& row=e->entries[i]; SDL_SetRenderDrawColor(renderer,55,50,45,255); SDL_RenderFillRect(renderer,&r); SDL_SetRenderDrawColor(renderer,190,170,130,255); SDL_RenderRect(renderer,&r); SDL_SetRenderDrawColor(renderer,255,255,255,255); SDL_RenderDebugText(renderer,r.x+16,r.y+13,ItemDatabase::Get(row.itemType).GetName().c_str()); auto drawButton=[&](SDL_FRect b,const char* label,int price){SDL_SetRenderDrawColor(renderer,70,90,65,255); SDL_RenderFillRect(renderer,&b); SDL_SetRenderDrawColor(renderer,220,200,160,255); SDL_RenderRect(renderer,&b); SDL_SetRenderDrawColor(renderer,255,255,255,255); std::string caption=std::string(label)+" "+std::to_string(price); SDL_RenderDebugText(renderer,b.x+18,b.y+11,caption.c_str());}; if(row.buyPrice) drawButton(GetShopBuyButtonRectangle(i),"BUY",*row.buyPrice); if(row.sellPrice) drawButton(GetShopSellButtonRectangle(i),"SELL",*row.sellPrice); }
    const auto b=GetShopCloseButtonRectangle(); SDL_SetRenderDrawColor(renderer,70,55,45,255); SDL_RenderFillRect(renderer,&b); SDL_SetRenderDrawColor(renderer,220,200,160,255); SDL_RenderRect(renderer,&b); SDL_SetRenderDrawColor(renderer,255,255,255,255); SDL_RenderDebugText(renderer,b.x+20,b.y+9,"CLOSE");
}

std::vector<Graphics::QuestRowLayout> Graphics::BuildQuestRowLayout(
    const std::vector<QuestPresentation> &quests) const
{
    const SDL_FRect content = GetSidePanelLayout().contentBounds;
    constexpr float headingSpace = 32.0f;
    constexpr float rowHeight = 80.0f;
    constexpr float rowGap = 8.0f;
    std::vector<QuestRowLayout> rows;
    rows.reserve(quests.size());
    for (std::size_t index = 0; index < quests.size(); ++index)
    {
        const float originY = content.y + headingSpace +
            static_cast<float>(index) * (rowHeight + rowGap);
        const float clippedTop = std::clamp(
            originY, content.y, content.y + content.h);
        const float clippedBottom = std::min(
            originY + rowHeight, content.y + content.h);
        const float clippedHeight = std::max(0.0f, clippedBottom - clippedTop);
        rows.push_back({quests[index],
            {content.x, clippedTop, content.w, clippedHeight},
            {content.x, originY},
            {content.x, originY + 28.0f},
            {content.x, originY + 52.0f},
            originY,
            clippedHeight < rowHeight});
    }
    return rows;
}

std::string Graphics::GetQuestStateLabel(QuestState state)
{
    switch (state)
    {
    case QuestState::UNAVAILABLE: return "UNAVAILABLE";
    case QuestState::AVAILABLE: return "AVAILABLE";
    case QuestState::ACTIVE: return "ACTIVE";
    case QuestState::READY_TO_COMPLETE: return "READY TO COMPLETE";
    case QuestState::COMPLETED: return "COMPLETED";
    }
    return "UNAVAILABLE";
}

std::string Graphics::GetQuestProgressLabel(const QuestPresentation &quest)
{
    return quest.objective + ": " + std::to_string(quest.progress) + " / " +
        std::to_string(quest.required);
}

Graphics::QuestPanelView Graphics::BuildQuestPanelView(const Player &player) const
{
    const SidePanelLayout layout = GetSidePanelLayout();
    const SDL_FRect bounds = layout.panelBounds;
    const SDL_FRect content = layout.contentBounds;
    QuestPresentationState presentationState;
    presentationState.Synchronize(player.GetQuestJournal());
    const auto &quests = presentationState.GetQuests();
    const QuestPresentation empty{};
    const QuestPresentation &record = quests.empty() ? empty : quests.front();
    const auto rows = BuildQuestRowLayout(quests);
    const SDL_FPoint titlePosition = rows.empty()
        ? SDL_FPoint{content.x, content.y + 32.0f}
        : rows.front().titlePosition;
    const SDL_FPoint statePosition = rows.empty()
        ? SDL_FPoint{content.x, content.y + 60.0f}
        : rows.front().statePosition;
    const SDL_FPoint progressPosition = rows.empty()
        ? SDL_FPoint{content.x, content.y + 84.0f}
        : rows.front().progressPosition;

    QuestPanelView view{
        bounds,
        content,
        {content.x, content.y},
        titlePosition,
        statePosition,
        progressPosition,
        "QUESTS",
        record.title.empty() ? "Unknown quest" : record.title,
        "UNAVAILABLE",
        "",
        selectedTab == SidePanelTab::QUESTS};

    view.state = GetQuestStateLabel(record.state);
    view.progress = GetQuestProgressLabel(record);
    return view;
}

void Graphics::DrawQuestPanel(const Player &player)
{
    questPresentationState.Synchronize(player.GetQuestJournal());
    const QuestPanelView view = BuildQuestPanelView(player);
    DrawSidePanelFrame(view.heading.c_str());
    DrawQuestRows(questPresentationState.GetQuests());
}

void Graphics::DrawQuestRows(const std::vector<QuestPresentation> &quests)
{
    const auto rows = BuildQuestRowLayout(quests);
    const SDL_FRect contentBounds = GetSidePanelLayout().contentBounds;
    const SDL_Rect clipRectangle{
        static_cast<int>(contentBounds.x),
        static_cast<int>(contentBounds.y),
        static_cast<int>(contentBounds.w),
        static_cast<int>(contentBounds.h)};
    SDL_SetRenderClipRect(renderer, &clipRectangle);
    for (const QuestRowLayout &row : rows)
    {
        if (row.bounds.h <= 0.0f) continue;
        const QuestPresentation &quest = row.quest;
        SDL_SetRenderDrawColor(renderer, 255, 215, 100, 255);
        SDL_RenderDebugText(renderer, row.titlePosition.x, row.titlePosition.y,
            quest.title.c_str());
        const std::string state = GetQuestStateLabel(quest.state);
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        SDL_RenderDebugText(renderer, row.statePosition.x, row.statePosition.y,
            state.c_str());
        const std::string progress = GetQuestProgressLabel(quest);
        SDL_RenderDebugText(renderer, row.progressPosition.x,
            row.progressPosition.y,
            progress.c_str());
    }
    SDL_SetRenderClipRect(renderer, nullptr);
}

void Graphics::Render(
    Map &map,
    const std::vector<std::unique_ptr<Entity>> &entities,
    const std::vector<ResourceNode> &resources,
    const std::vector<CraftingStation> &stations,
    const std::vector<GroundItem> &groundItems,
    const std::map<int, MeleeCombatFeedback> &combatFeedbacks,
    int playerX,
    int playerY,
    const Inventory &inventory,
    const Equipment &equipment,
    const Player &player,
    const Action *activeAction,
    int selectedCombatTargetID)
{
    const Uint64 presentationTick = SDL_GetTicks();
    if (lastPresentationTick != 0)
    {
        presentationDeltaSeconds = std::clamp(
            static_cast<float>(presentationTick - lastPresentationTick) /
                1000.0f,
            1.0f / 240.0f,
            0.05f);
    }
    lastPresentationTick = presentationTick;
    UpdateCamera(playerX, playerY, map);
    UpdateProgressionFeedback(player, inventory);
    displayedCurrency = inventory.GetItemAmount(ItemType::COINS);
    displayedLogCount = inventory.GetItemAmount(ItemType::LOG);
    displayedPlankCount = inventory.GetItemAmount(ItemType::WOODEN_PLANKS);
    displayedInventorySlots = inventory.GetSlots();
    recipeActionActive =
        activeAction != nullptr &&
        activeAction->GetType() == ActionType::RECIPE;
    const bool worldHoverBlocked = stationMenuOpen || worldContextMenu.open ||
        shopPresentationState.IsOpen() ||
        dialoguePresentationState.IsOpen() || gameGuideVisible ||
        IsPointInsideRectangle(mouseX, mouseY, GetSidePanelRectangle());
    worldHoverTarget = worldHoverBlocked
        ? WorldHoverTarget{}
        : ResolveWorldHoverTarget(
            mouseX + cameraPixelX,
            mouseY + cameraPixelY,
            entities,
            resources,
            stations,
            groundItems);
    SDL_SetRenderDrawColor(
        renderer,
        20,
        20,
        20,
        255);

    SDL_RenderClear(renderer);

    const SDL_Rect worldViewport{
        -static_cast<int>(std::round(cameraPixelX)),
        -static_cast<int>(std::round(cameraPixelY)),
        map.GetWidth() * TileSize,
        map.GetHeight() * TileSize};
    SDL_SetRenderViewport(renderer, &worldViewport);

    DrawMap(map);
    if (showGrid) DrawGrid(map);
    DrawClickedTile();
    DrawResources(resources);
    DrawStations(stations);
    DrawGroundItems(groundItems);
    DrawMonsters(entities, selectedCombatTargetID, player.GetID());
    DrawNPCs(entities);
    DrawPlayer(playerX, playerY);
    DrawPlayerHealthBar(player);
    DrawMonsterHealthBars(entities);
    DrawMonsterCombatFeedback(entities, combatFeedbacks);
    DrawWorldHoverFeedback();

    SDL_SetRenderViewport(renderer, nullptr);
    SDL_SetRenderScale(renderer, 1.0f, 1.0f);
    SDL_SetRenderClipRect(renderer, nullptr);
    DrawWorldHud(player);
    DrawCombatTargetPanel(
        entities, selectedCombatTargetID, player.GetID());
    DrawProgressionFeedback();
    DrawMinimap(map, entities, resources, stations, playerX, playerY);
    DrawSidePanelTabs();

    switch (selectedTab)
    {
    case SidePanelTab::INVENTORY:
        DrawInventory(inventory, &equipment, &player);
        break;

    case SidePanelTab::EQUIPMENT:
        DrawEquipment(equipment, &inventory, &player);
        break;

    case SidePanelTab::SKILLS:
        DrawSkills(player);
        break;

    case SidePanelTab::QUESTS:
        DrawQuestPanel(player);
        break;

    case SidePanelTab::PRAYER:
        DrawPlaceholderPanel("PRAYER");
        break;

    case SidePanelTab::MAGIC:
        DrawPlaceholderPanel("MAGIC");
        break;

    case SidePanelTab::SETTINGS:
        DrawSettings();
        break;
    }

    DrawActionProgress(
        activeAction);

    DrawStationMenu();
    if (shopPresentationState.IsOpen()) DrawShopPanel(); else DrawDialoguePanel();

    DrawWorldContextMenu();

    DrawGameGuide(player);

    SDL_RenderPresent(renderer);
}
