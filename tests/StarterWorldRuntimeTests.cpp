#include "TestSupport.h"

#include "../src/Command/ServerCommand.h"
#include "../src/Content/Export/StarterWorldSnapshot.h"
#include "../src/Content/Edit/StarterWorldEdit.h"
#include "../src/Quest/QuestSystem.h"
#include "../src/Content/Export/StarterWorldSnapshotInput.h"
#include "../src/Entity/Monster/Monster.h"
#include "../src/NPC/NPC.h"
#include "../src/NPC/NpcSpawnDatabase.h"
#include "../src/Persistence/PlayerSaveFileStore.h"
#include "../src/Persistence/PlayerSaveState.h"
#include "../src/Player/Player.h"
#include "../src/World/World.h"

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <limits>
#include <stdexcept>

struct WorldTestAccess
{
    static bool RemoveEntity(World &world, int id) { return world.RemoveEntity(id); }
};

namespace
{
    using namespace StarterWorldExport;

    struct TemporaryDirectory
    {
        std::filesystem::path path = std::filesystem::temp_directory_path() /
            ("embervale_runtime_import_" + std::to_string(
                std::chrono::steady_clock::now().time_since_epoch().count()));
        TemporaryDirectory() { std::filesystem::create_directories(path); }
        ~TemporaryDirectory()
        {
            std::error_code ignored;
            std::filesystem::remove_all(path, ignored);
        }
    };

    std::string ReadBytes(const std::filesystem::path &path)
    {
        std::ifstream input(path, std::ios::binary);
        return {std::istreambuf_iterator<char>{input}, std::istreambuf_iterator<char>{}};
    }

    bool SamePosition(const Position &left, const Position &right)
    {
        return left.GetX() == right.GetX() && left.GetY() == right.GetY();
    }

    Placement &Resource(Snapshot &snapshot, ResourceType type)
    {
        for (auto &placement : snapshot.placements)
            if (const auto *bound = std::get_if<ResourceType>(&placement.legacyDefinition);
                bound != nullptr && *bound == type)
                return placement;
        throw std::runtime_error("Missing test resource binding");
    }

    Placement &Spawn(Snapshot &snapshot, NpcSpawnId id)
    {
        for (auto &placement : snapshot.placements)
            if (const auto *bound = std::get_if<NpcSpawnDefinition>(&placement.legacyDefinition);
                bound != nullptr && bound->spawnId == id)
                return placement;
        throw std::runtime_error("Missing test spawn binding");
    }

    void Move(Placement &placement, int x, int y)
    {
        placement.position.SetPosition(x, y);
        if (auto *spawn = std::get_if<NpcSpawnDefinition>(&placement.legacyDefinition))
            spawn->spawnPosition = placement.position;
    }

    Entity *Actor(World &world, NpcSpawnId id)
    {
        for (const auto &entity : world.GetEntities())
        {
            if (auto *npc = dynamic_cast<NPC *>(entity.get()); npc && npc->GetNpcSpawnId() == id)
                return npc;
            if (auto *monster = dynamic_cast<Monster *>(entity.get()); monster && monster->GetNpcSpawnId() == id)
                return monster;
        }
        return nullptr;
    }

