#pragma once

#include <optional>
#include <string>
#include <string_view>
#include <variant>

#include "../Inventory/ItemType.h"
#include "../Skills/SkillType.h"

namespace RequirementSystem
{
    struct RequirementResult
    {
        bool satisfied = true;
        std::string message;
    };

    struct SkillLevelRequirement
    {
        SkillType skillType = SkillType::NONE;
        int requiredLevel = 0;
    };

    struct HeldItemRequirement
    {
        ItemType itemType = ItemType::NONE;
        int quantity = 0;
    };

    struct EquippedItemRequirement
    {
        ItemType itemType = ItemType::NONE;
    };

    using RequirementData = std::variant<
        SkillLevelRequirement,
        HeldItemRequirement,
        EquippedItemRequirement>;

    struct Requirement
    {
        RequirementData data;

        Requirement(const SkillLevelRequirement &requirement)
            : data(requirement)
        {
        }

        Requirement(const HeldItemRequirement &requirement)
            : data(requirement)
        {
        }

        Requirement(const EquippedItemRequirement &requirement)
            : data(requirement)
        {
        }
    };

    std::optional<std::string_view> GetSkillName(
        SkillType skillType);
}
