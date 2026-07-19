#pragma once

#include "../Equipment/EquipmentSlotType.h"
#include "../Skills/SkillType.h"

#include <array>
#include <cstddef>
#include <string>
#include <vector>

enum class PlayerSaveValidationCode
{
    UNSUPPORTED_VERSION,
    INVALID_ENTITY_ID,
    INVALID_CURRENT_HEALTH,
    NON_CANONICAL_EMPTY_INVENTORY_SLOT,
    UNKNOWN_INVENTORY_ITEM,
    INVALID_INVENTORY_QUANTITY,
    INVALID_NON_STACKABLE_QUANTITY,
    DUPLICATE_STACKABLE_ITEM,
    UNKNOWN_SKILL,
    DUPLICATE_SKILL,
    MISSING_SKILL,
    NEGATIVE_SKILL_XP,
    UNKNOWN_EQUIPMENT_SLOT,
    DUPLICATE_EQUIPMENT_SLOT,
    MISSING_EQUIPMENT_SLOT,
    UNKNOWN_EQUIPPED_ITEM,
    ITEM_NOT_EQUIPPABLE,
    EQUIPMENT_SLOT_MISMATCH,
    EQUIPMENT_REQUIREMENT_NOT_MET,
    INTERNAL_RESTORE_FAILURE
};

struct PlayerSaveValidationIssue
{
    PlayerSaveValidationCode code;
    int recordIndex = -1;
    std::string message;
};

class PlayerSaveValidationReport
{
public:
    bool IsValid() const;
    std::size_t GetErrorCount() const;
    const std::vector<PlayerSaveValidationIssue> &GetIssues() const;
    bool Contains(PlayerSaveValidationCode code) const;
    void AddIssue(
        PlayerSaveValidationCode code,
        int recordIndex,
        std::string message);

private:
    std::vector<PlayerSaveValidationIssue> issues;
};

inline constexpr std::array<SkillType, 5> PERSISTED_SKILL_TYPES{
    SkillType::ATTACK,
    SkillType::DEFENCE,
    SkillType::WOODCUTTING,
    SkillType::MINING,
    SkillType::SMITHING};

inline constexpr std::array<EquipmentSlotType, 5>
    PERSISTED_EQUIPMENT_SLOT_TYPES{
        EquipmentSlotType::HEAD,
        EquipmentSlotType::BODY,
        EquipmentSlotType::LEGS,
        EquipmentSlotType::WEAPON,
        EquipmentSlotType::SHIELD};
