#include "TestSupport.h"

#include <type_traits>
#include <vector>

#include "../src/Content/ContentValidationReport.h"
#include "../src/Content/ContentValidator.h"
#include "../src/Core/ContentStartupValidation.h"
#include "../src/Item/ItemDatabase.h"
#include "../src/Recipe/RecipeDatabase.h"
#include "../src/Requirement/Requirement.h"
#include "../src/Reward/GuaranteedRewardEntry.h"
#include "../src/Reward/RewardTableRegistry.h"
#include "../src/Reward/WeightedRewardEntry.h"
#include "../src/World/Development/DevelopmentWorldContent.h"
#include "../src/World/Object/Resource/ResourceDatabase.h"

static_assert(
    std::is_same_v<
        decltype(std::declval<const ContentValidationReport &>()
                     .GetIssues()),
        const std::vector<ContentValidationIssue> &>,
    "Content validation issue getter must be read-only");

namespace
{
    bool ContainsMessage(
        const ContentValidationReport &report,
        const std::string &messagePart)
    {
        for (const ContentValidationIssue &issue :
             report.GetIssues())
        {
            if (issue.message.find(messagePart) !=
                std::string::npos)
            {
                return true;
            }
        }

        return false;
    }

    int CountSeverity(
        const ContentValidationReport &report,
        ContentValidationSeverity severity)
    {
        int count = 0;

        for (const ContentValidationIssue &issue :
             report.GetIssues())
        {
            if (issue.severity == severity)
            {
                count++;
            }
        }

        return count;
    }

    ItemDefinition MakeValidItemDefinition(
        ItemType itemType)
    {
        return ItemDefinition(
            itemType,
            "Valid item",
            false,
            EquipmentSlotType::NONE,
            ToolType::NONE,
            SkillType::NONE,
            0,
            0);
    }

    ResourceDefinition MakeValidResourceDefinition(
        ResourceType resourceType)
    {
        return ResourceDefinition(
            resourceType,
            "Valid resource",
            SkillType::WOODCUTTING,
            1,
            ToolType::AXE,
            10,
            ItemType::LOG,
            1,
            50,
            1,
            DepletedVisualType::STUMP,
            5);
    }

    RecipeDefinition MakeValidRecipeDefinition(
        RecipeType recipeType)
    {
        return RecipeDefinition(
            recipeType,
            "Valid recipe",
            StationType::FURNACE,
            SkillType::SMITHING,
            1,
            5,
            2,
            std::vector<RecipeIngredient>{
                {ItemType::COPPER_ORE, 1}},
            ItemType::BRONZE_BAR,
            1);
    }
}

