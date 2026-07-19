#include "PlayerSaveState.h"

#include "../Inventory/InventorySlot.h"
#include "../Item/ItemDatabase.h"
#include "../Player/Player.h"
#include "../Player/PlayerInitializationMode.h"
#include "../Skills/Skill.h"

#include <algorithm>
#include <array>

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

    bool IsPersistedSkill(SkillType skillType)
    {
        return std::find(
                   PERSISTED_SKILL_TYPES.begin(),
                   PERSISTED_SKILL_TYPES.end(),
                   skillType) != PERSISTED_SKILL_TYPES.end();
    }

    bool IsPersistedEquipmentSlot(EquipmentSlotType slotType)
    {
        return std::find(
                   PERSISTED_EQUIPMENT_SLOT_TYPES.begin(),
                   PERSISTED_EQUIPMENT_SLOT_TYPES.end(),
                   slotType) != PERSISTED_EQUIPMENT_SLOT_TYPES.end();
    }

    const SavedSkillXP *FindSkill(
        const PlayerSaveData &saveData,
        SkillType skillType)
    {
        auto found = std::find_if(
            saveData.skills.begin(),
            saveData.skills.end(),
            [skillType](const SavedSkillXP &savedSkill)
            {
                return savedSkill.skillType == skillType;
            });
        return found == saveData.skills.end() ? nullptr : &*found;
    }

    int GetRestoredSkillLevel(
        const PlayerSaveData &saveData,
        SkillType skillType)
    {
        const SavedSkillXP *savedSkill = FindSkill(saveData, skillType);
        if (savedSkill == nullptr || savedSkill->xp < 0)
        {
            return 0;
        }

        Skill skill;
        skill.AddXP(savedSkill->xp);
        return skill.GetLevel();
    }
}

PlayerSaveData PlayerSaveState::Capture(const Player &player)
{
    PlayerSaveData saveData;
    saveData.positionX = player.GetPosition().GetX();
    saveData.positionY = player.GetPosition().GetY();
    saveData.currentHealth = player.GetCurrentHealth();

    const auto &inventorySlots = player.GetInventory().GetSlots();
    for (int index = 0; index < Inventory::SlotCount; ++index)
    {
        saveData.inventorySlots[index] = SavedInventorySlot{
            inventorySlots[index].GetItemType(),
            inventorySlots[index].GetAmount()};
    }

    for (SkillType skillType : PERSISTED_SKILL_TYPES)
    {
        saveData.skills.push_back(SavedSkillXP{
            skillType,
            player.GetSkills().GetSkill(skillType).GetXP()});
    }

    for (EquipmentSlotType slotType : PERSISTED_EQUIPMENT_SLOT_TYPES)
    {
        saveData.equipment.push_back(SavedEquipmentSlot{
            slotType,
            player.GetEquipment().GetEquippedItem(slotType)});
    }

    return saveData;
}