    void CompareWorlds(TestContext &test, World &compiled, World &imported)
    {
        bool tilesMatch = compiled.GetMap().GetWidth() == imported.GetMap().GetWidth() &&
            compiled.GetMap().GetHeight() == imported.GetMap().GetHeight();
        for (int y = 0; y < 100; ++y)
            for (int x = 0; x < 100; ++x)
                tilesMatch &= compiled.GetMap().GetTileType(x, y) == imported.GetMap().GetTileType(x, y) &&
                    compiled.GetMap().IsValidPosition(x, y) == imported.GetMap().IsValidPosition(x, y);
        test.Expect(tilesMatch, "Imported baseline preserves every ground/occupancy tile and collision result");
        test.ExpectEqual(imported.GetResources().size(), compiled.GetResources().size(), "Resource count is unchanged");
        test.ExpectEqual(imported.GetStations().size(), compiled.GetStations().size(), "Station count is unchanged");
        for (const auto &expected : compiled.GetResources())
        {
            const auto *actual = imported.GetResourceAt(expected.GetX(), expected.GetY());
            test.Expect(actual && actual->GetID() == expected.GetID() &&
                actual->GetResourceType() == expected.GetResourceType() &&
                actual->GetRemainingUses() == expected.GetRemainingUses(),
                "Resource identity, location, definition and live uses match");
        }
        for (const auto &expected : compiled.GetStations())
        {
            const auto *actual = imported.GetStationAt(expected.GetX(), expected.GetY());
            test.Expect(actual && actual->GetID() == expected.GetID() &&
                actual->GetStationType() == expected.GetStationType(), "Station identity and location match");
        }
        test.ExpectEqual(imported.GetEntities().size(), compiled.GetEntities().size(), "Actor population matches");
        for (const auto &expected : compiled.GetEntities())
        {
            const auto *actual = imported.GetEntityByID(expected->GetID());
            test.Expect(actual && actual->GetType() == expected->GetType() &&
                SamePosition(actual->GetPosition(), expected->GetPosition()),
                "Runtime actor IDs, kinds and positions match");
            if (const auto *monster = dynamic_cast<const Monster *>(expected.get()))
            {
                const auto *other = dynamic_cast<const Monster *>(actual);
                test.Expect(other && other->GetNpcType() == monster->GetNpcType() &&
                    other->GetNpcSpawnId() == monster->GetNpcSpawnId() &&
                    other->GetWanderRadius() == monster->GetWanderRadius() &&
                    other->GetWanderIntervalTicks() == monster->GetWanderIntervalTicks() &&
                    other->GetOriginalSpawnX() == monster->GetOriginalSpawnX() &&
                    other->GetOriginalSpawnY() == monster->GetOriginalSpawnY() &&
                    other->GetCurrentHealth() == monster->GetCurrentHealth(),
                    "Monster identity, home, wandering and health match");
            }
            if (const auto *npc = dynamic_cast<const NPC *>(expected.get()))
            {
                const auto *other = dynamic_cast<const NPC *>(actual);
                test.Expect(other && other->GetNpcType() == npc->GetNpcType() &&
                    other->GetNpcSpawnId() == npc->GetNpcSpawnId(), "Friendly NPC identity matches");
            }
        }
        for (const auto &spawn : NpcSpawnDatabase::GetStarterSpawns())
            test.ExpectEqual(imported.GetNpcSpawnManager().GetActiveCount(spawn.spawnId),
                compiled.GetNpcSpawnManager().GetActiveCount(spawn.spawnId), "Spawn ownership/capacity matches");
    }

    void CheckBaseline(TestContext &test)
    {
        auto snapshot = ParseSnapshot(SerializeCanonical(Capture()));
        std::reverse(snapshot.placements.begin(), snapshot.placements.end());
        World compiled(19U, 23U, 29U);
        World imported(snapshot, 19U, 23U, 29U);
        test.ExpectEqual(imported.GetEntities().size(), std::size_t{3}, "Only world-start actors exist initially");
        CompareWorlds(test, compiled, imported);
        test.ExpectEqual(imported.CreatePlayer(), compiled.CreatePlayer(), "Player creation keeps runtime ID allocation");
        test.ExpectEqual(imported.GetEntities().size(), std::size_t{11}, "First player creates all seven delayed actors");
        CompareWorlds(test, compiled, imported);
        test.ExpectEqual(imported.CreatePlayer(), compiled.CreatePlayer(), "Second player preserves ID order");
        test.ExpectEqual(imported.GetEntities().size(), std::size_t{12}, "Subsequent players do not duplicate starter actors");
        for (int tick = 0; tick < 30; ++tick)
        {
            compiled.Update();
            imported.Update();
        }
        CompareWorlds(test, compiled, imported);
    }

