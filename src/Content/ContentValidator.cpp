#include "ContentValidator.h"

#include "../Equipment/EquipmentSlotType.h"
#include "../Item/ItemDatabase.h"
#include "../Item/ToolType.h"
#include "../Recipe/RecipeDatabase.h"
#include "../Recipe/RecipeIngredient.h"
#include "../Requirement/RequirementEvaluator.h"
#include "../Reward/RewardTableRegistry.h"
#include "../NPC/NpcDefinitionDatabase.h"
#include "../NPC/NpcSpawnDatabase.h"
#include "../Skills/SkillType.h"
#include "../Stats/StatType.h"
#include "../World/Map.h"
#include "../World/Object/Resource/DepletedVisualType.h"
#include "../World/Object/Resource/ResourceDatabase.h"
#include "../World/Object/Station/StationType.h"

#include <limits>
#include <set>
#include <string>
#include <tuple>

namespace
{
    using GridPosition = std::pair<int, int>;

    bool IsKnownSkillType(SkillType skillType)
    {
        return skillType != SkillType::NONE &&
               RequirementSystem::GetSkillName(skillType).has_value();
    }

    bool IsValidEquipmentSlot(EquipmentSlotType equipmentSlotType)
    {
        switch (equipmentSlotType)
        {
        case EquipmentSlotType::HEAD:
        case EquipmentSlotType::BODY:
        case EquipmentSlotType::LEGS:
        case EquipmentSlotType::WEAPON:
        case EquipmentSlotType::SHIELD:
            return true;

        case EquipmentSlotType::NONE:
        case EquipmentSlotType::COUNT:
        default:
            return false;
        }
    }

    bool IsValidToolType(ToolType toolType)
    {
        switch (toolType)
        {
        case ToolType::AXE:
        case ToolType::PICKAXE:
            return true;

        case ToolType::NONE:
        default:
            return false;
        }
    }

    bool IsValidStationType(StationType stationType)
    {
        return stationType == StationType::FURNACE;
    }

    bool IsValidDepletedVisualType(DepletedVisualType visualType)
    {
        return visualType == DepletedVisualType::STUMP ||
               visualType == DepletedVisualType::ROCK_RUBBLE;
    }

    bool IsValidItemType(ItemType itemType)
    {
        if (itemType == ItemType::NONE)
        {
            return false;
        }

        return ItemDatabase::Get(itemType).GetItemType() == itemType;
    }

    std::string ItemTypeToString(ItemType itemType)
    {
        switch (itemType)
        {
        case ItemType::NONE:
            return "NONE";
        case ItemType::LOG:
            return "LOG";
        case ItemType::OAK_LOG:
            return "OAK_LOG";
        case ItemType::WILLOW_LOG:
            return "WILLOW_LOG";
        case ItemType::COPPER_ORE:
            return "COPPER_ORE";
        case ItemType::TIN_ORE:
            return "TIN_ORE";
        case ItemType::IRON_ORE:
            return "IRON_ORE";
        case ItemType::COAL:
            return "COAL";
        case ItemType::BRONZE_BAR:
            return "BRONZE_BAR";
        case ItemType::IRON_BAR:
            return "IRON_BAR";
        case ItemType::STEEL_BAR:
            return "STEEL_BAR";
        case ItemType::COINS:
            return "COINS";
        case ItemType::COOKED_MEAT:
            return "COOKED_MEAT";
        case ItemType::BRONZE_AXE:
            return "BRONZE_AXE";
        case ItemType::IRON_AXE:
            return "IRON_AXE";
        case ItemType::STEEL_AXE:
            return "STEEL_AXE";
        case ItemType::BRONZE_PICKAXE:
            return "BRONZE_PICKAXE";
        case ItemType::IRON_PICKAXE:
            return "IRON_PICKAXE";
        case ItemType::STEEL_PICKAXE:
            return "STEEL_PICKAXE";
        case ItemType::BRONZE_SWORD:
            return "BRONZE_SWORD";
        case ItemType::DEVELOPER_GODSWORD:
            return "DEVELOPER_GODSWORD";
        case ItemType::WOODEN_SHIELD:
            return "WOODEN_SHIELD";
        }

        return "UNKNOWN_ITEM";
    }

    std::string ResourceTypeToString(ResourceType resourceType)
    {
        switch (resourceType)
        {
        case ResourceType::NORMAL_TREE:
            return "NORMAL_TREE";
        case ResourceType::OAK_TREE:
            return "OAK_TREE";
        case ResourceType::WILLOW_TREE:
            return "WILLOW_TREE";
        case ResourceType::COPPER_ROCK:
            return "COPPER_ROCK";
        case ResourceType::TIN_ROCK:
            return "TIN_ROCK";
        case ResourceType::IRON_ROCK:
            return "IRON_ROCK";
        case ResourceType::COAL_ROCK:
            return "COAL_ROCK";
        }

        return "UNKNOWN_RESOURCE";
    }