PlayerSaveValidationReport PlayerSaveState::ValidateInternal(
    const PlayerSaveData &saveData,
    bool validateEquipmentRequirements)
{
    PlayerSaveValidationReport report;

    if (saveData.version != CURRENT_PLAYER_SAVE_VERSION)
    {
        report.AddIssue(
            PlayerSaveValidationCode::UNSUPPORTED_VERSION,
            -1,
            "Only player save version 1 is supported");
    }
    if (saveData.currentHealth <= 0)
    {
        report.AddIssue(
            PlayerSaveValidationCode::INVALID_CURRENT_HEALTH,
            -1,
            "Current health must be positive");
    }

    std::vector<ItemType> stackableItems;
    for (int index = 0; index < Inventory::SlotCount; ++index)
    {
        const SavedInventorySlot &slot = saveData.inventorySlots[index];
        if (slot.itemType == ItemType::NONE)
        {
            if (slot.quantity != 0)
            {
                report.AddIssue(
                    PlayerSaveValidationCode::NON_CANONICAL_EMPTY_INVENTORY_SLOT,
                    index,
                    "Empty inventory slots must have quantity zero");
            }
            continue;
        }
        if (!IsKnownItem(slot.itemType))
        {
            report.AddIssue(
                PlayerSaveValidationCode::UNKNOWN_INVENTORY_ITEM,
                index,
                "Inventory item type is unknown");
            continue;
        }
        if (slot.quantity <= 0)
        {
            report.AddIssue(
                PlayerSaveValidationCode::INVALID_INVENTORY_QUANTITY,
                index,
                "Occupied inventory slots require a positive quantity");
            continue;
        }

        const ItemDefinition &definition = ItemDatabase::Get(slot.itemType);
        if (!definition.IsStackable() && slot.quantity != 1)
        {
            report.AddIssue(
                PlayerSaveValidationCode::INVALID_NON_STACKABLE_QUANTITY,
                index,
                "Non-stackable inventory items require quantity one");
        }
        if (definition.IsStackable())
        {
            if (std::find(stackableItems.begin(), stackableItems.end(), slot.itemType) !=
                stackableItems.end())
            {
                report.AddIssue(
                    PlayerSaveValidationCode::DUPLICATE_STACKABLE_ITEM,
                    index,
                    "A stackable item may occupy only one inventory slot");
            }
            else
            {
                stackableItems.push_back(slot.itemType);
            }
        }
    }

    std::vector<SkillType> seenSkills;
    for (std::size_t index = 0; index < saveData.skills.size(); ++index)
    {
        const SavedSkillXP &savedSkill = saveData.skills[index];
        if (!IsPersistedSkill(savedSkill.skillType))
        {
            report.AddIssue(
                PlayerSaveValidationCode::UNKNOWN_SKILL,
                static_cast<int>(index),
                "Skill type is not persisted by version 1");
            continue;
        }
        if (std::find(seenSkills.begin(), seenSkills.end(), savedSkill.skillType) !=
            seenSkills.end())
        {
            report.AddIssue(
                PlayerSaveValidationCode::DUPLICATE_SKILL,
                static_cast<int>(index),
                "Skill record is duplicated");
        }
        else
        {
            seenSkills.push_back(savedSkill.skillType);
        }
        if (savedSkill.xp < 0)
        {
            report.AddIssue(
                PlayerSaveValidationCode::NEGATIVE_SKILL_XP,
                static_cast<int>(index),
                "Skill XP cannot be negative");
        }
    }
    for (SkillType skillType : PERSISTED_SKILL_TYPES)
    {
        if (std::find(seenSkills.begin(), seenSkills.end(), skillType) == seenSkills.end())
        {
            report.AddIssue(
                PlayerSaveValidationCode::MISSING_SKILL,
                static_cast<int>(skillType),
                "A required persisted skill is missing");
        }
    }

    std::vector<EquipmentSlotType> seenSlots;
    for (std::size_t index = 0; index < saveData.equipment.size(); ++index)
    {
        const SavedEquipmentSlot &savedSlot = saveData.equipment[index];
        if (!IsPersistedEquipmentSlot(savedSlot.slotType))
        {
            report.AddIssue(
                PlayerSaveValidationCode::UNKNOWN_EQUIPMENT_SLOT,
                static_cast<int>(index),
                "Equipment slot type is not persisted by version 1");
            continue;
        }
        if (std::find(seenSlots.begin(), seenSlots.end(), savedSlot.slotType) !=
            seenSlots.end())
        {
            report.AddIssue(
                PlayerSaveValidationCode::DUPLICATE_EQUIPMENT_SLOT,
                static_cast<int>(index),
                "Equipment slot record is duplicated");
        }
        else
        {
            seenSlots.push_back(savedSlot.slotType);
        }

        if (savedSlot.itemType == ItemType::NONE)
        {
            continue;
        }
        if (!IsKnownItem(savedSlot.itemType))
        {
            report.AddIssue(
                PlayerSaveValidationCode::UNKNOWN_EQUIPPED_ITEM,
                static_cast<int>(index),
                "Equipped item type is unknown");
            continue;
        }

        const ItemDefinition &definition = ItemDatabase::Get(savedSlot.itemType);
        if (!definition.IsEquippable())
        {
            report.AddIssue(
                PlayerSaveValidationCode::ITEM_NOT_EQUIPPABLE,
                static_cast<int>(index),
                "Equipped item is not equippable");
            continue;
        }
        if (definition.GetEquipmentSlot() != savedSlot.slotType)
        {
            report.AddIssue(
                PlayerSaveValidationCode::EQUIPMENT_SLOT_MISMATCH,
                static_cast<int>(index),
                "Equipped item belongs to a different slot");
            continue;
        }
        if (definition.HasSkillRequirement() &&
            !IsPersistedSkill(definition.GetRequiredSkill()))
        {
            report.AddIssue(
                PlayerSaveValidationCode::INTERNAL_RESTORE_FAILURE,
                static_cast<int>(index),
                "Equipped item definition has an unsupported required skill");
        }
        else if (validateEquipmentRequirements &&
                 definition.HasSkillRequirement() &&
                 GetRestoredSkillLevel(saveData, definition.GetRequiredSkill()) <
                     definition.GetRequiredSkillLevel())
        {
            report.AddIssue(
                PlayerSaveValidationCode::EQUIPMENT_REQUIREMENT_NOT_MET,
                static_cast<int>(index),
                "Restored skill level does not meet equipment requirement");
        }
    }
    for (EquipmentSlotType slotType : PERSISTED_EQUIPMENT_SLOT_TYPES)
    {
        if (std::find(seenSlots.begin(), seenSlots.end(), slotType) == seenSlots.end())
        {
            report.AddIssue(
                PlayerSaveValidationCode::MISSING_EQUIPMENT_SLOT,
                static_cast<int>(slotType),
                "A required persisted equipment slot is missing");
        }
    }

    // Position is intentionally not map-validated here. World integration owns
    // bounds, blocked-tile, and fallback-spawn policy.
    return report;
}