    void CheckMovedResource(TestContext &test)
    {
        auto snapshot = Capture();
        auto &tree = Resource(snapshot, ResourceType::NORMAL_TREE);
        const auto id = tree.id;
        snapshot.terrainRows[5][5] = 'S';
        // Keep this gathering test outside the elite monster's detection area.
        Move(tree, 40, 20);
        World world(snapshot, 31U, 37U, 41U);
        test.Expect(world.GetResourceAt(5, 5) == nullptr, "Moved tree no longer occupies its old location");
        test.Expect(world.GetMap().GetTileType(5, 5) == TileType::STONE &&
            world.GetMap().IsValidPosition(5, 5), "Old location restores authored ground, including an edited ground tile");
        auto *resource = world.GetResourceAt(40, 20);
        test.Expect(resource && resource->GetID() == 1 && resource->GetResourceType() == ResourceType::NORMAL_TREE,
            "Moved tree retains its definition and ordered runtime identity");
        test.Expect(!world.GetMap().IsValidPosition(40, 20), "Moved resource blocks its new tile");
        test.ExpectEqual(Resource(snapshot, ResourceType::NORMAL_TREE).id, id, "Moving keeps the authored stable ID");
        const int playerId = world.CreatePlayer();
        auto *player = dynamic_cast<Player *>(world.GetEntityByID(playerId));
        if (!player || !resource) return;
        player->GetPosition().SetPosition(39, 20);
        int axeSlot = -1;
        for (int slot = 0; slot < Inventory::SlotCount; ++slot)
            if (player->GetInventory().GetSlots()[slot].GetItemType() == ItemType::BRONZE_AXE)
                axeSlot = slot;
        test.Expect(axeSlot >= 0, "Gathering fixture has its starting axe");
        world.EnqueueCommand(UseInventoryItemCommand{playerId, axeSlot});
        world.Update();
        test.Expect(player->GetEquipment().IsEquipped(ItemType::BRONZE_AXE),
            "Gathering fixture equips the axe through the authoritative inventory command");
        world.QueueResourceInteraction(playerId, resource->GetID());
        for (int tick = 0; tick < 64 && player->GetInventory().GetItemAmount(ItemType::LOG) == 0; ++tick)
            world.Update();
        test.Expect(player->GetInventory().GetItemAmount(ItemType::LOG) > 0,
            "Authoritative gathering completes successfully at the imported tree position");
    }

