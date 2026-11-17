#pragma once

#include <SDL3/SDL.h>
#include <array>
#include <memory>
#include <map>
#include <optional>
#include <string>
#include <vector>

#include "../Action/Action.h"
#include "../Entity/Entity.h"
#include "../Combat/MeleeCombatFeedback.h"
#include "../Combat/MeleeAttackPresentation.h"
#include "../World/Object/Resource/ResourceNode.h"
#include "../World/Object/Station/CraftingStation.h"
#include "../World/Object/Station/StationType.h"
#include "../World/GroundItem.h"

#include "../Inventory/Inventory.h"
#include "../Equipment/Equipment.h"
#include "../Recipe/RecipeType.h"
#include "../Recipe/CraftingRecipeMatcher.h"
#include "../Recipe/CraftingSession.h"
#include "../Command/ServerCommand.h"
#include "../Command/CommandProcessingResult.h"
#include "../Dialogue/DialoguePresentationState.h"
#include "../Shop/ShopPresentationState.h"
#include "../Quest/QuestPresentationState.h"
#include "../Bank/Bank.h"
#include "../Bank/BankSession.h"
#include "../Progression/ProgressionPresentationState.h"

class Map;
class Player;
class Monster;
class NPC;
struct ActiveDialogueSession;
struct GraphicsTestAccess;

class Graphics
{
public:
    Graphics();
    ~Graphics();

    bool Initialize();
    void ShowGameGuide() { gameGuideVisible = true; }
    void ProcessEvents(bool &running);
    bool ConsumeClickedTile(int &tileX, int &tileY);
    bool ConsumeClickedTile(
        int &tileX,
        int &tileY,
        int &mouseX,
        int &mouseY);
    bool ConsumeWalkTile(int &tileX, int &tileY);
    bool ConsumeInventorySlotClick(
        int &slotIndex);
    bool ConsumeInventoryDropRequest(int &slotIndex, int &amount);
    bool ConsumeTutorialSkipRequest();
    bool ConsumeRunToggle();
    std::optional<CombatStyle> ConsumeCombatStyleRequest();

    bool ConsumeWeaponSlotClick();
    bool ConsumeRecipeRequest(
        RecipeType &recipeType);
    bool ConsumeCraftingGridRequest(
        CraftingRecipeMatcher::Grid &grid);
    bool ConsumeStationMenuClose();
    void SetRecipeCommandInFlight(bool inFlight);
    bool IsRecipeCommandInFlight() const;
    void ReconcileRecipeCommandResult(CommandResultCode result);
    void SynchronizeCraftingSession(
        const CraftingSessionSnapshot *session);
    const std::optional<CraftingSessionSnapshot> &
    GetCraftingSessionSnapshot() const;
    struct CraftingInteractionRequest
    {
        int unifiedSlotIndex = -1;
        ItemType inventoryItemType = ItemType::NONE;
        bool rightClick = false;
        bool shiftHeld = false;
        bool collectMatching = false;
        std::vector<int> dragSlotIndices;
        bool clearCrafting = false;
    };
    std::optional<CraftingInteractionRequest> ConsumeCraftingInteractionRequest();
    void SetCraftingInteractionInFlight(bool inFlight);
    void ReconcileCraftingInteractionResult(CommandResultCode result);
    std::optional<ServerCommandData> ConsumeDialogueCommand();
    std::optional<ServerCommandData> ConsumeShopCommand();
    void SynchronizeBank(int actorEntityID, const ActiveBankSession *session,
                         const Bank &bank);
    std::optional<ServerCommandData> ConsumeBankCommand();
    void SetShopCommandInFlight(bool inFlight);
    bool IsShopCommandInFlight() const;
    void ReconcileShopCommandResult(CommandResultCode result, bool closeCommand);
    void SynchronizeDialogue(
        int localActorEntityID,
        const std::vector<NpcTalkEvent> &publishedEvents,
        const ActiveDialogueSession *activeSession);
    const DialoguePresentationState &GetDialoguePresentationState() const
    {
        return dialoguePresentationState;
    }
    void SynchronizeShop(int actor,const std::vector<ShopOpenedEvent>& events,const ActiveShopSession* active);
    const ShopPresentationState &GetShopPresentationState() const { return shopPresentationState; }

