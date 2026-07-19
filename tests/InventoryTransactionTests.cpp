#include "TestSupport.h"

#include "../src/Inventory/Inventory.h"
#include "../src/Inventory/InventorySlot.h"
#include "../src/Inventory/ItemAmount.h"
#include "../src/Inventory/ItemType.h"
#include "../src/Item/ItemDatabase.h"

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
        test.Expect(
            !ItemDatabase::Get(ItemType::COAL).IsStackable(),
            "Coal is non-stackable");
        test.Expect(
            !ItemDatabase::Get(ItemType::COPPER_ORE).IsStackable(),
            "Copper ore is non-stackable");
        test.Expect(
            !ItemDatabase::Get(ItemType::TIN_ORE).IsStackable(),
            "Tin ore is non-stackable");
        test.Expect(
            !ItemDatabase::Get(ItemType::IRON_ORE).IsStackable(),
            "Iron ore is non-stackable");
        test.Expect(
            !ItemDatabase::Get(ItemType::BRONZE_BAR).IsStackable(),
            "Bronze bar is non-stackable");
        test.Expect(
            !ItemDatabase::Get(ItemType::IRON_BAR).IsStackable(),
            "Iron bar is non-stackable");
        test.Expect(
            !ItemDatabase::Get(ItemType::STEEL_BAR).IsStackable(),
            "Steel bar is non-stackable");
        test.Expect(
            ItemDatabase::Get(ItemType::COINS).IsStackable(),
            "Coins remain stackable");
    }

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
                {{ItemType::COINS, 5}}),
            "One stackable reward succeeds");
        test.ExpectEqual(
            inventory.GetItemAmount(ItemType::COINS),
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
                {{ItemType::COAL, 2}}),
            "Non-stackable material quantity succeeds");

        int firstCoalSlot =
            FindFirstSlot(
                inventory,
                ItemType::COAL);

        test.Expect(
            firstCoalSlot >= 0,
            "First coal slot exists");

        const auto &slots = inventory.GetSlots();

        bool foundSecondCoal = false;

        for (int index = 0; index < Inventory::SlotCount; ++index)
        {
            if (index == firstCoalSlot)
            {
                continue;
            }

            if (!slots[index].IsEmpty() &&
                slots[index].GetItemType() == ItemType::COAL &&
                slots[index].GetAmount() == 1)
            {
                foundSecondCoal = true;
                break;
            }
        }

        test.Expect(
            foundSecondCoal,
            "Adding quantity 2 of a non-stackable material uses two slots");
    }

    {
        Inventory inventory;

        test.Expect(
            inventory.TryAddItemsAtomically(
                {{ItemType::COINS, 3},
                 {ItemType::LOG, 2},
                 {ItemType::TIN_ORE, 1}}),
            "Multiple different rewards succeed");
        test.ExpectEqual(
            inventory.GetItemAmount(ItemType::COINS),
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
        inventory.AddItem(ItemType::COINS, 7);

        test.Expect(
            inventory.TryAddItemsAtomically(
                {{ItemType::COINS, 5}}),
            "Existing stack accepts additional stackable quantity");
        test.ExpectEqual(
            inventory.GetItemAmount(ItemType::COINS),
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
                {{ItemType::COINS, 0}}),
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
                {{ItemType::COINS, -3}}),
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
                {{ItemType::COINS, 1}}),
            "Insufficient capacity causes complete failure");
        test.Expect(
            SameSlots(inventory, before),
            "Insufficient capacity leaves all slots unchanged");
    }

    {
        Inventory inventory;
        inventory.AddItem(ItemType::COINS, INT_MAX);
        Inventory before = inventory;

        test.Expect(
            !inventory.TryAddItemsAtomically(
                {{ItemType::COINS, 1}}),
            "Quantity overflow fails");
        test.Expect(
            SameSlots(inventory, before),
            "Quantity overflow leaves all slots unchanged");
    }

    {
        Inventory inventory;

        test.Expect(
            inventory.TryAddItemsAtomically(
                {{ItemType::COINS, 1},
                 {ItemType::COPPER_ORE, 1}}),
            "Valid transaction commits all items");
        test.ExpectEqual(
            inventory.GetItemAmount(ItemType::COINS),
            1,
            "Valid transaction adds coins exactly once");
        test.ExpectEqual(
            inventory.GetItemAmount(ItemType::COPPER_ORE),
            1,
            "Valid transaction adds ore exactly once");
    }

    {
        Inventory inventory;
        inventory.AddItem(ItemType::COINS, 5);

        int coinSlot =
            FindFirstSlot(
                inventory,
                ItemType::COINS);

        test.Expect(
            coinSlot >= 0,
            "Stack slot exists for slot-removal failure tests");

        Inventory before = inventory;

        test.Expect(
            !inventory.RemoveItemFromSlot(
                -1,
                1),
            "Negative slot index fails");
        test.Expect(
            SameSlots(inventory, before),
            "Negative slot index leaves inventory unchanged");

        test.Expect(
            !inventory.RemoveItemFromSlot(
                Inventory::SlotCount,
                1),
            "Slot index equal to SlotCount fails");
        test.Expect(
            SameSlots(inventory, before),
            "SlotCount index failure leaves inventory unchanged");

        test.Expect(
            !inventory.RemoveItemFromSlot(
                Inventory::SlotCount + 1,
                1),
            "Slot index greater than SlotCount fails");
        test.Expect(
            SameSlots(inventory, before),
            "Out-of-range slot index leaves inventory unchanged");

        test.Expect(
            !inventory.RemoveItemFromSlot(
                coinSlot,
                0),
            "Quantity zero fails");
        test.Expect(
            SameSlots(inventory, before),
            "Zero quantity failure leaves inventory unchanged");

        test.Expect(
            !inventory.RemoveItemFromSlot(
                coinSlot,
                -3),
            "Negative quantity fails");
        test.Expect(
            SameSlots(inventory, before),
            "Negative quantity failure leaves inventory unchanged");

        test.Expect(
            !inventory.RemoveItemFromSlot(
                coinSlot,
                6),
            "Quantity greater than slot amount fails");
        test.Expect(
            SameSlots(inventory, before),
            "Excess quantity failure leaves inventory unchanged");

        int emptySlot = -1;

        const auto &slots =
            inventory.GetSlots();

        for (int index = 0; index < Inventory::SlotCount; ++index)
        {
            if (slots[index].IsEmpty())
            {
                emptySlot = index;
                break;
            }
        }

        test.Expect(
            emptySlot >= 0,
            "Empty slot exists for removal failure test");
        test.Expect(
            !inventory.RemoveItemFromSlot(
                emptySlot,
                1),
            "Empty slot removal fails");
        test.Expect(
            SameSlots(inventory, before),
            "Empty slot failure leaves inventory unchanged");
    }

    {
        Inventory inventory;

        test.Expect(
            inventory.AddItem(
                ItemType::COAL,
                2),
            "Two non-stackable items are added for slot-specific removal tests");

        int firstCoalSlot = -1;
        int secondCoalSlot = -1;

        const auto &beforeSlots =
            inventory.GetSlots();

        for (int index = 0; index < Inventory::SlotCount; ++index)
        {
            if (beforeSlots[index].IsEmpty() ||
                beforeSlots[index].GetItemType() != ItemType::COAL)
            {
                continue;
            }

            if (firstCoalSlot < 0)
            {
                firstCoalSlot = index;
            }
            else
            {
                secondCoalSlot = index;
                break;
            }
        }

        test.Expect(
            firstCoalSlot >= 0 && secondCoalSlot >= 0,
            "Duplicate non-stackable items occupy two distinct slots");

        Inventory before = inventory;

        test.Expect(
            inventory.RemoveItemFromSlot(
                firstCoalSlot,
                1),
            "Removing one non-stackable item succeeds");

        const auto &afterSlots =
            inventory.GetSlots();

        test.Expect(
            afterSlots[firstCoalSlot].IsEmpty(),
            "Removing one from a non-stackable slot empties that exact slot");
        test.Expect(
            !afterSlots[secondCoalSlot].IsEmpty() &&
                afterSlots[secondCoalSlot].GetItemType() == ItemType::COAL &&
                afterSlots[secondCoalSlot].GetAmount() == 1,
            "Removing one duplicate non-stackable item leaves the other slot unchanged");

        test.ExpectEqual(
            inventory.GetItemAmount(ItemType::COAL),
            before.GetItemAmount(ItemType::COAL) - 1,
            "Exactly one duplicate non-stackable item is removed");
    }

    {
        Inventory inventory;

        test.Expect(
            inventory.AddItem(
                ItemType::COINS,
                10),
            "Stackable item is added for selected-slot stack-removal tests");

        int coinSlot =
            FindFirstSlot(
                inventory,
                ItemType::COINS);

        test.Expect(
            coinSlot >= 0,
            "Coin stack slot exists");

        test.Expect(
            inventory.RemoveItemFromSlot(
                coinSlot,
                3),
            "Removing part of a stack succeeds");

        const auto &afterPartialRemoval =
            inventory.GetSlots();

        test.Expect(
            !afterPartialRemoval[coinSlot].IsEmpty() &&
                afterPartialRemoval[coinSlot].GetItemType() == ItemType::COINS,
            "Partial stack removal keeps selected slot occupied by same item type");
        test.ExpectEqual(
            afterPartialRemoval[coinSlot].GetAmount(),
            7,
            "Partial stack removal decreases only selected slot amount");

        test.Expect(
            inventory.RemoveItemFromSlot(
                coinSlot,
                7),
            "Removing the rest of the stack succeeds");

        const auto &afterFullRemoval =
            inventory.GetSlots();

        test.Expect(
            afterFullRemoval[coinSlot].IsEmpty(),
            "Removing entire stack empties selected slot");
    }

    {
        Inventory inventory;
        inventory.AddItem(ItemType::COAL, 1);
        inventory.AddItem(ItemType::COINS, 4);

        int coalSlot =
            FindFirstSlot(
                inventory,
                ItemType::COAL);
        int coinSlot =
            FindFirstSlot(
                inventory,
                ItemType::COINS);

        test.Expect(
            coalSlot >= 0 && coinSlot >= 0,
            "Slots exist for fallback-removal guard test");

        Inventory before = inventory;

        test.Expect(
            inventory.RemoveItemFromSlot(
                coalSlot,
                1),
            "Removing selected non-stackable slot succeeds");

        test.Expect(
            !inventory.RemoveItemFromSlot(
                coalSlot,
                1),
            "Second removal from now-empty selected slot fails");

        const auto &after =
            inventory.GetSlots();

        test.Expect(
            !after[coinSlot].IsEmpty() &&
                after[coinSlot].GetItemType() == ItemType::COINS &&
                after[coinSlot].GetAmount() == 4,
            "Failed removal from empty selected slot does not remove item from another slot");

        test.ExpectEqual(
            inventory.GetItemAmount(ItemType::COINS),
            before.GetItemAmount(ItemType::COINS),
            "No item-type fallback removes another matching or non-matching item");

        Inventory afterEmptyFail = inventory;

        test.Expect(
            !inventory.RemoveItemFromSlot(
                Inventory::SlotCount,
                1),
            "Invalid selected slot does not trigger fallback removal");
        test.Expect(
            SameSlots(inventory, afterEmptyFail),
            "Invalid selected slot failure leaves complete inventory unchanged");
    }

    return test.Finish();
}
