#include "Command/ServerCommand.h"
#include "Content/Import/StarterWorldRuntimeInput.h"
#include "Content/ContentValidator.h"
#include "Economy/EconomyBalanceModel.h"
#include "Item/ItemDatabase.h"
#include "NPC/NPC.h"
#include "Player/Player.h"
#include "Recipe/RecipeDatabase.h"
#include "Quest/QuestDefinitionDatabase.h"
#include "Shop/ShopDefinitionDatabase.h"
#include "Time/Clock.h"
#include "Tutorial/StarterJourney.h"
#include "World/World.h"

#include <algorithm>
#include <array>
#include <charconv>
#include <chrono>
#include <cstdlib>
#include <iostream>
#include <locale>
#include <memory>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace
{
    constexpr std::string_view Usage =
        "Usage: EmbervaleEconomyReport [--replay-timber-cycle <starting-coins> <cycles> [--world <snapshot.json>]]\n"
        "       EmbervaleEconomyReport --replay-starter-upgrade [--world <snapshot.json>]\n"
        "       EmbervaleEconomyReport --help\n"
        "No arguments: report current starter prices and recipes.\n"
        "Timber replay uses explicit seed coins; starter replay gathers all its own materials and coins.\n"
        "It never reads or writes player saves. Coins: 1..1000000; cycles: 1..10.\n";

    int Number(std::string_view text, int maximum)
    {
        int value = 0;
        const auto [end, error] = std::from_chars(text.data(), text.data() + text.size(), value);
        if (error != std::errc{} || end != text.data() + text.size() || value < 1 || value > maximum)
            throw std::invalid_argument("numeric argument is outside its supported range");
        return value;
    }

    std::string Quote(std::string_view text)
    {
        std::string out = "\"";
        constexpr char hex[] = "0123456789abcdef";
        for (unsigned char ch : text)
        {
            if (ch == '"' || ch == '\\') { out += '\\'; out += static_cast<char>(ch); }
            else if (ch < 32) { out += "\\u00"; out += hex[ch >> 4]; out += hex[ch & 15]; }
            else out += static_cast<char>(ch);
        }
        return out + '"';
    }

    struct QuietRuntime
    {
        std::ostringstream trace;
        std::streambuf *previous = std::cout.rdbuf(trace.rdbuf());
        ~QuietRuntime() { std::cout.rdbuf(previous); }
    };

    struct Replay
    {
        World &world;
        Player &player;
        int ticks = 0;
        int gatheringAttempts = 0;

        void Step()
        {
            if (++ticks > 20000)
                throw std::runtime_error("bounded replay exceeded 20000 ticks");
            world.Update();
            for (const auto &event : world.GetActionLifecycleEvents())
                if (const auto *completed = std::get_if<ActionCompletedEvent>(&event.data);
                    completed != nullptr && completed->ownerEntityID == player.GetID() &&
                    completed->actionType == ActionType::GATHERING)
                    ++gatheringAttempts;
            if (!player.IsAlive())
                throw std::runtime_error("replay character died");
        }

        void Submit(ServerCommandData command)
        {
            const auto id = world.EnqueueCommand(std::move(command));
            Step();
            for (const auto &result : world.GetCommandProcessingResults())
                if (result.commandID == id && result.resultCode == CommandResultCode::ACCEPTED)
                    return;
            throw std::runtime_error("authoritative replay command was rejected");
        }

        ShopSessionId OpenShop(int guideId)
        {
            Submit(NpcInteractionCommand{player.GetID(), guideId, NpcInteractionType::TRADE});
            for (int i = 0; i < 2000; ++i)
            {
                if (const auto *session = world.GetActiveShopSession(player.GetID()))
                    return session->sessionId;
                Step();
            }
            throw std::runtime_error("could not approach/open the supplies shop");
        }

        void OpenStation(int stationId, StationType expectedType = StationType::CRAFTING_TABLE)
        {
            const CraftingStation *target = nullptr;
            for (const auto &station : world.GetStations())
                if (station.GetID() == stationId) target = &station;
            if (target == nullptr) throw std::runtime_error("required station is missing");
            Submit(InteractCommand{player.GetID(), InteractionTargetType::STATION,
                stationId, Position{target->GetX(), target->GetY()}});
            for (int i = 0; i < 2000; ++i)
            {
                Step();
                StationType type = StationType::NONE;
                if (world.ConsumeOpenedStation(player.GetID(), type) && type == expectedType)
                    return;
            }
            throw std::runtime_error("could not approach/open the required station");
        }

        void Craft(const std::array<ItemType, 9> &grid)
        {
            Submit(CraftRecipeCommand{player.GetID(), grid});
            FinishAction();
        }

        void FinishAction()
        {
            for (int i = 0; world.GetActionForEntity(player.GetID()) != nullptr; ++i)
            {
                if (i >= 100)
                    throw std::runtime_error("craft action did not finish");
                Step();
            }
        }

        DialogueSessionId Talk(int guideId)
        {
            Submit(NpcInteractionCommand{player.GetID(), guideId, NpcInteractionType::TALK});
            for (int i = 0; i < 2000; ++i)
            {
                if (const auto *session = world.GetActiveDialogueSession(player.GetID()))
                    return session->sessionId;
                Step();
            }
            throw std::runtime_error("could not approach/talk to Mara");
        }

        int Slot(ItemType item) const
        {
            const auto &slots = player.GetInventory().GetSlots();
            for (int i = 0; i < Inventory::SlotCount; ++i)
                if (slots[i].GetItemType() == item && !slots[i].IsEmpty()) return i;
            throw std::runtime_error("required replay inventory item is missing");
        }

        void Equip(ItemType item)
        {
            Submit(UseInventoryItemCommand{player.GetID(), Slot(item)});
            if (!player.GetEquipment().IsEquipped(item))
                throw std::runtime_error("starter equipment did not equip");
        }

        void Gather(ResourceType type, ItemType item, int quantity)
        {
            while (player.GetInventory().GetItemAmount(item) < quantity)
            {
                const ResourceNode *closest = nullptr;
                int distance = 1000000;
                for (const auto &resource : world.GetResources())
                    if (resource.GetResourceType() == type && resource.IsActive())
                    {
                        const int candidate = std::abs(resource.GetX() - player.GetPosition().GetX()) +
                            std::abs(resource.GetY() - player.GetPosition().GetY());
                        if (candidate < distance) { closest = &resource; distance = candidate; }
                    }
                if (closest == nullptr) { Step(); continue; }
                const int before = player.GetInventory().GetItemAmount(item);
                Submit(InteractCommand{player.GetID(), InteractionTargetType::RESOURCE,
                    closest->GetID(), Position{closest->GetX(), closest->GetY()}});
                for (int i = 0; player.GetInventory().GetItemAmount(item) == before; ++i)
                {
                    if (i >= 2000) throw std::runtime_error("starter gathering did not produce its next item");
                    Step();
                }
            }
        }
    };

    std::string TimberReplay(int startingCoins, int cycles,
        const std::optional<StarterWorldExport::Snapshot> &snapshot)
    {
        QuietRuntime quiet;
        World world(snapshot);
        auto *player = dynamic_cast<Player *>(world.GetEntityByID(
            world.CreatePlayer(PlayerInitializationMode::FRESH_START)));
        if (player == nullptr || !player->GetInventory().AddItem(ItemType::COINS, startingCoins))
            throw std::runtime_error("could not prepare the isolated replay character");
        int guideId = 0, tableId = 0;
        for (const auto &entity : world.GetEntities())
            if (const auto *npc = dynamic_cast<const NPC *>(entity.get());
                npc != nullptr && npc->GetNpcType() == NpcType::DEVELOPMENT_GUIDE)
                guideId = npc->GetID();
        for (const auto &station : world.GetStations())
            if (station.GetStationType() == StationType::CRAFTING_TABLE)
                tableId = station.GetID();
        if (guideId == 0 || tableId == 0)
            throw std::runtime_error("selected world needs the guide and crafting table");

        const auto *log = ShopDefinitionDatabase::TryGetEntry(ShopId::DEVELOPMENT_GUIDE_SUPPLIES, ItemType::LOG);
        const auto *shield = ShopDefinitionDatabase::TryGetEntry(ShopId::DEVELOPMENT_GUIDE_SUPPLIES, ItemType::WOODEN_SHIELD);
        if (log == nullptr || shield == nullptr || !log->buyPrice || !shield->sellPrice)
            throw std::runtime_error("timber chain is not offered by the selected shop");
        Replay replay{world, *player};
        std::array<ItemType, 9> planks{};
        planks[4] = ItemType::LOG;
        const std::array<ItemType, 9> shieldGrid{
            ItemType::WOODEN_PLANKS, ItemType::NONE, ItemType::WOODEN_PLANKS,
            ItemType::WOODEN_PLANKS, ItemType::WOODEN_PLANKS, ItemType::WOODEN_PLANKS,
            ItemType::NONE, ItemType::WOODEN_PLANKS, ItemType::NONE};
        std::ostringstream result;
        result << "{\n  \"kind\": \"authoritative-timber-replay\",\n  \"profile\": \"fresh\",\n"
            << "  \"uses_imported_world\": " << (snapshot ? "true" : "false") << ",\n"
            << "  \"starting_coins\": " << startingCoins << ",\n  \"log_buy_price\": " << *log->buyPrice
            << ",\n  \"shield_sell_price\": " << *shield->sellPrice << ",\n  \"cycles\": [\n";
        for (int cycle = 0; cycle < cycles; ++cycle)
        {
            const int before = player->GetInventory().GetItemAmount(ItemType::COINS);
            auto session = replay.OpenShop(guideId);
            replay.Submit(ShopBuyCommand{player->GetID(), session, ItemType::LOG, 3});
            const int afterBuy = player->GetInventory().GetItemAmount(ItemType::COINS);
            replay.Submit(ShopCloseCommand{player->GetID(), session});
            replay.OpenStation(tableId);
            for (int i = 0; i < 3; ++i) replay.Craft(planks);
            if (player->GetInventory().GetItemAmount(ItemType::WOODEN_PLANKS) != 12)
                throw std::runtime_error("log conversion did not produce twelve planks");
            for (int i = 0; i < 2; ++i) replay.Craft(shieldGrid);
            replay.Submit(CloseStationCommand{player->GetID()});
            session = replay.OpenShop(guideId);
            replay.Submit(ShopSellCommand{player->GetID(), session, ItemType::WOODEN_SHIELD, 2});
            const int after = player->GetInventory().GetItemAmount(ItemType::COINS);
            replay.Submit(ShopCloseCommand{player->GetID(), session});
            if (player->GetInventory().GetItemAmount(ItemType::LOG) != 0 ||
                player->GetInventory().GetItemAmount(ItemType::WOODEN_PLANKS) != 0 ||
                player->GetInventory().GetItemAmount(ItemType::WOODEN_SHIELD) != 0)
                throw std::runtime_error("timber replay left unaccounted crafting materials");
            result << "    { \"cycle\": " << cycle + 1 << ", \"coins_before\": " << before
                << ", \"spent\": " << before - afterBuy << ", \"received\": " << after - afterBuy
                << ", \"coins_after\": " << after << ", \"net\": " << after - before
                << ", \"remaining_logs_planks_shields\": [0, 0, 0] }"
                << (cycle + 1 == cycles ? "\n" : ",\n");
        }
        result << "  ],\n  \"simulation_ticks\": " << replay.ticks << "\n}\n";
        return result.str();
    }

    std::string StarterReplay(const std::optional<StarterWorldExport::Snapshot> &snapshot)
    {
        QuietRuntime quiet;
        constexpr unsigned CombatSeed = 1337U, RewardSeed = 7331U, GatheringSeed = 9001U;
        auto owner = snapshot ? std::make_unique<World>(*snapshot, CombatSeed, RewardSeed, GatheringSeed) :
            std::make_unique<World>(CombatSeed, RewardSeed, GatheringSeed);
        World &world = *owner;
        auto *player = dynamic_cast<Player *>(world.GetEntityByID(world.CreatePlayer(PlayerInitializationMode::FRESH_START)));
        if (player == nullptr || player->GetInventory().GetItemAmount(ItemType::COINS) != 0)
            throw std::runtime_error("starter replay did not create an unfunded fresh character");
        Replay replay{world, *player};
        const auto plan = StarterJourney::GetMaterialPlan(true);
        const auto *gathering = QuestDefinitionDatabase::TryGet(QuestId::GATHERING_BASICS);
        const auto *mining = QuestDefinitionDatabase::TryGet(QuestId::MINING_BASICS);
        if (gathering == nullptr || mining == nullptr || gathering->objectiveItem != ItemType::LOG ||
            mining->objectiveItem != ItemType::COPPER_ORE)
            throw std::runtime_error("starter quest material bindings need review");
        int guideId = 0;
        for (const auto &entity : world.GetEntities())
            if (const auto *npc = dynamic_cast<const NPC *>(entity.get());
                npc != nullptr && npc->GetNpcType() == NpcType::DEVELOPMENT_GUIDE)
                guideId = npc->GetID();
        const auto stationId = [&](StationType type)
        {
            for (const auto &station : world.GetStations())
                if (station.GetStationType() == type) return station.GetID();
            throw std::runtime_error("starter route requires table, furnace and bank");
        };
        if (guideId == 0) throw std::runtime_error("starter route requires Mara");
        struct Phase { std::string name; int ticks; TutorialStep step; std::string objective; };
        std::vector<Phase> phases;
        const auto checkpoint = [&](const char *name)
        {
            const auto guidance = StarterJourney::Describe(*player, world.GetCraftingSession(player->GetID()));
            phases.push_back({name, replay.ticks, player->GetTutorialState().GetStep(), guidance.objective});
        };
        auto dialogue = replay.Talk(guideId);
        for (const auto id : {QuestId::GATHERING_BASICS, QuestId::MINING_BASICS})
            replay.Submit(QuestAcceptCommand{player->GetID(), guideId, dialogue, id});
        replay.Submit(DialogueCloseCommand{player->GetID(), dialogue});
        checkpoint("accept_basic_quests");

        replay.Equip(ItemType::BRONZE_AXE);
        const int logs = std::max(plan.normalLogs, gathering->requiredAmount);
        const int copper = std::max(plan.copperOre, mining->requiredAmount);
        replay.Gather(ResourceType::NORMAL_TREE, ItemType::LOG, logs);
        checkpoint("gather_normal_logs");
        replay.OpenStation(stationId(StationType::CRAFTING_TABLE));
        std::array<ItemType, 9> plankGrid{};
        plankGrid[4] = ItemType::LOG;
        while (player->GetInventory().GetItemAmount(ItemType::WOODEN_PLANKS) < plan.woodenPlanks)
            replay.Craft(plankGrid);
        const std::array<ItemType, 9> shieldGrid{
            ItemType::WOODEN_PLANKS, ItemType::NONE, ItemType::WOODEN_PLANKS,
            ItemType::WOODEN_PLANKS, ItemType::WOODEN_PLANKS, ItemType::WOODEN_PLANKS,
            ItemType::NONE, ItemType::WOODEN_PLANKS, ItemType::NONE};
        replay.Craft(shieldGrid);
        replay.Submit(CloseStationCommand{player->GetID()});
        replay.Equip(ItemType::WOODEN_SHIELD);
        const int firstUpgradeTick = replay.ticks;
        checkpoint("craft_and_equip_shield");

        replay.Equip(ItemType::BRONZE_PICKAXE);
        replay.Gather(ResourceType::COPPER_ROCK, ItemType::COPPER_ORE, copper);
        checkpoint("gather_copper");
        replay.Gather(ResourceType::TIN_ROCK, ItemType::TIN_ORE, plan.tinOre);
        checkpoint("gather_tin");
        replay.OpenStation(stationId(StationType::FURNACE), StationType::FURNACE);
        replay.Submit(StartRecipeCommand{player->GetID(), RecipeType::BRONZE_BAR});
        replay.FinishAction();
        if (player->GetInventory().GetItemAmount(ItemType::BRONZE_BAR) < plan.bronzeBars)
            throw std::runtime_error("starter smelting did not prepare the sword bars");
        checkpoint("smelt_bronze_bars");
        replay.Submit(StartRecipeCommand{player->GetID(), RecipeType::BRONZE_SWORD});
        replay.FinishAction();
        replay.Submit(CloseStationCommand{player->GetID()});
        replay.Equip(ItemType::BRONZE_SWORD);
        const int swordTick = replay.ticks;
        checkpoint("forge_and_equip_sword");

        replay.OpenStation(stationId(StationType::BANK), StationType::BANK);
        while (player->GetInventory().GetItemAmount(ItemType::LOG) > 0)
        {
            const auto *bank = world.GetBankSession(player->GetID());
            if (bank == nullptr) throw std::runtime_error("starter bank session did not open");
            const int slot = replay.Slot(ItemType::LOG);
            replay.Submit(BankDepositCommand{player->GetID(), bank->sessionId, player->GetBank().GetRevision(),
                slot, player->GetInventory().GetSlots()[slot].GetAmount()});
        }
        replay.Submit(BankCloseCommand{player->GetID(), world.GetBankSession(player->GetID())->sessionId});
        checkpoint("bank_spare_logs");
        dialogue = replay.Talk(guideId);
        for (const auto id : {QuestId::GATHERING_BASICS, QuestId::MINING_BASICS})
        {
            replay.Submit(QuestCompleteCommand{player->GetID(), guideId, dialogue, id});
            const auto *record = player->GetQuestJournal().TryGet(id);
            if (record == nullptr || record->state != QuestState::COMPLETED)
                throw std::runtime_error("starter quest reward did not complete");
        }
        replay.Submit(DialogueCloseCommand{player->GetID(), dialogue});
        const int rewards = player->GetInventory().GetItemAmount(ItemType::COINS);
        const auto shop = replay.OpenShop(guideId);
        replay.Submit(ShopBuyCommand{player->GetID(), shop, ItemType::COOKED_MEAT, 2});
        replay.Submit(ShopCloseCommand{player->GetID(), shop});
        checkpoint("claim_rewards_and_buy_food");

        if (player->GetTutorialState().GetStep() != TutorialStep::SAFE_COMBAT ||
            !player->GetEquipment().IsEquipped(ItemType::BRONZE_SWORD) ||
            !player->GetEquipment().IsEquipped(ItemType::WOODEN_SHIELD))
            throw std::runtime_error("starter preparation did not reach the combat tutorial with both upgrades");
        const auto tickMs = std::chrono::duration_cast<std::chrono::milliseconds>(Clock{}.GetTickInterval()).count();
        std::ostringstream out;
        out << "{\n  \"kind\": \"authoritative-starter-upgrade-replay\",\n  \"profile\": \"fresh\",\n"
            << "  \"uses_imported_world\": " << (snapshot ? "true" : "false") << ",\n"
            << "  \"starting_coins\": 0,\n  \"injected_materials_or_xp\": false,\n"
            << "  \"seeds\": {\"combat\": " << CombatSeed << ", \"rewards\": " << RewardSeed << ", \"gathering\": " << GatheringSeed << "},\n"
            << "  \"gathered\": {\"normal_logs\": " << logs << ", \"copper_ore\": " << copper << ", \"tin_ore\": " << plan.tinOre << "},\n"
            << "  \"gathering_attempts\": " << replay.gatheringAttempts << ",\n  \"phases\": [\n";
        int previous = 0;
        for (std::size_t i = 0; i < phases.size(); ++i)
        {
            const auto &phase = phases[i];
            out << "    {\"name\": " << Quote(phase.name) << ", \"ticks\": " << phase.ticks - previous <<
                ", \"cumulative_ticks\": " << phase.ticks << ", \"tutorial_step\": " << static_cast<int>(phase.step) <<
                ", \"next_objective\": " << Quote(phase.objective) << "}" << (i + 1 == phases.size() ? "\n" : ",\n");
            previous = phase.ticks;
        }
        out << "  ],\n  \"first_shield_equipped_tick\": " << firstUpgradeTick << ",\n"
            << "  \"first_sword_equipped_tick\": " << swordTick << ",\n  \"simulation_ticks\": " << replay.ticks << ",\n"
            << "  \"nominal_tick_milliseconds\": " << tickMs << ",\n"
            << "  \"quest_coins_earned\": " << rewards << ",\n"
            << "  \"food_coins_spent\": " << rewards - player->GetInventory().GetItemAmount(ItemType::COINS) << ",\n"
            << "  \"coins_remaining\": " << player->GetInventory().GetItemAmount(ItemType::COINS) << ",\n"
            << "  \"cooked_meat_carried\": " << player->GetInventory().GetItemAmount(ItemType::COOKED_MEAT) << ",\n"
            << "  \"logs_banked\": " << player->GetBank().GetItemAmount(ItemType::LOG) << ",\n"
            << "  \"tutorial_step\": \"SAFE_COMBAT\",\n  \"human_pacing_measured\": false\n}\n";
        return out.str();
    }

    std::string StarterReport()
    {
        const auto *shop = ShopDefinitionDatabase::TryGet(ShopId::DEVELOPMENT_GUIDE_SUPPLIES);
        if (shop == nullptr) throw std::runtime_error("starter shop is missing");
        std::ostringstream out;
        out.imbue(std::locale::classic());
        const auto costs = EconomyBalanceModel::GetProductionCosts(*shop);
        const bool valid = ContentValidator::ValidateShopDefinition(shop->id, *shop).IsValid();
        int questCoins = 0;
        for (const auto id : {QuestId::GATHERING_BASICS, QuestId::MINING_BASICS})
        {
            const auto *quest = QuestDefinitionDatabase::TryGet(id);
            if (quest == nullptr || quest->rewardItem != ItemType::COINS)
                throw std::runtime_error("starter quest budget needs an explicit review");
            questCoins += quest->rewardAmount;
        }
        out << "{\n  \"kind\": \"starter-economy-catalogue\",\n"
            << "  \"shop_validation_passed\": " << (valid ? "true" : "false") << ",\n"
            << "  \"production_costs_converged\": " << (costs.converged ? "true" : "false") << ",\n"
            << "  \"basic_quest_coin_rewards\": " << questCoins << ",\n  \"shop_entries\": [\n";
        for (std::size_t i = 0; i < shop->entries.size(); ++i)
        {
            const auto &entry = shop->entries[i];
            out << "    { \"item\": " << Quote(ItemDatabase::Get(entry.itemType).GetName()) << ", \"buy\": ";
            if (entry.buyPrice) out << *entry.buyPrice; else out << "null";
            out << ", \"sell\": ";
            if (entry.sellPrice) out << *entry.sellPrice; else out << "null";
            out << " }" << (i + 1 == shop->entries.size() ? "\n" : ",\n");
        }
        out << "  ],\n  \"recipe_shop_funded_margins\": [\n";
        const auto &types = RecipeDatabase::GetAllRecipeTypes();
        for (std::size_t i = 0; i < types.size(); ++i)
        {
            const auto &recipe = RecipeDatabase::Get(types[i]);
            const auto margin = EconomyBalanceModel::GetProductionMargin(recipe, *shop, costs);
            out << "    { \"recipe\": " << Quote(recipe.GetName()) << ", \"coins_per_batch\": ";
            if (margin) out << *margin; else out << "null";
            out << " }" << (i + 1 == types.size() ? "\n" : ",\n");
        }
        const auto plan = StarterJourney::GetMaterialPlan(true);
        out << "  ],\n  \"one_sword_and_shield_material_plan\": {\n"
            << "    \"normal_logs\": " << plan.normalLogs << ",\n"
            << "    \"copper_ore\": " << plan.copperOre << ",\n"
            << "    \"tin_ore\": " << plan.tinOre << ",\n"
            << "    \"leftover_planks\": " << plan.leftoverPlanks << ",\n"
            << "    \"leftover_bars\": " << plan.leftoverBars << "\n"
            << "  },\n  \"pacing_measured\": false\n}\n";
        return out.str();
    }
}