    std::string RecipeTypeToString(RecipeType recipeType)
    {
        switch (recipeType)
        {
        case RecipeType::NONE:
            return "NONE";
        case RecipeType::BRONZE_BAR:
            return "BRONZE_BAR";
        case RecipeType::IRON_BAR:
            return "IRON_BAR";
        case RecipeType::STEEL_BAR:
            return "STEEL_BAR";
        }

        return "UNKNOWN_RECIPE";
    }

    std::string RewardTableTypeToString(RewardTableType rewardTableType)
    {
        switch (rewardTableType)
        {
        case RewardTableType::NONE:
            return "NONE";
        case RewardTableType::DEVELOPMENT_MONSTER:
            return "DEVELOPMENT_MONSTER";
        }

        return "UNKNOWN_REWARD_TABLE";
    }

    std::string PositionToString(int x, int y)
    {
        return "(" + std::to_string(x) + "," + std::to_string(y) + ")";
    }

    void ValidateDefinitionRequirements(
        const std::string &category,
        const std::string &contentID,
        const std::vector<RequirementSystem::Requirement> &requirements,
        ContentValidationReport &report)
    {
        for (int index = 0;
             index < static_cast<int>(requirements.size());
             ++index)
        {
            RequirementSystem::RequirementResult requirementResult =
                RequirementSystem::RequirementEvaluator::ValidateDefinition(
                    requirements[index]);

            if (!requirementResult.satisfied)
            {
                report.AddError(
                    category,
                    contentID,
                    "definition requirement[" + std::to_string(index) + "] is invalid");
            }
        }
    }
}

ContentValidationReport ContentValidator::ValidateAll()
{
    ContentValidationReport report;

    ValidateAllItems(report);
    ValidateAllResources(report);
    ValidateAllRecipes(report);
    ValidateAllRewardTables(report);
    ValidateStarterWorldContent(report);

    return report;
}

ContentValidationReport ContentValidator::ValidateItemDefinition(
    ItemType registryItemType,
    const ItemDefinition &definition)
{
    ContentValidationReport report;
    AppendItemValidation(
        registryItemType,
        definition,
        report);
    return report;
}

ContentValidationReport ContentValidator::ValidateResourceDefinition(
    ResourceType registryResourceType,
    const ResourceDefinition &definition)
{
    ContentValidationReport report;
    AppendResourceValidation(
        registryResourceType,
        definition,
        report);
    return report;
}

ContentValidationReport ContentValidator::ValidateRecipeDefinition(
    RecipeType registryRecipeType,
    const RecipeDefinition &definition)
{
    ContentValidationReport report;
    AppendRecipeValidation(
        registryRecipeType,
        definition,
        report);
    return report;
}

ContentValidationReport ContentValidator::ValidateRewardTable(
    RewardTableType rewardTableType,
    const RewardTable &rewardTable)
{
    return ValidateRewardTableEntries(
        rewardTableType,
        rewardTable.GetGuaranteedEntries(),
        rewardTable.GetWeightedEntries(),
        rewardTable.GetTotalWeight());
}

ContentValidationReport ContentValidator::ValidateRewardTableEntries(
    RewardTableType rewardTableType,
    const std::vector<GuaranteedRewardEntry> &guaranteedEntries,
    const std::vector<WeightedRewardEntry> &weightedEntries,
    int storedTotalWeight)
{
    ContentValidationReport report;

    const std::string contentID =
        RewardTableTypeToString(rewardTableType);

    if (rewardTableType == RewardTableType::NONE)
    {
        report.AddError(
            "RewardTable",
            contentID,
            "reward-table type must be real content");
    }

    for (const GuaranteedRewardEntry &entry :
         guaranteedEntries)
    {
        if (!IsValidItemType(entry.itemType))
        {
            report.AddError(
                "RewardTable",
                contentID,
                "guaranteed reward item is invalid");
        }

        if (entry.minimumQuantity < 1 ||
            entry.maximumQuantity < entry.minimumQuantity)
        {
            report.AddError(
                "RewardTable",
                contentID,
                "guaranteed reward quantity range is invalid");
        }
    }

    long long totalWeight = 0;

    for (const WeightedRewardEntry &entry :
         weightedEntries)
    {
        if (!IsValidItemType(entry.itemType))
        {
            report.AddError(
                "RewardTable",
                contentID,
                "weighted reward item is invalid");
        }

        if (entry.minimumQuantity < 1 ||
            entry.maximumQuantity < entry.minimumQuantity)
        {
            report.AddError(
                "RewardTable",
                contentID,
                "weighted reward quantity range is invalid");
        }

        if (entry.weight <= 0)
        {
            report.AddError(
                "RewardTable",
                contentID,
                "weighted reward entry must have positive weight");
        }

        totalWeight += static_cast<long long>(entry.weight);
    }

    if (!weightedEntries.empty() &&
        totalWeight <= 0)
    {
        report.AddError(
            "RewardTable",
            contentID,
            "total weight must be positive when weighted entries exist");
    }

    if (totalWeight > static_cast<long long>(std::numeric_limits<int>::max()))
    {
        report.AddError(
            "RewardTable",
            contentID,
            "total weight overflow is unsafe");
    }
    else if (storedTotalWeight !=
             static_cast<int>(totalWeight))
    {
        report.AddError(
            "RewardTable",
            contentID,
            "stored total weight must match weighted entries");
    }

    if (rewardTableType == RewardTableType::DEVELOPMENT_MONSTER)
    {
        if (guaranteedEntries.empty() &&
            weightedEntries.empty())
        {
            report.AddError(
                "RewardTable",
                contentID,
                "development monster reward table must contain at least one reward");
        }
    }

    return report;
}

