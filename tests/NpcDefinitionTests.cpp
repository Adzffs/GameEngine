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
    const NpcDefinition *durable = NpcDefinitionDatabase::TryGet(
        NpcType::DURABLE_DEVELOPMENT_MONSTER);
    const NpcDefinition *elite = NpcDefinitionDatabase::TryGet(
        NpcType::ELITE_DEVELOPMENT_MONSTER);

    test.Expect(passive != nullptr, "Passive definition resolves by stable ID");
    test.Expect(aggressive != nullptr, "Aggressive definition resolves by stable ID");
    test.Expect(durable != nullptr, "Durable definition resolves by stable ID");
    test.Expect(elite != nullptr, "Elite definition resolves by stable ID");
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

    if (passive != nullptr && aggressive != nullptr &&
        durable != nullptr && elite != nullptr)
    {
        for (const NpcDefinition *definition : {passive, aggressive, durable, elite})
        {
            test.Expect(definition->combat.has_value(), "Monster has combat data");
            test.Expect(definition->combat->ratings.maximumHealth > 0,
                "Monster health is positive");
            test.Expect(definition->combat->attackDurationTicks > 0,
                "Monster attack cadence is positive");
            test.Expect(definition->combat->respawnDelayTicks > 0,
                "Monster respawn pacing is positive");
        }
        test.Expect(passive->combat->ratings.maximumHealth <
                aggressive->combat->ratings.maximumHealth &&
                aggressive->combat->ratings.maximumHealth <
                durable->combat->ratings.maximumHealth &&
                durable->combat->ratings.maximumHealth <
                elite->combat->ratings.maximumHealth,
            "Roster health rises through practice, threat, durable and elite tiers");
        test.Expect(durable->combat->ratings.defence >
                aggressive->combat->ratings.defence,
            "Stoneback is defined by substantially higher defence");
        test.Expect(elite->combat->ratings.attackAccuracy >
                durable->combat->ratings.attackAccuracy &&
                elite->combat->ratings.meleeStrength >
                durable->combat->ratings.meleeStrength,
            "Cinder Warden tests complete combat preparation");
        test.Expect(passive->combat->rewardTableType == RewardTableType::DEVELOPMENT_MONSTER &&
                aggressive->combat->rewardTableType == RewardTableType::AGGRESSIVE_MONSTER &&
                durable->combat->rewardTableType == RewardTableType::DURABLE_MONSTER &&
                elite->combat->rewardTableType == RewardTableType::ELITE_MONSTER,
            "Every roster role has a distinct reward identity");
        test.Expect(!passive->combat->aggression.has_value(), "Passive definition is non-aggressive");
        test.Expect(aggressive->combat->aggression.has_value(), "Aggressive definition is aggressive");
        test.ExpectEqual(aggressive->combat->aggression->detectionRadius, 4, "Detection radius is preserved");
        test.ExpectEqual(aggressive->combat->aggression->leashRadius, 7, "Leash radius is preserved");

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
    test.ExpectEqual(spawns.size(), static_cast<std::size_t>(4),
        "Four progression-role monster spawns are authored");
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
    test.Expect(spawns[2].spawnId == NpcSpawnId::DURABLE_DEVELOPMENT_SPAWN &&
            spawns[2].spawnPosition.GetX() == 16 &&
            spawns[2].spawnPosition.GetY() == 11,
        "Stoneback occupies the workshop-edge durable habitat");
    test.Expect(spawns[3].spawnId == NpcSpawnId::ELITE_DEVELOPMENT_SPAWN &&
            spawns[3].spawnPosition.GetX() == 22 &&
            spawns[3].spawnPosition.GetY() == 3,
        "Cinder Warden occupies the eastern elite arena");

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
        test.ExpectEqual(spawnedPassive->GetAttackDurationTicks(), 6, "Passive runtime attack duration comes from its definition");
        test.ExpectEqual(spawnedAggressive->GetAttackDurationTicks(), 4, "Aggressive runtime attack duration comes from its definition");
        test.ExpectEqual(spawnedPassive->GetMaximumHealth(), 20, "Passive runtime health comes from its definition");
        test.ExpectEqual(spawnedAggressive->GetMaximumHealth(), 35, "Aggressive runtime health comes from its definition");
        test.ExpectEqual(spawnedPassive->GetRespawnDefinition()->delayTicks, 8, "Passive runtime respawn delay comes from its definition");
        test.ExpectEqual(spawnedAggressive->GetRespawnDefinition()->delayTicks, 10, "Aggressive runtime respawn delay comes from its definition");
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
    Monster *spawnedDurable = FindMonster(
        world, NpcType::DURABLE_DEVELOPMENT_MONSTER);
    Monster *spawnedElite = FindMonster(
        world, NpcType::ELITE_DEVELOPMENT_MONSTER);
    test.Expect(spawnedDurable != nullptr && spawnedElite != nullptr,
        "Advanced encounters activate after the first player is registered");
    if (spawnedDurable != nullptr && spawnedElite != nullptr)
    {
        test.ExpectEqual(spawnedDurable->GetID(), 5,
            "Stoneback activates after stable player ID four");
        test.ExpectEqual(spawnedElite->GetID(), 6,
            "Cinder Warden activates after Stoneback");
        test.ExpectEqual(spawnedDurable->GetMaximumHealth(), 90,
            "Stoneback runtime uses its durable health profile");
        test.ExpectEqual(spawnedElite->GetMaximumHealth(), 140,
            "Cinder Warden runtime uses its elite health profile");
    }

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
        12,
        "Invalid position does not consume the next entity ID");

    test.ExpectEqual(
        world.CreateMonster(spawns[0].npcType, spawns[0]),
        0,
        "Authored spawn capacity prevents a duplicate runtime monster");
    test.ExpectEqual(
        world.CreatePlayer(),
        13,
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
