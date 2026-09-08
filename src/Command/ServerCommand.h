#pragma once

#include <cstdint>
#include <variant>

#include "../Equipment/EquipmentSlotType.h"
#include "../Recipe/RecipeType.h"
#include "../World/Position.h"
#include "../Interaction/InteractionIntent.h"
#include "../Dialogue/DialogueSessionId.h"
#include "../Dialogue/DialogueChoiceId.h"
#include "../Inventory/ItemType.h"
#include "../Shop/ShopSessionId.h"
#include "../Quest/QuestId.h"

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

using ServerCommandData = std::variant<
    MoveCommand,
    AttackCommand,
    UseInventoryItemCommand,
    UnequipItemCommand,
    StartRecipeCommand,
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
    QuestCompleteCommand>;

struct ServerCommand
{
    std::uint64_t commandID;
    ServerCommandData data;
};
