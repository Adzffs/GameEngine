#include "InventoryStackOperations.h"

#include "../Item/ItemDatabase.h"

#include <algorithm>
#include <limits>
#include <vector>

namespace
{
    bool IsKnownItem(ItemType itemType)
    {
        const std::vector<ItemType> &knownItems =
            ItemDatabase::GetAllItemTypes();

        return std::find(
                   knownItems.begin(),
                   knownItems.end(),
                   itemType) != knownItems.end();
    }

    int MaximumStackSize(ItemType itemType)
    {
        if (itemType == ItemType::NONE || !IsKnownItem(itemType))
        {
            return 0;
        }

        return ItemDatabase::Get(itemType).IsStackable()
                   ? std::numeric_limits<int>::max()
                   : 1;
    }

    bool IsCanonicalState(
        std::span<const InventorySlot> slots,
        const InventorySlot &cursor)
    {
        if (!InventoryStackOperations::IsCanonicalStack(cursor))
        {
            return false;
        }

        for (const InventorySlot &slot : slots)
        {
            if (!InventoryStackOperations::IsCanonicalStack(slot))
            {
                return false;
            }
        }

        return true;
    }

    bool IsValidIndex(std::size_t size, int index)
    {
        return index >= 0 && static_cast<std::size_t>(index) < size;
    }

    void AssignStack(
        InventorySlot &stack,
        ItemType itemType,
        int quantity)
    {
        if (itemType == ItemType::NONE || quantity <= 0)
        {
            stack.Clear();
            return;
        }

        stack.SetItem(itemType, quantity);
    }

    bool Commit(
        std::span<InventorySlot> destination,
        const std::vector<InventorySlot> &replacement,
        InventorySlot &cursor,
        const InventorySlot &replacementCursor)
    {
        if (destination.size() != replacement.size() ||
            !IsCanonicalState(replacement, replacementCursor))
        {
            return false;
        }

        std::copy(
            replacement.begin(),
            replacement.end(),
            destination.begin());
        cursor = replacementCursor;
        return true;
    }

    std::vector<int> UniqueValidIndices(
        std::size_t slotCount,
        std::span<const int> indices,
        bool &allIndicesValid)
    {
        std::vector<int> unique;
        allIndicesValid = true;

        for (int index : indices)
        {
            if (!IsValidIndex(slotCount, index))
            {
                allIndicesValid = false;
                return {};
            }

            if (std::find(unique.begin(), unique.end(), index) == unique.end())
            {
                unique.push_back(index);
            }
        }

        return unique;
    }
}

bool InventoryStackOperations::IsCanonicalStack(
    const InventorySlot &stack)
{
    const ItemType itemType = stack.GetItemType();
    const int quantity = stack.GetAmount();

    if (itemType == ItemType::NONE)
    {
        return quantity == 0;
    }

    if (!IsKnownItem(itemType) || quantity <= 0)
    {
        return false;
    }

    return quantity <= MaximumStackSize(itemType);
}

bool InventoryStackOperations::TryLeftClick(
    std::span<InventorySlot> slots,
    int slotIndex,
    InventorySlot &cursor)
{
    if (!IsValidIndex(slots.size(), slotIndex) ||
        !IsCanonicalState(slots, cursor))
    {
        return false;
    }

    std::vector<InventorySlot> simulated(slots.begin(), slots.end());
    InventorySlot simulatedCursor = cursor;
    InventorySlot &slot = simulated[static_cast<std::size_t>(slotIndex)];

    if (simulatedCursor.IsEmpty())
    {
        if (slot.IsEmpty())
        {
            return false;
        }
        simulatedCursor = slot;
        slot.Clear();
    }
    else if (slot.IsEmpty())
    {
        slot = simulatedCursor;
        simulatedCursor.Clear();
    }
    else if (slot.GetItemType() == simulatedCursor.GetItemType())
    {
        const int capacity =
            MaximumStackSize(slot.GetItemType()) - slot.GetAmount();
        if (capacity <= 0)
        {
            return false;
        }

        const int moved = std::min(capacity, simulatedCursor.GetAmount());
        AssignStack(slot, slot.GetItemType(), slot.GetAmount() + moved);
        AssignStack(
            simulatedCursor,
            simulatedCursor.GetItemType(),
            simulatedCursor.GetAmount() - moved);
    }
    else
    {
        std::swap(slot, simulatedCursor);
    }

    return Commit(slots, simulated, cursor, simulatedCursor);
}