PlayerSaveValidationReport PlayerSaveState::Validate(
    const PlayerSaveData &saveData)
{
    return ValidateInternal(saveData, true);
}

std::unique_ptr<Player> PlayerSaveState::TryCreatePlayer(
    int runtimeEntityID,
    const PlayerSaveData &saveData,
    PlayerSaveValidationReport &validationReport)
{
    validationReport = ValidateInternal(saveData, false);
    if (runtimeEntityID <= 0)
    {
        validationReport.AddIssue(
            PlayerSaveValidationCode::INVALID_ENTITY_ID,
            -1,
            "Runtime entity ID must be positive");
    }
    if (!validationReport.IsValid())
    {
        return nullptr;
    }

    std::unique_ptr<Player> player = std::make_unique<Player>(
        runtimeEntityID,
        PlayerInitializationMode::EMPTY);

    for (const SavedSkillXP &savedSkill : saveData.skills)
    {
        player->GetSkills().AddXP(savedSkill.skillType, savedSkill.xp);
    }

    for (std::size_t index = 0; index < saveData.equipment.size(); ++index)
    {
        const SavedEquipmentSlot &savedSlot = saveData.equipment[index];
        if (savedSlot.itemType == ItemType::NONE)
        {
            continue;
        }

        const ItemDefinition &definition = ItemDatabase::Get(savedSlot.itemType);
        if (definition.HasSkillRequirement() &&
            player->GetSkills()
                    .GetSkill(definition.GetRequiredSkill())
                    .GetLevel() < definition.GetRequiredSkillLevel())
        {
            validationReport.AddIssue(
                PlayerSaveValidationCode::EQUIPMENT_REQUIREMENT_NOT_MET,
                static_cast<int>(index),
                "Restored skill level does not meet equipment requirement");
        }
    }
    if (!validationReport.IsValid())
    {
        return nullptr;
    }

    std::array<InventorySlot, Inventory::SlotCount> replacementSlots;
    for (int index = 0; index < Inventory::SlotCount; ++index)
    {
        replacementSlots[index].SetItem(
            saveData.inventorySlots[index].itemType,
            saveData.inventorySlots[index].quantity);
    }
    if (!player->GetInventory().TryReplaceSlotsAtomically(replacementSlots))
    {
        validationReport.AddIssue(
            PlayerSaveValidationCode::INTERNAL_RESTORE_FAILURE,
            -1,
            "Validated inventory could not be restored");
        return nullptr;
    }

    for (const SavedEquipmentSlot &savedSlot : saveData.equipment)
    {
        if (savedSlot.itemType != ItemType::NONE &&
            !player->GetEquipment().Equip(savedSlot.slotType, savedSlot.itemType))
        {
            validationReport.AddIssue(
                PlayerSaveValidationCode::INTERNAL_RESTORE_FAILURE,
                -1,
                "Validated equipment could not be restored");
            return nullptr;
        }
    }

    player->RefreshDerivedState();
    player->RestoreHealthToFull();
    const int restoredHealth = std::min(
        saveData.currentHealth,
        player->GetMaximumHealth());
    player->ApplyDamage(player->GetMaximumHealth() - restoredHealth);
    player->GetPosition().SetPosition(saveData.positionX, saveData.positionY);
    return player;
}
