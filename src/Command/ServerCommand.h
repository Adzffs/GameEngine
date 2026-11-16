#pragma once

#include <cstdint>
#include <array>
#include <variant>

#include "../Equipment/EquipmentSlotType.h"
#include "../Recipe/RecipeType.h"
#include "../World/Position.h"
#include "../Interaction/InteractionIntent.h"
#include "../Dialogue/DialogueSessionId.h"
#include "../Dialogue/DialogueChoiceId.h"
#include "../Inventory/ItemType.h"
#include "../Inventory/Inventory.h"
#include "../Shop/ShopSessionId.h"
#include "../Quest/QuestId.h"
#include "../Recipe/CraftingSession.h"
#include "../Bank/BankSession.h"

struct MoveCommand
{
    int actorEntityID;
    Position destination;
};

struct AttackCommand
{
    int actorEntityID;
    int targetEntityID;
};

struct UseInventoryItemCommand
{
    int actorEntityID;
    int inventorySlotIndex;
};

struct DropInventoryItemCommand
{
    int actorEntityID;
    int inventorySlotIndex;
    int amount;
};

struct TakeGroundItemCommand
{
    int actorEntityID;
    int groundItemID;
};

struct UnequipItemCommand
{
    int actorEntityID;
    EquipmentSlotType equipmentSlot;
};

struct StartRecipeCommand
{
    int actorEntityID;
    RecipeType recipeType;
};

struct CraftRecipeCommand
{
    int actorEntityID;
    std::array<ItemType, 9> grid;
};

struct ReplaceCraftingSessionStateCommand
{
    int actorEntityID;
    CraftingSessionId sessionId;
    std::uint64_t expectedRevision;
    InventorySlot cursor;
    std::array<InventorySlot, 9> inputSlots;
};

struct ReplaceCraftingInteractionStateCommand
{
    int actorEntityID;
    CraftingSessionId sessionId;
    std::uint64_t expectedRevision;
    std::array<InventorySlot, Inventory::SlotCount> inventorySlots;
    InventorySlot cursor;
    std::array<InventorySlot, 9> inputSlots;
};

struct InteractCommand
{
    int actorEntityID;
    InteractionTargetType targetType;
    int targetObjectID;
    Position destination;
};

struct CloseStationCommand
{
    int actorEntityID;
};

struct NpcInteractionCommand
{
    int actorEntityID;
    int targetNpcEntityID;
    NpcInteractionType interactionType;
};

struct DialogueContinueCommand
{
    int actorEntityID;
    DialogueSessionId sessionId;
};

struct DialogueCloseCommand
{
    int actorEntityID;
    DialogueSessionId sessionId;
};

struct DialogueChooseCommand
{
    int actorEntityID;
    DialogueSessionId sessionId;
    DialogueChoiceId choiceId;
};

struct ShopBuyCommand
{
    int actorEntityID;
    ShopSessionId shopSessionId;
    ItemType itemType;
    int quantity;
};

struct ShopSellCommand
{
    int actorEntityID;
    ShopSessionId shopSessionId;
    ItemType itemType;
    int quantity;
};

struct ShopCloseCommand
{
    int actorEntityID;
    ShopSessionId shopSessionId;
};
struct QuestAcceptCommand{int actorEntityID; int npcEntityID; DialogueSessionId dialogueSessionId; QuestId questId;};
struct QuestCompleteCommand{int actorEntityID; int npcEntityID; DialogueSessionId dialogueSessionId; QuestId questId;};

struct BankDepositCommand
{
    int actorEntityID;
    BankSessionId sessionId;
    std::uint64_t expectedRevision;
    int inventorySlotIndex;
    int quantity;
};
struct BankWithdrawCommand
{
    int actorEntityID;
    BankSessionId sessionId;
    std::uint64_t expectedRevision;
    ItemType itemType;
    int quantity;
};
struct BankDepositInventoryCommand
{
    int actorEntityID;
    BankSessionId sessionId;
    std::uint64_t expectedRevision;
};
struct BankCloseCommand
{
    int actorEntityID;
    BankSessionId sessionId;
};
struct SkipTutorialCommand
{
    int actorEntityID;
};

using ServerCommandData = std::variant<
    MoveCommand,
    AttackCommand,
    UseInventoryItemCommand,
    DropInventoryItemCommand,
    TakeGroundItemCommand,
    UnequipItemCommand,
    StartRecipeCommand,
    CraftRecipeCommand,
    ReplaceCraftingSessionStateCommand,
    ReplaceCraftingInteractionStateCommand,
    InteractCommand,
    CloseStationCommand,
    NpcInteractionCommand,
    DialogueContinueCommand,
    DialogueChooseCommand,
    DialogueCloseCommand,
    ShopBuyCommand,
    ShopSellCommand,
    ShopCloseCommand,
    QuestAcceptCommand,
    QuestCompleteCommand,
    BankDepositCommand,
    BankWithdrawCommand,
    BankDepositInventoryCommand,
    BankCloseCommand,
    SkipTutorialCommand>;

struct ServerCommand
{
    std::uint64_t commandID;
    ServerCommandData data;
};
