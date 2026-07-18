#include "RequirementEvaluator.h"

#include "../Item/ItemDatabase.h"
#include "../Player/Player.h"

namespace RequirementSystem
{
    namespace
    {
        bool IsValidItemType(ItemType itemType)
        {
            if (itemType == ItemType::NONE)
            {
                return false;
            }

            return ItemDatabase::Get(itemType).GetItemType() == itemType;
        }

        RequirementResult InvalidRequirementResult()
        {
            return RequirementResult{
                false,
                "Invalid requirement"};
        }
    }

    std::optional<std::string_view> GetSkillName(
        SkillType skillType)
    {
        switch (skillType)
        {
        case SkillType::ATTACK:
            return "Attack";

        case SkillType::DEFENCE:
            return "Defence";

        case SkillType::WOODCUTTING:
            return "Woodcutting";

        case SkillType::MINING:
            return "Mining";

        case SkillType::SMITHING:
            return "Smithing";

        case SkillType::NONE:
        default:
            return std::nullopt;
        }
    }

    RequirementResult RequirementEvaluator::Evaluate(
        const Player &player,
        const Requirement &requirement)
    {
        return std::visit(
            [&](const auto &concreteRequirement)
            {
                return Evaluate(player, concreteRequirement);
            },
            requirement.data);
    }

    RequirementResult RequirementEvaluator::EvaluateAll(
        const Player &player,
        const std::vector<Requirement> &requirements)
    {
        for (const Requirement &requirement : requirements)
        {
            RequirementResult result = Evaluate(player, requirement);

            if (!result.satisfied)
            {
                return result;
            }
        }

        return RequirementResult{};
    }

    RequirementResult RequirementEvaluator::Evaluate(
        const Player &player,
        const SkillLevelRequirement &requirement)
    {
        std::optional<std::string_view> skillName =
            GetSkillName(requirement.skillType);

        if (!skillName.has_value() ||
            requirement.requiredLevel <= 0)
        {
            return InvalidRequirementResult();
        }

        const Skill &skill =
            player.GetSkills().GetSkill(
                requirement.skillType);

        if (skill.GetLevel() < requirement.requiredLevel)
        {
            return RequirementResult{
                false,
                "You need " +
                    std::string(*skillName) +
                    " level " +
                    std::to_string(
                        requirement.requiredLevel)};
        }

        return RequirementResult{};
    }

    RequirementResult RequirementEvaluator::Evaluate(
        const Player &player,
        const HeldItemRequirement &requirement)
    {
        if (!IsValidItemType(requirement.itemType) ||
            requirement.quantity <= 0)
        {
            return InvalidRequirementResult();
        }

        const Inventory &inventory =
            player.GetInventory();

        if (inventory.GetItemAmount(
                requirement.itemType) <
            requirement.quantity)
        {
            const ItemDefinition &definition =
                ItemDatabase::Get(requirement.itemType);

            return RequirementResult{
                false,
                "You need " +
                    std::to_string(requirement.quantity) +
                    " " +
                    definition.GetName()};
        }

        return RequirementResult{};
    }

    RequirementResult RequirementEvaluator::Evaluate(
        const Player &player,
        const EquippedItemRequirement &requirement)
    {
        if (!IsValidItemType(requirement.itemType))
        {
            return InvalidRequirementResult();
        }

        const Equipment &equipment =
            player.GetEquipment();

        if (!equipment.IsEquipped(
                requirement.itemType))
        {
            const ItemDefinition &definition =
                ItemDatabase::Get(requirement.itemType);

            return RequirementResult{
                false,
                "You need to equip " +
                    definition.GetName()};
        }

        return RequirementResult{};
    }
}