    void CheckMovedSpawns(TestContext &test)
    {
        auto snapshot = Capture();
        snapshot = StarterWorldEdit::MovePlacement(snapshot, "npc.development_guide.001", Position{30, 3}).candidate;
        snapshot = StarterWorldEdit::MovePlacement(snapshot, "monster.passive_development.001", Position{32, 3}).candidate;
        snapshot = StarterWorldEdit::MovePlacement(snapshot, "monster.durable_development.001", Position{34, 3}).candidate;
        snapshot = StarterWorldEdit::MovePlacement(snapshot, "npc.torren_ash.001", Position{36, 3}).candidate;
        snapshot = ParseSnapshot(SerializeCanonical(snapshot));
        World world(snapshot, 43U, 47U, 53U);
        const auto *guide = Actor(world, NpcSpawnId::DEVELOPMENT_GUIDE_SPAWN);
        test.Expect(guide && SamePosition(guide->GetPosition(), Position{30, 3}), "World-start NPC uses imported home");
        auto *passive = dynamic_cast<Monster *>(Actor(world, NpcSpawnId::PASSIVE_DEVELOPMENT_SPAWN));
        test.Expect(passive && passive->GetOriginalSpawnX() == 32 && passive->GetOriginalSpawnY() == 3,
            "Wandering monster retains the imported home for its lifecycle");
        test.Expect(Actor(world, NpcSpawnId::DURABLE_DEVELOPMENT_SPAWN) == nullptr,
            "Moved advanced monster still waits for first-player registration");
        world.CreatePlayer();
        const auto *durable = Actor(world, NpcSpawnId::DURABLE_DEVELOPMENT_SPAWN);
        const auto *torren = Actor(world, NpcSpawnId::TORREN_ASH_SPAWN);
        test.Expect(durable && SamePosition(durable->GetPosition(), Position{34, 3}), "Delayed monster uses imported home");
        test.Expect(torren && SamePosition(torren->GetPosition(), Position{36, 3}), "Delayed NPC uses imported home");
        if (passive)
        {
            const int id = passive->GetID();
            for (int tick = 0; tick < 12; ++tick) world.Update();
            test.Expect(std::abs(passive->GetPosition().GetX() - 32) <= passive->GetWanderRadius() &&
                std::abs(passive->GetPosition().GetY() - 3) <= passive->GetWanderRadius(),
                "Imported wanderer stays within its authored home radius");
            passive->ApplyDamage(passive->GetCurrentHealth());
            world.Update();
            test.Expect(world.HasScheduledMonsterRespawn(id), "Imported monster death uses the existing respawn scheduler");
            for (int tick = 0; tick < 8; ++tick) world.Update();
            test.Expect(passive->IsAlive() && passive->GetID() == id &&
                SamePosition(passive->GetPosition(), Position{32, 3}),
                "Respawn preserves identity and restores imported home");
            test.Expect(WorldTestAccess::RemoveEntity(world, id), "Test releases existing spawn capacity");
            const auto &authored = std::get<NpcSpawnDefinition>(
                Spawn(snapshot, NpcSpawnId::PASSIVE_DEVELOPMENT_SPAWN).legacyDefinition);
            const auto &oldCompiled = *NpcSpawnDatabase::TryGet(NpcSpawnId::PASSIVE_DEVELOPMENT_SPAWN);
            test.ExpectEqual(world.CreateMonster(oldCompiled.npcType, oldCompiled), 0,
                "Compiled coordinates cannot impersonate the selected imported spawn");
            auto forged = authored;
            forged.maximumActiveCount = 2;
            test.ExpectEqual(world.CreateMonster(forged.npcType, forged), 0,
                "Imported spawn authority still rejects forged capacity");
            const int replacement = world.CreateMonster(authored.npcType, authored);
            test.Expect(replacement > 0, "Selected authored definition can refill released capacity");
            test.ExpectEqual(world.CreateMonster(authored.npcType, authored), 0,
                "Imported spawns still enforce maximum active count");
        }
        if (guide)
        {
            const int guideId = guide->GetID();
            test.Expect(WorldTestAccess::RemoveEntity(world, guideId), "Test releases friendly NPC capacity");
            const auto &oldGuide = *NpcSpawnDatabase::TryGet(NpcSpawnId::DEVELOPMENT_GUIDE_SPAWN);
            test.ExpectEqual(world.CreateNpc(oldGuide.npcType, oldGuide), 0,
                "Old friendly NPC coordinates cannot bypass per-world authority");
            const auto &authoredGuide = std::get<NpcSpawnDefinition>(
                Spawn(snapshot, NpcSpawnId::DEVELOPMENT_GUIDE_SPAWN).legacyDefinition);
            test.Expect(world.CreateNpc(authoredGuide.npcType, authoredGuide) > 0,
                "Selected friendly NPC definition can refill released capacity");
        }
    }

