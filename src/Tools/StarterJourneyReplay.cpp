#include "StarterJourneyReplay.h"
#include "AuthoritativeReplay.h"
#include "Entity/Monster/Monster.h"
#include "Item/ItemDatabase.h"
#include "NPC/NPC.h"
#include "NPC/NpcDefinitionDatabase.h"
#include "Persistence/PlayerSaveTextCodec.h"

#include <chrono>
#include <filesystem>
#include <fstream>
#include <map>
#include <memory>
#include <set>
#include <sstream>
#include <stdexcept>
#include <vector>

namespace
{
    void Require(bool condition, const std::string &message)
    {
        if (!condition) throw std::runtime_error("starter journey: " + message);
    }

    std::string Quote(const std::string &text)
    {
        std::string out = "\"";
        constexpr char hex[] = "0123456789abcdef";
        for (const unsigned char ch : text)
        {
            if (ch == '"' || ch == '\\') { out += '\\'; out += static_cast<char>(ch); }
            else if (ch < 32) { out += "\\u00"; out += hex[ch >> 4]; out += hex[ch & 15]; }
            else out += static_cast<char>(ch);
        }
        return out + '"';
    }

    struct PrivateSaves
    {
        std::filesystem::path directory;
        PrivateSaves()
        {
            const auto base = std::filesystem::temp_directory_path();
            const auto nonce = std::chrono::steady_clock::now().time_since_epoch().count();
            for (int attempt = 0; attempt < 100; ++attempt)
            {
                auto candidate = base / ("embervale-journey-" +
                    std::to_string(nonce) + "-" + std::to_string(attempt));
                if (std::filesystem::create_directory(candidate))
                {
                    directory = std::move(candidate);
                    return;
                }
            }
            throw std::runtime_error("could not create private disposable replay saves");
        }
        ~PrivateSaves()
        {
            if (!directory.empty())
            {
                std::error_code error;
                std::filesystem::remove_all(directory, error);
            }
        }
        PrivateSaves(const PrivateSaves &) = delete;
        PrivateSaves &operator=(const PrivateSaves &) = delete;
    };

    std::string Read(const std::filesystem::path &path)
    {
        Require(std::filesystem::file_size(path) <= MAX_PLAYER_SAVE_TEXT_BYTES, "oversized replay save");
        std::ifstream file(path, std::ios::binary);
        Require(static_cast<bool>(file), "could not read checkpoint");
        return {std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>()};
    }

    Player &Actor(World &world, int id)
    {
        auto *player = dynamic_cast<Player *>(world.GetEntityByID(id));
        Require(player != nullptr, "actor disappeared");
        return *player;
    }

    std::unique_ptr<World> NewWorld(const StarterWorldExport::Snapshot &snapshot)
    {
        return std::make_unique<World>(snapshot, 1337U, 7331U, 9001U);
    }

    using Totals = std::map<ItemType, long long>;
    Totals Owned(World &world, const Player &player)
    {
        Totals totals;
        for (const auto item : ItemDatabase::GetAllItemTypes()) totals[item] = 0;
        for (const auto &slot : player.GetInventory().GetSlots())
            if (!slot.IsEmpty()) totals[slot.GetItemType()] += slot.GetAmount();
        for (int i = 0; i < static_cast<int>(EquipmentSlotType::COUNT); ++i)
        {
            const auto item = player.GetEquipment().GetEquippedItem(static_cast<EquipmentSlotType>(i));
            if (item != ItemType::NONE) ++totals[item];
        }
        for (const auto &entry : player.GetBank().GetEntries())
            totals[entry.itemType] += entry.quantity;
        if (const auto *pile = world.GetPlayerRecoveryPile(player.GetID()))
            for (const auto &item : pile->items) totals[item.itemType] += item.quantity;
        return totals;
    }

    CommandResultCode Command(AuthoritativeReplay &replay, ServerCommandData command,
        bool allowDeath = false)
    {
        const auto id = replay.world.EnqueueCommand(std::move(command));
        replay.Step(allowDeath);
        for (const auto &result : replay.world.GetCommandProcessingResults())
            if (result.commandID == id) return result.resultCode;
        throw std::runtime_error("starter journey: command result was not published");
    }