ContentValidationReport ContentValidator::ValidateRewardTableRegistration(
    RewardTableType rewardTableType,
    const RewardTable *rewardTable)
{
    ContentValidationReport report;

    if (rewardTableType == RewardTableType::NONE)
    {
        report.AddError(
            "RewardTable",
            RewardTableTypeToString(rewardTableType),
            "reward-table type must be real content");

        return report;
    }

    if (rewardTable == nullptr)
    {
        report.AddError(
            "RewardTable",
            RewardTableTypeToString(rewardTableType),
            "registry mapping is missing");
        return report;
    }

    return ValidateRewardTable(
        rewardTableType,
        *rewardTable);
}

ContentValidationReport ContentValidator::ValidateNpcDefinition(
    const NpcDefinition &definition)
{
    ContentValidationReport report;
    AppendNpcDefinitionValidation(definition, report);
    return report;
}

ContentValidationReport ContentValidator::ValidateNpcDefinitions(
    const std::vector<NpcDefinition> &definitions)
{
    ContentValidationReport report;
    std::set<NpcType> types;

    for (const NpcDefinition &definition : definitions)
    {
        if (!types.insert(definition.type).second)
        {
            report.AddError(
                "NpcDefinition",
                std::to_string(static_cast<int>(definition.type)),
                "definition ID must be unique");
        }
        AppendNpcDefinitionValidation(definition, report);
    }

    return report;
}

ContentValidationReport ContentValidator::ValidateNpcSpawnDefinition(
    const NpcSpawnDefinition &definition,
    int mapWidth,
    int mapHeight,
    bool spawnTileBlockedByObject)
{
    ContentValidationReport report;

    AppendNpcSpawnValidation(
        definition,
        mapWidth,
        mapHeight,
        spawnTileBlockedByObject,
        report);

    return report;
}

ContentValidationReport ContentValidator::ValidateNpcSpawnDefinitions(
    const std::vector<NpcSpawnDefinition> &definitions,
    int mapWidth,
    int mapHeight)
{
    ContentValidationReport report;
    std::set<std::tuple<NpcType, int, int, int, bool>> records;

    for (const NpcSpawnDefinition &definition : definitions)
    {
        AppendNpcSpawnValidation(
            definition,
            mapWidth,
            mapHeight,
            false,
            report);

        const auto record = std::make_tuple(
            definition.npcType,
            definition.spawnPosition.GetX(),
            definition.spawnPosition.GetY(),
            definition.wanderRadius,
            definition.respawns);
        if (!records.insert(record).second)
        {
            report.AddError(
                "StarterMonsterSpawn",
                PositionToString(
                    definition.spawnPosition.GetX(),
                    definition.spawnPosition.GetY()),
                "duplicate monster spawn definition");
        }
    }

    return report;
}

ContentValidationReport ContentValidator::ValidateResourceRequirementConsistency(
    const std::string &contentID,
    SkillType requiredSkill,
    int requiredLevel,
    const std::vector<RequirementSystem::Requirement> &requirements)
{
    ContentValidationReport report;

    int matchingSkillRequirements = 0;

    for (const RequirementSystem::Requirement &requirement :
         requirements)
    {
        const RequirementSystem::SkillLevelRequirement *skillRequirement =
            std::get_if<RequirementSystem::SkillLevelRequirement>(
                &requirement.data);

        if (skillRequirement == nullptr)
        {
            continue;
        }

        if (skillRequirement->skillType == requiredSkill &&
            skillRequirement->requiredLevel == requiredLevel)
        {
            matchingSkillRequirements++;
        }
        else
        {
            report.AddError(
                "Resource",
                contentID,
                "definition skill requirement does not match legacy required-skill fields");
        }
    }

    if (matchingSkillRequirements != 1)
    {
        report.AddError(
            "Resource",
            contentID,
            "definition must contain exactly one matching skill requirement");
    }

    return report;
}