    void CheckMovedGuideServices(TestContext &test)
    {
        const auto snapshot = StarterWorldEdit::MovePlacement(
            Capture(), "npc.development_guide.001", Position{30, 3}).candidate;
        // Reparse the same representation used by a saved and reopened draft.
        World world(ParseSnapshot(SerializeCanonical(snapshot)), 101U, 103U, 107U);
        auto *guide = dynamic_cast<NPC *>(Actor(world, NpcSpawnId::DEVELOPMENT_GUIDE_SPAWN));
        auto *player = dynamic_cast<Player *>(world.GetEntityByID(world.CreatePlayer()));
        if (!guide || !player) throw std::runtime_error("Missing relocated guide/player fixture");
        player->GetPosition().SetPosition(29, 3);
        const int actor = player->GetID();
        const int target = guide->GetID();
        world.EnqueueCommand(NpcInteractionCommand{actor, target, NpcInteractionType::TALK});
        world.Update();
        const auto *dialogue = world.GetActiveDialogueSession(actor);
        test.Expect(dialogue != nullptr, "Relocated guide opens its ordinary dialogue");
        if (!dialogue) throw std::runtime_error("Moved guide dialogue did not open");
        const auto initialSession = dialogue->sessionId;
        world.EnqueueCommand(QuestAcceptCommand{actor, target, initialSession, QuestId::GATHERING_BASICS});
        world.Update();
        const auto *quest = player->GetQuestJournal().TryGet(QuestId::GATHERING_BASICS);
        test.Expect(quest && quest->state == QuestState::ACTIVE,
            "Relocated guide accepts the existing quest through its real dialogue session");

        // Feed the existing objective event; this tests the relocated giver's
        // completion/reward binding, not the separately covered gathering roll.
        QuestSystem::RecordGathered(player->GetQuestJournal(), ItemType::LOG, 10);
        world.EnqueueCommand(DialogueCloseCommand{actor, initialSession});
        world.Update();
        world.EnqueueCommand(NpcInteractionCommand{actor, target, NpcInteractionType::TALK});
        world.Update();
        dialogue = world.GetActiveDialogueSession(actor);
        if (!dialogue) throw std::runtime_error("Moved guide completion dialogue did not reopen");
        const auto completionSession = dialogue->sessionId;
        const int beforeCoins = player->GetInventory().GetItemAmount(ItemType::COINS);
        world.EnqueueCommand(QuestCompleteCommand{actor, target, completionSession, QuestId::GATHERING_BASICS});
        world.EnqueueCommand(QuestCompleteCommand{actor, target, completionSession, QuestId::GATHERING_BASICS});
        world.Update();
        quest = player->GetQuestJournal().TryGet(QuestId::GATHERING_BASICS);
        test.Expect(quest && quest->state == QuestState::COMPLETED &&
            player->GetInventory().GetItemAmount(ItemType::COINS) == beforeCoins + 10,
            "Relocated guide completes and rewards the quest exactly once");
        if (const auto *open = world.GetActiveDialogueSession(actor))
        {
            world.EnqueueCommand(DialogueCloseCommand{actor, open->sessionId});
            world.Update();
        }
        world.EnqueueCommand(NpcInteractionCommand{actor, target, NpcInteractionType::TRADE});
        world.Update();
        const auto *shop = world.GetActiveShopSession(actor);
        test.Expect(shop != nullptr, "Relocated guide still opens its authored shop");
        if (!shop) throw std::runtime_error("Moved guide shop did not open");
        const auto shopSession = shop->sessionId;
        const int beforeFood = player->GetInventory().GetItemAmount(ItemType::COOKED_MEAT);
        const int beforeBuyCoins = player->GetInventory().GetItemAmount(ItemType::COINS);
        world.EnqueueCommand(ShopBuyCommand{actor, shopSession, ItemType::COOKED_MEAT, 1});
        world.Update();
        test.Expect(player->GetInventory().GetItemAmount(ItemType::COOKED_MEAT) == beforeFood + 1 &&
            player->GetInventory().GetItemAmount(ItemType::COINS) == beforeBuyCoins - 8,
            "Relocated guide shop keeps its ordinary validated purchase semantics");
    }

