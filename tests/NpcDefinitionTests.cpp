#include "TestSupport.h"

#include "../src/Content/ContentValidator.h"
#include "../src/Entity/Monster/Monster.h"
#include "../src/NPC/NpcDefinitionDatabase.h"
#include "../src/NPC/NpcSpawnDatabase.h"
#include "../src/World/Development/DevelopmentWorldContent.h"
#include "../src/World/World.h"

#include <set>
#include <vector>

namespace
{
    Monster *FindMonster(World &world, NpcType type)
    {
        for (const auto &entity : world.GetEntities())
        {
            Monster *monster = dynamic_cast<Monster *>(entity.get());
            if (monster != nullptr && monster->GetNpcType() == type)
                return monster;
        }
        return nullptr;
    }
}

int main()
{
    TestContext test;

    const NpcDefinition *passive = NpcDefinitionDatabase::TryGet(
        NpcType::PASSIVE_DEVELOPMENT_MONSTER);
    const NpcDefinition *aggressive = NpcDefinitionDatabase::TryGet(
        NpcType::AGGRESSIVE_DEVELOPMENT_MONSTER);

    test.Expect(passive != nullptr, "Passive definition resolves by stable ID");
    test.Expect(aggressive != nullptr, "Aggressive definition resolves by stable ID");
    test.Expect(
        NpcDefinitionDatabase::TryGet(static_cast<NpcType>(999)) == nullptr,
        "Unknown NPC definition ID fails safely");
    test.Expect(
        NpcDefinitionDatabase::TryGet(NpcType::NONE) == nullptr,
        "Default NPC definition ID does not resolve");

    std::set<NpcType> uniqueTypes;
    for (NpcType type : NpcDefinitionDatabase::GetAllNpcTypes())
        uniqueTypes.insert(type);
    test.ExpectEqual(
        uniqueTypes.size(),
        NpcDefinitionDatabase::GetAllNpcTypes().size(),
        "NPC definition IDs are unique");

    if (passive != nullptr && aggressive != nullptr)
    {
        for (const NpcDefinition *definition : {passive, aggressive})
        {
            test.Expect(definition->combat.has_value(), "Monster has combat data");
            test.ExpectEqual(definition->combat->ratings.attackAccuracy, 5, "Attack accuracy is preserved");
            test.ExpectEqual(definition->combat->ratings.meleeStrength, 4, "Melee strength is preserved");
            test.ExpectEqual(definition->combat->ratings.defence, 3, "Defence is preserved");
            test.ExpectEqual(definition->combat->ratings.maximumHealth, 30, "Maximum health is preserved");
            test.ExpectEqual(definition->combat->attackDurationTicks, 5, "Attack duration is preserved");
            test.ExpectEqual(definition->combat->respawnDelayTicks, 8, "Respawn delay is preserved");
            test.ExpectEqual(
                static_cast<int>(definition->combat->rewardTableType),
                static_cast<int>(RewardTableType::DEVELOPMENT_MONSTER),
                "Reward-table identity is preserved");
        }
        test.Expect(!passive->combat->aggression.has_value(), "Passive definition is non-aggressive");
        test.Expect(aggressive->combat->aggression.has_value(), "Aggressive definition is aggressive");
        test.ExpectEqual(aggressive->combat->aggression->detectionRadius, 5, "Detection radius is preserved");
        test.ExpectEqual(aggressive->combat->aggression->leashRadius, 8, "Leash radius is preserved");

        NpcDefinition emptyName = *passive;
        emptyName.name.clear();
        test.Expect(!ContentValidator::ValidateNpcDefinition(emptyName).IsValid(), "Empty NPC name is rejected");

        NpcDefinition invalidDuration = *passive;
        invalidDuration.combat->attackDurationTicks = 0;
        test.Expect(!ContentValidator::ValidateNpcDefinition(invalidDuration).IsValid(), "Non-positive attack duration is rejected");

        NpcDefinition negativeRatings = *passive;
        negativeRatings.combat->ratings.attackAccuracy = -1;
        test.Expect(!ContentValidator::ValidateNpcDefinition(negativeRatings).IsValid(), "Negative combat ratings are rejected");

        NpcDefinition invalidReward = *passive;
        invalidReward.combat->rewardTableType = RewardTableType::NONE;
        test.Expect(!ContentValidator::ValidateNpcDefinition(invalidReward).IsValid(), "Invalid reward-table reference is rejected");

        NpcDefinition invalidHealth = *passive;
        invalidHealth.combat->ratings.maximumHealth = 0;
        test.Expect(!ContentValidator::ValidateNpcDefinition(invalidHealth).IsValid(), "Non-positive health is rejected");

        NpcDefinition invalidRespawnDelay = *passive;
        invalidRespawnDelay.combat->respawnDelayTicks = -1;
        test.Expect(!ContentValidator::ValidateNpcDefinition(invalidRespawnDelay).IsValid(), "Negative respawn delay is rejected");

        NpcDefinition unknownDefinition = *passive;
        unknownDefinition.type = static_cast<NpcType>(999);
        test.Expect(!ContentValidator::ValidateNpcDefinition(unknownDefinition).IsValid(), "Unknown definition ID is rejected by validation");

        test.Expect(
            !ContentValidator::ValidateNpcDefinitions(
                 std::vector<NpcDefinition>{*passive, *passive})
                 .IsValid(),
            "Duplicate definition IDs are rejected directly");
    }

    const auto &spawns = NpcSpawnDatabase::GetStarterMonsterSpawns();
    test.ExpectEqual(spawns.size(), static_cast<std::size_t>(2), "Exactly two starter monster spawns remain");
    test.ExpectEqual(spawns[0].spawnPosition.GetX(), 6, "Passive spawn X is preserved");
    test.ExpectEqual(spawns[0].spawnPosition.GetY(), 1, "Passive spawn Y is preserved");
    test.ExpectEqual(spawns[1].spawnPosition.GetX(), 18, "Aggressive spawn X is preserved");
    test.ExpectEqual(spawns[1].spawnPosition.GetY(), 2, "Aggressive spawn Y is preserved");
    test.Expect(spawns[0].spawnId == NpcSpawnId::PASSIVE_DEVELOPMENT_SPAWN,
                "Passive spawn has its stable authored ID");
    test.Expect(spawns[1].spawnId == NpcSpawnId::AGGRESSIVE_DEVELOPMENT_SPAWN,
                "Aggressive spawn has its stable authored ID");
    test.Expect(NpcSpawnDatabase::TryGet(NpcSpawnId::NONE) == nullptr,
                "Unknown spawn ID fails safely");
    test.ExpectEqual(spawns[0].wanderRadius, 2, "Passive wander radius is authored");
    test.ExpectEqual(spawns[0].wanderIntervalTicks, 5, "Passive wander interval is authored");
    test.ExpectEqual(spawns[0].maximumActiveCount, 1, "Passive capacity is authored");
    test.ExpectEqual(spawns[1].wanderRadius, 0, "Aggressive wandering remains disabled");
    test.Expect(spawns[0].respawns && spawns[1].respawns, "Starter respawn flags are preserved");

    NpcSpawnDefinition secondPassive = spawns[0];
    secondPassive.spawnId = NpcSpawnId::AGGRESSIVE_DEVELOPMENT_SPAWN;
    secondPassive.spawnPosition.SetPosition(7, 1);
    test.Expect(
        ContentValidator::ValidateNpcSpawnDefinitions(
            std::vector<NpcSpawnDefinition>{spawns[0], secondPassive},
            DevelopmentWorldContent::MapWidth,
            DevelopmentWorldContent::MapHeight)
            .IsValid() == false,
        "Spawn identity cannot be paired with a different NPC type");
    NpcSpawnDefinition duplicateId = spawns[0];
    duplicateId.spawnPosition.SetPosition(8, 1);
    test.Expect(!ContentValidator::ValidateNpcSpawnDefinitions(
                     std::vector<NpcSpawnDefinition>{spawns[0], duplicateId},
                     DevelopmentWorldContent::MapWidth,
                     DevelopmentWorldContent::MapHeight).IsValid(),
                "Different records cannot reuse one stable spawn ID");
    test.Expect(
        !ContentValidator::ValidateNpcSpawnDefinitions(
             std::vector<NpcSpawnDefinition>{spawns[0], spawns[0]},
             DevelopmentWorldContent::MapWidth,
             DevelopmentWorldContent::MapHeight)
             .IsValid(),
        "Exact duplicate spawn records are rejected");

    NpcSpawnDefinition invalidWander = spawns[0];
    invalidWander.wanderRadius = -1;
    test.Expect(
        !ContentValidator::ValidateNpcSpawnDefinition(
             invalidWander,
             DevelopmentWorldContent::MapWidth,
             DevelopmentWorldContent::MapHeight)
             .IsValid(),
        "Negative wander radius is rejected");

    NpcSpawnDefinition invalidSpawnId = spawns[0];
    invalidSpawnId.spawnId = NpcSpawnId::NONE;
    test.Expect(!ContentValidator::ValidateNpcSpawnDefinition(
                    invalidSpawnId, DevelopmentWorldContent::MapWidth,
                    DevelopmentWorldContent::MapHeight).IsValid(),
                "NONE spawn ID is rejected");

    NpcSpawnDefinition unknownStableSpawnId = spawns[0];
    unknownStableSpawnId.spawnId = static_cast<NpcSpawnId>(999);
    test.Expect(!ContentValidator::ValidateNpcSpawnDefinition(
                    unknownStableSpawnId, DevelopmentWorldContent::MapWidth,
                    DevelopmentWorldContent::MapHeight).IsValid(),
                "Unknown spawn ID is rejected by content validation");

    NpcSpawnDefinition invalidInterval = spawns[0];
    invalidInterval.wanderIntervalTicks = 0;
    test.Expect(!ContentValidator::ValidateNpcSpawnDefinition(
                    invalidInterval, DevelopmentWorldContent::MapWidth,
                    DevelopmentWorldContent::MapHeight).IsValid(),
                "Enabled wandering requires a positive interval");

    NpcSpawnDefinition invalidCapacity = spawns[0];
    invalidCapacity.maximumActiveCount = 0;
    test.Expect(!ContentValidator::ValidateNpcSpawnDefinition(
                    invalidCapacity, DevelopmentWorldContent::MapWidth,
                    DevelopmentWorldContent::MapHeight).IsValid(),
                "Spawn capacity must be positive");

    NpcSpawnDefinition invalidDisabledInterval = spawns[1];
    invalidDisabledInterval.wanderIntervalTicks = 5;
    test.Expect(!ContentValidator::ValidateNpcSpawnDefinition(
                    invalidDisabledInterval, DevelopmentWorldContent::MapWidth,
                    DevelopmentWorldContent::MapHeight).IsValid(),
                "Disabled wandering requires exactly zero interval");

    NpcSpawnDefinition unknownSpawn = spawns[0];
    unknownSpawn.npcType = static_cast<NpcType>(999);
    test.Expect(
        !ContentValidator::ValidateNpcSpawnDefinition(
             unknownSpawn,
             DevelopmentWorldContent::MapWidth,
             DevelopmentWorldContent::MapHeight)
             .IsValid(),
        "Unknown spawn definition reference is rejected");

    World world;
    Monster *spawnedPassive = FindMonster(world, NpcType::PASSIVE_DEVELOPMENT_MONSTER);
    Monster *spawnedAggressive = FindMonster(world, NpcType::AGGRESSIVE_DEVELOPMENT_MONSTER);
    test.Expect(spawnedPassive != nullptr && spawnedAggressive != nullptr, "World creates both definition-driven monsters");
    if (spawnedPassive != nullptr && spawnedAggressive != nullptr)
    {
        test.ExpectEqual(spawnedPassive->GetID(), 2, "Passive creation order and ID are preserved");
        test.ExpectEqual(spawnedAggressive->GetID(), 3, "Aggressive creation order and ID are preserved");
        test.ExpectEqual(spawnedPassive->GetAttackDurationTicks(), 5, "Passive runtime attack duration comes from its definition");
        test.ExpectEqual(spawnedAggressive->GetAttackDurationTicks(), 5, "Aggressive runtime attack duration comes from its definition");
        test.ExpectEqual(spawnedPassive->GetMaximumHealth(), 30, "Passive runtime health comes from its definition");
        test.ExpectEqual(spawnedAggressive->GetMaximumHealth(), 30, "Aggressive runtime health comes from its definition");
        test.ExpectEqual(spawnedPassive->GetRespawnDefinition()->delayTicks, 8, "Passive runtime respawn delay comes from its definition");
        test.ExpectEqual(spawnedAggressive->GetRespawnDefinition()->delayTicks, 8, "Aggressive runtime respawn delay comes from its definition");
    }

    const std::size_t entityCount = world.GetEntities().size();
    test.ExpectEqual(
        world.CreateMonster(passive->type, unknownSpawn),
        0,
        "Invalid definition/spawn pairing fails safely");
    test.ExpectEqual(
        world.CreateMonster(unknownSpawn.npcType, unknownSpawn),
        0,
        "Unknown requested NPC type fails safely");
    NpcSpawnDefinition unknownSpawnId = spawns[0];
    unknownSpawnId.spawnId = static_cast<NpcSpawnId>(999);
    test.ExpectEqual(
        world.CreateMonster(unknownSpawnId.npcType, unknownSpawnId),
        0,
        "Unknown authored spawn ID creates no entity");
    test.ExpectEqual(
        world.CreateMonster(
            NpcType::AGGRESSIVE_DEVELOPMENT_MONSTER,
            spawns[0]),
        0,
        "Requested definition ID must match the spawn definition ID");
    test.ExpectEqual(world.GetEntities().size(), entityCount, "Invalid spawn creates no partial entity");
    test.ExpectEqual(
        world.CreatePlayer(),
        4,
        "Invalid definition-driven creation does not consume the next entity ID");

    NpcSpawnDefinition invalidPosition = spawns[0];
    invalidPosition.spawnPosition.SetPosition(-1, 1);
    const std::size_t countBeforeInvalidPosition = world.GetEntities().size();
    test.ExpectEqual(
        world.CreateMonster(invalidPosition.npcType, invalidPosition),
        0,
        "Definition-driven creation rejects an invalid position");
    test.ExpectEqual(
        world.GetEntities().size(),
        countBeforeInvalidPosition,
        "Invalid position leaves no partial entity");
    test.ExpectEqual(
        world.CreatePlayer(),
        5,
        "Invalid position does not consume the next entity ID");

    test.ExpectEqual(
        world.CreateMonster(spawns[0].npcType, spawns[0]),
        0,
        "Authored spawn capacity prevents a duplicate runtime monster");
    test.ExpectEqual(
        world.CreatePlayer(),
        6,
        "Capacity failure does not consume the next entity ID");

    World primitiveWorld;
    const int primitiveId = primitiveWorld.CreateMonster(
        2, 2, CombatRatings{1, 1, 1, 10});
    Monster *primitiveMonster = dynamic_cast<Monster *>(
        primitiveWorld.GetEntityByID(primitiveId));
    test.Expect(primitiveMonster != nullptr, "Primitive monster fixture creation remains available");
    if (primitiveMonster != nullptr)
    {
        test.Expect(primitiveMonster->GetNpcSpawnId() == NpcSpawnId::NONE,
                    "Primitive monster has no authored spawn ID");
        test.ExpectEqual(primitiveMonster->GetWanderRadius(), 0,
                         "Primitive monster receives no wandering configuration");
    }
    test.Expect(!primitiveWorld.GetNpcSpawnManager().GetSpawnId(primitiveId).has_value(),
                "Primitive monster receives no spawn-manager membership");
    test.ExpectEqual(primitiveWorld.GetNpcSpawnManager().GetActiveCount(
                         NpcSpawnId::PASSIVE_DEVELOPMENT_SPAWN), 1,
                     "Primitive monster consumes no authored spawn capacity");

    return test.Finish();
}