int main(int argc, char **argv)
{
    try
    {
        if (argc == 2 && std::string_view(argv[1]) == "--help")
        { std::cout << Usage; return 0; }
        if (argc == 1)
        { std::cout << StarterReport(); return 0; }
        if ((argc == 2 || argc == 4) && std::string_view(argv[1]) == "--replay-starter-upgrade")
        {
            std::optional<StarterWorldExport::Snapshot> snapshot;
            if (argc == 4)
            {
                if (std::string_view(argv[2]) != "--world")
                    throw std::invalid_argument("unsupported starter replay arguments");
                snapshot = StarterWorldImport::LoadFile(argv[3]);
            }
            std::cout << StarterReplay(snapshot);
            return std::cout ? 0 : 1;
        }
        if ((argc != 4 && argc != 6) || std::string_view(argv[1]) != "--replay-timber-cycle" ||
            (argc == 6 && std::string_view(argv[4]) != "--world"))
            throw std::invalid_argument("unsupported economy report arguments");
        const int coins = Number(argv[2], 1000000), cycles = Number(argv[3], 10);
        std::optional<StarterWorldExport::Snapshot> snapshot;
        if (argc == 6) snapshot = StarterWorldImport::LoadFile(argv[5]);
        std::cout << TimberReplay(coins, cycles, snapshot);
        return std::cout ? 0 : 1;
    }
    catch (const std::invalid_argument &error)
    { std::cerr << error.what() << '\n' << Usage; return 2; }
    catch (const std::exception &error)
    { std::cerr << "Economy report failed: " << error.what() << '\n'; return 1; }
}
