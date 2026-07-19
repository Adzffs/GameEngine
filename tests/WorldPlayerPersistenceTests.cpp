#include "TestSupport.h"

#include "../src/Entity/EntityType.h"
#include "../src/Command/ServerCommand.h"
#include "../src/Core/RandomSource.h"
#include "../src/Inventory/ItemType.h"
#include "../src/Persistence/PlayerSaveFileStore.h"
#include "../src/Persistence/PlayerSaveState.h"
#include "../src/Persistence/PlayerSaveTextCodec.h"
#include "../src/Player/Player.h"
#include "../src/World/Tile/TileType.h"
#include "../src/World/World.h"

#include <chrono>
#include <filesystem>
#include <fstream>
#include <limits>
#include <string>
#include <tuple>

struct WorldTestAccess
{
    using ReconstructionFunction = std::unique_ptr<Player> (*)(
        int, const PlayerSaveData &, PlayerSaveValidationReport &);

    static WorldPlayerLoadResult TryRegisterLoadedPlayer(
        World &world,
        PlayerSaveFileLoadResult fileResult,
        const PlayerSaveData &saveData,
        ReconstructionFunction reconstruction)
    {
        return world.TryRegisterLoadedPlayer(
            std::move(fileResult), saveData, reconstruction);
    }
};

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

    std::unique_ptr<Player> FailReconstruction(
        int,
        const PlayerSaveData &,
        PlayerSaveValidationReport &report)
    {
        report.AddIssue(PlayerSaveValidationCode::INTERNAL_RESTORE_FAILURE,
                        -1, "Injected deterministic reconstruction failure");
        return nullptr;
    }

    class TemporaryDirectory
    {
    public:
        TemporaryDirectory()
        {
            path = std::filesystem::temp_directory_path() /
                   ("gameengine_world_player_persistence_" +
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

    PlayerSaveData MakeSave(int x, int y)
    {
        Player source(77, PlayerInitializationMode::EMPTY);
        source.GetPosition().SetPosition(x, y);
        source.GetInventory().AddItem(ItemType::BRONZE_AXE, 1);
        return PlayerSaveState::Capture(source);
    }

    std::string ReadBytes(const std::filesystem::path &path)
    {
        std::ifstream stream(path, std::ios::binary);
        return std::string(std::istreambuf_iterator<char>(stream),
                           std::istreambuf_iterator<char>());
    }

    bool SamePermanentState(const Player &left, const Player &right)
    {
        const PlayerSaveData leftSave = PlayerSaveState::Capture(left);
        const PlayerSaveData rightSave = PlayerSaveState::Capture(right);
        if (leftSave.positionX != rightSave.positionX ||
            leftSave.positionY != rightSave.positionY ||
            leftSave.currentHealth != rightSave.currentHealth ||
            leftSave.skills.size() != rightSave.skills.size() ||
            leftSave.equipment.size() != rightSave.equipment.size()) return false;
        for (int index = 0; index < Inventory::SlotCount; ++index)
            if (leftSave.inventorySlots[index].itemType != rightSave.inventorySlots[index].itemType ||
                leftSave.inventorySlots[index].quantity != rightSave.inventorySlots[index].quantity) return false;
        for (std::size_t index = 0; index < leftSave.skills.size(); ++index)
            if (leftSave.skills[index].skillType != rightSave.skills[index].skillType ||
                leftSave.skills[index].xp != rightSave.skills[index].xp) return false;
        for (std::size_t index = 0; index < leftSave.equipment.size(); ++index)
            if (leftSave.equipment[index].slotType != rightSave.equipment[index].slotType ||
                leftSave.equipment[index].itemType != rightSave.equipment[index].itemType) return false;
        return true;
    }
}

int main()
{
    TestContext test;
    TemporaryDirectory fixture;

    {
        World world(1U, 2U, 3U);
        const auto missing = fixture.path / "missing" / "player.save";
        WorldPlayerLoadResult first = world.LoadOrCreatePlayerFromFile(missing);
        test.Expect(first.IsSuccess(), "Missing save creates a Player");
        test.Expect(first.WasCreatedNew(), "Missing save reports CREATED_NEW");
        test.ExpectEqual(first.GetPlayerEntityID(), 4, "First manual Player receives ID 4");
        test.Expect(first.GetFileLoadResult().IsNotFound(), "Nested result retains NOT_FOUND");
        test.Expect(!std::filesystem::exists(missing.parent_path()), "Load creates no directory");
        auto *player = dynamic_cast<Player *>(world.GetEntityByID(4));
        test.Expect(player != nullptr, "Created Player is registered");
        test.ExpectEqual(player->GetPosition().GetX(), 0, "Created Player spawn X is stable");
        test.ExpectEqual(player->GetInventory().GetItemAmount(ItemType::BRONZE_AXE), 1,
                         "Created Player retains development inventory");

        WorldPlayerLoadResult second = world.LoadOrCreatePlayerFromFile(missing);
        test.ExpectEqual(second.GetPlayerEntityID(), 5, "Second manual Player receives ID 5");
        WorldPlayerLoadResult third = world.LoadOrCreatePlayerFromFile(missing);
        test.ExpectEqual(third.GetPlayerEntityID(), 6, "Third manual Player receives ID 6");
    }

    const auto validPath = fixture.path / "valid.save";
    test.Expect(PlayerSaveFileStore::Save(validPath, MakeSave(1, 1)).IsSuccess(),
                "Fixture valid save is written");
    const std::string validBytes = ReadBytes(validPath);
    {
        World world(4U, 5U, 6U);
        WorldPlayerLoadResult loaded = world.LoadOrCreatePlayerFromFile(validPath);
        test.Expect(loaded.IsSuccess() && loaded.WasLoadedFromFile(),
                    "Valid save loads as existing");
        test.ExpectEqual(loaded.GetPlayerEntityID(), 4, "Loaded Player receives fresh ID");
        test.Expect(!loaded.UsedSpawnFallback(), "Valid position does not use fallback");
        auto *player = dynamic_cast<Player *>(world.GetEntityByID(4));
        test.ExpectEqual(player->GetPosition().GetX(), 1, "Valid saved X is preserved");
        test.ExpectEqual(player->GetPosition().GetY(), 1, "Valid saved Y is preserved");
        test.ExpectEqual(ReadBytes(validPath), validBytes, "Load does not rewrite valid file");

        const auto output = fixture.path / "saved.save";
        WorldPlayerSaveResult saved = world.SavePlayerToFile(4, output);
        test.Expect(saved.IsSuccess(), "Live Player saves successfully");
        test.Expect(saved.GetFileSaveResult().WasCommitted(), "Nested save reports commit");
        test.ExpectEqual(ReadBytes(output), validBytes, "World save emits canonical state");
    }

    for (const auto &[x, y, code] : {
             std::tuple{-1, 0, WorldPlayerPersistenceIssueCode::SAVED_POSITION_OUT_OF_BOUNDS},
             std::tuple{0, -1, WorldPlayerPersistenceIssueCode::SAVED_POSITION_OUT_OF_BOUNDS},
             std::tuple{100, 0, WorldPlayerPersistenceIssueCode::SAVED_POSITION_OUT_OF_BOUNDS},
             std::tuple{0, 100, WorldPlayerPersistenceIssueCode::SAVED_POSITION_OUT_OF_BOUNDS},
             std::tuple{std::numeric_limits<int>::min(), 0, WorldPlayerPersistenceIssueCode::SAVED_POSITION_OUT_OF_BOUNDS},
             std::tuple{std::numeric_limits<int>::max(), 0, WorldPlayerPersistenceIssueCode::SAVED_POSITION_OUT_OF_BOUNDS},
             std::tuple{10, 10, WorldPlayerPersistenceIssueCode::SAVED_POSITION_BLOCKED},
             std::tuple{5, 5, WorldPlayerPersistenceIssueCode::SAVED_POSITION_BLOCKED},
             std::tuple{14, 9, WorldPlayerPersistenceIssueCode::SAVED_POSITION_BLOCKED}})
    {
        const auto path = fixture.path / ("fallback_" + std::to_string(x) + "_" + std::to_string(y));
        PlayerSaveFileStore::Save(path, MakeSave(x, y));
        const std::string bytes = ReadBytes(path);
        World world(7U, 8U, 9U);
        WorldPlayerLoadResult loaded = world.LoadOrCreatePlayerFromFile(path);
        test.Expect(loaded.IsSuccess() && loaded.UsedSpawnFallback(), "Invalid position uses fallback");
        test.Expect(loaded.Contains(code), "Fallback reports the correct warning code");
        auto *player = dynamic_cast<Player *>(world.GetEntityByID(4));
        test.ExpectEqual(player->GetPosition().GetX(), 0, "Fallback X is development spawn");
        test.ExpectEqual(player->GetPosition().GetY(), 0, "Fallback Y is development spawn");
        test.ExpectEqual(ReadBytes(path), bytes, "Fallback load does not rewrite file");
        test.Expect(world.SavePlayerToFile(4, path).IsSuccess(), "Explicit save after fallback succeeds");
        test.Expect(ReadBytes(path).find("position_x=0\nposition_y=0") != std::string::npos,
                    "Explicit save captures fallback position");
    }

    {
        World world(10U, 11U, 12U);
        world.GetMap().SetTileType(0, 0, TileType::WALL);
        WorldPlayerLoadResult failed =
            world.LoadOrCreatePlayerFromFile(fixture.path / "still_missing.save");
        test.Expect(!failed.IsSuccess(), "Blocked development spawn fails creation");
        test.Expect(failed.Contains(WorldPlayerPersistenceIssueCode::DEVELOPMENT_SPAWN_INVALID),
                    "Blocked spawn reports DEVELOPMENT_SPAWN_INVALID");
        test.Expect(world.GetEntityByID(4) == nullptr, "Blocked spawn registers no Player");
        world.GetMap().SetTileType(0, 0, TileType::GRASS);
        test.ExpectEqual(world.CreatePlayer(), 4, "Blocked spawn failure consumes no ID");
    }

    {
        const auto invalidSavedPath = fixture.path / "invalid_saved_position.save";
        PlayerSaveFileStore::Save(invalidSavedPath, MakeSave(10, 10));
        const std::string original = ReadBytes(invalidSavedPath);
        World world(10U, 11U, 12U);
        world.GetMap().SetTileType(0, 0, TileType::WALL);
        WorldPlayerLoadResult failed = world.LoadOrCreatePlayerFromFile(invalidSavedPath);
        test.Expect(!failed.IsSuccess() && failed.GetPlayerEntityID() == 0,
                    "Invalid saved position with invalid fallback fails without an ID");
        test.Expect(failed.Contains(WorldPlayerPersistenceIssueCode::DEVELOPMENT_SPAWN_INVALID),
                    "Invalid fallback reports DEVELOPMENT_SPAWN_INVALID");
        test.ExpectEqual(ReadBytes(invalidSavedPath), original,
                         "Invalid fallback leaves existing save unchanged");
        world.GetMap().SetTileType(0, 0, TileType::GRASS);
        test.ExpectEqual(world.CreatePlayer(), 4, "Invalid fallback consumes no ID");
    }

    {
        const auto corrupt = fixture.path / "corrupt.save";
        std::ofstream(corrupt, std::ios::binary) << "corrupt";
        const std::string bytes = ReadBytes(corrupt);
        World world(13U, 14U, 15U);
        WorldPlayerLoadResult failed = world.LoadOrCreatePlayerFromFile(corrupt);
        test.Expect(!failed.IsSuccess(), "Corrupt save fails");
        test.Expect(failed.Contains(WorldPlayerPersistenceIssueCode::FILE_LOAD_FAILED),
                    "Corrupt save reports FILE_LOAD_FAILED");
        test.ExpectEqual(ReadBytes(corrupt), bytes, "Corrupt target remains unchanged");
        test.ExpectEqual(world.CreatePlayer(), 4, "Corrupt load consumes no ID");
    }

    {
        World world(16U, 17U, 18U);
        const auto untouched = fixture.path / "untouched" / "save";
        test.Expect(world.SavePlayerToFile(0, untouched).Contains(
                        WorldPlayerPersistenceIssueCode::INVALID_RUNTIME_ENTITY_ID),
                    "Zero ID is rejected");
        test.Expect(world.SavePlayerToFile(-1, untouched).Contains(
                        WorldPlayerPersistenceIssueCode::INVALID_RUNTIME_ENTITY_ID),
                    "Negative ID is rejected");
        test.Expect(world.SavePlayerToFile(99, untouched).Contains(
                        WorldPlayerPersistenceIssueCode::ENTITY_NOT_FOUND),
                    "Missing entity is rejected");
        test.Expect(world.SavePlayerToFile(1, untouched).Contains(
                        WorldPlayerPersistenceIssueCode::ENTITY_NOT_PLAYER),
                    "NPC is rejected");
        test.Expect(world.SavePlayerToFile(2, untouched).Contains(
                        WorldPlayerPersistenceIssueCode::ENTITY_NOT_PLAYER) &&
                        world.SavePlayerToFile(3, untouched).Contains(
                            WorldPlayerPersistenceIssueCode::ENTITY_NOT_PLAYER),
                    "Both development Monsters are rejected");
        test.Expect(!std::filesystem::exists(untouched.parent_path()),
                    "Invalid selection does not touch filesystem");
        const int playerID = world.CreatePlayer();
        auto *player = dynamic_cast<Player *>(world.GetEntityByID(playerID));
        player->ApplyDamage(player->GetCurrentHealth());
        test.Expect(world.SavePlayerToFile(playerID, untouched).Contains(
                        WorldPlayerPersistenceIssueCode::PLAYER_NOT_ALIVE),
                    "Dead Player is rejected before file storage");
    }

    {
        World world(19U, 20U, 21U);
        const int firstID = world.LoadOrCreatePlayerFromFile(validPath).GetPlayerEntityID();
        const int secondID = world.LoadOrCreatePlayerFromFile(validPath).GetPlayerEntityID();
        auto *first = dynamic_cast<Player *>(world.GetEntityByID(firstID));
        auto *second = dynamic_cast<Player *>(world.GetEntityByID(secondID));
        test.Expect(first != second && SamePermanentState(*first, *second),
                    "Repeated loads create independent Players with identical permanent state");
        first->GetPosition().SetPosition(2, 2);
        first->GetInventory().AddItem(ItemType::BRONZE_PICKAXE, 1);
        test.Expect(second->GetPosition().GetX() == 1 &&
                        second->GetInventory().GetItemAmount(ItemType::BRONZE_PICKAXE) == 0,
                    "Mutating one loaded Player does not affect the other");
        const auto firstOutput = fixture.path / "first_only.save";
        test.Expect(world.SavePlayerToFile(firstID, firstOutput).IsSuccess(),
                    "One of multiple loaded Players saves successfully");
        test.Expect(ReadBytes(firstOutput).find("position_x=2\nposition_y=2") != std::string::npos,
                    "Save captures only the selected loaded Player");
    }

    {
        PlayerSaveData temporaryHealth = MakeSave(1, 1);
        temporaryHealth.currentHealth = 150;
        const auto path = fixture.path / "temporary_health.save";
        test.Expect(PlayerSaveFileStore::Save(path, temporaryHealth).IsSuccess(),
                    "Temporary-health fixture saves");
        const std::string original = ReadBytes(path);
        World world(22U, 23U, 24U);
        WorldPlayerLoadResult loaded = world.LoadOrCreatePlayerFromFile(path);
        auto *player = dynamic_cast<Player *>(world.GetEntityByID(loaded.GetPlayerEntityID()));
        test.Expect(loaded.IsSuccess() && player->GetMaximumHealth() == 100 &&
                        player->GetCurrentHealth() == 100,
                    "Temporary health normalizes to permanent maximum during reconstruction");
        test.Expect(player->GetStatusEffectManager().GetActiveEffects().empty(),
                    "Temporary health load creates no status effect");
        test.Expect(!world.HasActiveMovementPath(player->GetID()) &&
                        world.GetActionForEntity(player->GetID()) == nullptr &&
                        !world.HasActiveStation(player->GetID()) &&
                        !world.HasPendingMeleeEngagement(player->GetID()) &&
                        world.GetActiveDialogueSession(player->GetID()) == nullptr &&
                        world.GetActiveShopSession(player->GetID()) == nullptr &&
                        world.GetMeleeCombatFeedbacks().find(player->GetID()) ==
                            world.GetMeleeCombatFeedbacks().end(),
                    "Loaded Player has no World-owned runtime state");
        test.ExpectEqual(ReadBytes(path), original, "Temporary-health load does not rewrite file");
        test.Expect(world.SavePlayerToFile(player->GetID(), path).IsSuccess(),
                    "Explicit save after health normalization succeeds");
        test.Expect(ReadBytes(path).find("current_health=100") != std::string::npos,
                    "Explicit save writes normalized health");
    }

    {
        World world(25U, 26U, 27U);
        const int playerID = world.CreatePlayer();
        const auto directoryTarget = fixture.path / "save_target_directory";
        std::filesystem::create_directory(directoryTarget);
        WorldPlayerSaveResult failed = world.SavePlayerToFile(playerID, directoryTarget);
        test.Expect(!failed.IsSuccess() && failed.GetPlayerEntityID() == 0,
                    "File-store save failure exposes no successful runtime ID");
        test.Expect(failed.Contains(WorldPlayerPersistenceIssueCode::FILE_SAVE_FAILED),
                    "File-store save failure reports FILE_SAVE_FAILED");
        test.Expect(!failed.GetFileSaveResult().IsSuccess(),
                    "Complete nested failed save result is retained");
    }

    {
        auto combat = std::make_unique<CountingRandomSource>();
        auto reward = std::make_unique<CountingRandomSource>();
        auto gathering = std::make_unique<CountingRandomSource>();
        CountingRandomSource *combatCounter = combat.get();
        CountingRandomSource *rewardCounter = reward.get();
        CountingRandomSource *gatheringCounter = gathering.get();
        World world(std::move(combat), std::move(reward), std::move(gathering));
        const auto fileResult = PlayerSaveFileStore::Load(validPath);
        const std::size_t entityCount = world.GetEntities().size();
        const std::vector<int> ids{world.GetEntities()[0]->GetID(),
                                   world.GetEntities()[1]->GetID(),
                                   world.GetEntities()[2]->GetID()};
        const std::size_t actionEvents = world.GetActionLifecycleEvents().size();
        const std::size_t deathEvents = world.GetEntityDiedEvents().size();
        const std::size_t dialogueEvents = world.GetNpcTalkEvents().size();
        const std::size_t shopEvents = world.GetShopOpenedEvents().size();
        const std::size_t transactionEvents = world.GetShopTransactionEvents().size();
        const std::size_t feedbackCount = world.GetMeleeCombatFeedbacks().size();

        WorldPlayerLoadResult failed = WorldTestAccess::TryRegisterLoadedPlayer(
            world, fileResult, *fileResult.GetSaveData(), &FailReconstruction);
        test.Expect(failed.GetStatus() == WorldPlayerLoadStatus::FAILURE &&
                        !failed.IsSuccess() && failed.GetPlayerEntityID() == 0,
                    "Injected World reconstruction failure returns FAILURE and no ID");
        test.Expect(failed.Contains(WorldPlayerPersistenceIssueCode::PLAYER_RECONSTRUCTION_FAILED) &&
                        failed.GetReconstructionValidationReport().Contains(
                            PlayerSaveValidationCode::INTERNAL_RESTORE_FAILURE),
                    "World reconstruction failure retains issue and validation report");
        test.Expect(failed.GetFileLoadResult().IsSuccess(),
                    "Reconstruction failure retains successful nested file load");
        test.ExpectEqual(ReadBytes(validPath), validBytes,
                         "Injected reconstruction failure does not rewrite save file");
        test.Expect(world.GetEntities().size() == entityCount &&
                        world.GetEntities()[0]->GetID() == ids[0] &&
                        world.GetEntities()[1]->GetID() == ids[1] &&
                        world.GetEntities()[2]->GetID() == ids[2],
                    "Reconstruction failure preserves existing entity count and order");
        test.ExpectEqual(world.CreatePlayer(), 4,
                         "World reconstruction failure does not consume candidate ID");
        test.Expect(combatCounter->calls == 0 && rewardCounter->calls == 0 &&
                        gatheringCounter->calls == 0,
                    "Reconstruction failure consumes no World RNG");
        test.Expect(world.GetActionLifecycleEvents().size() == actionEvents &&
                        world.GetEntityDiedEvents().size() == deathEvents &&
                        world.GetNpcTalkEvents().size() == dialogueEvents &&
                        world.GetShopOpenedEvents().size() == shopEvents &&
                        world.GetShopTransactionEvents().size() == transactionEvents &&
                        world.GetMeleeCombatFeedbacks().size() == feedbackCount,
                    "Reconstruction failure publishes no World events");

        const int callsBefore = combatCounter->calls + rewardCounter->calls + gatheringCounter->calls;
        test.Expect(world.SavePlayerToFile(4, fixture.path / "rng_save.save").IsSuccess(),
                    "RNG isolation save succeeds");
        test.Expect(world.LoadOrCreatePlayerFromFile(validPath).IsSuccess(),
                    "RNG isolation existing load succeeds");
        test.Expect(world.LoadOrCreatePlayerFromFile(fixture.path / "rng_missing.save").IsSuccess(),
                    "RNG isolation missing creation succeeds");
        test.ExpectEqual(combatCounter->calls + rewardCounter->calls + gatheringCounter->calls,
                         callsBefore, "Manual persistence operations consume no World RNG");
    }

    {
        World world(28U, 29U, 30U);
        const int playerID = world.CreatePlayer();
        Player *player = dynamic_cast<Player *>(world.GetEntityByID(playerID));
        player->GetPosition().SetPosition(1, 1);
        world.QueueMovementDestination(MovementDestinationRequest{playerID, 8, 1});
        world.Update();
        const auto destination = world.GetMovementDestination(playerID);
        const Position position = player->GetPosition();
        const int tick = world.GetCurrentTick();
        test.Expect(world.HasActiveMovementPath(playerID), "Movement fixture has active path");
        test.Expect(world.SavePlayerToFile(playerID, fixture.path / "movement.save").IsSuccess(),
                    "Save with movement succeeds");
        const auto destinationAfter = world.GetMovementDestination(playerID);
        test.Expect(world.HasActiveMovementPath(playerID) &&
                        destination.has_value() && destinationAfter.has_value() &&
                        destinationAfter->GetX() == destination->GetX() &&
                        destinationAfter->GetY() == destination->GetY() &&
                        player->GetPosition().GetX() == position.GetX() &&
                        player->GetPosition().GetY() == position.GetY() &&
                        world.GetCurrentTick() == tick,
                    "Save preserves movement, position, destination, and tick");
    }

    {
        World world(31U, 32U, 33U);
        const int playerID = world.CreatePlayer();
        Player *player = dynamic_cast<Player *>(world.GetEntityByID(playerID));
        player->GetPosition().SetPosition(2, 3);
        world.EnqueueCommand(NpcInteractionCommand{playerID, 1, NpcInteractionType::TALK});
        world.Update();
        const ActiveDialogueSession *before = world.GetActiveDialogueSession(playerID);
        const DialogueSessionId sessionID = before == nullptr ? 0 : before->sessionId;
        const std::size_t dialogueEvents = world.GetNpcTalkEvents().size();
        const std::size_t shopEvents = world.GetShopOpenedEvents().size();
        const int tick = world.GetCurrentTick();
        test.Expect(before != nullptr, "Dialogue fixture has active session");
        test.Expect(world.SavePlayerToFile(playerID, fixture.path / "dialogue.save").IsSuccess(),
                    "Save with dialogue succeeds");
        const ActiveDialogueSession *after = world.GetActiveDialogueSession(playerID);
        test.Expect(after != nullptr && after->sessionId == sessionID &&
                        world.GetNpcTalkEvents().size() == dialogueEvents &&
                        world.GetShopOpenedEvents().size() == shopEvents &&
                        world.GetCurrentTick() == tick,
                    "Save preserves dialogue and publishes no interface event");

        const PlayerSaveData playerBefore = PlayerSaveState::Capture(*player);
        PlayerSaveValidationReport snapshotReport;
        std::unique_ptr<Player> playerSnapshot = PlayerSaveState::TryCreatePlayer(
            99, playerBefore, snapshotReport);
        WorldPlayerLoadResult loaded = world.LoadOrCreatePlayerFromFile(validPath);
        test.Expect(loaded.IsSuccess() && loaded.GetPlayerEntityID() == 5,
                    "Second Player loads beside active existing Player");
        test.Expect(world.GetActiveDialogueSession(playerID) != nullptr &&
                        world.GetActiveDialogueSession(playerID)->sessionId == sessionID &&
                        playerSnapshot != nullptr && SamePermanentState(*player, *playerSnapshot),
                    "Loading another Player preserves existing dialogue and permanent state");
        const Player *loadedPlayer = dynamic_cast<const Player *>(
            world.GetEntityByID(loaded.GetPlayerEntityID()));
        test.Expect(loadedPlayer != nullptr &&
                        !world.HasActiveMovementPath(loaded.GetPlayerEntityID()) &&
                        world.GetActionForEntity(loaded.GetPlayerEntityID()) == nullptr &&
                        world.GetActiveDialogueSession(loaded.GetPlayerEntityID()) == nullptr,
                    "Newly loaded Player has no runtime state");
    }

    {
        World world(34U, 35U, 36U);
        const int playerID = world.CreatePlayer();
        Player *player = dynamic_cast<Player *>(world.GetEntityByID(playerID));
        const auto &slots = player->GetInventory().GetSlots();
        int axeSlot = -1;
        for (int index = 0; index < Inventory::SlotCount; ++index)
            if (slots[index].GetItemType() == ItemType::BRONZE_AXE) axeSlot = index;
        test.Expect(world.TryEquipInventoryItem(playerID, axeSlot), "Gathering fixture equips axe");
        player->GetPosition().SetPosition(4, 5);
        ResourceNode *resource = world.GetResourceAt(5, 5);
        world.QueueResourceInteraction(playerID, resource->GetID());
        world.Update();
        const Action *before = world.GetActionForEntity(playerID);
        const float progress = before == nullptr ? -1.0f : before->GetProgress();
        const std::size_t events = world.GetActionLifecycleEvents().size();
        const int tick = world.GetCurrentTick();
        test.Expect(before != nullptr, "Gathering fixture has active action");
        test.Expect(world.SavePlayerToFile(playerID, fixture.path / "action.save").IsSuccess(),
                    "Save with gathering action succeeds");
        const Action *after = world.GetActionForEntity(playerID);
        test.Expect(after != nullptr && after->GetProgress() == progress &&
                        world.GetActionLifecycleEvents().size() == events &&
                        world.GetCurrentTick() == tick,
                    "Save preserves action progress and publishes no cancellation");
    }

    {
        World world(37U, 38U, 39U);
        const int attackerID = world.CreatePlayer();
        const int defenderID = world.CreatePlayer();
        auto *attacker = dynamic_cast<Player *>(world.GetEntityByID(attackerID));
        auto *defender = dynamic_cast<Player *>(world.GetEntityByID(defenderID));
        attacker->GetPosition().SetPosition(1, 1);
        defender->GetPosition().SetPosition(2, 1);
        test.Expect(world.TryStartMeleeEngagement(attackerID, defenderID, 5),
                    "Melee fixture starts engagement");
        const Action *before = world.GetActionForEntity(attackerID);
        const int health = defender->GetCurrentHealth();
        const std::size_t feedback = world.GetMeleeCombatFeedbacks().size();
        test.Expect(world.SavePlayerToFile(attackerID, fixture.path / "melee.save").IsSuccess(),
                    "Save with melee engagement succeeds");
        test.Expect(world.GetActionForEntity(attackerID) == before &&
                        defender->GetCurrentHealth() == health &&
                        world.GetMeleeCombatFeedbacks().size() == feedback,
                    "Save preserves melee engagement without attack or feedback");
    }

    return test.Finish();
}
