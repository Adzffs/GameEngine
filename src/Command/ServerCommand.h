#pragma once

#include <cstdint>
#include <variant>

#include "../Equipment/EquipmentSlotType.h"
#include "../Recipe/RecipeType.h"
#include "../World/Position.h"

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

enum class InteractionTargetType
{
    RESOURCE,
    STATION
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

using ServerCommandData = std::variant<
    MoveCommand,
    AttackCommand,
    UseInventoryItemCommand,
    UnequipItemCommand,
    StartRecipeCommand,
    InteractCommand,
    CloseStationCommand>;

struct ServerCommand
{
    std::uint64_t commandID;
    ServerCommandData data;
};