ContentValidationReport ContentValidator::ValidateRecipeRequirementConsistency(
    const std::string &contentID,
    SkillType requiredSkill,
    int requiredLevel,
    const std::vector<RecipeIngredient> &ingredients,
    const std::vector<RequirementSystem::Requirement> &requirements)
{
    ContentValidationReport report;

    int matchingSkillRequirements = 0;
    std::set<std::pair<ItemType, int>> expectedHeldItemRequirements;
    std::set<std::pair<ItemType, int>> actualHeldItemRequirements;

    for (const RecipeIngredient &ingredient :
         ingredients)
    {
        expectedHeldItemRequirements.insert(
            std::pair<ItemType, int>{
                ingredient.itemType,
                ingredient.amount});
    }

    for (const RequirementSystem::Requirement &requirement :
         requirements)
    {
        if (const RequirementSystem::SkillLevelRequirement *skillRequirement =
                std::get_if<RequirementSystem::SkillLevelRequirement>(
                    &requirement.data);
            skillRequirement != nullptr)
        {
            if (skillRequirement->skillType == requiredSkill &&
                skillRequirement->requiredLevel == requiredLevel)
            {
                matchingSkillRequirements++;
            }
            else
            {
                report.AddError(
                    "Recipe",
                    contentID,
                    "definition skill requirement does not match legacy required-skill fields");
            }

            continue;
        }

        if (const RequirementSystem::HeldItemRequirement *heldRequirement =
                std::get_if<RequirementSystem::HeldItemRequirement>(
                    &requirement.data);
            heldRequirement != nullptr)
        {
            actualHeldItemRequirements.insert(
                std::pair<ItemType, int>{
                    heldRequirement->itemType,
                    heldRequirement->quantity});
        }
    }

    if (matchingSkillRequirements != 1)
    {
        report.AddError(
            "Recipe",
            contentID,
            "definition must contain exactly one matching skill requirement");
    }

    if (expectedHeldItemRequirements !=
        actualHeldItemRequirements)
    {
        report.AddError(
            "Recipe",
            contentID,
            "held-item requirements must exactly match ingredient list");
    }

    return report;
}

void ContentValidator::ValidateAllItems(
    ContentValidationReport &report)
{
    for (ItemType itemType :
         ItemDatabase::GetAllItemTypes())
    {
        const ItemDefinition &definition =
            ItemDatabase::Get(itemType);

        AppendItemValidation(
            itemType,
            definition,
            report);
    }
}

void ContentValidator::ValidateAllResources(
    ContentValidationReport &report)
{
    for (ResourceType resourceType :
         ResourceDatabase::GetAllResourceTypes())
    {
        const ResourceDefinition &definition =
            ResourceDatabase::Get(resourceType);

        AppendResourceValidation(
            resourceType,
            definition,
            report);
    }
}

void ContentValidator::ValidateAllRecipes(
    ContentValidationReport &report)
{
    for (RecipeType recipeType :
         RecipeDatabase::GetAllRecipeTypes())
    {
        const RecipeDefinition &definition =
            RecipeDatabase::Get(recipeType);

        AppendRecipeValidation(
            recipeType,
            definition,
            report);
    }
}

void ContentValidator::ValidateAllRewardTables(
    ContentValidationReport &report)
{
    for (RewardTableType rewardTableType :
         RewardTableRegistry::GetAllRewardTableTypes())
    {
        const RewardTable *rewardTable =
            RewardTableRegistry::TryGetRewardTable(
                rewardTableType);

        ContentValidationReport registrationReport =
            ValidateRewardTableRegistration(
                rewardTableType,
                rewardTable);

        for (const ContentValidationIssue &issue :
             registrationReport.GetIssues())
        {
            if (issue.severity ==
                ContentValidationSeverity::ERROR)
            {
                report.AddError(
                    issue.category,
                    issue.contentID,
                    issue.message);
            }
            else
            {
                report.AddWarning(
                    issue.category,
                    issue.contentID,
                    issue.message);
            }
        }
    }
}

