#pragma once

#include "Requirement.h"

#include <vector>

class Player;

namespace RequirementSystem
{
    class RequirementEvaluator
    {
    public:
        static RequirementResult Evaluate(
            const Player &player,
            const Requirement &requirement);

        static RequirementResult EvaluateAll(
            const Player &player,
            const std::vector<Requirement> &requirements);

        static RequirementResult ValidateDefinition(
            const Requirement &requirement);

        static RequirementResult ValidateDefinitionAll(
            const std::vector<Requirement> &requirements);

    private:
        static RequirementResult ValidateDefinition(
            const SkillLevelRequirement &requirement);

        static RequirementResult ValidateDefinition(
            const HeldItemRequirement &requirement);

        static RequirementResult ValidateDefinition(
            const EquippedItemRequirement &requirement);

        static RequirementResult Evaluate(
            const Player &player,
            const SkillLevelRequirement &requirement);

        static RequirementResult Evaluate(
            const Player &player,
            const HeldItemRequirement &requirement);

        static RequirementResult Evaluate(
            const Player &player,
            const EquippedItemRequirement &requirement);
    };
}
