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
#include "World/World.h"

#include <array>
#include <charconv>
#include <iostream>
#include <locale>
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
        "       EmbervaleEconomyReport --help\n"
        "No arguments: report current starter prices and recipes.\n"
        "Replay uses a fresh in-memory character with explicit seed coins, real movement, shop and crafting commands.\n"
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

    int IngredientAmount(const RecipeDefinition &recipe, ItemType type)
    {
        for (const auto &ingredient : recipe.GetIngredients())
            if (ingredient.itemType == type) return ingredient.amount;
        throw std::runtime_error("starter recipe no longer has the expected material binding");
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

        void Step()
        {
            if (++ticks > 20000)
                throw std::runtime_error("bounded replay exceeded 20000 ticks");
            world.Update();
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

        void OpenTable(int tableId)
        {
            const CraftingStation *target = nullptr;
            for (const auto &station : world.GetStations())
                if (station.GetID() == tableId) target = &station;
            if (target == nullptr) throw std::runtime_error("crafting table is missing");
            Submit(InteractCommand{player.GetID(), InteractionTargetType::STATION,
                tableId, Position{target->GetX(), target->GetY()}});
            for (int i = 0; i < 2000; ++i)
            {
                Step();
                StationType type = StationType::NONE;
                if (world.ConsumeOpenedStation(player.GetID(), type) && type == StationType::CRAFTING_TABLE)
                    return;
            }
            throw std::runtime_error("could not approach/open the crafting table");
        }

        void Craft(const std::array<ItemType, 9> &grid)
        {
            Submit(CraftRecipeCommand{player.GetID(), grid});
            for (int i = 0; world.GetActionForEntity(player.GetID()) != nullptr; ++i)
            {
                if (i >= 100)
                    throw std::runtime_error("craft action did not finish");
                Step();
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
            replay.OpenTable(tableId);
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
        const auto &sword = RecipeDatabase::Get(RecipeType::BRONZE_SWORD);
        const auto &shield = RecipeDatabase::Get(RecipeType::WOODEN_SHIELD);
        const auto &planks = RecipeDatabase::Get(RecipeType::WOODEN_PLANKS);
        const auto &bar = RecipeDatabase::Get(RecipeType::BRONZE_BAR);
        if (planks.GetOutputAmount() <= 0 || bar.GetOutputAmount() <= 0)
            throw std::runtime_error("starter processing recipes need positive outputs");
        const int plankAmount = IngredientAmount(sword, ItemType::WOODEN_PLANKS) +
            IngredientAmount(shield, ItemType::WOODEN_PLANKS);
        const int plankBatches = (plankAmount + planks.GetOutputAmount() - 1) / planks.GetOutputAmount();
        const int barAmount = IngredientAmount(sword, ItemType::BRONZE_BAR);
        const int barBatches = (barAmount + bar.GetOutputAmount() - 1) / bar.GetOutputAmount();
        out << "  ],\n  \"one_sword_and_shield_material_plan\": {\n"
            << "    \"normal_logs\": " << plankBatches * IngredientAmount(planks, ItemType::LOG) << ",\n"
            << "    \"copper_ore\": " << barBatches * IngredientAmount(bar, ItemType::COPPER_ORE) << ",\n"
            << "    \"tin_ore\": " << barBatches * IngredientAmount(bar, ItemType::TIN_ORE) << ",\n"
            << "    \"leftover_planks\": " << plankBatches * planks.GetOutputAmount() - plankAmount << ",\n"
            << "    \"leftover_bars\": " << barBatches * bar.GetOutputAmount() - barAmount << "\n"
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
