#include "StarterJourney.h"

#include "../Player/Player.h"
#include "../Item/ItemDatabase.h"
#include "../Quest/QuestDefinitionDatabase.h"
#include "../Recipe/RecipeDatabase.h"

#include <algorithm>
#include <stdexcept>
#include <utility>

namespace
{
    int Amount(const RecipeDefinition &recipe, ItemType item)
    {
        for (const auto &ingredient : recipe.GetIngredients())
            if (ingredient.itemType == item && ingredient.amount > 0)
                return ingredient.amount;
        throw std::runtime_error("starter material plan needs its recipe bindings reviewed");
    }

    std::int64_t Carried(const Player &player, ItemType item,
        const CraftingSessionSnapshot *crafting)
    {
        std::int64_t count = player.GetInventory().GetItemAmount(item);
        if (player.GetEquipment().IsEquipped(item)) ++count;
        if (crafting != nullptr && crafting->actorEntityID == player.GetID())
        {
            if (crafting->cursor.GetItemType() == item)
                count += crafting->cursor.GetAmount();
            for (const auto &slot : crafting->inputSlots)
                if (slot.GetItemType() == item) count += slot.GetAmount();
        }
        return count;
    }

    std::string Progress(std::int64_t held, int needed)
    {
        return std::to_string(std::min<std::int64_t>(held, needed)) + "/" + std::to_string(needed);
    }

    std::int64_t OreNeeded(std::int64_t barsOwned, int barsRequired, ItemType ore)
    {
        const auto &recipe = RecipeDatabase::Get(RecipeType::BRONZE_BAR);
        const auto remaining = std::max<std::int64_t>(0, barsRequired - barsOwned);
        return (remaining + recipe.GetOutputAmount() - 1) / recipe.GetOutputAmount() * Amount(recipe, ore);
    }

    StarterJourney::Guidance ToolHint(const Player &player, StarterJourney::Guidance guide)
    {
        using Destination = StarterJourney::Destination;
        const bool wood = guide.destination == Destination::NORMAL_TREE;
        const bool mine = guide.destination == Destination::COPPER || guide.destination == Destination::TIN;
        if (!wood && !mine) return guide;
        const auto weapon = player.GetEquipment().GetEquippedItem(EquipmentSlotType::WEAPON);
        const auto needed = wood ? ToolType::AXE : ToolType::PICKAXE;
        if (ItemDatabase::Get(weapon).GetToolType() == needed) return guide;
        const auto starterTool = wood ? ItemType::BRONZE_AXE : ItemType::BRONZE_PICKAXE;
        if (player.GetInventory().GetItemAmount(starterTool) == 0 &&
            player.GetBank().GetItemAmount(starterTool) > 0)
        {
            guide.objective = wood ? "Withdraw your axe" : "Withdraw your pickaxe";
            guide.instruction = "Keep your gathering tools with you on resource trips.";
            guide.destination = Destination::BANK;
        }
        else
        {
            guide.objective = wood ? "Equip an axe before gathering" : "Equip a pickaxe before mining";
            guide.instruction = wood ? "Left-click the Bronze axe in your bag." : "Left-click the Bronze pickaxe in your bag.";
            guide.destination = Destination::NONE;
        }
        return guide;
    }
}

StarterJourney::MaterialPlan StarterJourney::GetMaterialPlan(bool includeShield)
{
    const auto &sword = RecipeDatabase::Get(RecipeType::BRONZE_SWORD);
    const auto &shield = RecipeDatabase::Get(RecipeType::WOODEN_SHIELD);
    const auto &planks = RecipeDatabase::Get(RecipeType::WOODEN_PLANKS);
    const auto &bar = RecipeDatabase::Get(RecipeType::BRONZE_BAR);
    if (sword.GetOutputItem() != ItemType::BRONZE_SWORD || sword.GetOutputAmount() != 1 ||
        sword.GetIngredients().size() != 2 ||
        shield.GetOutputItem() != ItemType::WOODEN_SHIELD || shield.GetOutputAmount() != 1 ||
        shield.GetIngredients().size() != 1 ||
        planks.GetOutputItem() != ItemType::WOODEN_PLANKS || planks.GetOutputAmount() <= 0 ||
        planks.GetIngredients().size() != 1 ||
        bar.GetOutputItem() != ItemType::BRONZE_BAR || bar.GetOutputAmount() <= 0 ||
        bar.GetIngredients().size() != 2)
        throw std::runtime_error("starter material plan needs its recipe outputs reviewed");
    MaterialPlan plan;
    plan.woodenPlanks = Amount(sword, ItemType::WOODEN_PLANKS) +
        (includeShield ? Amount(shield, ItemType::WOODEN_PLANKS) : 0);
    plan.bronzeBars = Amount(sword, ItemType::BRONZE_BAR);
    const int plankBatches = (plan.woodenPlanks + planks.GetOutputAmount() - 1) / planks.GetOutputAmount();
    const int barBatches = (plan.bronzeBars + bar.GetOutputAmount() - 1) / bar.GetOutputAmount();
    plan.normalLogs = plankBatches * Amount(planks, ItemType::LOG);
    plan.copperOre = barBatches * Amount(bar, ItemType::COPPER_ORE);
    plan.tinOre = barBatches * Amount(bar, ItemType::TIN_ORE);
    plan.leftoverPlanks = plankBatches * planks.GetOutputAmount() - plan.woodenPlanks;
    plan.leftoverBars = barBatches * bar.GetOutputAmount() - plan.bronzeBars;
    return plan;
}