int main()
{
    TestContext test;

    {
        ContentValidationReport report =
            ContentValidator::ValidateAll();

        test.Expect(
            report.IsValid(),
            "Current committed content validates without errors");
        test.ExpectEqual(
            report.GetErrorCount(),
            0,
            "Current content has zero validation errors");
    }

    {
        const std::vector<ItemType> &itemTypes =
            ItemDatabase::GetAllItemTypes();
        const std::vector<ResourceType> &resourceTypes =
            ResourceDatabase::GetAllResourceTypes();
        const std::vector<RecipeType> &recipeTypes =
            RecipeDatabase::GetAllRecipeTypes();
        const std::vector<RewardTableType> &rewardTableTypes =
            RewardTableRegistry::GetAllRewardTableTypes();

        test.ExpectEqual(
            static_cast<int>(itemTypes.size()),
            21,
            "Every registered item type is enumerated");
        test.ExpectEqual(
            static_cast<int>(resourceTypes.size()),
            7,
            "Every registered resource type is enumerated");
        test.ExpectEqual(
            static_cast<int>(recipeTypes.size()),
            3,
            "Every registered recipe type is enumerated");
        test.ExpectEqual(
            static_cast<int>(rewardTableTypes.size()),
            1,
            "Every registered reward table type is enumerated");

        bool itemIDsMatch = true;

        for (ItemType itemType : itemTypes)
        {
            if (ItemDatabase::Get(itemType).GetItemType() != itemType)
            {
                itemIDsMatch = false;
                break;
            }
        }

        test.Expect(
            itemIDsMatch,
            "Enumerated item definitions match their IDs");

        bool hasCookedMeat = false;

        for (ItemType itemType : itemTypes)
        {
            if (itemType == ItemType::COOKED_MEAT)
            {
                hasCookedMeat = true;
                break;
            }
        }

        test.Expect(
            hasCookedMeat,
            "Cooked meat is included in deterministic item enumeration");

        const ItemDefinition &cookedMeat =
            ItemDatabase::Get(
                ItemType::COOKED_MEAT);

        test.ExpectEqual(
            cookedMeat.GetName(),
            std::string("Cooked meat"),
            "Cooked meat has a non-empty registered name");
        test.Expect(
            cookedMeat.IsFood(),
            "Cooked meat has a food definition");
        test.Expect(
            cookedMeat.GetFoodDefinition() != nullptr,
            "Cooked meat food definition is readable");
        test.ExpectEqual(
            cookedMeat.GetFoodDefinition()->healAmount,
            5,
            "Cooked meat heal amount is 5");
        test.Expect(
            !cookedMeat.IsEquippable(),
            "Cooked meat is non-equippable");
        test.ExpectEqual(
            static_cast<int>(cookedMeat.GetToolType()),
            static_cast<int>(ToolType::NONE),
            "Cooked meat is not a tool");
    }

    {
        ItemDefinition invalidDefinition(
            ItemType::NONE,
            "",
            true,
            static_cast<EquipmentSlotType>(999),
            static_cast<ToolType>(999),
            SkillType::NONE,
            1,
            -1);

        ContentValidationReport first =
            ContentValidator::ValidateItemDefinition(
                ItemType::NONE,
                invalidDefinition);
        ContentValidationReport second =
            ContentValidator::ValidateItemDefinition(
                ItemType::NONE,
                invalidDefinition);

        test.ExpectEqual(
            static_cast<int>(first.GetIssues().size()),
            static_cast<int>(second.GetIssues().size()),
            "Validation issue count is deterministic");

        bool sameOrder = true;

        for (int index = 0;
             index < static_cast<int>(first.GetIssues().size());
             ++index)
        {
            if (first.GetIssues()[index].message !=
                    second.GetIssues()[index].message ||
                first.GetIssues()[index].contentID !=
                    second.GetIssues()[index].contentID)
            {
                sameOrder = false;
                break;
            }
        }

        test.Expect(
            sameOrder,
            "Validation issue order is deterministic");
    }

    {
        ContentValidationReport noneIDReport =
            ContentValidator::ValidateItemDefinition(
                ItemType::NONE,
                MakeValidItemDefinition(ItemType::NONE));

        test.Expect(
            !noneIDReport.IsValid(),
            "Item NONE ID is rejected");

        ContentValidationReport mismatchReport =
            ContentValidator::ValidateItemDefinition(
                ItemType::LOG,
                MakeValidItemDefinition(ItemType::COAL));

        test.Expect(
            !mismatchReport.IsValid(),
            "Item ID mismatch is rejected");

        ItemDefinition emptyName(
            ItemType::LOG,
            "",
            false,
            EquipmentSlotType::NONE,
            ToolType::NONE,
            SkillType::NONE,
            0,
            0);

        test.Expect(
            !ContentValidator::ValidateItemDefinition(
                 ItemType::LOG,
                 emptyName)
                 .IsValid(),
            "Empty item name is rejected");

        ItemDefinition invalidSlot(
            ItemType::LOG,
            "Bad slot",
            false,
            static_cast<EquipmentSlotType>(999),
            ToolType::NONE,
            SkillType::NONE,
            0,
            0);

        test.Expect(
            !ContentValidator::ValidateItemDefinition(
                 ItemType::LOG,
                 invalidSlot)
                 .IsValid(),
            "Invalid equipment slot is rejected");

        ItemDefinition toolWithoutSlot(
            ItemType::BRONZE_AXE,
            "Tool without slot",
            false,
            EquipmentSlotType::NONE,
            ToolType::AXE,
            SkillType::WOODCUTTING,
            1,
            5);

        test.Expect(
            !ContentValidator::ValidateItemDefinition(
                 ItemType::BRONZE_AXE,
                 toolWithoutSlot)
                 .IsValid(),
            "Tool items without an equipment slot are rejected");

        ItemDefinition stackableEquipment(
            ItemType::BRONZE_SWORD,
            "Stackable sword",
            true,
            EquipmentSlotType::WEAPON,
            ToolType::NONE,
            SkillType::NONE,
            0,
            0);

        test.Expect(
            !ContentValidator::ValidateItemDefinition(
                 ItemType::BRONZE_SWORD,
                 stackableEquipment)
                 .IsValid(),
            "Equippable stackable item is rejected");

        ItemDefinition invalidTool(
            ItemType::BRONZE_AXE,
            "Invalid tool",
            false,
            EquipmentSlotType::WEAPON,
            static_cast<ToolType>(999),
            SkillType::WOODCUTTING,
            1,
            5);

        test.Expect(
            !ContentValidator::ValidateItemDefinition(
                 ItemType::BRONZE_AXE,
                 invalidTool)
                 .IsValid(),
            "Invalid tool type is rejected");

        ItemDefinition zeroToolDuration(
            ItemType::BRONZE_AXE,
            "Zero duration",
            false,
            EquipmentSlotType::WEAPON,
            ToolType::AXE,
            SkillType::WOODCUTTING,
            1,
            0);

        test.Expect(
            !ContentValidator::ValidateItemDefinition(
                 ItemType::BRONZE_AXE,
                 zeroToolDuration)
                 .IsValid(),
            "Gathering tool with zero duration is rejected");

        ItemDefinition malformedRequirement(
            ItemType::BRONZE_AXE,
            "Malformed requirement",
            false,
            EquipmentSlotType::WEAPON,
            ToolType::AXE,
            SkillType::NONE,
            2,
            5);

        test.Expect(
            !ContentValidator::ValidateItemDefinition(
                 ItemType::BRONZE_AXE,
                 malformedRequirement)
                 .IsValid(),
            "Malformed item requirement is rejected");

        ItemDefinition zeroFoodHealing(
            ItemType::COOKED_MEAT,
            "Zero food",
            false,
            EquipmentSlotType::NONE,
            ToolType::NONE,
            SkillType::NONE,
            0,
            0,
            StatBlock(),
            FoodDefinition{0});

        test.Expect(
            !ContentValidator::ValidateItemDefinition(
                 ItemType::COOKED_MEAT,
                 zeroFoodHealing)
                 .IsValid(),
            "Food with zero healing is rejected");

        ItemDefinition negativeFoodHealing(
            ItemType::COOKED_MEAT,
            "Negative food",
            false,
            EquipmentSlotType::NONE,
            ToolType::NONE,
            SkillType::NONE,
            0,
            0,
            StatBlock(),
            FoodDefinition{-5});

        test.Expect(
            !ContentValidator::ValidateItemDefinition(
                 ItemType::COOKED_MEAT,
                 negativeFoodHealing)
                 .IsValid(),
            "Food with negative healing is rejected");

        ItemDefinition foodWithEquipmentSlot(
            ItemType::COOKED_MEAT,
            "Food with slot",
            false,
            EquipmentSlotType::WEAPON,
            ToolType::NONE,
            SkillType::NONE,
            0,
            0,
            StatBlock(),
            FoodDefinition{5});

        test.Expect(
            !ContentValidator::ValidateItemDefinition(
                 ItemType::COOKED_MEAT,
                 foodWithEquipmentSlot)
                 .IsValid(),
            "Food with an equipment slot is rejected");

        ItemDefinition foodAsTool(
            ItemType::COOKED_MEAT,
            "Food tool",
            false,
            EquipmentSlotType::NONE,
            ToolType::AXE,
            SkillType::WOODCUTTING,
            1,
            5,
            StatBlock(),
            FoodDefinition{5});

        test.Expect(
            !ContentValidator::ValidateItemDefinition(
                 ItemType::COOKED_MEAT,
                 foodAsTool)
                 .IsValid(),
            "Food configured as a tool is rejected");

        StatBlock foodBonuses;
        foodBonuses.Set(
            StatType::MAX_HEALTH,
            1);

        ItemDefinition foodWithBonuses(
            ItemType::COOKED_MEAT,
            "Food bonus",
            false,
            EquipmentSlotType::NONE,
            ToolType::NONE,
            SkillType::NONE,
            0,
            0,
            foodBonuses,
            FoodDefinition{5});

        test.Expect(
            !ContentValidator::ValidateItemDefinition(
                 ItemType::COOKED_MEAT,
                 foodWithBonuses)
                 .IsValid(),
            "Food with equipment bonuses is rejected");
    }

    {
        ResourceDefinition invalidSkill =
            MakeValidResourceDefinition(ResourceType::NORMAL_TREE);
        invalidSkill = ResourceDefinition(
            ResourceType::NORMAL_TREE,
            "Invalid skill",
            static_cast<SkillType>(999),
            1,
            ToolType::AXE,
            10,
            ItemType::LOG,
            1,
            50,
            1,
            DepletedVisualType::STUMP,
            5);

        test.Expect(
            !ContentValidator::ValidateResourceDefinition(
                 ResourceType::NORMAL_TREE,
                 invalidSkill)
                 .IsValid(),
            "Resource with invalid skill is rejected");

        ResourceDefinition nonPositiveLevel(
            ResourceType::NORMAL_TREE,
            "Non-positive level",
            SkillType::WOODCUTTING,
            0,
            ToolType::AXE,
            10,
            ItemType::LOG,
            1,
            50,
            1,
            DepletedVisualType::STUMP,
            5);

        test.Expect(
            !ContentValidator::ValidateResourceDefinition(
                 ResourceType::NORMAL_TREE,
                 nonPositiveLevel)
                 .IsValid(),
            "Resource with non-positive level is rejected");

        ResourceDefinition invalidTool(
            ResourceType::NORMAL_TREE,
            "Invalid tool",
            SkillType::WOODCUTTING,
            1,
            ToolType::NONE,
            10,
            ItemType::LOG,
            1,
            50,
            1,
            DepletedVisualType::STUMP,
            5);

        test.Expect(
            !ContentValidator::ValidateResourceDefinition(
                 ResourceType::NORMAL_TREE,
                 invalidTool)
                 .IsValid(),
            "Resource with invalid required tool is rejected");

        ResourceDefinition invalidReward(
            ResourceType::NORMAL_TREE,
            "Invalid reward",
            SkillType::WOODCUTTING,
            1,
            ToolType::AXE,
            10,
            ItemType::NONE,
            1,
            50,
            1,
            DepletedVisualType::STUMP,
            5);

        test.Expect(
            !ContentValidator::ValidateResourceDefinition(
                 ResourceType::NORMAL_TREE,
                 invalidReward)
                 .IsValid(),
            "Resource with invalid reward item is rejected");

        ResourceDefinition nonPositiveAmount(
            ResourceType::NORMAL_TREE,
            "Invalid amount",
            SkillType::WOODCUTTING,
            1,
            ToolType::AXE,
            10,
            ItemType::LOG,
            0,
            50,
            1,
            DepletedVisualType::STUMP,
            5);

        test.Expect(
            !ContentValidator::ValidateResourceDefinition(
                 ResourceType::NORMAL_TREE,
                 nonPositiveAmount)
                 .IsValid(),
            "Resource with non-positive reward amount is rejected");

        ResourceDefinition invalidChance(
            ResourceType::NORMAL_TREE,
            "Invalid chance",
            SkillType::WOODCUTTING,
            1,
            ToolType::AXE,
            10,
            ItemType::LOG,
            1,
            0,
            1,
            DepletedVisualType::STUMP,
            5);

        test.Expect(
            !ContentValidator::ValidateResourceDefinition(
                 ResourceType::NORMAL_TREE,
                 invalidChance)
                 .IsValid(),
            "Resource with invalid success chance is rejected");

        ResourceDefinition nonPositiveUses(
            ResourceType::NORMAL_TREE,
            "Invalid uses",
            SkillType::WOODCUTTING,
            1,
            ToolType::AXE,
            10,
            ItemType::LOG,
            1,
            50,
            0,
            DepletedVisualType::STUMP,
            5);

        test.Expect(
            !ContentValidator::ValidateResourceDefinition(
                 ResourceType::NORMAL_TREE,
                 nonPositiveUses)
                 .IsValid(),
            "Resource with non-positive uses is rejected");

        ResourceDefinition invalidRespawn(
            ResourceType::NORMAL_TREE,
            "Invalid respawn",
            SkillType::WOODCUTTING,
            1,
            ToolType::AXE,
            10,
            ItemType::LOG,
            1,
            50,
            1,
            DepletedVisualType::STUMP,
            0);

        test.Expect(
            !ContentValidator::ValidateResourceDefinition(
                 ResourceType::NORMAL_TREE,
                 invalidRespawn)
                 .IsValid(),
            "Resource with non-positive respawn delay is rejected");

        std::vector<RequirementSystem::Requirement> mismatchedRequirements{
            RequirementSystem::SkillLevelRequirement{
                SkillType::MINING,
                7}};

        test.Expect(
            !ContentValidator::ValidateResourceRequirementConsistency(
                 "NORMAL_TREE",
                 SkillType::WOODCUTTING,
                 1,
                 mismatchedRequirements)
                 .IsValid(),
            "Resource requirement and legacy skill fields mismatch is rejected");
    }

    {
        RecipeDefinition invalidSkill(
            RecipeType::BRONZE_BAR,
            "Invalid skill",
            StationType::FURNACE,
            static_cast<SkillType>(999),
            1,
            5,
            2,
            std::vector<RecipeIngredient>{{ItemType::COPPER_ORE, 1}},
            ItemType::BRONZE_BAR,
            1);

        test.Expect(
            !ContentValidator::ValidateRecipeDefinition(
                 RecipeType::BRONZE_BAR,
                 invalidSkill)
                 .IsValid(),
            "Recipe with invalid skill is rejected");

        RecipeDefinition nonPositiveLevel(
            RecipeType::BRONZE_BAR,
            "Invalid level",
            StationType::FURNACE,
            SkillType::SMITHING,
            0,
            5,
            2,
            std::vector<RecipeIngredient>{{ItemType::COPPER_ORE, 1}},
            ItemType::BRONZE_BAR,
            1);

        test.Expect(
            !ContentValidator::ValidateRecipeDefinition(
                 RecipeType::BRONZE_BAR,
                 nonPositiveLevel)
                 .IsValid(),
            "Recipe with non-positive level is rejected");

        RecipeDefinition invalidDuration(
            RecipeType::BRONZE_BAR,
            "Invalid duration",
            StationType::FURNACE,
            SkillType::SMITHING,
            1,
            5,
            0,
            std::vector<RecipeIngredient>{{ItemType::COPPER_ORE, 1}},
            ItemType::BRONZE_BAR,
            1);

        test.Expect(
            !ContentValidator::ValidateRecipeDefinition(
                 RecipeType::BRONZE_BAR,
                 invalidDuration)
                 .IsValid(),
            "Recipe with non-positive duration is rejected");

        RecipeDefinition invalidStation(
            RecipeType::BRONZE_BAR,
            "Invalid station",
            StationType::NONE,
            SkillType::SMITHING,
            1,
            5,
            2,
            std::vector<RecipeIngredient>{{ItemType::COPPER_ORE, 1}},
            ItemType::BRONZE_BAR,
            1);

        test.Expect(
            !ContentValidator::ValidateRecipeDefinition(
                 RecipeType::BRONZE_BAR,
                 invalidStation)
                 .IsValid(),
            "Recipe with invalid station is rejected");

        RecipeDefinition emptyIngredients(
            RecipeType::BRONZE_BAR,
            "Empty ingredients",
            StationType::FURNACE,
            SkillType::SMITHING,
            1,
            5,
            2,
            std::vector<RecipeIngredient>{},
            ItemType::BRONZE_BAR,
            1);

        test.Expect(
            !ContentValidator::ValidateRecipeDefinition(
                 RecipeType::BRONZE_BAR,
                 emptyIngredients)
                 .IsValid(),
            "Recipe with empty ingredient list is rejected");

        RecipeDefinition invalidIngredientItem(
            RecipeType::BRONZE_BAR,
            "Invalid ingredient",
            StationType::FURNACE,
            SkillType::SMITHING,
            1,
            5,
            2,
            std::vector<RecipeIngredient>{{ItemType::NONE, 1}},
            ItemType::BRONZE_BAR,
            1);

        test.Expect(
            !ContentValidator::ValidateRecipeDefinition(
                 RecipeType::BRONZE_BAR,
                 invalidIngredientItem)
                 .IsValid(),
            "Recipe with invalid ingredient item is rejected");

        RecipeDefinition invalidIngredientQuantity(
            RecipeType::BRONZE_BAR,
            "Invalid ingredient quantity",
            StationType::FURNACE,
            SkillType::SMITHING,
            1,
            5,
            2,
            std::vector<RecipeIngredient>{{ItemType::COPPER_ORE, 0}},
            ItemType::BRONZE_BAR,
            1);

        test.Expect(
            !ContentValidator::ValidateRecipeDefinition(
                 RecipeType::BRONZE_BAR,
                 invalidIngredientQuantity)
                 .IsValid(),
            "Recipe with non-positive ingredient quantity is rejected");

        RecipeDefinition duplicateIngredient(
            RecipeType::BRONZE_BAR,
            "Duplicate ingredient",
            StationType::FURNACE,
            SkillType::SMITHING,
            1,
            5,
            2,
            std::vector<RecipeIngredient>{{ItemType::COPPER_ORE, 1},
                                          {ItemType::COPPER_ORE, 1}},
            ItemType::BRONZE_BAR,
            1);

        test.Expect(
            !ContentValidator::ValidateRecipeDefinition(
                 RecipeType::BRONZE_BAR,
                 duplicateIngredient)
                 .IsValid(),
            "Recipe with duplicate ingredient entries is rejected");

        RecipeDefinition invalidOutput(
            RecipeType::BRONZE_BAR,
            "Invalid output",
            StationType::FURNACE,
            SkillType::SMITHING,
            1,
            5,
            2,
            std::vector<RecipeIngredient>{{ItemType::COPPER_ORE, 1}},
            ItemType::NONE,
            1);

        test.Expect(
            !ContentValidator::ValidateRecipeDefinition(
                 RecipeType::BRONZE_BAR,
                 invalidOutput)
                 .IsValid(),
            "Recipe with invalid output item is rejected");

        RecipeDefinition invalidOutputQuantity(
            RecipeType::BRONZE_BAR,
            "Invalid output quantity",
            StationType::FURNACE,
            SkillType::SMITHING,
            1,
            5,
            2,
            std::vector<RecipeIngredient>{{ItemType::COPPER_ORE, 1}},
            ItemType::BRONZE_BAR,
            0);

        test.Expect(
            !ContentValidator::ValidateRecipeDefinition(
                 RecipeType::BRONZE_BAR,
                 invalidOutputQuantity)
                 .IsValid(),
            "Recipe with non-positive output quantity is rejected");

        std::vector<RequirementSystem::Requirement> ingredientMismatchRequirements{
            RequirementSystem::SkillLevelRequirement{SkillType::SMITHING, 1},
            RequirementSystem::HeldItemRequirement{ItemType::TIN_ORE, 2}};

        test.Expect(
            !ContentValidator::ValidateRecipeRequirementConsistency(
                 "BRONZE_BAR",
                 SkillType::SMITHING,
                 1,
                 std::vector<RecipeIngredient>{{ItemType::COPPER_ORE, 1}},
                 ingredientMismatchRequirements)
                 .IsValid(),
            "Recipe ingredient requirement mismatch is rejected");

        std::vector<RequirementSystem::Requirement> skillMismatchRequirements{
            RequirementSystem::SkillLevelRequirement{SkillType::MINING, 1},
            RequirementSystem::HeldItemRequirement{ItemType::COPPER_ORE, 1}};

        test.Expect(
            !ContentValidator::ValidateRecipeRequirementConsistency(
                 "BRONZE_BAR",
                 SkillType::SMITHING,
                 1,
                 std::vector<RecipeIngredient>{{ItemType::COPPER_ORE, 1}},
                 skillMismatchRequirements)
                 .IsValid(),
            "Recipe skill requirement mismatch is rejected");
    }

    {
        const RewardTable *developmentTable =
            RewardTableRegistry::TryGetRewardTable(
                RewardTableType::DEVELOPMENT_MONSTER);

        test.Expect(
            developmentTable != nullptr,
            "Development reward table exists");

        if (developmentTable != nullptr)
        {
            test.Expect(
                ContentValidator::ValidateRewardTable(
                    RewardTableType::DEVELOPMENT_MONSTER,
                    *developmentTable)
                    .IsValid(),
                "Development reward table validates successfully");
        }

        ContentValidationReport invalidItemReport =
            ContentValidator::ValidateRewardTableEntries(
                RewardTableType::DEVELOPMENT_MONSTER,
                std::vector<GuaranteedRewardEntry>{
                    {ItemType::NONE, 1, 1}},
                std::vector<WeightedRewardEntry>{},
                0);

        test.Expect(
            !invalidItemReport.IsValid(),
            "Reward table with invalid item entry is rejected");

        ContentValidationReport invalidQuantityReport =
            ContentValidator::ValidateRewardTableEntries(
                RewardTableType::DEVELOPMENT_MONSTER,
                std::vector<GuaranteedRewardEntry>{
                    {ItemType::COAL, 0, 1}},
                std::vector<WeightedRewardEntry>{},
                0);

        test.Expect(
            !invalidQuantityReport.IsValid(),
            "Reward table with invalid quantity range is rejected");

        ContentValidationReport invalidWeightReport =
            ContentValidator::ValidateRewardTableEntries(
                RewardTableType::DEVELOPMENT_MONSTER,
                std::vector<GuaranteedRewardEntry>{},
                std::vector<WeightedRewardEntry>{
                    {ItemType::COAL, 1, 1, 0}},
                0);

        test.Expect(
            !invalidWeightReport.IsValid(),
            "Reward table with invalid weight is rejected");

        ContentValidationReport overflowWeightReport =
            ContentValidator::ValidateRewardTableEntries(
                RewardTableType::DEVELOPMENT_MONSTER,
                std::vector<GuaranteedRewardEntry>{},
                std::vector<WeightedRewardEntry>{
                    {ItemType::COAL, 1, 1, std::numeric_limits<int>::max()},
                    {ItemType::TIN_ORE, 1, 1, 1}},
                std::numeric_limits<int>::max());

        test.Expect(
            !overflowWeightReport.IsValid(),
            "Reward table weight overflow is reported safely");

        ContentValidationReport missingRegistryMapping =
            ContentValidator::ValidateRewardTableRegistration(
                RewardTableType::DEVELOPMENT_MONSTER,
                nullptr);

        test.Expect(
            !missingRegistryMapping.IsValid(),
            "Missing reward table registry mapping is detected");
    }

    {
        const std::vector<DevelopmentMonsterSpawnDefinition> &starterSpawns =
            DevelopmentWorldContent::GetStarterMonsterSpawns();

        test.Expect(
            starterSpawns.size() >= 2,
            "Starter content includes both passive and aggressive monster definitions");

        const DevelopmentMonsterSpawnDefinition &validSpawn =
            starterSpawns.front();

        const DevelopmentMonsterSpawnDefinition &aggressiveSpawn =
            starterSpawns.at(1);

        test.Expect(
            ContentValidator::ValidateMonsterSpawnDefinition(
                validSpawn,
                DevelopmentWorldContent::MapWidth,
                DevelopmentWorldContent::MapHeight,
                false)
                .IsValid(),
            "Valid development monster spawn definition succeeds");

        test.Expect(
            !validSpawn.aggressionDefinition.has_value(),
            "Passive starter monster has no aggression definition");

        test.Expect(
            aggressiveSpawn.aggressionDefinition.has_value(),
            "Aggressive starter monster has aggression definition");

        test.Expect(
            ContentValidator::ValidateMonsterSpawnDefinition(
                aggressiveSpawn,
                DevelopmentWorldContent::MapWidth,
                DevelopmentWorldContent::MapHeight,
                false)
                .IsValid(),
            "Aggressive starter monster spawn definition succeeds");

        DevelopmentMonsterSpawnDefinition outOfBounds =
            validSpawn;
        outOfBounds.spawnX = -1;

        test.Expect(
            !ContentValidator::ValidateMonsterSpawnDefinition(
                 outOfBounds,
                 DevelopmentWorldContent::MapWidth,
                 DevelopmentWorldContent::MapHeight,
                 false)
                 .IsValid(),
            "Out-of-bounds monster spawn is rejected");

        DevelopmentMonsterSpawnDefinition blockedTileSpawn =
            validSpawn;
        blockedTileSpawn.spawnX = 10;
        blockedTileSpawn.spawnY = 10;

        test.Expect(
            !ContentValidator::ValidateMonsterSpawnDefinition(
                 blockedTileSpawn,
                 DevelopmentWorldContent::MapWidth,
                 DevelopmentWorldContent::MapHeight,
                 false)
                 .IsValid(),
            "Blocked-tile monster spawn is rejected");

        DevelopmentMonsterSpawnDefinition invalidHealth =
            validSpawn;
        invalidHealth.ratings.maximumHealth = 0;

        test.Expect(
            !ContentValidator::ValidateMonsterSpawnDefinition(
                 invalidHealth,
                 DevelopmentWorldContent::MapWidth,
                 DevelopmentWorldContent::MapHeight,
                 false)
                 .IsValid(),
            "Non-positive monster maximum health is rejected");

        DevelopmentMonsterSpawnDefinition invalidRewardTable =
            validSpawn;
        invalidRewardTable.rewardTableType =
            RewardTableType::NONE;

        test.Expect(
            !ContentValidator::ValidateMonsterSpawnDefinition(
                 invalidRewardTable,
                 DevelopmentWorldContent::MapWidth,
                 DevelopmentWorldContent::MapHeight,
                 false)
                 .IsValid(),
            "Invalid monster reward-table reference is rejected");

        DevelopmentMonsterSpawnDefinition invalidRespawn =
            validSpawn;
        invalidRespawn.respawnDefinition =
            MonsterRespawnDefinition{0};

        test.Expect(
            !ContentValidator::ValidateMonsterSpawnDefinition(
                 invalidRespawn,
                 DevelopmentWorldContent::MapWidth,
                 DevelopmentWorldContent::MapHeight,
                 false)
                 .IsValid(),
            "Invalid monster respawn delay is rejected");

        DevelopmentMonsterSpawnDefinition zeroDetectionAggression =
            aggressiveSpawn;
        zeroDetectionAggression.aggressionDefinition =
            MonsterAggressionDefinition{0, 8};

        test.Expect(
            !ContentValidator::ValidateMonsterSpawnDefinition(
                 zeroDetectionAggression,
                 DevelopmentWorldContent::MapWidth,
                 DevelopmentWorldContent::MapHeight,
                 false)
                 .IsValid(),
            "Aggression definition with zero detection radius is rejected");

        DevelopmentMonsterSpawnDefinition negativeDetectionAggression =
            aggressiveSpawn;
        negativeDetectionAggression.aggressionDefinition =
            MonsterAggressionDefinition{-1, 8};

        test.Expect(
            !ContentValidator::ValidateMonsterSpawnDefinition(
                 negativeDetectionAggression,
                 DevelopmentWorldContent::MapWidth,
                 DevelopmentWorldContent::MapHeight,
                 false)
                 .IsValid(),
            "Aggression definition with negative detection radius is rejected");

        DevelopmentMonsterSpawnDefinition zeroLeashAggression =
            aggressiveSpawn;
        zeroLeashAggression.aggressionDefinition =
            MonsterAggressionDefinition{5, 0};

        test.Expect(
            !ContentValidator::ValidateMonsterSpawnDefinition(
                 zeroLeashAggression,
                 DevelopmentWorldContent::MapWidth,
                 DevelopmentWorldContent::MapHeight,
                 false)
                 .IsValid(),
            "Aggression definition with zero leash radius is rejected");

        DevelopmentMonsterSpawnDefinition shortLeashAggression =
            aggressiveSpawn;
        shortLeashAggression.aggressionDefinition =
            MonsterAggressionDefinition{5, 4};

        test.Expect(
            !ContentValidator::ValidateMonsterSpawnDefinition(
                 shortLeashAggression,
                 DevelopmentWorldContent::MapWidth,
                 DevelopmentWorldContent::MapHeight,
                 false)
                 .IsValid(),
            "Aggression definition with leash smaller than detection is rejected");

        DevelopmentMonsterSpawnDefinition overlappingAggressiveSpawn =
            aggressiveSpawn;

        test.Expect(
            !ContentValidator::ValidateMonsterSpawnDefinition(
                 overlappingAggressiveSpawn,
                 DevelopmentWorldContent::MapWidth,
                 DevelopmentWorldContent::MapHeight,
                 true)
                 .IsValid(),
            "Aggressive monster spawn overlap with blocked starter object is rejected");
    }

    {
        ContentValidationReport validReport =
            ContentValidator::ValidateAll();

        test.Expect(
            ContentStartupValidation::CanStartFromReport(validReport),
            "Valid content report allows startup to continue");

        ContentValidationReport invalidReport;
        invalidReport.AddError(
            "Test",
            "INVALID",
            "forced invalid report");

        test.Expect(
            !ContentStartupValidation::CanStartFromReport(invalidReport),
            "Invalid content report prevents startup continuation");

        ContentValidationReport secondRun =
            ContentValidator::ValidateAll();

        test.Expect(
            secondRun.IsValid(),
            "Validator remains callable and does not terminate process");
    }

    {
        ContentValidationReport report;
        report.AddWarning(
            "Test",
            "WARN",
            "warning-only report");

        test.Expect(
            report.IsValid(),
            "Warnings alone do not invalidate report");
        test.ExpectEqual(
            report.GetWarningCount(),
            1,
            "Warning count is tracked");
        test.ExpectEqual(
            CountSeverity(
                report,
                ContentValidationSeverity::WARNING),
            1,
            "Warning severity is preserved");
        test.Expect(
            ContainsMessage(
                report,
                "warning-only"),
            "Issue message text is preserved");
    }

    return test.Finish();
}
