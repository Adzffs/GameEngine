#include "TestSupport.h"

#include "../src/Inventory/Inventory.h"
#include "../src/Inventory/InventorySlot.h"
#include "../src/Inventory/ItemAmount.h"
#include "../src/Inventory/ItemType.h"

#include <climits>
#include <vector>

namespace
{
    bool SameSlots(
        const Inventory &left,
        const Inventory &right)
    {
        const auto &leftSlots = left.GetSlots();
        const auto &rightSlots = right.GetSlots();

        for (int index = 0; index < Inventory::SlotCount; ++index)
        {
            if (leftSlots[index].IsEmpty() != rightSlots[index].IsEmpty())
            {
                return false;
            }

            if (leftSlots[index].GetItemType() != rightSlots[index].GetItemType())
            {
                return false;
            }

            if (leftSlots[index].GetAmount() != rightSlots[index].GetAmount())
            {
                return false;
            }
        }

        return true;
    }

    int FindFirstSlot(
        const Inventory &inventory,
        ItemType itemType)
    {
        const auto &slots = inventory.GetSlots();

        for (int index = 0; index < Inventory::SlotCount; ++index)
        {
            if (!slots[index].IsEmpty() &&
                slots[index].GetItemType() == itemType)
            {
                return index;
            }
        }

        return -1;
    }
}

int main()
{
    TestContext test;

    {
        Inventory inventory;
        Inventory before = inventory;

        test.Expect(
            inventory.TryAddItemsAtomically({}),
            "Empty transaction succeeds");
        test.Expect(
            SameSlots(inventory, before),
            "Empty transaction leaves inventory unchanged");
    }

    {
        Inventory inventory;

        test.Expect(
            inventory.TryAddItemsAtomically(
                {{ItemType::COAL, 5}}),
            "One stackable reward succeeds");
        test.ExpectEqual(
            inventory.GetItemAmount(ItemType::COAL),
            5,
            "Stackable quantity is applied");
    }

    {
        Inventory inventory;

        test.Expect(
            inventory.TryAddItemsAtomically(
                {{ItemType::LOG, 1}}),
            "One non-stackable reward succeeds");
        test.ExpectEqual(
            inventory.GetItemAmount(ItemType::LOG),
            1,
            "Non-stackable quantity is applied");
    }

    {
        Inventory inventory;

        test.Expect(
            inventory.TryAddItemsAtomically(
                {{ItemType::COAL, 3},
                 {ItemType::LOG, 2},
                 {ItemType::TIN_ORE, 1}}),
            "Multiple different rewards succeed");
        test.ExpectEqual(
            inventory.GetItemAmount(ItemType::COAL),
            3,
            "Stackable in mixed transaction is added");
        test.ExpectEqual(
            inventory.GetItemAmount(ItemType::LOG),
            2,
            "Non-stackable quantity in mixed transaction is added");
        test.ExpectEqual(
            inventory.GetItemAmount(ItemType::TIN_ORE),
            1,
            "Second non-stackable item in mixed transaction is added");
    }

    {
        Inventory inventory;

        test.Expect(
            inventory.TryAddItemsAtomically(
                {{ItemType::LOG, 2}}),
            "Multiple non-stackable items can be added in one entry");

        int firstLogSlot =
            FindFirstSlot(
                inventory,
                ItemType::LOG);

        test.Expect(
            firstLogSlot >= 0,
            "First non-stackable slot exists");

        const auto &slots = inventory.GetSlots();

        bool foundSecond = false;

        for (int index = 0; index < Inventory::SlotCount; ++index)
        {
            if (index == firstLogSlot)
            {
                continue;
            }

            if (!slots[index].IsEmpty() &&
                slots[index].GetItemType() == ItemType::LOG &&
                slots[index].GetAmount() == 1)
            {
                foundSecond = true;
                break;
            }
        }

        test.Expect(
            foundSecond,
            "Multiple non-stackable quantity uses separate slots");
    }

    {
        Inventory inventory;
        inventory.AddItem(ItemType::COAL, 7);

        test.Expect(
            inventory.TryAddItemsAtomically(
                {{ItemType::COAL, 5}}),
            "Existing stack accepts additional stackable quantity");
        test.ExpectEqual(
            inventory.GetItemAmount(ItemType::COAL),
            12,
            "Existing stack receives the full added quantity");
    }

    {
        Inventory inventory;
        Inventory before = inventory;

        test.Expect(
            !inventory.TryAddItemsAtomically(
                {{static_cast<ItemType>(9999), 1}}),
            "Invalid item causes complete failure");
        test.Expect(
            SameSlots(inventory, before),
            "Invalid item leaves all slots unchanged");
    }

    {
        Inventory inventory;
        Inventory before = inventory;

        test.Expect(
            !inventory.TryAddItemsAtomically(
                {{ItemType::COAL, 0}}),
            "Zero quantity causes complete failure");
        test.Expect(
            SameSlots(inventory, before),
            "Zero quantity leaves all slots unchanged");
    }

    {
        Inventory inventory;
        Inventory before = inventory;

        test.Expect(
            !inventory.TryAddItemsAtomically(
                {{ItemType::COAL, -3}}),
            "Negative quantity causes complete failure");
        test.Expect(
            SameSlots(inventory, before),
            "Negative quantity leaves all slots unchanged");
    }

    {
        Inventory inventory;
        inventory.AddItem(ItemType::LOG, Inventory::SlotCount);
        Inventory before = inventory;

        test.Expect(
            !inventory.TryAddItemsAtomically(
                {{ItemType::COAL, 1}}),
            "Insufficient capacity causes complete failure");
        test.Expect(
            SameSlots(inventory, before),
            "Insufficient capacity leaves all slots unchanged");
    }

    {
        Inventory inventory;
        inventory.AddItem(ItemType::COAL, INT_MAX);
        Inventory before = inventory;

        test.Expect(
            !inventory.TryAddItemsAtomically(
                {{ItemType::COAL, 1}}),
            "Quantity overflow fails");
        test.Expect(
            SameSlots(inventory, before),
            "Quantity overflow leaves all slots unchanged");
    }

    {
        Inventory inventory;

        test.Expect(
            inventory.TryAddItemsAtomically(
                {{ItemType::COAL, 1},
                 {ItemType::COPPER_ORE, 1}}),
            "Valid transaction commits all items");
        test.ExpectEqual(
            inventory.GetItemAmount(ItemType::COAL),
            1,
            "Valid transaction adds coal exactly once");
        test.ExpectEqual(
            inventory.GetItemAmount(ItemType::COPPER_ORE),
            1,
            "Valid transaction adds ore exactly once");
    }

    return test.Finish();
}