    void CheckPlayerPersistence(TestContext &test)
    {
        TemporaryDirectory directory;
        const auto path = directory.path / "player.save";
        World oldWorld(59U, 61U, 67U);
        const int oldId = oldWorld.CreatePlayer();
        auto *oldPlayer = dynamic_cast<Player *>(oldWorld.GetEntityByID(oldId));
        oldPlayer->GetPosition().SetPosition(25, 5);
        oldPlayer->GetInventory().AddItem(ItemType::COINS, 123);
        test.Expect(oldWorld.SavePlayerToFile(oldId, path).IsSuccess(), "Save fixture is produced by the actual World lifecycle");
        const auto bytes = ReadBytes(path);
        auto snapshot = Capture();
        Move(Resource(snapshot, ResourceType::NORMAL_TREE), 25, 5);
        snapshot.playerSpawn.SetPosition(27, 7);
        World imported(snapshot, 59U, 61U, 67U);
        const auto result = imported.LoadOrCreatePlayerFromFile(path);
        test.Expect(result.IsSuccess() && result.WasLoadedFromFile() && result.UsedSpawnFallback(),
            "A saved player on a newly blocked tile loads with an explicit fallback");
        const auto *loaded = dynamic_cast<const Player *>(imported.GetEntityByID(result.GetPlayerEntityID()));
        test.Expect(loaded && SamePosition(loaded->GetPosition(), snapshot.playerSpawn) &&
            loaded->GetInventory().GetItemAmount(ItemType::COINS) == 123,
            "Fallback uses the selected safe spawn and preserves inventory");
        test.ExpectEqual(ReadBytes(path), bytes, "Loading edited content does not rewrite the original save");
        test.ExpectEqual(imported.GetEntities().size(), std::size_t{11}, "Loaded player triggers delayed actors once");
        const int freshId = imported.CreatePlayer();
        const auto *fresh = imported.GetEntityByID(freshId);
        test.Expect(fresh && SamePosition(fresh->GetPosition(), snapshot.playerSpawn), "Direct player creation uses selected spawn");
        test.ExpectEqual(imported.GetEntities().size(), std::size_t{12}, "A second registration does not repeat delayed actors");
        World noFile(snapshot, 59U, 61U, 67U);
        const auto newResult = noFile.LoadOrCreatePlayerFromFile(directory.path / "absent.save");
        test.Expect(newResult.IsSuccess() && newResult.WasCreatedNew() &&
            SamePosition(noFile.GetEntityByID(newResult.GetPlayerEntityID())->GetPosition(), snapshot.playerSpawn),
            "Missing save creation also uses the selected spawn");
        auto *respawning = dynamic_cast<Player *>(noFile.GetEntityByID(newResult.GetPlayerEntityID()));
        respawning->GetPosition().SetPosition(28, 7);
        respawning->ApplyDamage(respawning->GetCurrentHealth());
        for (int tick = 0; tick < 16; ++tick) noFile.Update();
        test.Expect(respawning->IsAlive() && SamePosition(respawning->GetPosition(), snapshot.playerSpawn),
            "Player death recovery returns to the imported safe spawn");
    }