std::int64_t StarterJourney::CountOwned(const Player &player, ItemType item,
    const CraftingSessionSnapshot *crafting)
{
    return Carried(player, item, crafting) + player.GetBank().GetItemAmount(item);
}

bool StarterJourney::MaterialsReady(const Player &player, TutorialStep step,
    const CraftingSessionSnapshot *crafting)
{
    const auto plan = GetMaterialPlan(false);
    const auto count = [&](ItemType item) { return CountOwned(player, item, crafting); };
    if (step == TutorialStep::EQUIP_WEAPON)
        return player.GetEquipment().IsEquipped(ItemType::BRONZE_SWORD);
    const bool sword = count(ItemType::BRONZE_SWORD) > 0;
    switch (step)
    {
    case TutorialStep::GATHER_LOG:
        return sword || count(ItemType::WOODEN_PLANKS) >= plan.woodenPlanks ||
            count(ItemType::LOG) >= plan.normalLogs;
    case TutorialStep::MAKE_PLANKS:
        return sword || count(ItemType::WOODEN_PLANKS) >= plan.woodenPlanks;
    case TutorialStep::MINE_ORES:
    {
        const auto bars = count(ItemType::BRONZE_BAR);
        return sword || (count(ItemType::COPPER_ORE) >= OreNeeded(bars, plan.bronzeBars, ItemType::COPPER_ORE) &&
            count(ItemType::TIN_ORE) >= OreNeeded(bars, plan.bronzeBars, ItemType::TIN_ORE));
    }
    case TutorialStep::SMELT_BRONZE:
        return sword || count(ItemType::BRONZE_BAR) >= plan.bronzeBars;
    case TutorialStep::CRAFT_WEAPON:
        return sword;
    default:
        return false;
    }
}