    void OpenStationMenu(
        StationType stationType);
    bool IsStationMenuOpen() const;
    StationType GetOpenStationType() const;
    void DrawStationMenu();
    void DrawActionProgress(
        const Action *activeAction);

    float CalculateHealthRatio(
        int currentHealth,
        int maximumHealth) const;
    void AdjustCameraZoom(int wheelDirection);
    float GetCameraZoom() const { return cameraZoom; }
    static std::string GetCombatFeedbackLabel(
        const MeleeCombatFeedback &feedback);

    SDL_FRect GetMonsterHealthBarBackgroundRectangle(
        const Monster &monster,
        int cameraTileX = 0,
        int cameraTileY = 0) const;

    SDL_FRect GetPlayerScreenRectangle(
        const Player &player,
        int cameraTileX = 0,
        int cameraTileY = 0) const;

    SDL_FRect GetPlayerHealthBarBackgroundRectangle(
        const Player &player,
        int cameraTileX = 0,
        int cameraTileY = 0) const;

    SDL_FRect GetPlayerHealthBarFillRectangle(
        const Player &player,
        int cameraTileX = 0,
        int cameraTileY = 0) const;

    SDL_FPoint GetPlayerCombatFeedbackPosition(
        const Player &player,
        int cameraTileX = 0,
        int cameraTileY = 0) const;

    SDL_FRect GetMonsterHealthBarFillRectangle(
        const Monster &monster,
        int cameraTileX = 0,
        int cameraTileY = 0) const;

    SDL_FRect GetMonsterCorpseRectangle(
        const Monster &monster,
        int cameraTileX = 0,
        int cameraTileY = 0) const;

    SDL_FPoint GetMonsterCombatFeedbackPosition(
        const Monster &monster,
        int cameraTileX = 0,
        int cameraTileY = 0) const;

    void Render(
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
        int selectedCombatTargetID = -1,
        int deathRecoveryTicksRemaining = -1,
        int deathKillerEntityID = -1,
        int deathX = 0,
        int deathY = 0,
        int recoveryPileX = -1,
        int recoveryPileY = -1,
        const std::map<int, MeleeAttackPresentation> *attackPresentations = nullptr);

    enum class WorldHoverTargetType
    {
        NONE,
        GROUND_ITEM,
        FRIENDLY_NPC,
        MONSTER,
        STATION,
        RESOURCE
    };
    struct WorldHoverTarget
    {
        WorldHoverTargetType type = WorldHoverTargetType::NONE;
        int id = -1;
        SDL_FRect bounds{};
        std::string name;
        std::string primaryAction;
        std::string examineText;

        bool HasTarget() const
        {
            return type != WorldHoverTargetType::NONE;
        }
    };
    WorldHoverTarget ResolveWorldHoverTarget(
        float screenX,
        float screenY,
        const std::vector<std::unique_ptr<Entity>> &entities,
        const std::vector<ResourceNode> &resources,
        const std::vector<CraftingStation> &stations,
        const std::vector<GroundItem> &groundItems = {}) const;

    struct WorldContextMenu
    {
        bool open = false;
        WorldHoverTarget target;
        int tileX = 0;
        int tileY = 0;
        int screenX = 0;
        int screenY = 0;
        SDL_FRect bounds{};
    };

