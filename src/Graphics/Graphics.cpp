#include "Graphics.h"
#include <iostream>
#include "../World/Map.h"
#include "../Player/Player.h"
#include "../Entity/Monster/Monster.h"
#include "../NPC/NPC.h"
#include "../NPC/NpcDefinitionDatabase.h"
#include "../Item/ItemDatabase.h"
#include "../Quest/QuestDefinitionDatabase.h"
#include "../Combat/Combatant.h"
#include "../World/Object/Resource/ResourceDatabase.h"
#include "../World/Object/Resource/DepletedVisualType.h"
#include "../Entity/EntityType.h"
#include <string>
#include <array>
#include <algorithm>
#include <map>

namespace
{
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

    return true;
}
void Graphics::OpenStationMenu(
    StationType stationType)
{
    openStationType = stationType;
    stationMenuOpen =
        stationType != StationType::NONE;
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
    float mouseY)
{
    if (!stationMenuOpen ||
        openStationType !=
            StationType::FURNACE)
    {
        return;
    }

    constexpr float menuWidth =
        360.0f;

    constexpr float menuHeight =
        280.0f;

    const float menuX =
        (WindowWidth - menuWidth) /
        2.0f;

    const float menuY =
        (WindowHeight - menuHeight) /
        2.0f;

    SDL_FRect bronzeButton{
        menuX + 18.0f,
        menuY + 52.0f,
        menuWidth - 36.0f,
        42.0f};

    SDL_FRect ironButton{
        menuX + 18.0f,
        menuY + 104.0f,
        menuWidth - 36.0f,
        42.0f};

    SDL_FRect steelButton{
        menuX + 18.0f,
        menuY + 156.0f,
        menuWidth - 36.0f,
        42.0f};

    if (IsPointInsideRectangle(
            mouseX,
            mouseY,
            bronzeButton))
    {
        requestedRecipeType =
            RecipeType::BRONZE_BAR;

        recipeRequestPending = true;
        return;
    }

    if (IsPointInsideRectangle(
            mouseX,
            mouseY,
            ironButton))
    {
        requestedRecipeType =
            RecipeType::IRON_BAR;

        recipeRequestPending = true;
        return;
    }

    if (IsPointInsideRectangle(
            mouseX,
            mouseY,
            steelButton))
    {
        requestedRecipeType =
            RecipeType::STEEL_BAR;

        recipeRequestPending = true;
    }
}

