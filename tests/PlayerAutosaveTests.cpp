#include "TestSupport.h"
#include "../src/Persistence/WorldPlayerPersistenceLifecycle.h"
#include "../src/Persistence/PlayerSaveFileStore.h"
#include "../src/Player/Player.h"
#include "../src/World/World.h"
#include "../src/Core/RandomSource.h"

#include <chrono>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <iostream>
#include <memory>

using namespace std::chrono_literals;
using Lifecycle = WorldPlayerPersistenceLifecycle;
using Status = PlayerAutosaveStatus;

namespace
{
    struct Fixture
    {
        std::filesystem::path path = std::filesystem::temp_directory_path() /
            ("embervale_autosave_" + std::to_string(Lifecycle::Clock::now().time_since_epoch().count()));
        Fixture() { std::filesystem::create_directories(path); }
        ~Fixture() { std::error_code error; std::filesystem::remove_all(path, error); }
    };
    std::string Bytes(const std::filesystem::path &path)
    {
        std::ifstream file(path, std::ios::binary);
        return {std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>()};
    }
    Player &Actor(World &world, int id)
    {
        return *dynamic_cast<Player *>(world.GetEntityByID(id));
    }
    int SavedCoins(const std::filesystem::path &path)
    {
        World restored;
        const auto loaded = restored.LoadOrCreatePlayerFromFile(path);
        if (!loaded.IsSuccess() || !loaded.WasLoadedFromFile()) return -1;
        return Actor(restored, loaded.GetPlayerEntityID()).GetInventory().GetItemAmount(ItemType::COINS);
    }
    class FatalRolls final : public RandomSource
    {
        int NextIntInclusive(int, int) override { return calls++ == 0 ? 9 : 1; }
        int calls = 0;
    };
}

