#include "TestSupport.h"

#include "../src/Inventory/ItemType.h"
#include "../src/Core/RandomSource.h"
#include "../src/Movement/MovementDestinationRequest.h"
#include "../src/Persistence/PlayerSaveFileStore.h"
#include "../src/Persistence/PlayerSaveState.h"
#include "../src/Persistence/WorldPlayerPersistenceLifecycle.h"
#include "../src/Player/Player.h"
#include "../src/World/World.h"

#include <chrono>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <memory>
#include <string>
#include <utility>

namespace
{
    class CountingRandomSource final : public RandomSource
    {
    public:
        int NextIntInclusive(int minimum, int) override
        {
            ++calls;
            return minimum;
        }
        int calls = 0;
    };

    class TemporaryDirectory
    {
    public:
        TemporaryDirectory()
        {
            path = std::filesystem::temp_directory_path() /
                   ("gameengine_player_lifecycle_" +
                    std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
            std::filesystem::create_directories(path);
        }
        ~TemporaryDirectory()
        {
            std::error_code error;
            std::filesystem::remove_all(path, error);
        }
        std::filesystem::path path;
    };

    std::string ReadBytes(const std::filesystem::path &path)
    {
        std::ifstream stream(path, std::ios::binary);
        return std::string(std::istreambuf_iterator<char>(stream),
                           std::istreambuf_iterator<char>());
    }