    enum class SidePanelTab
    {
        SKILLS,
        QUESTS,
        INVENTORY,
        EQUIPMENT,
        RECIPES,
        SETTINGS
    };
    struct QuestPanelView
    {
        SDL_FRect bounds;
        SDL_FRect contentBounds;
        SDL_FPoint headingPosition;
        SDL_FPoint titlePosition;
        SDL_FPoint statePosition;
        SDL_FPoint progressPosition;
        std::string heading;
        std::string title;
        std::string state;
        std::string progress;
        bool visible;
    };
    struct SidePanelLayout
    {
        SDL_FRect panelBounds;
        SDL_FRect contentBounds;
        std::array<SDL_FRect, 6> tabBounds;
    };
    enum class DialogueControlType
    {
        GENERAL_CLOSE,
        AUTHORED_CHOICE,
        TRADE,
        QUEST_ACTION,
        PRIMARY
    };
    struct DialogueControlLayout
    {
        DialogueControlType type;
        std::size_t index;
        SDL_FRect bounds;
    };
    struct DialoguePanelLayout
    {
        SDL_FRect panelBounds;
        std::vector<DialogueControlLayout> controls;
    };
    struct QuestRowLayout
    {
        QuestPresentation quest;
        SDL_FRect bounds;
        SDL_FPoint titlePosition;
        SDL_FPoint statePosition;
        SDL_FPoint progressPosition;
        float originY;
        bool clipped;
    };
    struct FurnaceMenuLayout
    {
        SDL_FRect panelBounds;
        SDL_FRect closeButton;
        std::array<SDL_FRect, 10> recipeButtons;
    };
    struct CraftingTableLayout
    {
        SDL_FRect panelBounds;
        SDL_FRect closeButton;
        std::array<SDL_FRect, 9> gridSlots;
        std::array<SDL_FRect, Inventory::SlotCount> inventorySlots;
        std::array<SDL_FRect, 4> ingredientButtons;
        SDL_FRect clearButton;
        SDL_FRect craftButton;
        SDL_FRect previewBounds;
    };
    SidePanelLayout GetSidePanelLayout() const;
    DialoguePanelLayout GetDialoguePanelLayout() const;
    std::vector<QuestRowLayout> BuildQuestRowLayout(
        const std::vector<QuestPresentation> &quests) const;
    FurnaceMenuLayout GetFurnaceMenuLayout() const;
    CraftingTableLayout GetCraftingTableLayout() const;
    const CraftingRecipeMatcher::Grid &GetCraftingGrid() const;
    static std::string GetQuestStateLabel(QuestState state);
    static std::string GetQuestProgressLabel(
        const QuestPresentation &quest);
    static std::string GetQuestCategoryLabel(QuestCategory category);
    static std::string GetQuestRewardLabel(const QuestPresentation &quest);
    SDL_FRect GetSidePanelRectangle() const;
    QuestPanelView BuildQuestPanelView(const Player &player) const;
    bool HandleSidePanelClick(
        float mouseX,
        float mouseY,
        bool rightClick = false);
    bool IsSidePanelOpen() const { return sidePanelOpen; }

    void DrawSidePanelTabs();
    void DrawGameGuide(const Player &player);
    void DrawWorldHud(const Player &player);
    void DrawSettings();
    void DrawSidePanelFrame(const char *title, float headingYOffset = 10.0f);

    void HandleStationMenuClick(
        float mouseX,
        float mouseY,
        bool rightClick = false,
        bool shiftHeld = false,
        bool doubleClick = false);

    bool IsPointInsideRectangle(
        float pointX,
        float pointY,
        const SDL_FRect &rectangle) const;

    void DrawPlaceholderPanel(
        const char *title);
    void DrawRecipeBook();
    std::vector<RecipeType> BuildDisplayedRecipeBookEntries() const;

