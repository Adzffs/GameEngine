#pragma once

#include "../Inventory/Inventory.h"
#include "../Item/ItemDatabase.h"

#include <cstdint>
#include <limits>
#include <map>
#include <vector>

struct BankEntry
{
    ItemType itemType = ItemType::NONE;
    int quantity = 0;
};

class Bank
{
public:
    static constexpr std::size_t DistinctItemCapacity = 120;

    std::uint64_t GetRevision() const { return revision; }

    int GetItemAmount(ItemType itemType) const
    {
        const auto found = items.find(itemType);
        return found == items.end() ? 0 : found->second;
    }

    std::vector<BankEntry> GetEntries() const
    {
        std::vector<BankEntry> entries;
        entries.reserve(items.size());
        for (const auto &[itemType, quantity] : items)
            entries.push_back({itemType, quantity});
        return entries;
    }

    bool TryDeposit(Inventory &inventory, int slotIndex, int quantity)
    {
        if (slotIndex < 0 || slotIndex >= Inventory::SlotCount || quantity <= 0)
            return false;
        const InventorySlot slot = inventory.GetSlots()[slotIndex];
        if (slot.IsEmpty() || slot.GetAmount() < quantity) return false;
        const auto found = items.find(slot.GetItemType());
        if (found == items.end() && items.size() >= DistinctItemCapacity)
            return false;
        const int current = found == items.end() ? 0 : found->second;
        if (current > std::numeric_limits<int>::max() - quantity) return false;
        if (!inventory.RemoveItemFromSlot(slotIndex, quantity)) return false;
        items[slot.GetItemType()] = current + quantity;
        ++revision;
        return true;
    }

    bool TryWithdraw(Inventory &inventory, ItemType itemType, int quantity)
    {
        const auto found = items.find(itemType);
        if (quantity <= 0 || found == items.end() || found->second < quantity ||
            !inventory.CanAddItem(itemType, quantity) ||
            !inventory.AddItem(itemType, quantity))
            return false;
        found->second -= quantity;
        if (found->second == 0) items.erase(found);
        ++revision;
        return true;
    }

    bool TryDepositInventory(Inventory &inventory)
    {
        Inventory simulatedInventory = inventory;
        Bank simulatedBank = *this;
        bool moved = false;
        for (int slotIndex = 0; slotIndex < Inventory::SlotCount; ++slotIndex)
        {
            const InventorySlot slot = simulatedInventory.GetSlots()[slotIndex];
            if (slot.IsEmpty()) continue;
            if (!simulatedBank.TryDeposit(
                    simulatedInventory, slotIndex, slot.GetAmount()))
                return false;
            moved = true;
        }
        if (!moved) return false;
        inventory = simulatedInventory;
        items = std::move(simulatedBank.items);
        ++revision;
        return true;
    }

    bool TryRestore(const std::vector<BankEntry> &entries)
    {
        Bank restored;
        for (const BankEntry &entry : entries)
        {
            if (entry.itemType == ItemType::NONE || entry.quantity <= 0 ||
                restored.items.contains(entry.itemType) ||
                restored.items.size() >= DistinctItemCapacity)
                return false;
            restored.items[entry.itemType] = entry.quantity;
        }
        items = std::move(restored.items);
        revision = 0;
        return true;
    }

private:
    std::map<ItemType, int> items;
    std::uint64_t revision = 0;
};
