#include "TestSupport.h"

#include "../src/Inventory/InventoryStackOperations.h"

#include <array>
#include <climits>
#include <map>
#include <random>

namespace
{
    InventorySlot Stack(ItemType itemType, int quantity)
    {
        InventorySlot stack;
        stack.SetItem(itemType, quantity);
        return stack;
    }

    std::map<ItemType, long long> Totals(
        std::span<const InventorySlot> slots,
        const InventorySlot &cursor)
    {
        std::map<ItemType, long long> totals;
        for (const InventorySlot &slot : slots)
        {
            if (!slot.IsEmpty())
            {
                totals[slot.GetItemType()] += slot.GetAmount();
            }
        }
        if (!cursor.IsEmpty())
        {
            totals[cursor.GetItemType()] += cursor.GetAmount();
        }
        return totals;
    }

    bool Same(
        std::span<const InventorySlot> left,
        std::span<const InventorySlot> right)
    {
        if (left.size() != right.size())
        {
            return false;
        }
        for (std::size_t index = 0; index < left.size(); ++index)
        {
            if (left[index].GetItemType() != right[index].GetItemType() ||
                left[index].GetAmount() != right[index].GetAmount())
            {
                return false;
            }
        }
        return true;
    }
}

int main()
{
    TestContext test;

    {
        InventorySlot empty;
        const InventorySlot malformedEmpty = Stack(ItemType::NONE, 1);
        const InventorySlot malformedKnown = Stack(ItemType::LOG, 2);
        const InventorySlot unknown = Stack(static_cast<ItemType>(9999), 1);
        test.Expect(InventoryStackOperations::IsCanonicalStack(empty),
                    "Canonical empty stack is valid");
        test.Expect(!InventoryStackOperations::IsCanonicalStack(malformedEmpty),
                    "Empty stack with quantity rejects");
        test.Expect(!InventoryStackOperations::IsCanonicalStack(malformedKnown),
                    "Non-stackable quantity above one rejects");
        test.Expect(!InventoryStackOperations::IsCanonicalStack(unknown),
                    "Unknown item rejects before definition access");
    }

    {
        std::array<InventorySlot, 4> slots{};
        slots[0] = Stack(ItemType::COINS, 9);
        InventorySlot cursor;
        const auto before = Totals(slots, cursor);

        test.Expect(InventoryStackOperations::TryLeftClick(slots, 0, cursor),
                    "Left click picks up complete stack");
        test.Expect(slots[0].IsEmpty() &&
                        cursor.GetItemType() == ItemType::COINS &&
                        cursor.GetAmount() == 9,
                    "Complete stack moves to cursor");
        test.Expect(before == Totals(slots, cursor),
                    "Pickup conserves every item");

        test.Expect(InventoryStackOperations::TryLeftClick(slots, 1, cursor),
                    "Left click places complete stack");
        test.Expect(cursor.IsEmpty() &&
                        slots[1].GetItemType() == ItemType::COINS &&
                        slots[1].GetAmount() == 9,
                    "Complete stack moves to destination");
        test.Expect(before == Totals(slots, cursor),
                    "Placement conserves every item");
    }

    {
        std::array<InventorySlot, 3> slots{};
        slots[0] = Stack(ItemType::COINS, 4);
        slots[1] = Stack(ItemType::LOG, 1);
        InventorySlot cursor = Stack(ItemType::COINS, 6);
        const auto before = Totals(slots, cursor);

        test.Expect(InventoryStackOperations::TryLeftClick(slots, 0, cursor),
                    "Left click merges matching stacks");
        test.ExpectEqual(slots[0].GetAmount(), 10,
                         "Merge destination receives exact quantity");
        test.Expect(cursor.IsEmpty(), "Merge empties cursor");
        test.Expect(before == Totals(slots, cursor),
                    "Merge conserves every item");

        cursor = Stack(ItemType::COINS, 3);
        const auto swapBefore = Totals(slots, cursor);
        test.Expect(InventoryStackOperations::TryLeftClick(slots, 1, cursor),
                    "Left click swaps incompatible stacks");
        test.Expect(slots[1].GetItemType() == ItemType::COINS &&
                        slots[1].GetAmount() == 3 &&
                        cursor.GetItemType() == ItemType::LOG &&
                        cursor.GetAmount() == 1,
                    "Swap preserves exact stack identities");
        test.Expect(swapBefore == Totals(slots, cursor),
                    "Swap conserves every item");
    }

    {
        std::array<InventorySlot, 3> slots{};
        slots[0] = Stack(ItemType::COINS, 5);
        InventorySlot cursor;
        const auto before = Totals(slots, cursor);

        test.Expect(InventoryStackOperations::TryRightClick(slots, 0, cursor),
                    "Right click picks up rounded-up half");
        test.ExpectEqual(cursor.GetAmount(), 3,
                         "Odd stack cursor receives rounded-up half");
        test.ExpectEqual(slots[0].GetAmount(), 2,
                         "Odd stack leaves rounded-down half");
        test.Expect(InventoryStackOperations::TryRightClick(slots, 1, cursor),
                    "Right click places one into empty slot");
        test.ExpectEqual(slots[1].GetAmount(), 1,
                         "Empty slot receives exactly one");
        test.ExpectEqual(cursor.GetAmount(), 2,
                         "Cursor decreases by exactly one");
        test.Expect(InventoryStackOperations::TryRightClick(slots, 1, cursor),
                    "Right click adds one to matching stack");
        test.ExpectEqual(slots[1].GetAmount(), 2,
                         "Matching stack increases by one");
        test.ExpectEqual(cursor.GetAmount(), 1,
                         "Matching placement consumes one cursor item");
        test.Expect(before == Totals(slots, cursor),
                    "Right-click sequence conserves every item");
    }

    {
        std::array<InventorySlot, 5> slots{};
        InventorySlot cursor = Stack(ItemType::COINS, 8);
        const std::array<int, 5> visited{0, 1, 1, 2, 3};
        const auto before = Totals(slots, cursor);
        test.Expect(InventoryStackOperations::TryDistributeEvenly(
                        slots, visited, cursor),
                    "Left drag distributes across unique visited slots");
        test.Expect(slots[0].GetAmount() == 2 &&
                        slots[1].GetAmount() == 2 &&
                        slots[2].GetAmount() == 2 &&
                        slots[3].GetAmount() == 2,
                    "Duplicate visits do not receive duplicate shares");
        test.Expect(cursor.IsEmpty(),
                    "Even distribution consumes exact cursor quantity");
        test.Expect(before == Totals(slots, cursor),
                    "Even distribution conserves every item");
    }

    {
        std::array<InventorySlot, 4> slots{};
        InventorySlot cursor = Stack(ItemType::COINS, 7);
        const std::array<int, 3> visited{0, 1, 2};
        test.Expect(InventoryStackOperations::TryDistributeEvenly(
                        slots, visited, cursor),
                    "Uneven quantity distributes deterministically");
        test.Expect(slots[0].GetAmount() == 3 &&
                        slots[1].GetAmount() == 2 &&
                        slots[2].GetAmount() == 2 &&
                        cursor.IsEmpty(),
                    "Earlier visited slots receive deterministic remainder");
    }

    {
        std::array<InventorySlot, 5> slots{};
        slots[1] = Stack(ItemType::LOG, 1);
        InventorySlot cursor = Stack(ItemType::COINS, 4);
        const std::array<int, 5> visited{0, 1, 2, 2, 3};
        const auto before = Totals(slots, cursor);
        test.Expect(InventoryStackOperations::TryDistributeOneEach(
                        slots, visited, cursor),
                    "Right drag places one in each compatible unique slot");
        test.Expect(slots[0].GetAmount() == 1 &&
                        slots[1].GetItemType() == ItemType::LOG &&
                        slots[2].GetAmount() == 1 &&
                        slots[3].GetAmount() == 1 &&
                        cursor.GetAmount() == 1,
                    "Right drag skips incompatible and duplicate visits");
        test.Expect(before == Totals(slots, cursor),
                    "Right drag conserves every item");
    }

    {
        std::array<InventorySlot, 5> slots{};
        slots[0] = Stack(ItemType::COINS, 2);
        slots[1] = Stack(ItemType::LOG, 1);
        slots[2] = Stack(ItemType::COINS, 4);
        InventorySlot cursor = Stack(ItemType::COINS, 3);
        const auto before = Totals(slots, cursor);
        test.Expect(InventoryStackOperations::TryCollectMatching(slots, cursor),
                    "Double-click collection gathers matching stacks");
        test.ExpectEqual(cursor.GetAmount(), 9,
                         "Collection moves every matching quantity");
        test.Expect(slots[0].IsEmpty() && slots[2].IsEmpty() &&
                        slots[1].GetItemType() == ItemType::LOG,
                    "Collection leaves non-matching stacks untouched");
        test.Expect(before == Totals(slots, cursor),
                    "Collection conserves every item");
    }

    {
        std::array<InventorySlot, 2> source{};
        std::array<InventorySlot, 3> destination{};
        source[0] = Stack(ItemType::COINS, 7);
        destination[0] = Stack(ItemType::COINS, 2);
        test.Expect(InventoryStackOperations::TryTransfer(
                        source, 0, destination),
                    "Shift transfer merges into matching destination");
        test.Expect(source[0].IsEmpty() &&
                        destination[0].GetAmount() == 9,
                    "Matching destination receives complete stack");

        source[1] = Stack(ItemType::LOG, 1);
        test.Expect(InventoryStackOperations::TryTransfer(
                        source, 1, destination),
                    "Non-stackable transfer uses empty destination");
        test.Expect(source[1].IsEmpty() &&
                        destination[1].GetItemType() == ItemType::LOG &&
                        destination[1].GetAmount() == 1,
                    "Non-stackable transfer preserves exact stack");
    }

    {
        std::array<InventorySlot, 2> slots{};
        slots[0] = Stack(ItemType::LOG, 1);
        InventorySlot cursor = Stack(ItemType::COINS, 3);
        const auto slotsBefore = slots;
        const InventorySlot cursorBefore = cursor;
        const std::array<int, 2> invalidVisited{0, 3};

        test.Expect(!InventoryStackOperations::TryLeftClick(slots, -1, cursor),
                    "Negative click index rejects");
        test.Expect(!InventoryStackOperations::TryRightClick(slots, 2, cursor),
                    "Past-end click index rejects");
        test.Expect(!InventoryStackOperations::TryDistributeEvenly(
                        slots, invalidVisited, cursor),
                    "Any invalid drag index rejects complete gesture");
        test.Expect(Same(slots, slotsBefore) &&
                        cursor.GetItemType() == cursorBefore.GetItemType() &&
                        cursor.GetAmount() == cursorBefore.GetAmount(),
                    "Rejected operations leave slots and cursor unchanged");
    }

    {
        std::array<InventorySlot, 2> slots{};
        slots[0] = Stack(ItemType::COINS, INT_MAX);
        InventorySlot cursor = Stack(ItemType::COINS, 1);
        const auto slotsBefore = slots;
        const InventorySlot cursorBefore = cursor;
        const std::array<int, 1> visited{0};

        test.Expect(!InventoryStackOperations::TryLeftClick(slots, 0, cursor),
                    "Full matching stack rejects merge without overflow");
        test.Expect(!InventoryStackOperations::TryRightClick(slots, 0, cursor),
                    "Full matching stack rejects single placement");
        test.Expect(!InventoryStackOperations::TryDistributeEvenly(
                        slots, visited, cursor),
                    "Full matching stack rejects drag placement");
        test.Expect(Same(slots, slotsBefore) &&
                        cursor.GetItemType() == cursorBefore.GetItemType() &&
                        cursor.GetAmount() == cursorBefore.GetAmount(),
                    "Overflow rejections leave all quantities exact");
    }

    {
        std::array<InventorySlot, 6> slots{};
        slots[0] = Stack(ItemType::COINS, 19);
        slots[1] = Stack(ItemType::LOG, 1);
        InventorySlot cursor;
        const auto initialTotals = Totals(slots, cursor);
        std::mt19937 random(24681357U);

        for (int step = 0; step < 1000; ++step)
        {
            const int operation = static_cast<int>(random() % 5U);
            const int index = static_cast<int>(random() % slots.size());
            if (operation == 0)
            {
                InventoryStackOperations::TryLeftClick(slots, index, cursor);
            }
            else if (operation == 1)
            {
                InventoryStackOperations::TryRightClick(slots, index, cursor);
            }
            else if (operation == 2 || operation == 3)
            {
                const std::array<int, 3> visited{
                    index,
                    static_cast<int>(random() % slots.size()),
                    static_cast<int>(random() % slots.size())};
                if (operation == 2)
                {
                    InventoryStackOperations::TryDistributeEvenly(
                        slots, visited, cursor);
                }
                else
                {
                    InventoryStackOperations::TryDistributeOneEach(
                        slots, visited, cursor);
                }
            }
            else
            {
                InventoryStackOperations::TryCollectMatching(slots, cursor);
            }

            bool canonical = InventoryStackOperations::IsCanonicalStack(cursor);
            for (const InventorySlot &slot : slots)
            {
                canonical = canonical &&
                            InventoryStackOperations::IsCanonicalStack(slot);
            }
            test.Expect(canonical,
                        "Randomized operation preserves canonical stacks");
            test.Expect(initialTotals == Totals(slots, cursor),
                        "Randomized operation preserves total item quantities");
        }
    }

    return test.Finish();
}