int main(int argc, char **argv)
{
    // The companion process test kills this child after the checkpoint, with
    // changed live state and no controlled Shutdown/destructor path.
    if (argc == 3 && std::string(argv[1]) == "--checkpoint-and-wait")
    {
        World world;
        Lifecycle lifecycle(argv[2], PlayerInitializationMode::EMPTY);
        const Lifecycle::TimePoint start{};
        const auto loaded = lifecycle.Start(world, start);
        if (!loaded.IsSuccess()) return 2;
        auto &player = Actor(world, loaded.GetPlayerEntityID());
        player.GetInventory().AddItem(ItemType::COINS, 71);
        world.Update();
        if (lifecycle.PollAutosave(world, start + 60s).status != Status::SAVED) return 3;
        player.GetInventory().AddItem(ItemType::COINS, 1);
        std::cout << "checkpoint ready" << std::endl;
        std::cin.get();
        return 4; // Reaching normal exit is a failure of the interruption fixture.
    }
    if (argc == 3 && std::string(argv[1]) == "--verify-checkpoint")
        return SavedCoins(argv[2]) == 71 ? 0 : 5;

    TestContext test;
    Fixture fixture;
    const Lifecycle::TimePoint start{};
    {
        const auto path = fixture.path / "periodic" / "player.save";
        World world;
        Lifecycle lifecycle(path, PlayerInitializationMode::EMPTY);
        test.Expect(lifecycle.PollAutosave(world, start + 1h).status == Status::INACTIVE,
            "Unstarted lifecycle cannot write");
        const auto loaded = lifecycle.Start(world, start);
        test.Expect(loaded.IsSuccess(), "Persistent fresh character starts");
        auto &player = Actor(world, loaded.GetPlayerEntityID());
        player.GetInventory().AddItem(ItemType::COINS, 10);
        test.Expect(lifecycle.PollAutosave(world, start + 59999ms).status == Status::NOT_DUE &&
                    !std::filesystem::exists(path.parent_path()),
            "First minute creates no directory, sidecar or save");
        world.Update();
        test.Expect(lifecycle.PollAutosave(world, start + 60s).status == Status::SAVED &&
                    SavedCoins(path) == 10,
            "Minute checkpoint reloads without controlled shutdown");
        player.GetInventory().AddItem(ItemType::COINS, 1);
        test.Expect(lifecycle.PollAutosave(world, start + 60s).status == Status::NOT_DUE &&
                    lifecycle.PollAutosave(world, start + 119s).status == Status::NOT_DUE &&
                    SavedCoins(path) == 10,
            "Catch-up ticks and early polls cannot repeat a checkpoint");
        test.Expect(lifecycle.PollAutosave(world, start + 1h).status == Status::SAVED &&
                    SavedCoins(path) == 11 &&
                    lifecycle.PollAutosave(world, start + 1h).status == Status::NOT_DUE,
            "Large time jump writes one current snapshot without catch-up bursts");
        player.GetInventory().AddItem(ItemType::COINS, 1);
        test.Expect(lifecycle.SaveNow(world, start + 3601s).IsSuccess() &&
                    lifecycle.PollAutosave(world, start + 3660s).status == Status::NOT_DUE &&
                    lifecycle.PollAutosave(world, start + 3661s).status == Status::SAVED,
            "Explicit saves restart the minute deadline");
        test.Expect(lifecycle.Shutdown(world, start + 3662s).IsSuccess(), "Controlled exit still saves");
        const auto bytes = Bytes(path);
        player.GetInventory().AddItem(ItemType::COINS, 1);
        test.Expect(lifecycle.PollAutosave(world, start + 10h).status == Status::INACTIVE &&
                    Bytes(path) == bytes, "Successful shutdown disarms autosave");
    }
    {
        const auto path = fixture.path / "retry.save";
        World world;
        Lifecycle lifecycle(path, PlayerInitializationMode::EMPTY);
        const auto loaded = lifecycle.Start(world, start);
        auto &player = Actor(world, loaded.GetPlayerEntityID());
        player.GetInventory().AddItem(ItemType::COINS, 20);
        test.Expect(lifecycle.PollAutosave(world, start + 60s).status == Status::SAVED,
            "Retry fixture establishes a valid canonical save");
        const auto original = Bytes(path);
        // A non-empty sidecar directory cannot be cleaned or overwritten, even
        // with elevated filesystem privileges. The canonical file stays intact.
        auto blocked = path; blocked += ".tmp";
        std::filesystem::create_directories(blocked);
        std::ofstream(blocked / "blocker") << "keep";
        player.GetInventory().AddItem(ItemType::COINS, 5);
        const auto failed = lifecycle.PollAutosave(world, start + 120s);
        test.Expect(failed.status == Status::FAILED && failed.operation &&
                    failed.operation->GetWorldSaveResult().Contains(WorldPlayerPersistenceIssueCode::FILE_SAVE_FAILED) &&
                    Bytes(path) == original,
            "Filesystem failure reports a typed error and preserves valid save bytes");
        test.Expect(lifecycle.GetState() == WorldPlayerPersistenceLifecycleState::ACTIVE,
            "Failed periodic checkpoint leaves gameplay persistence active");
        std::filesystem::remove_all(blocked);
        test.Expect(lifecycle.PollAutosave(world, start + 129999ms).status == Status::NOT_DUE &&
                    Bytes(path) == original, "Repair does not bypass the retry backoff");
        test.Expect(lifecycle.PollAutosave(world, start + 130s).status == Status::SAVED &&
                    SavedCoins(path) == 25, "Ten-second retry saves the latest state after repair");
        const auto corrupt = fixture.path / "corrupt.save";
        std::ofstream(corrupt) << "not a save";
        auto backup = corrupt; backup += ".bak";
        std::ofstream(backup, std::ios::binary) << original;
        Lifecycle rejected(corrupt);
        World invalid;
        test.Expect(!rejected.Start(invalid, start).IsSuccess() &&
                    rejected.PollAutosave(invalid, start + 1h).status == Status::INACTIVE &&
                    Bytes(corrupt) == "not a save" && Bytes(backup) == original,
            "Corrupt startup never arms autosave or erases recovery artifacts");
    }
    {
        World world;
        const auto path = fixture.path / "crafting.save";
        Lifecycle lifecycle(path, PlayerInitializationMode::EMPTY);
        const auto loaded = lifecycle.Start(world, start);
        const int id = loaded.GetPlayerEntityID();
        auto &player = Actor(world, id);
        player.GetPosition().SetPosition(16, 9);
        world.QueueStationInteraction(id, world.GetStationAt(17, 9)->GetID());
        world.Update();
        const auto *session = world.GetCraftingSession(id);
        test.Expect(session != nullptr, "Crafting fixture opens an authoritative session");
        if (session)
        {
            player.GetInventory().AddItem(ItemType::COINS, 7);
            player.GetInventory().AddItem(ItemType::LOG, 1);
            ReplaceCraftingSessionStateCommand move{};
            move.actorEntityID = id;
            move.sessionId = session->sessionId;
            move.expectedRevision = session->revision;
            move.cursor.SetItem(ItemType::COINS, 5);
            move.inputSlots[4].SetItem(ItemType::LOG, 1);
            world.EnqueueCommand(move);
            world.Update();
            test.Expect(lifecycle.PollAutosave(world, start + 60s).status == Status::SAVED,
                "Periodic checkpoint projects open cursor and grid into the saved bag");
            World restored;
            const auto reload = restored.LoadOrCreatePlayerFromFile(path);
            test.Expect(reload.IsSuccess() &&
                Actor(restored, reload.GetPlayerEntityID()).GetInventory().GetItemAmount(ItemType::COINS) == 7 &&
                Actor(restored, reload.GetPlayerEntityID()).GetInventory().GetItemAmount(ItemType::LOG) == 1 &&
                restored.GetCraftingSession(reload.GetPlayerEntityID()) == nullptr,
                "Interruption/reload returns escrow exactly once without a stale crafting session");
            const auto original = Bytes(path);
            const auto revision = world.GetCraftingSession(id)->revision;
            test.Expect(player.GetInventory().GetItemAmount(ItemType::COINS) == 2 &&
                world.GetCraftingSession(id)->cursor.GetAmount() == 5 &&
                world.GetCraftingSession(id)->inputSlots[4].GetAmount() == 1 &&
                player.GetInventory().AddItem(ItemType::LOG, 27), "Snapshot leaves live ownership intact");
            const auto failed = lifecycle.PollAutosave(world, start + 120s);
            test.Expect(failed.status == Status::FAILED && failed.operation &&
                failed.operation->GetWorldSaveResult().Contains(WorldPlayerPersistenceIssueCode::CRAFTING_ITEMS_NOT_SAVED) &&
                Bytes(path) == original && world.GetCraftingSession(id)->revision == revision &&
                world.GetCraftingSession(id)->inputSlots[4].GetAmount() == 1,
                "Full-bag escrow failure preserves old save, session and inventory");
            player.GetInventory().RemoveItem(ItemType::LOG, 1);
            test.Expect(lifecycle.PollAutosave(world, start + 130s).status == Status::SAVED,
                "Freeing bag space permits the scheduled retry without closing crafting");
            World retried;
            const auto again = retried.LoadOrCreatePlayerFromFile(path);
            test.Expect(again.IsSuccess() &&
                Actor(retried, again.GetPlayerEntityID()).GetInventory().GetItemAmount(ItemType::LOG) == 27 &&
                Actor(retried, again.GetPlayerEntityID()).GetInventory().GetItemAmount(ItemType::COINS) == 7,
                "Retried projection contains bag and escrow quantities exactly once");
        }
    }
    {
        World world(std::make_unique<FatalRolls>());
        const auto path = fixture.path / "death.save";
        Lifecycle lifecycle(path, PlayerInitializationMode::EMPTY);
        const auto loaded = lifecycle.Start(world, start);
        const int id = loaded.GetPlayerEntityID();
        auto &player = Actor(world, id);
        player.GetInventory().AddItem(ItemType::COAL, 3);
        test.Expect(lifecycle.SaveNow(world, start).IsSuccess(), "Death fixture establishes prior checkpoint");
        const auto beforeDeath = Bytes(path);
        CombatRatings ratings;
        ratings.attackAccuracy = 10; ratings.meleeStrength = 15;
        ratings.defence = 1; ratings.maximumHealth = 30;
        const int monster = world.CreateMonster(11, 10, ratings);
        player.GetPosition().SetPosition(10, 10);
        player.ApplyDamage(player.GetCurrentHealth() - 1);
        world.TryStartMeleeAttack(monster, id, 1);
        world.Update();
        test.Expect(!player.IsAlive() && world.GetPlayerRecoveryPile(id) != nullptr,
            "Real melee death creates a protected recovery pile");
        test.Expect(lifecycle.PollAutosave(world, start + 60s).status == Status::WAITING_FOR_RESPAWN &&
                    Bytes(path) == beforeDeath, "Due save waits through death without replacing a valid save");
        for (int tick = 0; tick < 3; ++tick)
        {
            world.Update();
            test.Expect(lifecycle.PollAutosave(world, start + 61s).status == Status::WAITING_FOR_RESPAWN,
                "Death deferral stays due without filesystem retries");
        }
        world.Update();
        test.Expect(player.IsAlive() &&
                    lifecycle.PollAutosave(world, start + 62s).status == Status::SAVED,
            "First committed respawn tick immediately satisfies the deferred checkpoint");
        World restored;
        const auto reload = restored.LoadOrCreatePlayerFromFile(path);
        const int restoredID = reload.GetPlayerEntityID();
        const auto *pile = restored.GetPlayerRecoveryPile(restoredID);
        test.Expect(reload.IsSuccess() && pile != nullptr &&
                    Actor(restored, restoredID).GetInventory().GetItemAmount(ItemType::COAL) == 0,
            "Reload retains recovery ownership without duplicating items in the bag");
        if (pile)
        {
            Actor(restored, restoredID).GetPosition().SetPosition(pile->position.GetX(), pile->position.GetY());
            restored.Update();
            restored.Update();
            test.Expect(Actor(restored, restoredID).GetInventory().GetItemAmount(ItemType::COAL) == 3 &&
                        restored.GetPlayerRecoveryPile(restoredID) == nullptr,
                "Restored recovery pile reclaims exactly once");
        }
    }
    return test.Finish();
}
