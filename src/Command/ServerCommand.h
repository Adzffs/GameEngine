#pragma once

#include <cstdint>
#include <variant>

#include "../Equipment/EquipmentSlotType.h"
#include "../Recipe/RecipeType.h"
#include "../World/Position.h"
#include "../Interaction/InteractionIntent.h"

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

using ServerCommandData = std::variant<
    MoveCommand,
    AttackCommand,
    UseInventoryItemCommand,
    UnequipItemCommand,
    StartRecipeCommand,
    InteractCommand,
    CloseStationCommand,
    NpcInteractionCommand>;

struct ServerCommand
{
    std::uint64_t commandID;
    ServerCommandData data;
};