void Graphics::DrawStationMenu()
{
    if (!stationMenuOpen)
    {
        return;
    }

    constexpr float menuWidth =
        360.0f;

    constexpr float menuHeight =
        280.0f;

    const float menuX =
        (WindowWidth - menuWidth) /
        2.0f;

    const float menuY =
        (WindowHeight - menuHeight) /
        2.0f;

    SDL_FRect menuRectangle{
        menuX,
        menuY,
        menuWidth,
        menuHeight};

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

    if (openStationType !=
        StationType::FURNACE)
    {
        return;
    }

    SDL_FRect bronzeButton{
        menuX + 18.0f,
        menuY + 52.0f,
        menuWidth - 36.0f,
        42.0f};

    SDL_FRect ironButton{
        menuX + 18.0f,
        menuY + 104.0f,
        menuWidth - 36.0f,
        42.0f};

    SDL_FRect steelButton{
        menuX + 18.0f,
        menuY + 156.0f,
        menuWidth - 36.0f,
        42.0f};

    SDL_SetRenderDrawColor(
        renderer,
        60,
        60,
        65,
        255);

    SDL_RenderFillRect(
        renderer,
        &bronzeButton);

    SDL_RenderFillRect(
        renderer,
        &ironButton);

    SDL_RenderFillRect(
        renderer,
        &steelButton);

    SDL_SetRenderDrawColor(
        renderer,
        175,
        175,
        180,
        255);

    SDL_RenderRect(
        renderer,
        &bronzeButton);

    SDL_RenderRect(
        renderer,
        &ironButton);

    SDL_RenderRect(
        renderer,
        &steelButton);

    SDL_SetRenderDrawColor(
        renderer,
        255,
        255,
        255,
        255);

    SDL_RenderDebugText(
        renderer,
        menuX + 18.0f,
        menuY + 18.0f,
        "FURNACE");

    SDL_RenderDebugText(
        renderer,
        bronzeButton.x + 12.0f,
        bronzeButton.y + 14.0f,
        "BRONZE BAR - 1 COPPER + 1 TIN");

    SDL_RenderDebugText(
        renderer,
        ironButton.x + 12.0f,
        ironButton.y + 14.0f,
        "IRON BAR - 1 IRON ORE");

    SDL_RenderDebugText(
        renderer,
        steelButton.x + 12.0f,
        steelButton.y + 14.0f,
        "STEEL BAR - 1 IRON + 2 COAL");

    SDL_RenderDebugText(
        renderer,
        menuX + 18.0f,
        menuY + 240.0f,
        "PRESS ESC TO CLOSE");
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
    float mouseY)
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

        if (event.type == SDL_EVENT_KEY_DOWN &&
            !event.key.repeat)
        {
            if (event.key.scancode ==
                SDL_SCANCODE_ESCAPE)
            {
                if (shopPresentationState.IsOpen())
                {
                    if (!pendingShopCommand)
                        pendingShopCommand = shopPresentationState.MakeClose();
                    continue;
                }
                if (stationMenuOpen)
                {
                    stationMenuOpen = false;

                    openStationType =
                        StationType::NONE;

                    stationMenuClosePending = true;
                }

                continue;
            }
        }

        if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN &&
            event.button.button == SDL_BUTTON_LEFT)
        {
            if (renderer != nullptr &&
                !SDL_ConvertEventToRenderCoordinates(renderer, &event))
            {
                continue;
            }
            float mouseX = event.button.x;
            float mouseY = event.button.y;

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

            if (HandleSidePanelClick(
                    mouseX,
                    mouseY))
            {
                continue;
            }

            if (stationMenuOpen)
            {
                HandleStationMenuClick(
                    event.button.x,
                    event.button.y);

                continue;
            }

            clickedTileX =
                static_cast<int>(mouseX) / TileSize;

            clickedTileY =
                static_cast<int>(mouseY) / TileSize;

            clickedMouseX =
                static_cast<int>(mouseX);

            clickedMouseY =
                static_cast<int>(mouseY);

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
        stationMenuOpen = false;
        openStationType = StationType::NONE;
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
    if (shopPresentationState.IsOpen()) { dialoguePresentationState.Dismiss(); stationMenuOpen=false; openStationType=StationType::NONE; }
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
void Graphics::ReconcileShopCommandResult(CommandResultCode result, bool closeCommand){ if(result==CommandResultCode::ACCEPTED){shopPresentationState.ClearRejection();return;} shopPresentationState.ReconcileRejectedCommand(); if(closeCommand) shopPresentationState.Dismiss(); }

SDL_FRect Graphics::GetDialogueCloseButtonRectangle() const
{
    return SDL_FRect{960.0f, 462.0f, 80.0f, 30.0f};
}

SDL_FRect Graphics::GetDialogueContinueButtonRectangle() const
{
    return SDL_FRect{850.0f, 626.0f, 190.0f, 38.0f};
}

SDL_FRect Graphics::GetDialogueChoiceButtonRectangle(
    std::size_t choiceIndex) const
{
    return SDL_FRect{
        240.0f,
        558.0f + static_cast<float>(choiceIndex) * 44.0f,
        800.0f,
        38.0f};
}

SDL_FRect Graphics::GetDialogueTradeButtonRectangle() const {
    const auto *event = dialoguePresentationState.GetEvent();
    const std::size_t index = event == nullptr ? 0 : event->choices.size();
    return GetDialogueChoiceButtonRectangle(index);
}
SDL_FRect Graphics::GetDialogueQuestButtonRectangle() const {auto b=GetDialogueTradeButtonRectangle();b.y+=44.0f;return b;}
SDL_FRect Graphics::GetShopRowRectangle(std::size_t i) const { return SDL_FRect{160.0f,150.0f+static_cast<float>(i)*48.0f,960.0f,42.0f}; }
SDL_FRect Graphics::GetShopBuyButtonRectangle(std::size_t i) const { auto r=GetShopRowRectangle(i); return SDL_FRect{r.x+r.w-300.0f,r.y+4.0f,120.0f,r.h-8.0f}; }
SDL_FRect Graphics::GetShopSellButtonRectangle(std::size_t i) const { auto r=GetShopRowRectangle(i); return SDL_FRect{r.x+r.w-160.0f,r.y+4.0f,120.0f,r.h-8.0f}; }
SDL_FRect Graphics::GetShopCloseButtonRectangle() const { return SDL_FRect{1020.0f,100.0f,90.0f,32.0f}; }

bool Graphics::HandleShopClick(float x,float y)
{
    if(!shopPresentationState.IsOpen()||pendingShopCommand)return true;
    if(IsPointInsideRectangle(x,y,GetShopCloseButtonRectangle())) { pendingShopCommand=shopPresentationState.MakeClose(); return true; }
    const auto* e=shopPresentationState.GetEvent();
    for(std::size_t i=0;i<e->entries.size();++i){ const auto& row=e->entries[i]; if(row.buyPrice && IsPointInsideRectangle(x,y,GetShopBuyButtonRectangle(i))) { pendingShopCommand=shopPresentationState.MakeBuy(i); return true; } if(row.sellPrice && IsPointInsideRectangle(x,y,GetShopSellButtonRectangle(i))) { pendingShopCommand=shopPresentationState.MakeSell(i); return true; } }
    return true;
}

bool Graphics::HandleDialogueClick(float mouseX, float mouseY)
{
    if (!dialoguePresentationState.IsOpen()) return false;
    if (pendingDialogueCommand.has_value()) return true;

    if (IsPointInsideRectangle(
            mouseX, mouseY, GetDialogueCloseButtonRectangle()))
    {
        if (dialoguePresentationState.GetEvent()->isTerminal)
        {
            dialoguePresentationState.DismissTerminal();
        }
        else
        {
            pendingDialogueCommand = dialoguePresentationState.MakeCloseCommand();
        }
        return true;
    }

    const NpcTalkEvent *event = dialoguePresentationState.GetEvent();
    if(event->questAction!=QuestDialogueAction::NONE&&IsPointInsideRectangle(mouseX,mouseY,GetDialogueQuestButtonRectangle())){pendingDialogueCommand=dialoguePresentationState.MakeQuestCommand();return true;}
    if (dialoguePresentationState.CanTrade() && IsPointInsideRectangle(mouseX, mouseY, GetDialogueTradeButtonRectangle()))
    {
        pendingDialogueCommand=dialoguePresentationState.MakeTradeCommand();
        return true;
    }

    if (event->nodeKind == DialogueNodeKind::CONTINUE &&
        IsPointInsideRectangle(
            mouseX, mouseY, GetDialogueContinueButtonRectangle()))
    {
        pendingDialogueCommand =
            dialoguePresentationState.MakeContinueCommand();
        return true;
    }

    if (event->isTerminal &&
        IsPointInsideRectangle(
            mouseX, mouseY, GetDialogueContinueButtonRectangle()))
    {
        dialoguePresentationState.DismissTerminal();
        return true;
    }

    if (event->nodeKind == DialogueNodeKind::CHOICE)
    {
        for (std::size_t index = 0; index < event->choices.size(); ++index)
        {
            if (IsPointInsideRectangle(
                    mouseX, mouseY,
                    GetDialogueChoiceButtonRectangle(index)))
            {
                pendingDialogueCommand =
                    dialoguePresentationState.MakeChoiceCommand(index);
                return true;
            }
        }
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

bool Graphics::ConsumeStationMenuClose()
{
    if (!stationMenuClosePending)
    {
        return false;
    }

    stationMenuClosePending = false;

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
    SDL_FRect playerRectangle =
        MakeEntityTileRectangle(
            playerX,
            playerY,
            4.0f,
            0,
            0);

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

void Graphics::DrawPlayerHealthBar(
    const Player &player)
{
    SDL_FRect background =
        GetPlayerHealthBarBackgroundRectangle(player);

    SDL_FRect fill =
        GetPlayerHealthBarFillRectangle(player);

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
    const std::vector<std::unique_ptr<Entity>> &entities)
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

            SDL_SetRenderDrawColor(
                renderer,
                155,
                50,
                60,
                255);

            SDL_RenderFillRect(
                renderer,
                &monsterRectangle);

            SDL_SetRenderDrawColor(
                renderer,
                220,
                110,
                120,
                255);

            SDL_RenderRect(
                renderer,
                &monsterRectangle);
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
            feedback.hit ? 255 : 230,
            feedback.hit ? 220 : 230,
            feedback.hit ? 70 : 230,
            255);

        std::string feedbackText = feedback.hit
                                       ? std::to_string(feedback.actualDamageApplied)
                                       : "MISS";

        SDL_RenderDebugText(
            renderer,
            textPosition.x,
            textPosition.y,
            feedbackText.c_str());
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
Graphics::SidePanelLayout Graphics::GetSidePanelLayout() const
{
    constexpr float panelWidth = 192.0f;
    constexpr float panelHeight = 348.0f;
    constexpr float panelMargin = 20.0f;
    constexpr float contentInset = 10.0f;
    constexpr float tabWidth = 25.0f;
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
    SDL_SetRenderDrawColor(renderer, 30, 30, 30, 240);
    SDL_RenderFillRect(renderer, &layout.panelBounds);
    SDL_SetRenderDrawColor(renderer, 180, 180, 180, 255);
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
        const SDL_FRect &tabRectangle = layout.tabBounds[tabIndex];

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
            tabRectangle.x + 4.0f,
            tabRectangle.y + 11.0f,
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
    const Inventory &inventory)
{
    constexpr int columns = 4;
    constexpr int rows = 7;

    constexpr float slotSize = 40.0f;
    constexpr float slotGap = 4.0f;
    constexpr float padding = 10.0f;
    constexpr float headerHeight = 24.0f;

    static_assert(
        padding * 2.0f + columns * slotSize + (columns - 1) * slotGap == 192.0f &&
        padding * 2.0f + headerHeight + rows * slotSize + (rows - 1) * slotGap == 348.0f);
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
}

void Graphics::DrawEquipment(
    const Equipment &equipment)
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

void Graphics::DrawDialoguePanel()
{
    const NpcTalkEvent *event = dialoguePresentationState.GetEvent();
    if (event == nullptr) return;

    const SDL_FRect panel{220.0f, 450.0f, 840.0f, 230.0f};
    SDL_SetRenderDrawColor(renderer, 24, 22, 20, 248);
    SDL_RenderFillRect(renderer, &panel);
    SDL_SetRenderDrawColor(renderer, 205, 180, 120, 255);
    SDL_RenderRect(renderer, &panel);

    const SDL_FRect closeButton = GetDialogueCloseButtonRectangle();
    SDL_SetRenderDrawColor(renderer, 70, 55, 45, 255);
    SDL_RenderFillRect(renderer, &closeButton);
    SDL_SetRenderDrawColor(renderer, 220, 200, 160, 255);
    SDL_RenderRect(renderer, &closeButton);

    SDL_SetRenderDrawColor(renderer, 255, 244, 210, 255);
    SDL_RenderDebugText(renderer, 240.0f, 468.0f,
                        dialoguePresentationState.GetNpcName().c_str());
    SDL_RenderDebugText(renderer, closeButton.x + 20.0f,
                        closeButton.y + 9.0f, "CLOSE");

    std::string firstLine = event->text.substr(0, 92);
    SDL_RenderDebugText(renderer, 240.0f, 510.0f, firstLine.c_str());
    if (event->text.size() > firstLine.size())
    {
        std::string secondLine = event->text.substr(firstLine.size(), 92);
        SDL_RenderDebugText(renderer, 240.0f, 528.0f, secondLine.c_str());
    }

    if (event->nodeKind == DialogueNodeKind::CHOICE)
    {
        for (std::size_t index = 0; index < event->choices.size(); ++index)
        {
            const SDL_FRect button = GetDialogueChoiceButtonRectangle(index);
            SDL_SetRenderDrawColor(renderer, 55, 50, 45, 255);
            SDL_RenderFillRect(renderer, &button);
            SDL_SetRenderDrawColor(renderer, 190, 170, 130, 255);
            SDL_RenderRect(renderer, &button);
            SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
            const std::string label = std::to_string(index + 1) + ". " +
                                      event->choices[index].text;
            SDL_RenderDebugText(renderer, button.x + 12.0f,
                                button.y + 12.0f, label.c_str());
        }
    }
    else
    {
        const SDL_FRect button = GetDialogueContinueButtonRectangle();
        SDL_SetRenderDrawColor(renderer, 55, 50, 45, 255);
        SDL_RenderFillRect(renderer, &button);
        SDL_SetRenderDrawColor(renderer, 190, 170, 130, 255);
        SDL_RenderRect(renderer, &button);
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        SDL_RenderDebugText(
            renderer, button.x + 55.0f, button.y + 12.0f,
            event->isTerminal ? "CLOSE" : "CONTINUE");
    }
    if (dialoguePresentationState.CanTrade()) {
        const SDL_FRect b=GetDialogueTradeButtonRectangle(); SDL_SetRenderDrawColor(renderer,55,50,45,255); SDL_RenderFillRect(renderer,&b); SDL_SetRenderDrawColor(renderer,190,170,130,255); SDL_RenderRect(renderer,&b); SDL_SetRenderDrawColor(renderer,255,255,255,255); SDL_RenderDebugText(renderer,b.x+12,b.y+12,"Trade");
    }
    if(event->questAction!=QuestDialogueAction::NONE){const auto b=GetDialogueQuestButtonRectangle();SDL_SetRenderDrawColor(renderer,55,50,45,255);SDL_RenderFillRect(renderer,&b);SDL_SetRenderDrawColor(renderer,190,170,130,255);SDL_RenderRect(renderer,&b);SDL_SetRenderDrawColor(renderer,255,255,255,255);SDL_RenderDebugText(renderer,b.x+12,b.y+12,event->questAction==QuestDialogueAction::ACCEPT_GATHERING_BASICS?"Accept Gathering Basics":"Complete Gathering Basics");}
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
Graphics::QuestPanelView Graphics::BuildQuestPanelView(const Player &player) const
{
    const SidePanelLayout layout = GetSidePanelLayout();
    const SDL_FRect bounds = layout.panelBounds;
    const SDL_FRect content = layout.contentBounds;
    const auto &record = player.GetQuestJournal().Get();
    const QuestDefinition *definition = QuestDefinitionDatabase::TryGet(record.id);

    QuestPanelView view{
        bounds,
        content,
        {content.x, content.y},
        {content.x, content.y + 32.0f},
        {content.x, content.y + 60.0f},
        {content.x, content.y + 84.0f},
        "QUESTS",
        definition == nullptr ? "Unknown quest" : definition->name,
        "UNAVAILABLE",
        "",
        selectedTab == SidePanelTab::QUESTS};

    switch (record.state)
    {
    case QuestState::AVAILABLE:
        view.state = "AVAILABLE";
        view.progress = "Logs: 0 / " +
            std::to_string(definition == nullptr ? 0 : definition->requiredAmount);
        break;
    case QuestState::ACTIVE:
        view.state = "ACTIVE";
        view.progress = "Logs: " + std::to_string(record.progress) + " / " +
            std::to_string(definition == nullptr ? 0 : definition->requiredAmount);
        break;
    case QuestState::READY_TO_COMPLETE:
        view.state = "READY TO COMPLETE";
        view.progress = "Logs: " + std::to_string(record.progress) + " / " +
            std::to_string(definition == nullptr ? 0 : definition->requiredAmount);
        break;
    case QuestState::COMPLETED:
        view.state = "COMPLETED";
        view.progress = "Logs: " + std::to_string(record.progress) + " / " +
            std::to_string(definition == nullptr ? 0 : definition->requiredAmount);
        break;
    default:
        break;
    }
    return view;
}

void Graphics::DrawQuestPanel(const Player &player)
{
    const QuestPanelView view = BuildQuestPanelView(player);
    DrawSidePanelFrame(view.heading.c_str());

    const SDL_Rect clipRectangle{
        static_cast<int>(view.contentBounds.x),
        static_cast<int>(view.contentBounds.y),
        static_cast<int>(view.contentBounds.w),
        static_cast<int>(view.contentBounds.h)};
    SDL_SetRenderClipRect(renderer, &clipRectangle);
    SDL_SetRenderDrawColor(renderer, 255, 215, 100, 255);
    SDL_RenderDebugText(renderer, view.titlePosition.x, view.titlePosition.y, view.title.c_str());
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderDebugText(renderer, view.statePosition.x, view.statePosition.y, view.state.c_str());
    SDL_RenderDebugText(renderer, view.progressPosition.x, view.progressPosition.y, view.progress.c_str());
    SDL_SetRenderClipRect(renderer, nullptr);
}

void Graphics::Render(
    Map &map,
    const std::vector<std::unique_ptr<Entity>> &entities,
    const std::vector<ResourceNode> &resources,
    const std::vector<CraftingStation> &stations,
    const std::map<int, MeleeCombatFeedback> &combatFeedbacks,
    int playerX,
    int playerY,
    const Inventory &inventory,
    const Equipment &equipment,
    const Player &player,
    const Action *activeAction)
{
    displayedCurrency = inventory.GetItemAmount(ItemType::COINS);
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
    DrawStations(stations);
    DrawMonsters(entities);
    DrawNPCs(entities);
    DrawPlayer(playerX, playerY);
    DrawPlayerHealthBar(player);
    DrawMonsterHealthBars(entities);
    DrawMonsterCombatFeedback(entities, combatFeedbacks);

    SDL_SetRenderViewport(renderer, nullptr);
    SDL_SetRenderScale(renderer, 1.0f, 1.0f);
    SDL_SetRenderClipRect(renderer, nullptr);
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
        DrawPlaceholderPanel("SETTINGS");
        break;
    }

    DrawActionProgress(
        activeAction);

    DrawStationMenu();
    if (shopPresentationState.IsOpen()) DrawShopPanel(); else DrawDialoguePanel();

    SDL_RenderPresent(renderer);
}
