#include "TestSupport.h"

#include "../src/Content/ContentValidator.h"
#include "../src/Inventory/ItemType.h"
#include "../src/Item/ItemDatabase.h"
#include "../src/Item/ItemTagDatabase.h"
#include "../src/Persistence/PersistenceTokenCodec.h"
#include "../src/Player/Player.h"
#include "../src/Player/PlayerInitializationMode.h"
#include "../src/Recipe/CraftingRecipeMatcher.h"
#include "../src/Recipe/RecipeDatabase.h"
#include "../src/Recipe/RecipeSystem.h"

int main()
{
    TestContext test;

    test.ExpectEqual(
        static_cast<int>(RecipeType::WOODEN_PLANKS),
        4,
        "Wooden-planks recipe keeps a stable appended numeric ID");
    test.ExpectEqual(
        static_cast<int>(RecipeType::WOODEN_SHIELD),
        5,
        "Wooden-shield recipe keeps a stable appended numeric ID");
    test.ExpectEqual(
        static_cast<int>(RecipeType::BRONZE_AXE), 6,
        "Forged equipment recipes append after existing recipe IDs");
    test.ExpectEqual(
        static_cast<int>(RecipeType::BRONZE_SWORD), 12,
        "Forged equipment recipe range remains stable");
    test.ExpectEqual(
        static_cast<int>(RecipeType::OAK_PLANKS), 13,
        "Oak plank recipe appends after forged equipment IDs");
    test.ExpectEqual(
        static_cast<int>(RecipeType::WILLOW_PLANKS), 14,
        "Willow plank recipe keeps its stable appended ID");
    test.ExpectEqual(
        static_cast<int>(ItemType::WOODEN_PLANKS),
        22,
        "Wooden planks keep a stable appended numeric item ID");
    test.ExpectEqual(
        static_cast<int>(StationType::CRAFTING_TABLE),
        2,
        "Crafting Table keeps a stable appended station ID");
    test.Expect(
        ItemDatabase::Get(ItemType::WOODEN_PLANKS).IsStackable(),
        "Wooden planks are a stackable material");
    test.Expect(
        PersistenceTokenCodec::TryGetItemToken(
            ItemType::WOODEN_PLANKS) == "WOODEN_PLANKS" &&
            PersistenceTokenCodec::TryParseItemToken(
                "WOODEN_PLANKS") == ItemType::WOODEN_PLANKS,
        "Wooden planks have a stable persistence token");

    const RecipeDefinition &planks =
        RecipeDatabase::Get(RecipeType::WOODEN_PLANKS);
    const RecipeDefinition &shield =
        RecipeDatabase::Get(RecipeType::WOODEN_SHIELD);
    const RecipeDefinition &ironPickaxe =
        RecipeDatabase::Get(RecipeType::IRON_PICKAXE);
    const RecipeDefinition &steelAxe =
        RecipeDatabase::Get(RecipeType::STEEL_AXE);
    const RecipeDefinition &oakPlanks =
        RecipeDatabase::Get(RecipeType::OAK_PLANKS);
    const RecipeDefinition &willowPlanks =
        RecipeDatabase::Get(RecipeType::WILLOW_PLANKS);

    test.Expect(
        planks.GetRequiredStationType() ==
                StationType::CRAFTING_TABLE &&
            planks.GetCraftingPattern().type ==
                CraftingPatternType::SHAPELESS &&
            planks.GetOutputItem() == ItemType::WOODEN_PLANKS &&
            planks.GetOutputAmount() == 4 &&
            !planks.RepeatsWhileValid(),
        "One Log authoritatively defines four non-repeating Wooden planks");
    test.Expect(
        shield.GetRequiredStationType() ==
                StationType::CRAFTING_TABLE &&
            shield.GetCraftingPattern().type ==
                CraftingPatternType::SHAPED &&
            shield.GetIngredients().size() == 1 &&
            shield.GetIngredients()[0].itemType ==
                ItemType::WOODEN_PLANKS &&
            shield.GetIngredients()[0].amount == 6 &&
            !shield.RepeatsWhileValid(),
        "Wooden shield is a six-plank shaped non-repeating recipe");
    test.Expect(
        ironPickaxe.GetRequiredStationType() == StationType::FURNACE &&
            ironPickaxe.GetRequiredSkill() == SkillType::SMITHING &&
            ironPickaxe.GetRequiredLevel() == 10 &&
            ironPickaxe.GetIngredients().size() == 2 &&
            ironPickaxe.GetIngredients()[0].itemType == ItemType::IRON_BAR &&
            ironPickaxe.GetIngredients()[0].amount == 3 &&
            ironPickaxe.GetIngredients()[1].itemType == ItemType::WOODEN_PLANKS &&
            ironPickaxe.GetIngredients()[1].amount == 2 &&
            ironPickaxe.GetOutputItem() == ItemType::IRON_PICKAXE,
        "Iron pickaxe has a deterministic mid-tier forging path");
    test.Expect(
        steelAxe.GetRequiredLevel() == 20 &&
            steelAxe.GetIngredients()[0].itemType == ItemType::STEEL_BAR &&
            steelAxe.GetOutputItem() == ItemType::STEEL_AXE,
        "Steel axe has a deterministic high-tier alternative to its rare drop");
    test.Expect(
        oakPlanks.GetRequiredLevel() == 5 &&
            oakPlanks.GetOutputItem() == ItemType::WOODEN_PLANKS &&
            oakPlanks.GetOutputAmount() == 6 &&
            !oakPlanks.RepeatsWhileValid() &&
            willowPlanks.GetRequiredLevel() == 10 &&
            willowPlanks.GetOutputAmount() == 8 &&
            !willowPlanks.RepeatsWhileValid(),
        "Higher timber tiers convert to progressively more planks");

    CraftingRecipeMatcher::Grid grid{};
    grid[8] = ItemType::LOG;
    test.Expect(
        CraftingRecipeMatcher::FindMatch(grid) == &planks,
        "Shapeless Log recipe matches in any grid slot");
    grid[0] = ItemType::LOG;
    test.Expect(
        CraftingRecipeMatcher::FindMatch(grid) == nullptr,
        "Shapeless recipe rejects extra materials");

    grid = {};
    grid[3] = ItemType::OAK_LOG;
    test.Expect(
        CraftingRecipeMatcher::FindMatch(grid) == &oakPlanks,
        "A single Oak log discovers the improved Oak plank recipe");
    grid = {};
    grid[5] = ItemType::WILLOW_LOG;
    test.Expect(
        CraftingRecipeMatcher::FindMatch(grid) == &willowPlanks,
        "A single Willow log discovers the best plank-yield recipe");

    grid = {
        ItemType::WOODEN_PLANKS, ItemType::NONE, ItemType::WOODEN_PLANKS,
        ItemType::WOODEN_PLANKS, ItemType::WOODEN_PLANKS, ItemType::WOODEN_PLANKS,
        ItemType::NONE, ItemType::WOODEN_PLANKS, ItemType::NONE};
    test.Expect(
        CraftingRecipeMatcher::FindMatch(grid) == &shield,
        "Exact authored shield grid matches the shield recipe");
    const CraftingRecipeMatcher::Grid rotatedShield{
        ItemType::NONE, ItemType::WOODEN_PLANKS, ItemType::NONE,
        ItemType::WOODEN_PLANKS, ItemType::WOODEN_PLANKS, ItemType::WOODEN_PLANKS,
        ItemType::WOODEN_PLANKS, ItemType::NONE, ItemType::WOODEN_PLANKS};
    test.Expect(
        CraftingRecipeMatcher::FindMatch(rotatedShield) == nullptr,
        "Shaped recipes do not rotate implicitly");

    const CraftingPattern translatedPattern{
        CraftingPatternType::SHAPED,
        {
            ItemType::LOG, ItemType::LOG, ItemType::NONE,
            ItemType::LOG, ItemType::NONE, ItemType::NONE,
            ItemType::NONE, ItemType::NONE, ItemType::NONE,
        }};
    const CraftingRecipeMatcher::Grid translatedGrid{
        ItemType::NONE, ItemType::NONE, ItemType::NONE,
        ItemType::NONE, ItemType::LOG, ItemType::LOG,
        ItemType::NONE, ItemType::LOG, ItemType::NONE};
    test.Expect(
        CraftingRecipeMatcher::Matches(
            translatedPattern,
            translatedGrid),
        "Shaped patterns can translate within the grid");
    const CraftingRecipeMatcher::Grid rotatedPatternGrid{
        ItemType::NONE, ItemType::NONE, ItemType::NONE,
        ItemType::NONE, ItemType::LOG, ItemType::LOG,
        ItemType::NONE, ItemType::NONE, ItemType::LOG};
    test.Expect(
        !CraftingRecipeMatcher::Matches(
            translatedPattern,
            rotatedPatternGrid),
        "Translated shaped patterns still reject rotation and mirroring");

    const CraftingPattern asymmetricPattern{
        CraftingPatternType::SHAPED,
        {
            ItemType::LOG, ItemType::WOODEN_PLANKS, ItemType::NONE,
            ItemType::LOG, ItemType::NONE, ItemType::NONE,
        },
        false,
        4};
    const CraftingRecipeMatcher::Grid mirroredAsymmetricGrid{
        ItemType::WOODEN_PLANKS, ItemType::LOG, ItemType::NONE,
        ItemType::NONE, ItemType::LOG, ItemType::NONE,
    };
    test.Expect(
        !CraftingRecipeMatcher::Matches(
            asymmetricPattern,
            mirroredAsymmetricGrid),
        "Asymmetric recipe rejects horizontal mirror when disabled");
    CraftingPattern mirrorEnabled = asymmetricPattern;
    mirrorEnabled.allowHorizontalMirror = true;
    test.Expect(
        CraftingRecipeMatcher::Matches(
            mirrorEnabled,
            mirroredAsymmetricGrid),
        "Asymmetric recipe accepts horizontal mirror when enabled");
    test.ExpectEqual(
        mirrorEnabled.priority,
        4,
        "Crafting pattern preserves explicit conflict priority");

    const CraftingPattern logTagPattern{
        CraftingPatternType::SHAPELESS,
        {CraftingIngredient::ForTag(ItemTag::LOGS)}};
    for (ItemType log : {ItemType::LOG, ItemType::OAK_LOG,
                         ItemType::WILLOW_LOG})
    {
        CraftingRecipeMatcher::Grid taggedGrid{};
        taggedGrid[4] = log;
        test.Expect(
            CraftingRecipeMatcher::Matches(logTagPattern, taggedGrid),
            "LOGS tag matches each authoritative log material");
    }
    CraftingRecipeMatcher::Grid oreGrid{};
    oreGrid[0] = ItemType::COPPER_ORE;
    test.Expect(
        !CraftingRecipeMatcher::Matches(logTagPattern, oreGrid),
        "LOGS tag rejects non-log materials");
    test.Expect(
        ItemTagDatabase::IsValid(ItemTag::ORES) &&
            ItemTagDatabase::Contains(ItemTag::ORES, ItemType::IRON_ORE) &&
            !ItemTagDatabase::Contains(ItemTag::ORES, ItemType::COAL),
        "Item tags have explicit validated membership");

    const CraftingPattern exactAndTagPattern{
        CraftingPatternType::SHAPELESS,
        {ItemType::LOG, CraftingIngredient::ForTag(ItemTag::LOGS)}};
    CraftingRecipeMatcher::Grid ambiguousGrid{};
    ambiguousGrid[0] = ItemType::LOG;
    ambiguousGrid[1] = ItemType::OAK_LOG;
    test.Expect(
        CraftingRecipeMatcher::Matches(exactAndTagPattern, ambiguousGrid),
        "Shapeless matching assigns exact ingredients before broad tags");

    const RecipeDefinition validTaggedRecipe(
        RecipeType::WOODEN_PLANKS, "Tagged logs", StationType::CRAFTING_TABLE,
        SkillType::NONE, 0, 0, 1, {{ItemType::LOG, 1}},
        ItemType::WOODEN_PLANKS, 4, logTagPattern, false);
    test.Expect(
        ContentValidator::ValidateRecipeDefinition(
            RecipeType::WOODEN_PLANKS, validTaggedRecipe).IsValid(),
        "Content validation accepts a valid tag backed by a declared material");
    CraftingPattern invalidTagPattern = logTagPattern;
    invalidTagPattern.slots[0] = CraftingIngredient::ForTag(
        static_cast<ItemTag>(999));
    const RecipeDefinition invalidTaggedRecipe(
        RecipeType::WOODEN_PLANKS, "Invalid tag", StationType::CRAFTING_TABLE,
        SkillType::NONE, 0, 0, 1, {{ItemType::LOG, 1}},
        ItemType::WOODEN_PLANKS, 4, invalidTagPattern, false);
    test.Expect(
        !ContentValidator::ValidateRecipeDefinition(
            RecipeType::WOODEN_PLANKS, invalidTaggedRecipe).IsValid(),
        "Content validation rejects unknown crafting tags");

    Player player(1, PlayerInitializationMode::EMPTY);
    player.GetInventory().AddItem(ItemType::LOG, 1);
    test.Expect(
        RecipeSystem::TryCreateRecipe(
            player,
            RecipeType::WOODEN_PLANKS),
        "Plank transaction succeeds with one Log");
    test.Expect(
        player.GetInventory().GetItemAmount(ItemType::LOG) == 0 &&
            player.GetInventory().GetItemAmount(
                ItemType::WOODEN_PLANKS) == 4,
        "Plank transaction consumes one Log and adds exactly four planks");

    player.GetInventory().AddItem(ItemType::WOODEN_PLANKS, 2);
    test.Expect(
        RecipeSystem::TryCreateRecipe(
            player,
            RecipeType::WOODEN_SHIELD),
        "Shield transaction succeeds with six Wooden planks");
    test.Expect(
        player.GetInventory().GetItemAmount(
            ItemType::WOODEN_PLANKS) == 0 &&
            player.GetInventory().GetItemAmount(
                ItemType::WOODEN_SHIELD) == 1,
        "Shield transaction consumes exactly six planks and adds one shield");

    Player smith(3, PlayerInitializationMode::EMPTY);
    smith.GetInventory().AddItem(ItemType::BRONZE_BAR, 3);
    smith.GetInventory().AddItem(ItemType::WOODEN_PLANKS, 1);
    test.Expect(
        RecipeSystem::TryCreateRecipe(smith, RecipeType::BRONZE_SWORD),
        "A level-one smith can forge the first combat upgrade");
    test.Expect(
        smith.GetInventory().GetItemAmount(ItemType::BRONZE_BAR) == 0 &&
            smith.GetInventory().GetItemAmount(ItemType::WOODEN_PLANKS) == 0 &&
            smith.GetInventory().GetItemAmount(ItemType::BRONZE_SWORD) == 1,
        "Forging consumes metal and timber atomically and creates the equipment");

    Player lumberer(4);
    lumberer.GetInventory().AddItem(ItemType::OAK_LOG, 1);
    lumberer.GetInventory().AddItem(ItemType::WILLOW_LOG, 1);
    test.Expect(
        RecipeSystem::TryCreateRecipe(lumberer, RecipeType::OAK_PLANKS) &&
            RecipeSystem::TryCreateRecipe(lumberer, RecipeType::WILLOW_PLANKS),
        "A skilled woodcutter can process both higher timber tiers");
    test.Expect(
        lumberer.GetInventory().GetItemAmount(ItemType::OAK_LOG) == 0 &&
            lumberer.GetInventory().GetItemAmount(ItemType::WILLOW_LOG) == 0 &&
            lumberer.GetInventory().GetItemAmount(ItemType::WOODEN_PLANKS) == 14,
        "Oak and Willow yields combine into fourteen useful planks");

    Player fullPlayer(2, PlayerInitializationMode::EMPTY);
    fullPlayer.GetInventory().AddItem(ItemType::WOODEN_PLANKS, 7);
    while (fullPlayer.GetInventory().AddItem(ItemType::LOG, 1))
    {
    }
    test.Expect(
        !RecipeSystem::TryCreateRecipe(
            fullPlayer,
            RecipeType::WOODEN_SHIELD),
        "Full inventory rejects shield output when the plank stack remains");
    test.Expect(
        fullPlayer.GetInventory().GetItemAmount(
            ItemType::WOODEN_PLANKS) == 7 &&
            fullPlayer.GetInventory().GetItemAmount(
                ItemType::WOODEN_SHIELD) == 0,
        "Failed output insertion rolls back all ingredient removal atomically");

    test.Expect(
        RecipeDatabase::TryGet(static_cast<RecipeType>(999)) == nullptr,
        "Unknown recipe IDs are rejected without throwing");
    test.Expect(
        ContentValidator::ValidateAll().IsValid(),
        "Complete crafting content catalogue validates");

    return test.Finish();
}