void ContentValidator::ValidateStarterWorldContent(
    ContentValidationReport &report)
{
    if (DevelopmentWorldContent::MapWidth <= 0 ||
        DevelopmentWorldContent::MapHeight <= 0)
    {
        report.AddError(
            "StarterWorld",
            "MAP",
            "map dimensions must be positive");

        return;
    }

    Map map(
        DevelopmentWorldContent::MapWidth,
        DevelopmentWorldContent::MapHeight);

    std::set<GridPosition> blockedObjectPositions;

    for (const DevelopmentResourcePlacementDefinition &resourcePlacement :
         DevelopmentWorldContent::GetStarterResourcePlacements())
    {
        const std::string contentID =
            ResourceTypeToString(resourcePlacement.resourceType) + "@" +
            PositionToString(resourcePlacement.x, resourcePlacement.y);

        const ResourceDefinition &resourceDefinition =
            ResourceDatabase::Get(resourcePlacement.resourceType);

        if (resourceDefinition.GetResourceType() !=
            resourcePlacement.resourceType)
        {
            report.AddError(
                "StarterResourcePlacement",
                contentID,
                "resource type is not registered");
        }

        if (!map.IsValidPosition(
                resourcePlacement.x,
                resourcePlacement.y))
        {
            report.AddError(
                "StarterResourcePlacement",
                contentID,
                "resource placement must be on a valid walkable tile");
            continue;
        }

        if (!blockedObjectPositions.insert(
                                       GridPosition{
                                           resourcePlacement.x,
                                           resourcePlacement.y})
                 .second)
        {
            report.AddError(
                "StarterResourcePlacement",
                contentID,
                "resource placement overlaps another blocked object placement");
        }
    }

    for (const DevelopmentStationPlacementDefinition &stationPlacement :
         DevelopmentWorldContent::GetStarterStationPlacements())
    {
        const std::string contentID =
            PositionToString(
                stationPlacement.x,
                stationPlacement.y);

        if (!IsValidStationType(stationPlacement.stationType))
        {
            report.AddError(
                "StarterStationPlacement",
                contentID,
                "station type is invalid");
        }

        if (!map.IsValidPosition(
                stationPlacement.x,
                stationPlacement.y))
        {
            report.AddError(
                "StarterStationPlacement",
                contentID,
                "station placement must be on a valid walkable tile");
            continue;
        }

        if (!blockedObjectPositions.insert(
                                       GridPosition{
                                           stationPlacement.x,
                                           stationPlacement.y})
                 .second)
        {
            report.AddError(
                "StarterStationPlacement",
                contentID,
                "station placement overlaps another blocked object placement");
        }
    }

    std::set<GridPosition> occupiedEntitySpawnPositions;

    for (const DevelopmentNpcSpawnDefinition &npcSpawn :
         DevelopmentWorldContent::GetStarterNPCSpawns())
    {
        const std::string contentID =
            PositionToString(
                npcSpawn.spawnX,
                npcSpawn.spawnY);

        if (!map.IsValidPosition(
                npcSpawn.spawnX,
                npcSpawn.spawnY))
        {
            report.AddError(
                "StarterNPCSpawn",
                contentID,
                "NPC spawn must be on a valid walkable tile");
            continue;
        }

        if (blockedObjectPositions.find(
                GridPosition{
                    npcSpawn.spawnX,
                    npcSpawn.spawnY}) !=
            blockedObjectPositions.end())
        {
            report.AddError(
                "StarterNPCSpawn",
                contentID,
                "NPC spawn cannot overlap blocked starter object placement");
        }

        if (!occupiedEntitySpawnPositions.insert(
                                             GridPosition{
                                                 npcSpawn.spawnX,
                                                 npcSpawn.spawnY})
                 .second)
        {
            report.AddError(
                "StarterNPCSpawn",
                contentID,
                "multiple starter entities cannot share one spawn tile");
        }
    }

    std::set<NpcType> registeredNpcTypes;
    for (NpcType npcType : NpcDefinitionDatabase::GetAllNpcTypes())
    {
        if (!registeredNpcTypes.insert(npcType).second)
        {
            report.AddError("NpcDefinition", "duplicate", "definition ID must be unique");
            continue;
        }

        const NpcDefinition *definition = NpcDefinitionDatabase::TryGet(npcType);
        if (definition == nullptr)
        {
            report.AddError("NpcDefinition", "missing", "registered definition must resolve");
            continue;
        }
        AppendNpcDefinitionValidation(*definition, report);
    }

    std::set<std::pair<int, int>> monsterSpawnPositions;
    for (const NpcSpawnDefinition &monsterSpawn :
         NpcSpawnDatabase::GetStarterMonsterSpawns())
    {
        const GridPosition spawnPosition{
            monsterSpawn.spawnPosition.GetX(),
            monsterSpawn.spawnPosition.GetY()};

        const bool blockedByObject =
            blockedObjectPositions.find(spawnPosition) !=
            blockedObjectPositions.end();

        AppendNpcSpawnValidation(
            monsterSpawn,
            DevelopmentWorldContent::MapWidth,
            DevelopmentWorldContent::MapHeight,
            blockedByObject,
            report);

        if (!occupiedEntitySpawnPositions.insert(spawnPosition)
                 .second)
        {
            report.AddError(
                "StarterMonsterSpawn",
                PositionToString(
                    monsterSpawn.spawnPosition.GetX(),
                    monsterSpawn.spawnPosition.GetY()),
                "multiple starter entities cannot share one spawn tile");
        }

        const std::pair<int, int> spawnKey{
            monsterSpawn.spawnPosition.GetX(),
            monsterSpawn.spawnPosition.GetY()};
        if (!monsterSpawnPositions.insert(spawnKey).second)
        {
            report.AddError("StarterMonsterSpawn", PositionToString(spawnKey.first, spawnKey.second), "duplicate monster spawn definition");
        }
    }
}

