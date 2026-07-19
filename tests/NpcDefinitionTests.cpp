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
            test.ExpectEqual(definition->combatRatings.attackAccuracy, 5, "Attack accuracy is preserved");
            test.ExpectEqual(definition->combatRatings.meleeStrength, 4, "Melee strength is preserved");
            test.ExpectEqual(definition->combatRatings.defence, 3, "Defence is preserved");
            test.ExpectEqual(definition->combatRatings.maximumHealth, 30, "Maximum health is preserved");
            test.ExpectEqual(definition->attackDurationTicks, 5, "Attack duration is preserved");
            test.ExpectEqual(definition->respawnDelayTicks, 8, "Respawn delay is preserved");
            test.ExpectEqual(
                static_cast<int>(definition->rewardTableType),
                static_cast<int>(RewardTableType::DEVELOPMENT_MONSTER),
                "Reward-table identity is preserved");
        }
        test.Expect(!passive->aggressionDefinition.has_value(), "Passive definition is non-aggressive");
        test.Expect(aggressive->aggressionDefinition.has_value(), "Aggressive definition is aggressive");
        test.ExpectEqual(aggressive->aggressionDefinition->detectionRadius, 5, "Detection radius is preserved");
        test.ExpectEqual(aggressive->aggressionDefinition->leashRadius, 8, "Leash radius is preserved");

        NpcDefinition emptyName = *passive;
        emptyName.name.clear();
        test.Expect(!ContentValidator::ValidateNpcDefinition(emptyName).IsValid(), "Empty NPC name is rejected");

        NpcDefinition invalidDuration = *passive;
        invalidDuration.attackDurationTicks = 0;
        test.Expect(!ContentValidator::ValidateNpcDefinition(invalidDuration).IsValid(), "Non-positive attack duration is rejected");

        NpcDefinition negativeRatings = *passive;
        negativeRatings.combatRatings.attackAccuracy = -1;
        test.Expect(!ContentValidator::ValidateNpcDefinition(negativeRatings).IsValid(), "Negative combat ratings are rejected");

        NpcDefinition invalidReward = *passive;
        invalidReward.rewardTableType = RewardTableType::NONE;
        test.Expect(!ContentValidator::ValidateNpcDefinition(invalidReward).IsValid(), "Invalid reward-table reference is rejected");

        NpcDefinition invalidHealth = *passive;
        invalidHealth.combatRatings.maximumHealth = 0;
        test.Expect(!ContentValidator::ValidateNpcDefinition(invalidHealth).IsValid(), "Non-positive health is rejected");

        NpcDefinition invalidRespawnDelay = *passive;
        invalidRespawnDelay.respawnDelayTicks = -1;
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
    test.Expect(spawns[0].respawns && spawns[1].respawns, "Starter respawn flags are preserved");

    NpcSpawnDefinition secondPassive = spawns[0];
    secondPassive.spawnPosition.SetPosition(7, 1);
    test.Expect(
        ContentValidator::ValidateNpcSpawnDefinitions(
            std::vector<NpcSpawnDefinition>{spawns[0], secondPassive},
            DevelopmentWorldContent::MapWidth,
            DevelopmentWorldContent::MapHeight)
            .IsValid(),
        "Separate spawns may share one NPC definition ID");
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
        world.CreateMonster(secondPassive.npcType, secondPassive),
        6,
        "A second valid spawn may use the same NPC definition");

    return test.Finish();
}