    void DrawMap(Map &map);
    void DrawGrid(const Map &map);
    void DrawClickedTile();
    void DrawPlayer(int playerX, int playerY);
    void DrawPlayerHealthBar(
        const Player &player);
    SDL_FRect GetMonsterScreenRectangle(
        const Monster &monster,
        int cameraTileX = 0,
        int cameraTileY = 0) const;
    std::optional<int> GetMonsterAtScreenPosition(
        int mouseX,
        int mouseY,
        const std::vector<std::unique_ptr<Entity>> &entities,
        int cameraTileX = 0,
        int cameraTileY = 0) const;
    std::optional<int> GetFriendlyNpcAtScreenPosition(
        int mouseX,
        int mouseY,
        const std::vector<std::unique_ptr<Entity>> &entities,
        int cameraTileX = 0,
        int cameraTileY = 0) const;
    SDL_FRect GetFriendlyNpcScreenRectangle(
        const NPC &npc,
        int cameraTileX = 0,
        int cameraTileY = 0) const;
    SDL_FRect GetDialogueCloseButtonRectangle() const;
    SDL_FRect GetDialogueContinueButtonRectangle() const;
    SDL_FRect GetDialogueChoiceButtonRectangle(std::size_t choiceIndex) const;
    SDL_FRect GetDialogueTradeButtonRectangle() const;
    SDL_FRect GetDialogueQuestButtonRectangle(
        std::size_t actionIndex = 0) const;
    SDL_FRect GetShopRowRectangle(std::size_t index) const;
    SDL_FRect GetShopBuyButtonRectangle(std::size_t index) const;
    SDL_FRect GetShopSellButtonRectangle(std::size_t index) const;
    SDL_FRect GetShopCloseButtonRectangle() const;
    std::string GetShopItemDetail(ItemType itemType) const;
    void DrawShopPanel();
    void DrawQuestPanel(const Player &player);
    void DrawQuestRows(const std::vector<QuestPresentation> &quests);
    void DrawDialoguePanel();
    void DrawNPCs(
        const std::vector<std::unique_ptr<Entity>> &entities);
    void DrawMonsters(
        const std::vector<std::unique_ptr<Entity>> &entities,
        int selectedCombatTargetID,
        int localPlayerID);
    void DrawCombatTargetPanel(
        const std::vector<std::unique_ptr<Entity>> &entities,
        int selectedCombatTargetID,
        int localPlayerID);
    void DrawDeathRecoveryOverlay(
        int ticksRemaining,
        int killerEntityID,
        int deathX,
        int deathY);
    void DrawMonsterHealthBars(
        const std::vector<std::unique_ptr<Entity>> &entities);
    void DrawMonsterCombatFeedback(
        const std::vector<std::unique_ptr<Entity>> &entities,
        const std::map<int, MeleeCombatFeedback> &combatFeedbacks);
    void DrawMeleeAttackPresentations(
        const std::vector<std::unique_ptr<Entity>> &entities,
        const std::map<int, MeleeAttackPresentation> &presentations);
    void DrawResources(
        const std::vector<ResourceNode> &resources);
    void DrawStations(
        const std::vector<CraftingStation> &stations);
    void DrawGroundItems(const std::vector<GroundItem> &groundItems);
    void DrawInventory(
        const Inventory &inventory,
        const Equipment *equipment = nullptr,
        const Player *player = nullptr);
    void DrawSkills(const Player &player);
    void DrawEquipment(
        const Equipment &equipment,
        const Inventory *inventory = nullptr,
        const Player *player = nullptr);
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
    void DrawSwordIcon(
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
    void DrawFoodIcon(
        ItemType itemType,
        float x,
        float y,
        float size);
    SDL_Window *window;
    SDL_Renderer *renderer;

    bool clickPending;
    bool hasClickedTile;
    bool walkClickPending = false;
    int walkTileX = 0;
    int walkTileY = 0;

    int clickedTileX;
    int clickedTileY;
    int clickedMouseX;
    int clickedMouseY;

    static constexpr int WindowWidth = 1280;
    static constexpr int WindowHeight = 720;
    static constexpr int TileSize = 32;
    SidePanelTab selectedTab;
    bool sidePanelOpen;

private:
    friend struct GraphicsTestAccess;

    bool inventorySlotClickPending;
    int clickedInventorySlotIndex;
    bool inventoryDropPending = false;
    int dropInventorySlotIndex = -1;
    int dropInventoryAmount = 0;
    bool tutorialSkipPending = false;
    std::optional<CombatStyle> combatStyleRequest;

    bool weaponSlotClickPending;
    bool recipeRequestPending;
    RecipeType requestedRecipeType;
    bool craftingGridRequestPending = false;
    CraftingRecipeMatcher::Grid requestedCraftingGrid{};
    CraftingRecipeMatcher::Grid craftingGrid{};
    ItemType selectedCraftingItem = ItemType::NONE;
    bool recipeCommandInFlight = false;
    bool recipeActionActive = false;
    std::optional<CraftingSessionSnapshot> craftingSessionSnapshot;
    std::optional<CraftingInteractionRequest> pendingCraftingInteraction;
    bool craftingInteractionInFlight = false;
    bool craftingDragActive = false;
    bool craftingDragRightClick = false;
    std::vector<int> craftingDragSlotIndices;
    int displayedLogCount = 0;
    int displayedOakLogCount = 0;
    int displayedWillowLogCount = 0;
    int displayedPlankCount = 0;
    int displayedWoodcuttingLevel = 1;
    int displayedMiningLevel = 1;
    int displayedSmithingLevel = 1;
    std::vector<RecipeType> displayedKnownRecipes;
    bool recipeKnowledgeInitialized = false;
    bool recipeCanCraftOnly = false;
    int recipeBookPage = 0;
    RecipeType selectedRecipeBookRecipe = RecipeType::NONE;
    std::array<InventorySlot, Inventory::SlotCount> displayedInventorySlots{};
    std::string stationFeedback;

    float visualPlayerX = 0.0f;
    float visualPlayerY = 0.0f;
    bool visualPlayerInitialized = false;
    float cameraPixelX = 0.0f;
    float cameraPixelY = 0.0f;
    float cameraZoom = 1.0f;
    bool cameraInitialized = false;
    bool runTogglePending = false;
    int minimapOriginX = 0;
    int minimapOriginY = 0;
    Uint64 lastPresentationTick = 0;
    float presentationDeltaSeconds = 1.0f / 60.0f;
    bool gameGuideVisible = false;
    bool showGrid = false;
    float mouseX = 0.0f;
    float mouseY = 0.0f;
    WorldHoverTarget worldHoverTarget;
    WorldContextMenu worldContextMenu;
    std::string examineMessage;
    Uint64 examineMessageUntil = 0;
    struct ActiveProgressionNotice
    {
        ProgressionEventType type;
        std::string text;
        float remainingSeconds;
    };
    ProgressionPresentationState progressionPresentationState;
    std::vector<ActiveProgressionNotice> progressionNotices;

    bool stationMenuOpen;
    StationType openStationType;
    bool stationMenuClosePending;
    DialoguePresentationState dialoguePresentationState;
    std::optional<ServerCommandData> pendingDialogueCommand;
    ShopPresentationState shopPresentationState;
    std::optional<ServerCommandData> pendingShopCommand;
    std::optional<ServerCommandData> pendingBankCommand;
    int bankActorEntityID = -1;
    BankSessionId bankSessionId = InvalidBankSessionId;
    std::uint64_t bankRevision = 0;
    std::vector<BankEntry> displayedBankEntries;
    int bankQuantityMode = 1;
    std::string bankSearch;
    bool shopCommandInFlight = false;
    int displayedCurrency = 0;
    QuestPresentationState questPresentationState;

    bool HandleDialogueClick(float mouseX, float mouseY);
    bool HandleShopClick(float mouseX, float mouseY);
    void DismissStationMenu(bool queueCloseCommand);
    int GetDisplayedCraftingItemCount(ItemType itemType) const;
    int GetDisplayedInventoryItemCount(ItemType itemType) const;
    int GetDisplayedSkillLevel(SkillType skillType) const;
    bool HasDisplayedRecipeRequirements(
        const RecipeDefinition &recipe,
        bool ingredientsAreInCraftingGrid = false) const;
    bool IsDisplayedRecipeKnown(RecipeType recipeType) const;
    std::string GetDisplayedRecipeStatus(
        const RecipeDefinition &recipe,
        bool ingredientsAreInCraftingGrid = false) const;
    int GetCraftingGridItemCount(ItemType itemType) const;
    void DrawCraftingItemIcon(
        ItemType itemType,
        const SDL_FRect &bounds);
    void DrawWorldHoverFeedback();
    void UpdateCamera(int playerX, int playerY, const Map &map);
    SDL_FPoint ScreenToWorldPoint(float screenX, float screenY) const;
    bool HandleMinimapClick(float screenX, float screenY);
    void DrawMinimap(
        Map &map,
        const std::vector<std::unique_ptr<Entity>> &entities,
        const std::vector<ResourceNode> &resources,
        const std::vector<CraftingStation> &stations,
        int playerX,
        int playerY,
        int recoveryPileX = -1,
        int recoveryPileY = -1);
    void UpdateProgressionFeedback(
        const Player &player,
        const Inventory &inventory);
    void DrawProgressionFeedback();
    void OpenWorldContextMenu(float screenX, float screenY);
    bool HandleWorldContextMenuClick(float screenX, float screenY);
    void DrawWorldContextMenu();
    void DrawBankPanel();
    void HandleBankPanelClick(float screenX, float screenY);
};
