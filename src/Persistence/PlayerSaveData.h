#pragma once

#include "../Equipment/EquipmentSlotType.h"
#include "../Inventory/Inventory.h"
#include "../Inventory/ItemType.h"
#include "../Skills/SkillType.h"

#include <array>
#include <vector>

inline constexpr int CURRENT_PLAYER_SAVE_VERSION = 1;

struct SavedInventorySlot
{
    ItemType itemType = ItemType::NONE;
    int quantity = 0;
};

struct SavedSkillXP
{
    SkillType skillType = SkillType::NONE;
    int xp = 0;
};

struct SavedEquipmentSlot
{
    EquipmentSlotType slotType = EquipmentSlotType::NONE;
    ItemType itemType = ItemType::NONE;
};

struct PlayerSaveData
{
    int version = CURRENT_PLAYER_SAVE_VERSION;
    int positionX = 0;
    int positionY = 0;
    int currentHealth = 1;
    std::array<SavedInventorySlot, Inventory::SlotCount> inventorySlots{};
    std::vector<SavedSkillXP> skills;
    std::vector<SavedEquipmentSlot> equipment;
};
