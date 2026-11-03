#pragma once

#include "InventorySlot.h"

#include <span>

class InventoryStackOperations
{
public:
    static bool IsCanonicalStack(const InventorySlot &stack);

    static bool TryLeftClick(
        std::span<InventorySlot> slots,
        int slotIndex,
        InventorySlot &cursor);

    static bool TryRightClick(
        std::span<InventorySlot> slots,
        int slotIndex,
        InventorySlot &cursor);

    static bool TryDistributeEvenly(
        std::span<InventorySlot> slots,
        std::span<const int> visitedSlotIndices,
        InventorySlot &cursor);

    static bool TryDistributeOneEach(
        std::span<InventorySlot> slots,
        std::span<const int> visitedSlotIndices,
        InventorySlot &cursor);

    static bool TryCollectMatching(
        std::span<InventorySlot> slots,
        InventorySlot &cursor);

    static bool TryTransfer(
        std::span<InventorySlot> sourceSlots,
        int sourceIndex,
        std::span<InventorySlot> destinationSlots);
};