    PlayerSaveData MakeSave(int x, int y, int health = 100)
    {
        Player player(91, PlayerInitializationMode::DEVELOPMENT_DEFAULTS);
        player.GetPosition().SetPosition(x, y);
        player.GetInventory().AddItem(ItemType::BRONZE_PICKAXE, 2);
        PlayerSaveData save = PlayerSaveState::Capture(player);
        save.currentHealth = health;
        return save;
    }
}

int main()
{
    TestContext test;
    TemporaryDirectory fixture;

    {
        const auto path = fixture.path / "restart" / "player.save";
        World firstWorld(1U, 2U, 3U);
        WorldPlayerPersistenceLifecycle first(path);
        auto startup = first.Start(firstWorld);
        test.Expect(startup.IsSuccess() && startup.WasCreatedNew() &&
                startup.GetStatus() == WorldPlayerPersistenceStartupStatus::CREATED_NEW,
                    "Missing save starts by creating the development Player");
        test.ExpectEqual(startup.GetPlayerEntityID(), 4, "Lifecycle tracks runtime Player ID 4");
        test.Expect(!std::filesystem::exists(path.parent_path()),
                    "Startup creates no save directory or sidecar");
        auto *player = dynamic_cast<Player *>(firstWorld.GetEntityByID(4));
        player->GetPosition().SetPosition(2, 3);
        player->GetInventory().AddItem(ItemType::BRONZE_PICKAXE, 1);
        firstWorld.Update();
        firstWorld.Update();
        test.Expect(!std::filesystem::exists(path), "Ticks do not autosave a new Player");
        auto shutdown = first.Shutdown(firstWorld);
        test.Expect(shutdown.IsSuccess() && shutdown.DidSave(),
                    "Controlled shutdown writes the first save");
        test.Expect(std::filesystem::exists(path), "Shutdown creates the canonical save");

        World secondWorld(4U, 5U, 6U);
        WorldPlayerPersistenceLifecycle second(path);
        auto restart = second.Start(secondWorld);
        test.Expect(restart.IsSuccess() && restart.WasLoadedFromFile() &&
                restart.GetStatus() == WorldPlayerPersistenceStartupStatus::LOADED_EXISTING,
                    "Second session loads the existing Player");
        test.ExpectEqual(restart.GetPlayerEntityID(), 4, "Restart receives a fresh runtime ID 4");
        auto *restored = dynamic_cast<Player *>(secondWorld.GetEntityByID(4));
        test.Expect(restored != nullptr && restored->GetPosition().GetX() == 2 &&
                        restored->GetPosition().GetY() == 3 &&
                        restored->GetInventory().GetItemAmount(ItemType::BRONZE_PICKAXE) >= 1,
                    "Restart restores representative permanent state");
        test.Expect(!secondWorld.HasActiveMovementPath(4) &&
                        secondWorld.GetActionForEntity(4) == nullptr &&
                        secondWorld.GetActiveDialogueSession(4) == nullptr,
                    "Restart restores no representative runtime state");
        const std::string beforeMutation = ReadBytes(path);
        restored->GetPosition().SetPosition(4, 4);
        secondWorld.Update();
        test.ExpectEqual(ReadBytes(path), beforeMutation, "World ticks leave committed bytes unchanged");
        auto savedNow = second.SaveNow(secondWorld);
        test.Expect(savedNow.IsSuccess() && savedNow.DidSave() &&
                        savedNow.GetStatus() == WorldPlayerPersistenceOperationStatus::SAVED,
                    "SaveNow is the explicit autosave boundary");
        test.Expect(ReadBytes(path).find("position_x=4\nposition_y=4") != std::string::npos,
                    "SaveNow captures the mutation");
        restored->GetPosition().SetPosition(5, 4);
        auto restartShutdown = second.Shutdown(secondWorld);
        test.Expect(restartShutdown.IsSuccess() && restartShutdown.GetStatus() ==
                        WorldPlayerPersistenceOperationStatus::SAVED,
                    "Restart shutdown saves latest state");
        test.Expect(ReadBytes(path).find("position_x=5\nposition_y=4") != std::string::npos,
                    "Shutdown captures state after SaveNow");
        auto duplicateShutdown = second.Shutdown(secondWorld);
        test.Expect(duplicateShutdown.IsSuccess() && !duplicateShutdown.DidSave() &&
                        duplicateShutdown.GetStatus() ==
                            WorldPlayerPersistenceOperationStatus::ALREADY_SHUT_DOWN,
                    "Second shutdown is a successful no-op");
        auto afterShutdownSave = second.SaveNow(secondWorld);
        test.Expect(!afterShutdownSave.IsSuccess() &&
                        afterShutdownSave.GetStatus() ==
                            WorldPlayerPersistenceOperationStatus::FAILURE &&
                        afterShutdownSave.Contains(
                            WorldPlayerPersistenceLifecycleIssueCode::NOT_ACTIVE),
                    "SaveNow after shutdown is rejected");
    }

    {
        World world(7U, 8U, 9U);
        WorldPlayerPersistenceLifecycle lifecycle(fixture.path / "idempotent.save");
        auto saveBeforeStart = lifecycle.SaveNow(world);
        test.Expect(!saveBeforeStart.IsSuccess() &&
                        saveBeforeStart.GetStatus() ==
                            WorldPlayerPersistenceOperationStatus::FAILURE &&
                        saveBeforeStart.Contains(
                            WorldPlayerPersistenceLifecycleIssueCode::NOT_ACTIVE),
                    "SaveNow before Start is rejected");
        auto shutdownBeforeStart = lifecycle.Shutdown(world);
        test.Expect(!shutdownBeforeStart.IsSuccess() &&
                        shutdownBeforeStart.GetStatus() ==
                            WorldPlayerPersistenceOperationStatus::FAILURE &&
                        shutdownBeforeStart.Contains(
                            WorldPlayerPersistenceLifecycleIssueCode::NOT_ACTIVE),
                    "Shutdown before Start is rejected");
        auto firstStart = lifecycle.Start(world);
        test.Expect(firstStart.IsSuccess() && firstStart.GetStatus() ==
                        WorldPlayerPersistenceStartupStatus::CREATED_NEW,
                    "Lifecycle starts after pre-start no-ops");
        auto secondStart = lifecycle.Start(world);
        test.Expect(!secondStart.IsSuccess() &&
                        secondStart.GetStatus() == WorldPlayerPersistenceStartupStatus::FAILURE &&
                        secondStart.Contains(
                            WorldPlayerPersistenceLifecycleIssueCode::ALREADY_STARTED),
                    "Start twice fails deterministically");
        test.Expect(world.GetEntityByID(5) == nullptr && world.CreatePlayer() == 5,
                    "Second Start creates no Player and consumes no runtime ID");
    }

    {
        const auto path = fixture.path / "corrupt.save";
        std::ofstream(path, std::ios::binary) << "corrupt";
        const std::string bytes = ReadBytes(path);
        World world(10U, 11U, 12U);
        WorldPlayerPersistenceLifecycle lifecycle(path);
        auto startup = lifecycle.Start(world);
        test.Expect(!startup.IsSuccess() &&
                        startup.GetStatus() == WorldPlayerPersistenceStartupStatus::FAILURE &&
                        startup.Contains(
                            WorldPlayerPersistenceLifecycleIssueCode::STARTUP_LOAD_OR_CREATE_FAILED),
                    "Corrupt startup surfaces the lifecycle boundary failure");
        test.Expect(lifecycle.GetState() == WorldPlayerPersistenceLifecycleState::FAILED &&
                        !lifecycle.GetTrackedPlayerEntityID().has_value(),
                    "Corrupt startup enters FAILED without a tracked ID");
        test.Expect(world.GetEntityByID(4) == nullptr && ReadBytes(path) == bytes,
                    "Corrupt startup creates no Player and preserves bytes");
        auto failedSave = lifecycle.SaveNow(world);
        auto failedShutdown = lifecycle.Shutdown(world);
        test.Expect(!failedSave.IsSuccess() &&
                        failedSave.GetStatus() ==
                            WorldPlayerPersistenceOperationStatus::FAILURE &&
                        failedSave.Contains(
                            WorldPlayerPersistenceLifecycleIssueCode::NOT_ACTIVE) &&
                        !failedShutdown.IsSuccess() &&
                        failedShutdown.GetStatus() ==
                            WorldPlayerPersistenceOperationStatus::FAILURE &&
                        failedShutdown.Contains(
                            WorldPlayerPersistenceLifecycleIssueCode::NOT_ACTIVE) &&
                        ReadBytes(path) == bytes,
                    "Failed lifecycle operations do not touch the corrupt file");
        test.ExpectEqual(world.CreatePlayer(), 4, "Corrupt startup consumes no runtime ID");
        auto secondStart = lifecycle.Start(world);
        test.Expect(!secondStart.IsSuccess() &&
                        secondStart.GetStatus() == WorldPlayerPersistenceStartupStatus::FAILURE &&
                        secondStart.Contains(
                            WorldPlayerPersistenceLifecycleIssueCode::ALREADY_STARTED),
                    "Start after startup failure is rejected");
    }

    {
        World world(13U, 14U, 15U);
        WorldPlayerPersistenceLifecycle invalid(std::filesystem::path{});
        auto result = invalid.Start(world);
        test.Expect(!result.IsSuccess() &&
                        result.GetStatus() == WorldPlayerPersistenceStartupStatus::FAILURE &&
                        result.Contains(
                            WorldPlayerPersistenceLifecycleIssueCode::INVALID_SAVE_PATH),
                    "Explicit empty path is an error");
        test.Expect(invalid.GetState() == WorldPlayerPersistenceLifecycleState::FAILED &&
                        world.GetEntityByID(4) == nullptr,
                    "Empty path performs no World operation");
    }

    {
        const auto parent = fixture.path / "blocked_parent";
        const auto path = parent / "player.save";
        World world(16U, 17U, 18U);
        WorldPlayerPersistenceLifecycle lifecycle(path);
        test.Expect(lifecycle.Start(world).IsSuccess(), "Retry fixture starts with missing path");
        std::ofstream(parent, std::ios::binary) << "regular file";
        auto failed = lifecycle.Shutdown(world);
        test.Expect(!failed.IsSuccess() &&
                        failed.GetStatus() ==
                            WorldPlayerPersistenceOperationStatus::FAILURE &&
                        failed.Contains(WorldPlayerPersistenceLifecycleIssueCode::SAVE_FAILED) &&
                        failed.GetWorldSaveResult().Contains(
                            WorldPlayerPersistenceIssueCode::FILE_SAVE_FAILED) &&
                        lifecycle.GetState() == WorldPlayerPersistenceLifecycleState::ACTIVE,
                    "Failed shutdown remains ACTIVE for retry");
        std::filesystem::remove(parent);
        auto retried = lifecycle.Shutdown(world);
        test.Expect(retried.IsSuccess() &&
                        retried.GetStatus() == WorldPlayerPersistenceOperationStatus::SAVED &&
                        std::filesystem::exists(path),
                    "Shutdown succeeds after correcting external path failure");
    }

    {
        const auto path = fixture.path / "dead.save";
        World world(19U, 20U, 21U);
        WorldPlayerPersistenceLifecycle lifecycle(path);
        auto startup = lifecycle.Start(world);
        auto *player = dynamic_cast<Player *>(world.GetEntityByID(startup.GetPlayerEntityID()));
        player->ApplyDamage(player->GetCurrentHealth());
        auto failed = lifecycle.SaveNow(world);
        test.Expect(!failed.IsSuccess() &&
                        failed.GetStatus() ==
                            WorldPlayerPersistenceOperationStatus::FAILURE &&
                        failed.Contains(WorldPlayerPersistenceLifecycleIssueCode::SAVE_FAILED) &&
                        failed.GetWorldSaveResult().Contains(
                            WorldPlayerPersistenceIssueCode::PLAYER_NOT_ALIVE),
                    "Dead tracked Player failure preserves the nested World result");
        test.Expect(lifecycle.GetState() == WorldPlayerPersistenceLifecycleState::ACTIVE &&
                        !std::filesystem::exists(path),
                    "Dead tracked Player remains tracked and creates no save");
    }

    {
        const auto path = fixture.path / "fallback.save";
        test.Expect(PlayerSaveFileStore::Save(path, MakeSave(100, 100)).IsSuccess(),
                    "Fallback fixture is committed");
        const std::string original = ReadBytes(path);
        World world(22U, 23U, 24U);
        WorldPlayerPersistenceLifecycle lifecycle(path);
        auto startup = lifecycle.Start(world);
        auto *player = dynamic_cast<Player *>(world.GetEntityByID(4));
        test.Expect(startup.WasLoadedFromFile() &&
                        startup.GetStatus() == WorldPlayerPersistenceStartupStatus::LOADED_EXISTING &&
                        player->GetPosition().GetX() == 0 &&
                        ReadBytes(path) == original,
                    "Position fallback is authoritative in memory but does not rewrite startup");
        test.Expect(lifecycle.Shutdown(world).GetStatus() ==
                        WorldPlayerPersistenceOperationStatus::SAVED,
                    "Fallback shutdown reports SAVED");
        test.Expect(ReadBytes(path).find("position_x=0\nposition_y=0") != std::string::npos,
                    "Shutdown persists the fallback position");
    }

    {
        auto combat = std::make_unique<CountingRandomSource>();
        auto reward = std::make_unique<CountingRandomSource>();
        auto gathering = std::make_unique<CountingRandomSource>();
        CountingRandomSource *combatCounter = combat.get();
        CountingRandomSource *rewardCounter = reward.get();
        CountingRandomSource *gatheringCounter = gathering.get();
        World world(std::move(combat), std::move(reward), std::move(gathering));
        const auto path = fixture.path / "runtime_isolation.save";
        WorldPlayerPersistenceLifecycle lifecycle(path);
        auto startup = lifecycle.Start(world);
        auto *player = dynamic_cast<Player *>(world.GetEntityByID(startup.GetPlayerEntityID()));
        player->GetPosition().SetPosition(1, 1);
        world.QueueMovementDestination(MovementDestinationRequest{player->GetID(), 8, 1});
        world.Update();
        const auto destination = world.GetMovementDestination(player->GetID());
        const Position position = player->GetPosition();
        const int tick = world.GetCurrentTick();
        const std::size_t actionEvents = world.GetActionLifecycleEvents().size();
        const std::size_t deathEvents = world.GetEntityDiedEvents().size();
        const std::size_t dialogueEvents = world.GetNpcTalkEvents().size();
        const std::size_t shopEvents = world.GetShopOpenedEvents().size();
        const std::size_t transactionEvents = world.GetShopTransactionEvents().size();
        const std::size_t feedback = world.GetMeleeCombatFeedbacks().size();
        const int randomCalls = combatCounter->calls + rewardCounter->calls + gatheringCounter->calls;
        test.Expect(world.HasActiveMovementPath(player->GetID()),
                    "Lifecycle runtime-isolation fixture has active movement");
        auto saveNow = lifecycle.SaveNow(world);
        test.Expect(saveNow.IsSuccess() &&
                saveNow.GetStatus() == WorldPlayerPersistenceOperationStatus::SAVED,
                    "SaveNow succeeds with active movement");
        const auto destinationAfterSave = world.GetMovementDestination(player->GetID());
        test.Expect(world.HasActiveMovementPath(player->GetID()) && destination.has_value() &&
                        destinationAfterSave.has_value() &&
                        destinationAfterSave->GetX() == destination->GetX() &&
                        destinationAfterSave->GetY() == destination->GetY() &&
                        player->GetPosition().GetX() == position.GetX() &&
                        player->GetPosition().GetY() == position.GetY() &&
                        world.GetCurrentTick() == tick,
                    "SaveNow preserves movement, position, destination, and tick");
        auto shutdown = lifecycle.Shutdown(world);
        test.Expect(shutdown.IsSuccess() &&
                        shutdown.GetStatus() == WorldPlayerPersistenceOperationStatus::SAVED &&
                        world.HasActiveMovementPath(player->GetID()),
                    "Shutdown preserves runtime movement until World destruction");
        test.Expect(world.GetActionLifecycleEvents().size() == actionEvents &&
                        world.GetEntityDiedEvents().size() == deathEvents &&
                        world.GetNpcTalkEvents().size() == dialogueEvents &&
                        world.GetShopOpenedEvents().size() == shopEvents &&
                        world.GetShopTransactionEvents().size() == transactionEvents &&
                        world.GetMeleeCombatFeedbacks().size() == feedback,
                    "Lifecycle saves publish no representative gameplay events");
        test.ExpectEqual(combatCounter->calls + rewardCounter->calls + gatheringCounter->calls,
                         randomCalls, "Lifecycle saves consume no World RNG");
    }

    {
        const auto path = fixture.path / "temporary_health.save";
        test.Expect(PlayerSaveFileStore::Save(path, MakeSave(1, 1, 150)).IsSuccess(),
                    "Temporary-health fixture is committed");
        const std::string original = ReadBytes(path);
        World world(25U, 26U, 27U);
        WorldPlayerPersistenceLifecycle lifecycle(path);
        auto startup = lifecycle.Start(world);
        test.Expect(startup.IsSuccess() &&
                        startup.GetStatus() == WorldPlayerPersistenceStartupStatus::LOADED_EXISTING,
                    "Temporary-health lifecycle starts");
        auto *player = dynamic_cast<Player *>(world.GetEntityByID(4));
        test.Expect(player->GetCurrentHealth() == 100 && ReadBytes(path) == original,
                    "Temporary health normalizes without startup rewrite");
        test.Expect(lifecycle.Shutdown(world).GetStatus() ==
                        WorldPlayerPersistenceOperationStatus::SAVED,
                    "Temporary-health shutdown reports SAVED");
        test.Expect(ReadBytes(path).find("current_health=100") != std::string::npos,
                    "Shutdown persists normalized permanent health");
    }

    return test.Finish();
}