    int Npc(World &world, NpcType type)
    {
        for (const auto &entity : world.GetEntities())
        {
            if (const auto *monster = dynamic_cast<const Monster *>(entity.get());
                monster && monster->GetNpcType() == type) return monster->GetID();
            if (const auto *npc = dynamic_cast<const NPC *>(entity.get());
                npc && npc->GetNpcType() == type) return npc->GetID();
        }
        throw std::runtime_error("starter journey: required authored NPC is missing");
    }

    int BankStation(World &world)
    {
        for (const auto &station : world.GetStations())
            if (station.GetStationType() == StationType::BANK) return station.GetID();
        throw std::runtime_error("starter journey: bank is missing");
    }

    struct Stage
    {
        std::string name;
        int tick, worldTick, session, health, tutorial;
    };
    struct SaveCheck
    {
        std::string stage;
        bool recoveryPile;
        int remainingPileTicks;
        bool idleCheckpointChecked;
    };
    struct Restored
    {
        std::unique_ptr<World> world;
        int actorID;
        std::filesystem::path checkpoint;
    };

    Restored RoundTrip(AuthoritativeReplay &replay, const std::string &name,
        const StarterWorldExport::Snapshot &snapshot,
        const PrivateSaves &saves, std::vector<SaveCheck> &checks)
    {
        const auto file = saves.directory / (name + ".save");
        Require(replay.world.SavePlayerToFile(replay.player.GetID(), file).IsSuccess(),
            "could not save " + name);
        const auto bytes = Read(file);
        auto restored = NewWorld(snapshot);
        const auto loaded = restored->LoadOrCreatePlayerFromFile(file, PlayerInitializationMode::FRESH_START);
        Require(loaded.IsSuccess() && loaded.WasLoadedFromFile() && !loaded.UsedSpawnFallback(),
            "could not restore " + name + " without a fallback");
        auto &player = Actor(*restored, loaded.GetPlayerEntityID());
        const auto second = saves.directory / (name + "-roundtrip.save");
        Require(restored->SavePlayerToFile(player.GetID(), second).IsSuccess() && Read(second) == bytes,
            "canonical state changed across " + name + " save/reload");
        const auto live = saves.directory / (name + "-live.save");
        Require(replay.world.SavePlayerToFile(replay.player.GetID(), live).IsSuccess() && Read(live) == bytes,
            "save changed live state at " + name);
        Require(Owned(replay.world, replay.player) == Owned(*restored, player),
            "owned items changed across " + name);
        const auto step = player.GetTutorialState().GetStep();
        const bool pendingCheckpoint = step == TutorialStep::LOOT_AND_BANK ||
            step == TutorialStep::RETURN_AND_CHOOSE;
        if (pendingCheckpoint)
        {
            restored->Update();
            Require(player.GetTutorialState().GetStep() == step,
                "loaded tutorial checkpoint advanced without its required action");
        }
        const auto *pile = restored->GetPlayerRecoveryPile(player.GetID());
        if (pile)
            Require(pile->ownerPlayerEntityID == player.GetID(), "loaded pile ownership was not remapped");
        checks.push_back({name, pile != nullptr, pile ? pile->expiryTick - restored->GetCurrentTick() : 0,
            pendingCheckpoint});
        return {std::move(restored), player.GetID(), file};
    }
}