void ContentValidator::AppendItemValidation(
    ItemType registryItemType,
    const ItemDefinition &definition,
    ContentValidationReport &report)
{
    const std::string contentID =
        ItemTypeToString(registryItemType);

    if (registryItemType == ItemType::NONE)
    {
        report.AddError(
            "Item",
            contentID,
            "registry item ID must be real content");
    }

    if (definition.GetItemType() == ItemType::NONE)
    {
        report.AddError(
            "Item",
            contentID,
            "definition item ID must be real content");
    }

    if (definition.GetItemType() != registryItemType)
    {
        report.AddError(
            "Item",
            contentID,
            "definition item ID must match registry key");
    }

    if (definition.GetName().empty())
    {
        report.AddError(
            "Item",
            contentID,
            "name must be non-empty");
    }

    EquipmentSlotType equipmentSlot =
        definition.GetEquipmentSlot();

    if (equipmentSlot != EquipmentSlotType::NONE &&
        !IsValidEquipmentSlot(equipmentSlot))
    {
        report.AddError(
            "Item",
            contentID,
            "equipment slot is invalid");
    }

    if (definition.IsEquippable() &&
        !IsValidEquipmentSlot(equipmentSlot))
    {
        report.AddError(
            "Item",
            contentID,
            "equippable item must use a real equipment slot");
    }

    if (!definition.IsEquippable() &&
        equipmentSlot != EquipmentSlotType::NONE)
    {
        report.AddError(
            "Item",
            contentID,
            "non-equippable item must use EquipmentSlotType::NONE");
    }

    ToolType toolType =
        definition.GetToolType();

    if (toolType != ToolType::NONE &&
        equipmentSlot == EquipmentSlotType::NONE)
    {
        report.AddError(
            "Item",
            contentID,
            "tool items must use a real equipment slot");
    }

    if (toolType != ToolType::NONE &&
        !IsValidToolType(toolType))
    {
        report.AddError(
            "Item",
            contentID,
            "tool type is invalid");
    }

    const int actionDurationTicks =
        definition.GetActionDurationTicks();

    if (actionDurationTicks < 0)
    {
        report.AddError(
            "Item",
            contentID,
            "action duration must be non-negative");
    }

    if ((toolType == ToolType::AXE ||
         toolType == ToolType::PICKAXE) &&
        actionDurationTicks <= 0)
    {
        report.AddError(
            "Item",
            contentID,
            "gathering tool duration must be positive");
    }

    if (definition.IsEquippable() &&
        definition.IsStackable())
    {
        report.AddError(
            "Item",
            contentID,
            "equippable items must not be stackable");
    }

    const StatBlock &bonuses =
        definition.GetEquipmentStatBonuses();

    for (int rawStatType = 0;
         rawStatType < static_cast<int>(StatType::COUNT);
         ++rawStatType)
    {
        StatType statType =
            static_cast<StatType>(rawStatType);

        if (bonuses.Get(statType) < 0)
        {
            report.AddError(
                "Item",
                contentID,
                "equipment stat bonuses must not be negative");
            break;
        }
    }

    if (definition.IsFood())
    {
        const FoodDefinition *foodDefinition =
            definition.GetFoodDefinition();

        if (foodDefinition == nullptr)
        {
            report.AddError(
                "Item",
                contentID,
                "food items must have a valid food definition");
        }
        else if (foodDefinition->healAmount <= 0)
        {
            report.AddError(
                "Item",
                contentID,
                "food healing amount must be positive");
        }

        if (definition.IsEquippable())
        {
            report.AddError(
                "Item",
                contentID,
                "food items must not be equippable");
        }

        if (definition.GetEquipmentSlot() !=
            EquipmentSlotType::NONE)
        {
            report.AddError(
                "Item",
                contentID,
                "food items must use EquipmentSlotType::NONE");
        }

        if (definition.GetToolType() !=
            ToolType::NONE)
        {
            report.AddError(
                "Item",
                contentID,
                "food items must not be gathering tools");
        }

        for (int rawStatType = 0;
             rawStatType < static_cast<int>(StatType::COUNT);
             ++rawStatType)
        {
            StatType statType =
                static_cast<StatType>(rawStatType);

            if (bonuses.Get(statType) != 0)
            {
                report.AddError(
                    "Item",
                    contentID,
                    "food items must not have equipment stat bonuses");
                break;
            }
        }
    }

    ValidateDefinitionRequirements(
        "Item",
        contentID,
        definition.GetRequirements(),
        report);
}