    void CheckDisconnectedSaveRecovery(TestContext &test)
    {
        TemporaryDirectory directory;
        const auto path = directory.path / "isolated-player.save";
        Player source(99, PlayerInitializationMode::EMPTY);
        source.GetPosition().SetPosition(30, 30);
        source.GetInventory().AddItem(ItemType::COINS, 123);
        auto save = PlayerSaveState::Capture(source);
        save.recoveryPile.active = true;
        save.recoveryPile.positionX = 31;
        save.recoveryPile.positionY = 30;
        save.recoveryPile.remainingTicks = 100;
        save.recoveryPile.items.push_back({ItemType::COINS, 77});
        test.Expect(PlayerSaveFileStore::Save(path, save).IsSuccess(),
            "Disconnected-player fixture retains a saved inventory recovery pile");
        const auto originalBytes = ReadBytes(path);

        auto snapshot = Capture();
        for (auto &row : snapshot.terrainRows) row[25] = 'W';
        snapshot.playerSpawn.SetPosition(2, 0);
        World imported(snapshot, 73U, 79U, 83U);
        test.Expect(imported.GetMap().IsValidPosition(30, 30),
            "Saved player cell is walkable but separated from all starter services");
        const auto result = imported.LoadOrCreatePlayerFromFile(path);
        test.Expect(result.IsSuccess() && result.UsedSpawnFallback() &&
            result.Contains(WorldPlayerPersistenceIssueCode::SAVED_POSITION_UNREACHABLE),
            "Imported terrain cannot strand a restored player on a disconnected walkable island");
        auto *player = dynamic_cast<Player *>(imported.GetEntityByID(result.GetPlayerEntityID()));
        test.Expect(player && SamePosition(player->GetPosition(), snapshot.playerSpawn),
            "Disconnected player is restored at the selected reachable safe spawn");
        const auto *pile = imported.GetPlayerRecoveryPile(result.GetPlayerEntityID());
        test.Expect(result.Contains(WorldPlayerPersistenceIssueCode::RECOVERY_PILE_RELOCATED) &&
            pile && SamePosition(pile->position, snapshot.playerSpawn) &&
            pile->items.size() == 1 && pile->items[0].itemType == ItemType::COINS &&
            pile->items[0].quantity == 77 && pile->expiryTick == 100,
            "Unsafe saved pile moves to safety without losing items or remaining lifetime");
        imported.Update();
        test.Expect(player && player->GetInventory().GetItemAmount(ItemType::COINS) == 200 &&
            imported.GetPlayerRecoveryPile(result.GetPlayerEntityID()) == nullptr,
            "Relocated inventory is recovered through the ordinary live pile lifecycle");
        test.ExpectEqual(ReadBytes(path), originalBytes,
            "Recovery migration leaves the original save bytes untouched until an explicit save");

        World compiled(73U, 79U, 83U);
        for (int y = 0; y < compiled.GetMap().GetHeight(); ++y)
            compiled.GetMap().SetTileType(25, y, TileType::WATER);
        const auto legacyResult = compiled.LoadOrCreatePlayerFromFile(path);
        const auto *legacyPlayer = compiled.GetEntityByID(legacyResult.GetPlayerEntityID());
        const auto *legacyPile = compiled.GetPlayerRecoveryPile(legacyResult.GetPlayerEntityID());
        test.Expect(legacyResult.IsSuccess() && !legacyResult.UsedSpawnFallback() &&
            legacyPlayer && SamePosition(legacyPlayer->GetPosition(), Position{30, 30}) &&
            legacyPile && SamePosition(legacyPile->position, Position{31, 30}),
            "Compiled-world save restoration retains its original walkability-only behavior");
        test.Expect(!legacyResult.Contains(WorldPlayerPersistenceIssueCode::SAVED_POSITION_UNREACHABLE) &&
            !legacyResult.Contains(WorldPlayerPersistenceIssueCode::RECOVERY_PILE_RELOCATED),
            "Imported-only migration warnings do not appear for the compiled world");
    }

    void CheckConstructorRejection(TestContext &test)
    {
        auto snapshot = Capture();
        snapshot.width = std::numeric_limits<int>::max();
        bool rejected = false;
        try { World invalid(snapshot, 71U); }
        catch (const std::runtime_error &) { rejected = true; }
        test.Expect(rejected, "Invalid dimensions are rejected before Map can allocate or index terrain");
        snapshot = Capture();
        Move(Resource(snapshot, ResourceType::NORMAL_TREE), 0, 0);
        rejected = false;
        try { World invalid(snapshot, 71U); }
        catch (const std::runtime_error &) { rejected = true; }
        test.Expect(rejected, "A resource blocking the player spawn is rejected before runtime creation");
    }
}

int main()
{
    TestContext test;
    try
    {
        CheckBaseline(test);
        CheckMovedResource(test);
        CheckMovedSpawns(test);
        CheckMovedGuideServices(test);
        CheckPlayerPersistence(test);
        CheckDisconnectedSaveRecovery(test);
        CheckConstructorRejection(test);
    }
    catch (const std::exception &error)
    {
        test.Expect(false, std::string{"Unexpected runtime importer failure: "} + error.what());
    }
    return test.Finish();
}
