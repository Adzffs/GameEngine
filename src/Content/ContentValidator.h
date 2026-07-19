#pragma once

#include "ContentValidationReport.h"

#include "../Item/ItemDefinition.h"
#include "../Recipe/RecipeDefinition.h"
#include "../Reward/RewardTable.h"
#include "../Reward/RewardTableType.h"
#include "../Reward/GuaranteedRewardEntry.h"
#include "../Reward/WeightedRewardEntry.h"
#include "../Requirement/Requirement.h"
#include "../World/Development/DevelopmentWorldContent.h"
#include "../World/Object/Resource/ResourceDefinition.h"

class ContentValidator
{
public:
    static ContentValidationReport ValidateAll();

    static ContentValidationReport ValidateItemDefinition(
        ItemType registryItemType,
        const ItemDefinition &definition);

    static ContentValidationReport ValidateResourceDefinition(
        ResourceType registryResourceType,
        const ResourceDefinition &definition);

    static ContentValidationReport ValidateRecipeDefinition(
        RecipeType registryRecipeType,
        const RecipeDefinition &definition);

    static ContentValidationReport ValidateRewardTable(
        RewardTableType rewardTableType,
        const RewardTable &rewardTable);

    static ContentValidationReport ValidateRewardTableEntries(
        RewardTableType rewardTableType,
        const std::vector<GuaranteedRewardEntry> &guaranteedEntries,
        const std::vector<WeightedRewardEntry> &weightedEntries,
        int storedTotalWeight);

    static ContentValidationReport ValidateRewardTableRegistration(
        RewardTableType rewardTableType,
        const RewardTable *rewardTable);

    static ContentValidationReport ValidateMonsterSpawnDefinition(
        const DevelopmentMonsterSpawnDefinition &definition,
        int mapWidth,
        int mapHeight,
        bool spawnTileBlockedByObject = false);

    static ContentValidationReport ValidateResourceRequirementConsistency(
        const std::string &contentID,
        SkillType requiredSkill,
        int requiredLevel,
        const std::vector<RequirementSystem::Requirement> &requirements);

    static ContentValidationReport ValidateRecipeRequirementConsistency(
        const std::string &contentID,
        SkillType requiredSkill,
        int requiredLevel,
        const std::vector<RecipeIngredient> &ingredients,
        const std::vector<RequirementSystem::Requirement> &requirements);

private:
    static void ValidateAllItems(
        ContentValidationReport &report);

    static void ValidateAllResources(
        ContentValidationReport &report);

    static void ValidateAllRecipes(
        ContentValidationReport &report);

    static void ValidateAllRewardTables(
        ContentValidationReport &report);

    static void ValidateStarterWorldContent(
        ContentValidationReport &report);

    static void AppendItemValidation(
        ItemType registryItemType,
        const ItemDefinition &definition,
        ContentValidationReport &report);

    static void AppendResourceValidation(
        ResourceType registryResourceType,
        const ResourceDefinition &definition,
        ContentValidationReport &report);

    static void AppendRecipeValidation(
        RecipeType registryRecipeType,
        const RecipeDefinition &definition,
        ContentValidationReport &report);

    static void AppendRewardTableValidation(
        RewardTableType rewardTableType,
        const RewardTable &rewardTable,
        ContentValidationReport &report);

    static void AppendMonsterSpawnValidation(
        const DevelopmentMonsterSpawnDefinition &definition,
        int mapWidth,
        int mapHeight,
        bool spawnTileBlockedByObject,
        ContentValidationReport &report);
};