StarterJourneyReplayResult RunStarterJourneyContinuation(World &world, Player &player,
    const StarterWorldExport::Snapshot &snapshot, int preparedTicks)
{
    PrivateSaves saves;
    AuthoritativeReplay replay{world, player, preparedTicks};
    std::vector<Stage> stages;
    std::vector<SaveCheck> saveChecks;
    const auto checkpoint = [&](const char *name, const AuthoritativeReplay &session, int index = 0)
    {
        stages.push_back({name, session.ticks, session.world.GetCurrentTick(), index,
            session.player.GetCurrentHealth(), static_cast<int>(session.player.GetTutorialState().GetStep())});
    };
    checkpoint("prepared", replay);
    RoundTrip(replay, "prepared", snapshot, saves, saveChecks);
    int foodUsed = 0, healthRestored = 0;
    bool healedCombatInjury = false;
    std::string foodStage = "equipment_health";
    std::vector<std::string> foodEvents;
    const int heal = ItemDatabase::Get(ItemType::COOKED_MEAT).GetFoodDefinition()->healAmount;
    const auto eat = [&](AuthoritativeReplay &session)
    {
        const int beforeHP = session.player.GetCurrentHealth();
        const int beforeFood = session.player.GetInventory().GetItemAmount(ItemType::COOKED_MEAT);
        session.Submit(UseInventoryItemCommand{session.player.GetID(), session.Slot(ItemType::COOKED_MEAT)});
        int incoming = 0;
        for (const auto &event : session.world.GetMeleeCombatEvents())
            if (event.defenderEntityID == session.player.GetID()) incoming += event.actualDamageApplied;
        const int restored = std::min(heal, session.player.GetMaximumHealth() - beforeHP);
        Require(session.player.GetInventory().GetItemAmount(ItemType::COOKED_MEAT) == beforeFood - 1 &&
            session.player.GetCurrentHealth() == beforeHP + restored - incoming,
            "food command did not commit one item and the expected capped healing");
        ++foodUsed;
        healthRestored += restored;
        healedCombatInjury = healedCombatInjury || foodStage != "equipment_health";
        foodEvents.push_back("{\"stage\": " + Quote(foodStage) + ", \"health_before\": " +
            std::to_string(beforeHP) + ", \"healed\": " + std::to_string(restored) +
            ", \"same_tick_incoming_damage\": " + std::to_string(incoming) + "}");
    };

    const int preparedHealth = player.GetCurrentHealth();
    const int equippedMaximumHealth = player.GetMaximumHealth();
    if (preparedHealth < equippedMaximumHealth) eat(replay);
    Require(player.GetCurrentHealth() == player.GetMaximumHealth(), "equipment health gap was not healed");
    const int fullFood = player.GetInventory().GetItemAmount(ItemType::COOKED_MEAT);
    Require(Command(replay, UseInventoryItemCommand{player.GetID(), replay.Slot(ItemType::COOKED_MEAT)}) ==
        CommandResultCode::GAMEPLAY_REJECTED &&
        player.GetInventory().GetItemAmount(ItemType::COOKED_MEAT) == fullFood,
        "full-health food use must reject without consuming food");
    checkpoint("check_food_before_combat", replay);
    foodStage = "first_combat";

    std::map<ItemType, int> inventoryBeforeCombat;
    for (const auto item : ItemDatabase::GetAllItemTypes())
        inventoryBeforeCombat[item] = player.GetInventory().GetItemAmount(item);
    const int target = Npc(world, NpcType::PASSIVE_DEVELOPMENT_MONSTER);
    int damageTaken = 0, damageDealt = 0, firstKillTick = -1, minimumHealth = player.GetCurrentHealth();
    const auto observeCombat = [&]
    {
        minimumHealth = std::min(minimumHealth, player.GetCurrentHealth());
        for (const auto &event : world.GetMeleeCombatEvents())
        {
            if (event.defenderEntityID == player.GetID()) damageTaken += event.actualDamageApplied;
            if (event.attackerEntityID == player.GetID()) damageDealt += event.actualDamageApplied;
        }
        for (const auto &event : world.GetEntityDiedEvents())
            if (event.deadEntityID == target && event.killerEntityID == player.GetID())
                firstKillTick = replay.ticks;
    };
    replay.Submit(AttackCommand{player.GetID(), target});
    observeCombat();
    for (int bound = 0; firstKillTick < 0; ++bound)
    {
        Require(bound < 2000, "starter fight exceeded its bound");
        if (player.GetCurrentHealth() <= player.GetMaximumHealth() - heal &&
            player.GetInventory().GetItemAmount(ItemType::COOKED_MEAT) > 0)
            eat(replay);
        else replay.Step();
        observeCombat();
    }
    checkpoint("defeat_mossling", replay);
    RoundTrip(replay, "awaiting-bank", snapshot, saves, saveChecks);
    std::vector<ItemAmount> loot;
    for (const auto item : ItemDatabase::GetAllItemTypes())
    {
        const int gain = player.GetInventory().GetItemAmount(item) - inventoryBeforeCombat[item];
        if (gain > 0) loot.push_back({item, gain});
    }
    Require(!loot.empty(), "starter kill did not commit its guaranteed inventory loot");
    foodStage = "after_first_combat";
    if (player.GetCurrentHealth() < player.GetMaximumHealth() &&
        player.GetInventory().GetItemAmount(ItemType::COOKED_MEAT) > 0) eat(replay);
    checkpoint("food_after_combat", replay);
    Require(damageDealt > 0 && player.IsAlive(), "starter fight did not produce a living winner");

    // Monster rewards go directly to inventory. Exercise ground pickup with
    // an explicitly player-dropped owned item; never invent a monster drop.
    const auto held = Owned(world, player);
    std::set<int> oldGroundIDs;
    for (const auto &item : world.GetGroundItems()) oldGroundIDs.insert(item.id);
    replay.Submit(DropInventoryItemCommand{player.GetID(), replay.Slot(loot.front().itemType), 1});
    int droppedID = -1;
    for (const auto &item : world.GetGroundItems())
        if (!oldGroundIDs.contains(item.id)) droppedID = item.id;
    Require(droppedID >= 0, "owned loot-type item did not create a player drop");
    replay.Submit(TakeGroundItemCommand{player.GetID(), droppedID});
    for (int bound = 0; world.GetGroundItemByID(droppedID); ++bound)
    {
        Require(bound < 2000, "could not approach/pick up the player-dropped item");
        replay.Step();
    }
    Require(Owned(world, player) == held, "drop/pickup changed owned quantities");
    checkpoint("drop_and_reclaim_owned_loot_type", replay);
    int lootQuantity = 0;
    for (const auto &item : loot) lootQuantity += item.quantity;
    const bool waitedForPostCombatBank =
        player.GetTutorialState().GetStep() == TutorialStep::LOOT_AND_BANK;
    const auto ownedBeforeBank = Owned(world, player);
    std::map<ItemType, int> bankBefore;
    for (const auto &item : loot) bankBefore[item.itemType] = player.GetBank().GetItemAmount(item.itemType);
    replay.OpenStation(BankStation(world), StationType::BANK);
    for (const auto &drop : loot)
    {
        int remaining = drop.quantity;
        while (remaining > 0)
        {
            const auto *bank = world.GetBankSession(player.GetID());
            Require(bank != nullptr, "post-combat bank session disappeared");
            const int slot = replay.Slot(drop.itemType);
            const int amount = std::min(remaining, player.GetInventory().GetSlots()[slot].GetAmount());
            replay.Submit(BankDepositCommand{player.GetID(), bank->sessionId,
                player.GetBank().GetRevision(), slot, amount});
            remaining -= amount;
        }
    }
    replay.Submit(BankCloseCommand{player.GetID(), world.GetBankSession(player.GetID())->sessionId});
    Require(Owned(world, player) == ownedBeforeBank, "banking loot changed total owned quantities");
    for (const auto &item : loot)
        Require(player.GetBank().GetItemAmount(item.itemType) == bankBefore[item.itemType] + item.quantity,
            "bank did not receive the complete earned loot quantity");
    checkpoint("bank_combat_loot", replay);
    RoundTrip(replay, "awaiting-mara", snapshot, saves, saveChecks);
    const bool waitedForReturn =
        player.GetTutorialState().GetStep() == TutorialStep::RETURN_AND_CHOOSE;
    const auto dialogue = replay.Talk(Npc(world, NpcType::DEVELOPMENT_GUIDE));
    Require(player.GetTutorialState().IsCompleted() && !player.GetTutorialState().IsSkipped(),
        "returning to Mara did not complete the normal tutorial");
    replay.Submit(DialogueCloseCommand{player.GetID(), dialogue});
    checkpoint("return_to_mara", replay);
    auto afterCombat = RoundTrip(replay, "post-combat", snapshot, saves, saveChecks);

    Totals expected = Owned(world, player);
    const int predator = Npc(world, NpcType::AGGRESSIVE_DEVELOPMENT_MONSTER);
    const Position danger = world.GetEntityByID(predator)->GetPosition();
    // Intentional negative case: walk into authored danger, then stand without
    // attacking/eating. Damage, death and respawn still use production systems.
    Require(Command(replay, MoveCommand{player.GetID(), danger}, true) == CommandResultCode::ACCEPTED,
        "could not enter the deliberate death exercise");
    foodStage = "death_exercise";
    for (int bound = 0; player.IsAlive(); ++bound)
    {
        Require(bound < 2000, "authored aggressive enemy did not resolve the death exercise");
        if (!healedCombatInjury && player.GetInventory().GetItemAmount(ItemType::COOKED_MEAT) > 0 &&
            player.GetCurrentHealth() <= player.GetMaximumHealth() - heal)
        {
            eat(replay);
            --expected[ItemType::COOKED_MEAT];
        }
        else replay.Step(true);
    }
    checkpoint("deliberate_combat_death", replay);
    const auto *recovery = world.GetPlayerDeathRecovery(player.GetID());
    const auto *pile = world.GetPlayerRecoveryPile(player.GetID());
    Require(recovery && pile && recovery->killerEntityID == predator,
        "actual enemy death did not create recovery metadata and protected items");
    Require(Owned(world, player) == expected, "death lost or duplicated an owned item");
    Totals protectedItems;
    for (const auto &item : pile->items) protectedItems[item.itemType] += item.quantity;
    const int deathTick = replay.ticks, respawnDelay = recovery->respawnTick - world.GetCurrentTick();
    const auto savedBytes = Read(afterCombat.checkpoint);
    const auto rejected = world.SavePlayerToFile(player.GetID(), afterCombat.checkpoint);
    const bool deadSavePreserved = !rejected.IsSuccess() &&
        rejected.Contains(WorldPlayerPersistenceIssueCode::PLAYER_NOT_ALIVE) &&
        Read(afterCombat.checkpoint) == savedBytes;
    Require(deadSavePreserved, "dead save did not preserve the previous living checkpoint");
    for (int bound = 0; !player.IsAlive(); ++bound)
    {
        Require(bound < 100, "respawn delay did not expire");
        replay.Step(true);
    }
    Require(replay.ticks - deathTick == respawnDelay &&
        player.GetCurrentHealth() == player.GetMaximumHealth() &&
        world.GetPlayerDeathRecovery(player.GetID()) == nullptr,
        "respawn did not restore a living player at the scheduled tick");
    const auto spawn = snapshot.playerSpawn;
    Require(player.GetPosition().GetX() == spawn.GetX() && player.GetPosition().GetY() == spawn.GetY(),
        "respawn did not return to the authored safe point");
    checkpoint("respawn_at_safe_point", replay);
    Require(Owned(world, player) == expected, "respawn changed protected ownership");
    auto resumed = RoundTrip(replay, "pending-recovery", snapshot, saves, saveChecks);
    Require(resumed.world->GetPlayerRecoveryPile(resumed.actorID), "reload lost the protected recovery pile");
    auto &loadedPlayer = Actor(*resumed.world, resumed.actorID);
    AuthoritativeReplay continued{*resumed.world, loadedPlayer, replay.ticks};
    const Position recoveryPosition = resumed.world->GetPlayerRecoveryPile(resumed.actorID)->position;
    continued.Submit(MoveCommand{resumed.actorID, recoveryPosition});
    for (int bound = 0; resumed.world->GetPlayerRecoveryPile(resumed.actorID); ++bound)
    {
        Require(bound < 2000, "could not walk back to the persisted recovery pile");
        continued.Step();
    }
    Require(Owned(*resumed.world, loadedPlayer) == expected,
        "reclaiming the persisted pile lost or duplicated an item");
    checkpoint("reclaim_persisted_pile", continued, 1);
    continued.Submit(MoveCommand{resumed.actorID, loadedPlayer.GetPosition()});
    continued.Step();
    Require(!resumed.world->GetPlayerRecoveryPile(resumed.actorID) &&
        Owned(*resumed.world, loadedPlayer) == expected,
        "repeated updates duplicated the recovered pile");
    const auto returnDialogue = continued.Talk(Npc(*resumed.world, NpcType::DEVELOPMENT_GUIDE));
    continued.Submit(DialogueCloseCommand{resumed.actorID, returnDialogue});
    checkpoint("return_safely_after_recovery", continued, 1);
    Require(foodUsed > 0 && healedCombatInjury, "no actual combat-injury food use was exercised");
    RoundTrip(continued, "recovered", snapshot, saves, saveChecks);
    Require(Owned(*resumed.world, loadedPlayer) == expected &&
        loadedPlayer.GetTutorialState().IsCompleted() &&
        loadedPlayer.GetBank().GetItemAmount(ItemType::LOG) == 8 &&
        loadedPlayer.GetEquipment().IsEquipped(ItemType::BRONZE_SWORD) &&
        loadedPlayer.GetEquipment().IsEquipped(ItemType::WOODEN_SHIELD),
        "post-recovery progress, bank or equipment did not survive");
    const bool ordered = waitedForPostCombatBank && waitedForReturn;
    std::ostringstream out;
    out << "{\n  \"all_checks_passed\": " << (ordered ? "true" : "false") << ",\n"
        << "  \"authoritative_commands_only\": true,\n  \"injected_damage_stats_items_or_xp\": false,\n"
        << "  \"human_pacing_measured\": false,\n  \"visible_playthrough_performed\": false,\n"
        << "  \"primary_play_sessions\": 2,\n  \"world_and_rng_restart_on_reload\": true,\n"
        << "  \"first_kill_tick\": " << firstKillTick << ",\n  \"first_combat_damage_dealt\": " << damageDealt
        << ",\n  \"first_combat_damage_taken\": " << damageTaken << ",\n  \"first_combat_minimum_health\": " << minimumHealth
        << ",\n  \"food_consumed\": " << foodUsed << ",\n  \"actual_health_restored\": " << healthRestored
        << ",\n  \"full_health_food_rejected_without_consumption\": true,\n  \"loot_items_received_and_banked\": " << loot.size()
        << ",\n  \"loot_quantity\": " << lootQuantity << ",\n  \"waited_for_post_combat_bank\": "
        << (waitedForPostCombatBank ? "true" : "false") << ",\n  \"waited_for_return_to_mara\": " << (waitedForReturn ? "true" : "false")
        << ",\n  \"dead_save_rejected_preserving_checkpoint\": true,\n  \"respawn_delay_ticks\": " << respawnDelay
        << ",\n  \"protected_items_conserved\": true,\n  \"recovery_reclaimed_exactly_once\": true,\n"
        << "  \"total_simulation_ticks\": " << continued.ticks << ",\n  \"stages\": [\n";
    for (std::size_t i = 0; i < stages.size(); ++i)
    {
        const auto &stage = stages[i];
        out << "    {\"name\": " << Quote(stage.name) << ", \"cumulative_ticks\": " << stage.tick
            << ", \"world_tick\": " << stage.worldTick << ", \"session\": " << stage.session
            << ", \"health\": " << stage.health << ", \"tutorial_step\": " << stage.tutorial << "}"
            << (i + 1 == stages.size() ? "\n" : ",\n");
    }
    out << "  ],\n  \"protected_pile_items\": [";
    std::size_t itemIndex = 0;
    for (const auto &[item, amount] : protectedItems)
        out << (itemIndex++ ? "," : "") << "{\"item\": " << Quote(ItemDatabase::Get(item).GetName())
            << ", \"quantity\": " << amount << "}";
    out << "],\n  \"monster_loot_delivery\": \"direct_to_inventory\",\n  \"ground_pickup_exercised_with_owned_loot_type\": true,\n  \"food_events\": [";
    for (std::size_t i = 0; i < foodEvents.size(); ++i)
        out << foodEvents[i] << (i + 1 == foodEvents.size() ? "" : ",");
    out << "],\n  \"prepared_health\": " << preparedHealth
        << ",\n  \"equipped_maximum_health\": " << equippedMaximumHealth
        << ",\n  \"save_round_trips\": [\n";
    for (std::size_t i = 0; i < saveChecks.size(); ++i)
    {
        const auto &check = saveChecks[i];
        out << "    {\"stage\": " << Quote(check.stage)
            << ", \"canonical_bytes_equal\": true, \"live_state_unchanged\": true, \"items_equal\": true"
            << ", \"pending_recovery_pile\": " << (check.recoveryPile ? "true" : "false")
            << ", \"pile_remaining_ticks\": " << check.remainingPileTicks
            << ", \"reload_idle_tick_checked\": " << (check.idleCheckpointChecked ? "true" : "false") << "}"
            << (i + 1 == saveChecks.size() ? "\n" : ",\n");
    }
    out << "  ]\n}\n";
    return {out.str(), ordered};
}