void ContentValidator::AppendResourceValidation(
    ResourceType registryResourceType,
    const ResourceDefinition &definition,
    ContentValidationReport &report)
{
    const std::string contentID =
        ResourceTypeToString(registryResourceType);

    if (definition.GetResourceType() !=
        registryResourceType)
    {
        report.AddError(
            "Resource",
            contentID,
            "definition resource type must match registry key");
    }

    if (definition.GetName().empty())
    {
        report.AddError(
            "Resource",
            contentID,
            "name must be non-empty");
    }

    if (!IsKnownSkillType(
            definition.GetRequiredSkill()))
    {
        report.AddError(
            "Resource",
            contentID,
            "required skill is invalid");
    }

    if (definition.GetRequiredSkillLevel() <= 0)
    {
        report.AddError(
            "Resource",
            contentID,
            "required level must be positive");
    }

    ToolType requiredTool =
        definition.GetRequiredToolType();

    if (requiredTool == ToolType::NONE ||
        !IsValidToolType(requiredTool))
    {
        report.AddError(
            "Resource",
            contentID,
            "required tool type is invalid");
    }

    ItemType rewardItem =
        definition.GetItemReward();

    if (!IsValidItemType(rewardItem))
    {
        report.AddError(
            "Resource",
            contentID,
            "reward item is invalid");
    }

    if (definition.GetItemAmount() <= 0)
    {
        report.AddError(
            "Resource",
            contentID,
            "reward amount must be positive");
    }

    if (definition.GetXPReward() < 0)
    {
        report.AddError(
            "Resource",
            contentID,
            "XP reward must be non-negative");
    }

    if (definition.GetBaseSuccessChance() < 1 ||
        definition.GetBaseSuccessChance() > 100)
    {
        report.AddError(
            "Resource",
            contentID,
            "base success chance must be within 1..100");
    }

    if (definition.GetMaxUses() <= 0)
    {
        report.AddError(
            "Resource",
            contentID,
            "maximum uses must be positive");
    }

    if (definition.GetRespawnTicks() <= 0)
    {
        report.AddError(
            "Resource",
            contentID,
            "respawn ticks must be positive");
    }

    if (!IsValidDepletedVisualType(
            definition.GetDepletedVisualType()))
    {
        report.AddError(
            "Resource",
            contentID,
            "depleted visual type is invalid");
    }

    ValidateDefinitionRequirements(
        "Resource",
        contentID,
        definition.GetRequirements(),
        report);

    ContentValidationReport consistencyReport =
        ValidateResourceRequirementConsistency(
            contentID,
            definition.GetRequiredSkill(),
            definition.GetRequiredSkillLevel(),
            definition.GetRequirements());

    for (const ContentValidationIssue &issue :
         consistencyReport.GetIssues())
    {
        report.AddError(
            issue.category,
            issue.contentID,
            issue.message);
    }
}

void ContentValidator::AppendRecipeValidation(
    RecipeType registryRecipeType,
    const RecipeDefinition &definition,
    ContentValidationReport &report)
{
    const std::string contentID =
        RecipeTypeToString(registryRecipeType);

    if (registryRecipeType == RecipeType::NONE)
    {
        report.AddError(
            "Recipe",
            contentID,
            "registry recipe type must be real content");
    }

    if (definition.GetRecipeType() == RecipeType::NONE)
    {
        report.AddError(
            "Recipe",
            contentID,
            "definition recipe type must be real content");
    }

    if (definition.GetRecipeType() != registryRecipeType)
    {
        report.AddError(
            "Recipe",
            contentID,
            "definition recipe type must match registry key");
    }

    if (definition.GetName().empty())
    {
        report.AddError(
            "Recipe",
            contentID,
            "name must be non-empty");
    }

    if (!IsKnownSkillType(
            definition.GetRequiredSkill()))
    {
        report.AddError(
            "Recipe",
            contentID,
            "required skill is invalid");
    }

    if (definition.GetRequiredLevel() <= 0)
    {
        report.AddError(
            "Recipe",
            contentID,
            "required level must be positive");
    }

    if (definition.GetActionDurationTicks() <= 0)
    {
        report.AddError(
            "Recipe",
            contentID,
            "action duration must be positive");
    }

    if (!IsValidStationType(
            definition.GetRequiredStationType()))
    {
        report.AddError(
            "Recipe",
            contentID,
            "required station is invalid");
    }

    const std::vector<RecipeIngredient> &ingredients =
        definition.GetIngredients();

    if (ingredients.empty())
    {
        report.AddError(
            "Recipe",
            contentID,
            "ingredient list must be non-empty");
    }

    std::set<ItemType> seenIngredientTypes;

    for (const RecipeIngredient &ingredient :
         ingredients)
    {
        if (!IsValidItemType(ingredient.itemType))
        {
            report.AddError(
                "Recipe",
                contentID,
                "ingredient item is invalid");
        }

        if (ingredient.amount <= 0)
        {
            report.AddError(
                "Recipe",
                contentID,
                "ingredient quantity must be positive");
        }

        if (!seenIngredientTypes.insert(ingredient.itemType).second)
        {
            report.AddError(
                "Recipe",
                contentID,
                "duplicate ingredient entries are not allowed");
        }
    }

    if (!IsValidItemType(
            definition.GetOutputItem()))
    {
        report.AddError(
            "Recipe",
            contentID,
            "output item is invalid");
    }

    if (definition.GetOutputAmount() <= 0)
    {
        report.AddError(
            "Recipe",
            contentID,
            "output quantity must be positive");
    }

    if (definition.GetXPReward() < 0)
    {
        report.AddError(
            "Recipe",
            contentID,
            "XP reward must be non-negative");
    }

    ValidateDefinitionRequirements(
        "Recipe",
        contentID,
        definition.GetRequirements(),
        report);

    ContentValidationReport consistencyReport =
        ValidateRecipeRequirementConsistency(
            contentID,
            definition.GetRequiredSkill(),
            definition.GetRequiredLevel(),
            ingredients,
            definition.GetRequirements());

    for (const ContentValidationIssue &issue :
         consistencyReport.GetIssues())
    {
        report.AddError(
            issue.category,
            issue.contentID,
            issue.message);
    }
}