bool InventoryStackOperations::TryRightClick(
    std::span<InventorySlot> slots,
    int slotIndex,
    InventorySlot &cursor)
{
    if (!IsValidIndex(slots.size(), slotIndex) ||
        !IsCanonicalState(slots, cursor))
    {
        return false;
    }

    std::vector<InventorySlot> simulated(slots.begin(), slots.end());
    InventorySlot simulatedCursor = cursor;
    InventorySlot &slot = simulated[static_cast<std::size_t>(slotIndex)];

    if (simulatedCursor.IsEmpty())
    {
        if (slot.IsEmpty())
        {
            return false;
        }

        const int pickedUp = slot.GetAmount() / 2 + slot.GetAmount() % 2;
        AssignStack(simulatedCursor, slot.GetItemType(), pickedUp);
        AssignStack(slot, slot.GetItemType(), slot.GetAmount() - pickedUp);
    }
    else if (slot.IsEmpty())
    {
        AssignStack(slot, simulatedCursor.GetItemType(), 1);
        AssignStack(
            simulatedCursor,
            simulatedCursor.GetItemType(),
            simulatedCursor.GetAmount() - 1);
    }
    else if (slot.GetItemType() == simulatedCursor.GetItemType() &&
             slot.GetAmount() < MaximumStackSize(slot.GetItemType()))
    {
        AssignStack(slot, slot.GetItemType(), slot.GetAmount() + 1);
        AssignStack(
            simulatedCursor,
            simulatedCursor.GetItemType(),
            simulatedCursor.GetAmount() - 1);
    }
    else
    {
        return false;
    }

    return Commit(slots, simulated, cursor, simulatedCursor);
}

bool InventoryStackOperations::TryDistributeEvenly(
    std::span<InventorySlot> slots,
    std::span<const int> visitedSlotIndices,
    InventorySlot &cursor)
{
    if (!IsCanonicalState(slots, cursor) || cursor.IsEmpty())
    {
        return false;
    }

    bool indicesValid = false;
    const std::vector<int> unique = UniqueValidIndices(
        slots.size(), visitedSlotIndices, indicesValid);
    if (!indicesValid || unique.empty())
    {
        return false;
    }

    std::vector<InventorySlot> simulated(slots.begin(), slots.end());
    InventorySlot simulatedCursor = cursor;
    std::vector<int> active;
    active.reserve(unique.size());

    for (int index : unique)
    {
        const InventorySlot &slot = simulated[static_cast<std::size_t>(index)];
        if ((slot.IsEmpty() || slot.GetItemType() == cursor.GetItemType()) &&
            slot.GetAmount() < MaximumStackSize(cursor.GetItemType()))
        {
            active.push_back(index);
        }
    }

    if (active.empty())
    {
        return false;
    }

    int remaining = simulatedCursor.GetAmount();
    while (remaining > 0 && !active.empty())
    {
        const int count = static_cast<int>(active.size());
        const int baseShare = remaining / count;
        const int extraCount = remaining % count;
        std::vector<int> stillActive;
        int movedThisPass = 0;

        for (int position = 0; position < count; ++position)
        {
            InventorySlot &slot =
                simulated[static_cast<std::size_t>(active[position])];
            const int capacity =
                MaximumStackSize(cursor.GetItemType()) - slot.GetAmount();
            const int desired = baseShare + (position < extraCount ? 1 : 0);
            const int moved = std::min(capacity, desired);

            if (moved > 0)
            {
                AssignStack(
                    slot,
                    cursor.GetItemType(),
                    slot.GetAmount() + moved);
                remaining -= moved;
                movedThisPass += moved;
            }

            if (capacity > moved)
            {
                stillActive.push_back(active[position]);
            }
        }

        if (movedThisPass == 0)
        {
            break;
        }
        active = std::move(stillActive);
    }

    if (remaining == simulatedCursor.GetAmount())
    {
        return false;
    }

    AssignStack(simulatedCursor, cursor.GetItemType(), remaining);
    return Commit(slots, simulated, cursor, simulatedCursor);
}

