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
#include "../NPC/NpcDefinition.h"
#include "../NPC/NpcSpawnDefinition.h"
#include "../Dialogue/DialogueDefinition.h"
#include "../Shop/ShopDefinition.h"

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

    static ContentValidationReport ValidateNpcDefinition(
        const NpcDefinition &definition);

    static ContentValidationReport ValidateNpcDefinitions(
        const std::vector<NpcDefinition> &definitions);

    static ContentValidationReport ValidateShopDefinition(
        ShopId registryShopId, const ShopDefinition &definition);
    static ContentValidationReport ValidateShopDefinitions(
        const std::vector<ShopDefinition> &definitions);

    static ContentValidationReport ValidateDialogueDefinition(
        const DialogueDefinition &definition);
    static ContentValidationReport ValidateDialogueDefinitions(
        const std::vector<DialogueDefinition> &definitions);

    static ContentValidationReport ValidateNpcSpawnDefinition(
        const NpcSpawnDefinition &definition,
        int mapWidth,
        int mapHeight,
        bool spawnTileBlockedByObject = false);

    static ContentValidationReport ValidateNpcSpawnDefinitions(
        const std::vector<NpcSpawnDefinition> &definitions,
        int mapWidth,
        int mapHeight);

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

    static void AppendNpcDefinitionValidation(
        const NpcDefinition &definition,
        ContentValidationReport &report);
    static void AppendShopDefinitionValidation(
        ShopId registryShopId, const ShopDefinition &definition,
        ContentValidationReport &report);

    static void AppendDialogueDefinitionValidation(
        const DialogueDefinition &definition,
        ContentValidationReport &report);

    static void AppendNpcSpawnValidation(
        const NpcSpawnDefinition &definition,
        int mapWidth,
        int mapHeight,
        bool spawnTileBlockedByObject,
        ContentValidationReport &report);
};