void ContentValidator::AppendRewardTableValidation(
    RewardTableType rewardTableType,
    const RewardTable &rewardTable,
    ContentValidationReport &report)
{
    ContentValidationReport rewardReport =
        ValidateRewardTable(
            rewardTableType,
            rewardTable);

    for (const ContentValidationIssue &issue :
         rewardReport.GetIssues())
    {
        if (issue.severity == ContentValidationSeverity::ERROR)
        {
            report.AddError(
                issue.category,
                issue.contentID,
                issue.message);
        }
        else
        {
            report.AddWarning(
                issue.category,
                issue.contentID,
                issue.message);
        }
    }
}

void ContentValidator::AppendNpcDefinitionValidation(
    const NpcDefinition &definition,
    ContentValidationReport &report)
{
    const std::string contentID = std::to_string(static_cast<int>(definition.type));
    if (definition.type == NpcType::NONE ||
        NpcDefinitionDatabase::TryGet(definition.type) == nullptr)
        report.AddError("NpcDefinition", contentID, "definition ID must be real content");
    if (definition.name.empty())
        report.AddError("NpcDefinition", contentID, "name must not be empty");
    if (definition.combatRatings.attackAccuracy < 0 ||
        definition.combatRatings.meleeStrength < 0 ||
        definition.combatRatings.defence < 0)
        report.AddError("NpcDefinition", contentID, "combat ratings must not be negative");
    if (definition.combatRatings.maximumHealth <= 0)
        report.AddError("NpcDefinition", contentID, "maximum health must be positive");
    if (definition.attackDurationTicks <= 0)
        report.AddError("NpcDefinition", contentID, "attack duration must be positive");
    if (definition.respawnDelayTicks < 0)
        report.AddError("NpcDefinition", contentID, "respawn delay must not be negative");
    if (RewardTableRegistry::TryGetRewardTable(definition.rewardTableType) == nullptr)
        report.AddError("NpcDefinition", contentID, "reward-table reference is invalid");
    if (definition.aggressionDefinition.has_value())
    {
        const MonsterAggressionDefinition &aggression = definition.aggressionDefinition.value();
        if (aggression.detectionRadius <= 0)
            report.AddError("NpcDefinition", contentID, "aggression detection radius must be positive");
        if (aggression.leashRadius <= 0)
            report.AddError("NpcDefinition", contentID, "aggression leash radius must be positive");
        if (aggression.leashRadius < aggression.detectionRadius)
            report.AddError("NpcDefinition", contentID, "aggression leash radius must be at least detection radius");
    }
}

void ContentValidator::AppendNpcSpawnValidation(
    const NpcSpawnDefinition &definition,
    int mapWidth,
    int mapHeight,
    bool spawnTileBlockedByObject,
    ContentValidationReport &report)
{
    const std::string contentID =
        PositionToString(
            definition.spawnPosition.GetX(),
            definition.spawnPosition.GetY());

    if (mapWidth <= 0 ||
        mapHeight <= 0)
    {
        report.AddError(
            "StarterMonsterSpawn",
            contentID,
            "map dimensions must be positive");
        return;
    }

    Map map(mapWidth, mapHeight);

    if (!map.IsValidPosition(
            definition.spawnPosition.GetX(),
            definition.spawnPosition.GetY()))
    {
        report.AddError(
            "StarterMonsterSpawn",
            contentID,
            "monster spawn must be in bounds on a walkable tile");
    }

    if (spawnTileBlockedByObject)
    {
        report.AddError(
            "StarterMonsterSpawn",
            contentID,
            "monster spawn cannot overlap blocked starter object placement");
    }

    const NpcDefinition *npcDefinition =
        NpcDefinitionDatabase::TryGet(definition.npcType);
    if (npcDefinition == nullptr)
    {
        report.AddError(
            "StarterMonsterSpawn",
            contentID,
            "NPC definition reference is invalid");
    }
    if (definition.wanderRadius < 0)
    {
        report.AddError("StarterMonsterSpawn", contentID, "wander radius must not be negative");
    }
    if (definition.respawns && npcDefinition != nullptr &&
        npcDefinition->respawnDelayTicks <= 0)
    {
        report.AddError(
            "StarterMonsterSpawn",
            contentID,
            "referenced definition must have a positive respawn delay when respawn is enabled");
    }
}
