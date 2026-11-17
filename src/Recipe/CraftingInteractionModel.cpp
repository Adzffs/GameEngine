#include "CraftingInteractionModel.h"

#include "../Inventory/InventoryStackOperations.h"

#include <algorithm>

CraftingInteractionModel::CraftingInteractionModel(
    const Inventory &inventory, const CraftingSessionSnapshot &session)
    : sessionId(session.sessionId), actorEntityID(session.actorEntityID),
      revision(session.revision), cursor(session.cursor)
{
    std::copy(inventory.GetSlots().begin(), inventory.GetSlots().end(), slots.begin());
    std::copy(session.inputSlots.begin(), session.inputSlots.end(),
              slots.begin() + InventorySlotCount);
}

bool CraftingInteractionModel::TryLeftClick(int index)
{
    return InventoryStackOperations::TryLeftClick(slots, index, cursor);
}

bool CraftingInteractionModel::TryRightClick(int index)
{
    return InventoryStackOperations::TryRightClick(slots, index, cursor);
}

bool CraftingInteractionModel::TryLeftDrag(std::span<const int> indices)
{
    return InventoryStackOperations::TryDistributeEvenly(slots, indices, cursor);
}

bool CraftingInteractionModel::TryRightDrag(std::span<const int> indices)
{
    return InventoryStackOperations::TryDistributeOneEach(slots, indices, cursor);
}

bool CraftingInteractionModel::TryCollectMatching()
{
    return InventoryStackOperations::TryCollectMatching(slots, cursor);
}

bool CraftingInteractionModel::TryShiftTransfer(int index)
{
    if (index < 0 || index >= TotalSlotCount) return false;
    std::span<InventorySlot> inventory(slots.data(), InventorySlotCount);
    std::span<InventorySlot> crafting(slots.data() + InventorySlotCount,
                                      CraftingSlotCount);
    return index < InventorySlotCount
        ? InventoryStackOperations::TryTransfer(inventory, index, crafting)
        : InventoryStackOperations::TryTransfer(
              crafting, index - InventorySlotCount, inventory);
}

ReplaceCraftingInteractionStateCommand CraftingInteractionModel::BuildCommand() const
{
    ReplaceCraftingInteractionStateCommand command{};
    command.actorEntityID = actorEntityID;
    command.sessionId = sessionId;
    command.expectedRevision = revision;
    std::copy_n(slots.begin(), InventorySlotCount, command.inventorySlots.begin());
    command.cursor = cursor;
    std::copy_n(slots.begin() + InventorySlotCount, CraftingSlotCount,
                command.inputSlots.begin());
    return command;
}