bool InventoryStackOperations::TryDistributeOneEach(
    std::span<InventorySlot> slots,
    std::span<const int> visitedSlotIndices,
    InventorySlot &cursor)
{
    if (!IsCanonicalState(slots, cursor) || cursor.IsEmpty())
    {
        return false;
    }

    bool indicesValid = false;
    const std::vector<int> unique = UniqueValidIndices(
        slots.size(), visitedSlotIndices, indicesValid);
    if (!indicesValid || unique.empty())
    {
        return false;
    }

    std::vector<InventorySlot> simulated(slots.begin(), slots.end());
    InventorySlot simulatedCursor = cursor;
    int remaining = simulatedCursor.GetAmount();

    for (int index : unique)
    {
        if (remaining == 0)
        {
            break;
        }

        InventorySlot &slot = simulated[static_cast<std::size_t>(index)];
        if ((!slot.IsEmpty() && slot.GetItemType() != cursor.GetItemType()) ||
            slot.GetAmount() >= MaximumStackSize(cursor.GetItemType()))
        {
            continue;
        }

        AssignStack(slot, cursor.GetItemType(), slot.GetAmount() + 1);
        --remaining;
    }

    if (remaining == simulatedCursor.GetAmount())
    {
        return false;
    }

    AssignStack(simulatedCursor, cursor.GetItemType(), remaining);
    return Commit(slots, simulated, cursor, simulatedCursor);
}

bool InventoryStackOperations::TryCollectMatching(
    std::span<InventorySlot> slots,
    InventorySlot &cursor)
{
    if (!IsCanonicalState(slots, cursor) || cursor.IsEmpty())
    {
        return false;
    }

    int capacity =
        MaximumStackSize(cursor.GetItemType()) - cursor.GetAmount();
    if (capacity <= 0)
    {
        return false;
    }

    std::vector<InventorySlot> simulated(slots.begin(), slots.end());
    InventorySlot simulatedCursor = cursor;
    int collected = 0;

    for (InventorySlot &slot : simulated)
    {
        if (capacity == 0)
        {
            break;
        }
        if (slot.IsEmpty() || slot.GetItemType() != cursor.GetItemType())
        {
            continue;
        }

        const int moved = std::min(capacity, slot.GetAmount());
        AssignStack(slot, slot.GetItemType(), slot.GetAmount() - moved);
        capacity -= moved;
        collected += moved;
    }

    if (collected == 0)
    {
        return false;
    }

    AssignStack(
        simulatedCursor,
        cursor.GetItemType(),
        cursor.GetAmount() + collected);
    return Commit(slots, simulated, cursor, simulatedCursor);
}

bool InventoryStackOperations::TryTransfer(
    std::span<InventorySlot> sourceSlots,
    int sourceIndex,
    std::span<InventorySlot> destinationSlots)
{
    const InventorySlot emptyCursor;
    if (!IsValidIndex(sourceSlots.size(), sourceIndex) ||
        sourceSlots.data() == destinationSlots.data() ||
        !IsCanonicalState(sourceSlots, emptyCursor) ||
        !IsCanonicalState(destinationSlots, emptyCursor))
    {
        return false;
    }

    if (sourceSlots[static_cast<std::size_t>(sourceIndex)].IsEmpty())
    {
        return false;
    }

    std::vector<InventorySlot> simulatedSource(
        sourceSlots.begin(), sourceSlots.end());
    std::vector<InventorySlot> simulatedDestination(
        destinationSlots.begin(), destinationSlots.end());
    InventorySlot &source =
        simulatedSource[static_cast<std::size_t>(sourceIndex)];
    int remaining = source.GetAmount();

    for (InventorySlot &destination : simulatedDestination)
    {
        if (remaining == 0)
        {
            break;
        }
        if (destination.IsEmpty() ||
            destination.GetItemType() != source.GetItemType())
        {
            continue;
        }

        const int capacity =
            MaximumStackSize(source.GetItemType()) - destination.GetAmount();
        const int moved = std::min(capacity, remaining);
        AssignStack(
            destination,
            destination.GetItemType(),
            destination.GetAmount() + moved);
        remaining -= moved;
    }

    for (InventorySlot &destination : simulatedDestination)
    {
        if (remaining == 0)
        {
            break;
        }
        if (!destination.IsEmpty())
        {
            continue;
        }

        const int moved = std::min(
            MaximumStackSize(source.GetItemType()), remaining);
        AssignStack(destination, source.GetItemType(), moved);
        remaining -= moved;
    }

    if (remaining == source.GetAmount())
    {
        return false;
    }

    AssignStack(source, source.GetItemType(), remaining);
    if (!IsCanonicalState(simulatedSource, emptyCursor) ||
        !IsCanonicalState(simulatedDestination, emptyCursor))
    {
        return false;
    }

    std::copy(
        simulatedSource.begin(),
        simulatedSource.end(),
        sourceSlots.begin());
    std::copy(
        simulatedDestination.begin(),
        simulatedDestination.end(),
        destinationSlots.begin());
    return true;
}
