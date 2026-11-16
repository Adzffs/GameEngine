#pragma once

#include <array>
#include <span>

#include "CraftingSession.h"
#include "../Command/ServerCommand.h"
#include "../Inventory/Inventory.h"

class CraftingInteractionModel
{
public:
    static constexpr int InventorySlotCount = Inventory::SlotCount;
    static constexpr int CraftingSlotCount = 9;
    static constexpr int TotalSlotCount = InventorySlotCount + CraftingSlotCount;

    CraftingInteractionModel(const Inventory &inventory,
                             const CraftingSessionSnapshot &session);

    bool TryLeftClick(int unifiedSlotIndex);
    bool TryRightClick(int unifiedSlotIndex);
    bool TryLeftDrag(std::span<const int> unifiedSlotIndices);
    bool TryRightDrag(std::span<const int> unifiedSlotIndices);
    bool TryCollectMatching();
    bool TryShiftTransfer(int unifiedSlotIndex);

    ReplaceCraftingInteractionStateCommand BuildCommand() const;
    const InventorySlot &GetCursor() const { return cursor; }
    const std::array<InventorySlot, TotalSlotCount> &GetSlots() const { return slots; }

private:
    CraftingSessionId sessionId;
    int actorEntityID;
    std::uint64_t revision;
    std::array<InventorySlot, TotalSlotCount> slots{};
    InventorySlot cursor;
};