StarterJourney::Guidance StarterJourney::Describe(const Player &player,
    const CraftingSessionSnapshot *crafting)
{
    const auto plan = GetMaterialPlan(false);
    const auto withShield = GetMaterialPlan(true);
    const auto count = [&](ItemType item) { return CountOwned(player, item, crafting); };
    const auto banked = [&](ItemType item, std::int64_t needed = 1)
    { return Carried(player, item, crafting) < needed && player.GetBank().GetItemAmount(item) > 0; };
    Guidance guide;
    switch (player.GetTutorialState().GetStep())
    {
    case TutorialStep::MEET_GUIDE:
        return {"Talk to Mara, the village warden",
            "Accept basic quests before gathering for their rewards.",
            "Right-click Mara, then choose Talk. H opens your plan.", Destination::GUIDE};
    case TutorialStep::GATHER_LOG:
        guide = {"Gather a normal Log for your first equipment",
            "Logs for sword " + Progress(count(ItemType::LOG), plan.normalLogs) +
                " | With shield " + Progress(count(ItemType::LOG), withShield.normalLogs),
            "Keep " + std::to_string(withShield.normalLogs) + " Logs if you also want a shield.", Destination::NORMAL_TREE};
        if (banked(ItemType::LOG))
            guide = {"Withdraw a Log for the workshop", guide.progress,
                "Your bank already holds timber; withdraw it first.", Destination::BANK};
        break;
    case TutorialStep::MAKE_PLANKS:
        guide = {"Make Wooden planks at a crafting table",
            "Sword needs " + std::to_string(plan.woodenPlanks) + " plank; sword + shield need " +
                std::to_string(withShield.woodenPlanks) + ".",
            "Keep a plank for the sword before using the rest.", Destination::TABLE};
        if (count(ItemType::LOG) == 0)
            guide = {"Collect another normal Log", guide.progress,
                "Planks for the sword are still needed.", Destination::NORMAL_TREE};
        else if (banked(ItemType::LOG))
            guide = {"Withdraw timber before crafting", guide.progress,
                "Banked materials must be in your bag to use them.", Destination::BANK};
        break;
    case TutorialStep::MINE_ORES:
    {
        const auto bars = count(ItemType::BRONZE_BAR);
        const auto copper = count(ItemType::COPPER_ORE) + plan.copperOre - OreNeeded(bars, plan.bronzeBars, ItemType::COPPER_ORE);
        const auto tin = count(ItemType::TIN_ORE) + plan.tinOre - OreNeeded(bars, plan.bronzeBars, ItemType::TIN_ORE);
        guide = {"Mine Copper and Tin for your Bronze sword",
            "Copper " + Progress(copper, plan.copperOre) + " | Tin " + Progress(tin, plan.tinOre),
            "Already smelted bars count toward these materials.",
            copper < plan.copperOre ? Destination::COPPER : Destination::TIN};
        break;
    }
    case TutorialStep::SMELT_BRONZE:
        guide = {"Smelt enough Bronze bars for the sword",
            "Bronze bars " + Progress(count(ItemType::BRONZE_BAR), plan.bronzeBars),
            "Use Copper + Tin at the furnace. Save your plank.", Destination::FURNACE};
        if (banked(ItemType::COPPER_ORE, OreNeeded(count(ItemType::BRONZE_BAR), plan.bronzeBars, ItemType::COPPER_ORE)) ||
            banked(ItemType::TIN_ORE, OreNeeded(count(ItemType::BRONZE_BAR), plan.bronzeBars, ItemType::TIN_ORE)))
            guide = {"Withdraw your ore before smelting", guide.progress,
                "Banked materials must be in your bag to use them.", Destination::BANK};
        else if (!MaterialsReady(player, TutorialStep::MINE_ORES, crafting))
            guide = {"Replace the missing sword materials", guide.progress,
                "Each Bronze bar needs Copper and Tin.",
                count(ItemType::COPPER_ORE) < OreNeeded(count(ItemType::BRONZE_BAR), plan.bronzeBars, ItemType::COPPER_ORE) ?
                    Destination::COPPER : Destination::TIN};
        break;
    case TutorialStep::CRAFT_WEAPON:
        guide = {"Forge a Bronze sword at the furnace",
            std::to_string(plan.bronzeBars) + " Bronze bars + " + std::to_string(plan.woodenPlanks) + " Wooden plank",
            "Buying a sword from Mara is another way to prepare.", Destination::FURNACE};
        if (banked(ItemType::BRONZE_BAR, plan.bronzeBars) || banked(ItemType::WOODEN_PLANKS, plan.woodenPlanks))
            guide = {"Withdraw the sword materials", guide.progress,
                "Keep the bars and plank together in your bag.", Destination::BANK};
        else if (!MaterialsReady(player, TutorialStep::SMELT_BRONZE, crafting))
        {
            const bool oreReady = MaterialsReady(player, TutorialStep::MINE_ORES, crafting);
            guide = {oreReady ? "Smelt the remaining Bronze bars" : "Gather the missing sword ore", guide.progress,
                "The sword needs all its bars before forging.", oreReady ? Destination::FURNACE :
                count(ItemType::COPPER_ORE) < OreNeeded(count(ItemType::BRONZE_BAR), plan.bronzeBars, ItemType::COPPER_ORE) ?
                    Destination::COPPER : Destination::TIN};
        }
        else if (count(ItemType::WOODEN_PLANKS) < plan.woodenPlanks)
            guide = {banked(ItemType::LOG) ? "Withdraw timber for the sword plank" : "Make the remaining Wooden planks", guide.progress,
                "The sword also needs a Wooden plank.", banked(ItemType::LOG) ? Destination::BANK :
                    count(ItemType::LOG) > 0 ? Destination::TABLE : Destination::NORMAL_TREE};
        break;
    case TutorialStep::EQUIP_WEAPON:
        guide = {"Equip your Bronze sword",
            "Prepare your Bronze sword for combat.",
            "Left-click the sword in your bag to equip it.", Destination::NONE};
        if (banked(ItemType::BRONZE_SWORD))
            guide = {"Withdraw your Bronze sword", guide.progress,
                "Take it out of the bank, then left-click it in your bag.", Destination::BANK};
        else if (count(ItemType::BRONZE_SWORD) == 0)
            guide = {"Replace your Bronze sword", guide.progress,
                "Forge another at the furnace, or buy one from Mara.", Destination::FURNACE};
        break;
    case TutorialStep::SAFE_COMBAT:
        return {"Defeat a creature and return safely",
            "A passive Mossling is the suggested first fight.",
            "Carry food. Left-click food in your bag to heal.", Destination::MOSSLING};
    case TutorialStep::LOOT_AND_BANK:
        return {"Deposit an item in the bank",
            "Click ground loot to pick it up when nearby.",
            "Keep your gathering tools when storing spare materials.", Destination::BANK};
    case TutorialStep::RETURN_AND_CHOOSE:
    {
        const auto *quest = player.GetQuestJournal().TryGet(QuestId::GATHERING_BASICS);
        const auto *definition = QuestDefinitionDatabase::TryGet(QuestId::GATHERING_BASICS);
        if (quest == nullptr || definition == nullptr)
            throw std::runtime_error("starter journey requires Gathering Basics");
        if (quest->state == QuestState::ACTIVE)
            return ToolHint(player, {"Finish Gathering Basics, then return to Mara",
                "Normal Logs gathered " + Progress(quest->progress, definition->requiredAmount),
                "Only successful gathering after accepting counts.", Destination::NORMAL_TREE});
        return {"Return to Mara and claim Gathering Basics",
            quest->state == QuestState::AVAILABLE ? "Accept the quest before gathering its normal Logs." : "Collect your reward and choose your next activity.",
            "Mining Basics offers a separate Copper-gathering reward.", Destination::GUIDE};
    }
    case TutorialStep::COMPLETE:
        return {"Choose your next adventure", "Try more quests, better tools or a tougher creature.",
            "H keeps the starter material plan available.", Destination::NONE};
    }
    return ToolHint(player, std::move(guide));
}
