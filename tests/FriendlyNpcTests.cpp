#include "TestSupport.h"
#include "../src/Combat/Combatant.h"
#include "../src/Content/ContentValidator.h"
#include "../src/Entity/Monster/Monster.h"
#include "../src/NPC/NPC.h"
#include "../src/NPC/NpcDefinitionDatabase.h"
#include "../src/NPC/NpcSpawnDatabase.h"
#include "../src/World/World.h"

#include <string>
#include <utility>
#include <vector>

int main()
{
    TestContext test;
    const NpcDefinition *guide = NpcDefinitionDatabase::TryGet(NpcType::DEVELOPMENT_GUIDE);
    const NpcDefinition *torren = NpcDefinitionDatabase::TryGet(NpcType::TORREN_ASH);
    test.Expect(guide != nullptr, "Development guide definition resolves");
    if (guide != nullptr)
    {
        test.Expect(guide->kind == NpcKind::FRIENDLY, "Guide is friendly");
        test.Expect(!guide->combat.has_value(), "Guide has no combat profile");
        test.ExpectEqual(guide->interactions.size(), std::size_t{2}, "Guide exposes two interactions");
        test.Expect(guide->interactions[0] == NpcInteractionType::TALK, "Guide exposes TALK first");
        test.Expect(guide->interactions[1] == NpcInteractionType::TRADE, "Guide exposes TRADE second");
        test.Expect(guide->shopId == ShopId::DEVELOPMENT_GUIDE_SUPPLIES,
                    "Guide references development supplies");
        test.Expect(guide->dialogueId == DialogueId::DEVELOPMENT_GUIDE_INTRO,
                    "Guide references the authored dialogue");
        test.Expect(guide->name == "Mara Vale" &&
                        !guide->role.empty() && !guide->motivation.empty() &&
                        !guide->speechStyle.empty() &&
                        !guide->relationship.empty() &&
                        !guide->examineText.empty(),
                    "Mara has a complete authored character identity");

        NpcDefinition invalidFriendly = *guide;
        invalidFriendly.combat = NpcCombatDefinition{CombatRatings{1,1,1,1}, 1,
            RewardTableType::DEVELOPMENT_MONSTER, std::nullopt, 1};
        test.Expect(!ContentValidator::ValidateNpcDefinition(invalidFriendly).IsValid(),
                    "Friendly combat data is rejected");
        NpcDefinition unknownKind = *guide;
        unknownKind.kind = static_cast<NpcKind>(999);
        test.Expect(!ContentValidator::ValidateNpcDefinition(unknownKind).IsValid(),
                    "Unknown NPC kind is rejected");
        NpcDefinition duplicateTalk = *guide;
        duplicateTalk.interactions.push_back(NpcInteractionType::TALK);
        test.Expect(!ContentValidator::ValidateNpcDefinition(duplicateTalk).IsValid(),
                    "Duplicate interactions are rejected");
        NpcDefinition missingDialogue = *guide;
        missingDialogue.dialogueId = DialogueId::NONE;
        test.Expect(!ContentValidator::ValidateNpcDefinition(missingDialogue).IsValid(),
                    "TALK without a dialogue is rejected");
        NpcDefinition missingCharacterIdentity = *guide;
        missingCharacterIdentity.motivation.clear();
        test.Expect(!ContentValidator::ValidateNpcDefinition(
                        missingCharacterIdentity).IsValid(),
                    "Friendly characters require a motivation");
    }
    test.Expect(torren != nullptr && torren->name == "Torren Ash" &&
                    torren->dialogueId == DialogueId::TORREN_ASH_INTRO &&
                    torren->interactions.size() == 1 &&
                    torren->interactions[0] == NpcInteractionType::TALK &&
                    !torren->role.empty() && !torren->relationship.empty(),
                "Torren has a complete workshop character definition");
    const std::vector<std::pair<NpcType, std::string>> supportingCast{
        {NpcType::TORREN_ASH, "Torren Ash"},
        {NpcType::ELIA_REED, "Elia Reed"},
        {NpcType::NESSA_THORNE, "Nessa Thorne"},
        {NpcType::BRANN_DEEPWELL, "Brann Deepwell"},
        {NpcType::SERA_FLINT, "Sera Flint"}};
    for (const auto &[type, name] : supportingCast)
    {
        const NpcDefinition *character = NpcDefinitionDatabase::TryGet(type);
        test.Expect(character != nullptr && character->name == name &&
                        character->kind == NpcKind::FRIENDLY &&
                        !character->role.empty() &&
                        !character->motivation.empty() &&
                        !character->speechStyle.empty() &&
                        !character->relationship.empty() &&
                        !character->examineText.empty(),
                    "Every supporting villager has a complete character identity");
    }

    const auto &spawns = NpcSpawnDatabase::GetStarterSpawns();
    test.ExpectEqual(spawns.size(), std::size_t{10},
                    "Starter world defines six villagers and four monsters");
    test.Expect(spawns.front().spawnId == NpcSpawnId::DEVELOPMENT_GUIDE_SPAWN,
                "Guide spawn is first in authored order");
    test.ExpectEqual(spawns.front().spawnPosition.GetX(), 3, "Guide X is preserved");
    test.ExpectEqual(spawns.front().spawnPosition.GetY(), 3, "Guide Y is preserved");
    test.ExpectEqual(spawns.front().maximumActiveCount, 1, "Guide capacity is one");
    test.ExpectEqual(spawns.front().wanderRadius, 0, "Guide wandering is disabled");
    test.Expect(!spawns.front().respawns, "Guide respawning is disabled");
    const NpcSpawnDefinition *torrenSpawn =
        NpcSpawnDatabase::TryGet(NpcSpawnId::TORREN_ASH_SPAWN);
    test.Expect(torrenSpawn != nullptr &&
                    torrenSpawn->spawnPosition.GetX() == 16 &&
                    torrenSpawn->spawnPosition.GetY() == 10,
                "Torren has a stable workshop spawn");

    World world;
    NPC *runtimeGuide = dynamic_cast<NPC *>(world.GetEntityByID(1));
    test.Expect(runtimeGuide != nullptr, "Guide is runtime entity one");
    if (runtimeGuide != nullptr)
    {
        test.Expect(runtimeGuide->GetNpcType() == NpcType::DEVELOPMENT_GUIDE,
                    "Runtime guide stores stable type");
        test.Expect(runtimeGuide->GetNpcSpawnId() == NpcSpawnId::DEVELOPMENT_GUIDE_SPAWN,
                    "Runtime guide stores stable spawn ID");
        test.Expect(dynamic_cast<Combatant *>(runtimeGuide) == nullptr,
                    "Runtime guide is not a combatant");
    }
    test.Expect(dynamic_cast<Monster *>(world.GetEntityByID(2)) != nullptr,
                "Passive monster remains entity two");
    test.Expect(dynamic_cast<Monster *>(world.GetEntityByID(3)) != nullptr,
                "Aggressive monster remains entity three");
    test.ExpectEqual(world.CreatePlayer(), 4, "Player remains entity four");
    NPC *runtimeTorren = dynamic_cast<NPC *>(world.GetEntityByID(7));
    test.Expect(runtimeTorren != nullptr &&
                    runtimeTorren->GetNpcType() == NpcType::TORREN_ASH &&
                    runtimeTorren->GetNpcSpawnId() == NpcSpawnId::TORREN_ASH_SPAWN,
                "Torren enters the live world after the player and advanced encounters");
    const std::vector<NpcType> runtimeCast{
        NpcType::TORREN_ASH, NpcType::ELIA_REED, NpcType::NESSA_THORNE,
        NpcType::BRANN_DEEPWELL, NpcType::SERA_FLINT};
    for (std::size_t index = 0; index < runtimeCast.size(); ++index)
    {
        NPC *character = dynamic_cast<NPC *>(
            world.GetEntityByID(7 + static_cast<int>(index)));
        test.Expect(character != nullptr &&
                        character->GetNpcType() == runtimeCast[index],
                    "Every supporting villager enters the live starter world");
    }
    test.ExpectEqual(world.GetNpcSpawnManager().GetActiveCount(
                         NpcSpawnId::DEVELOPMENT_GUIDE_SPAWN), 1,
                     "Guide consumes authored spawn capacity");
    test.Expect(!world.GetNpcSpawnManager().IsWanderDue(1, 1000),
                "Guide never receives an eligible wander schedule");
    NpcSpawnDefinition mismatchedGuide = spawns.front();
    mismatchedGuide.npcType = NpcType::PASSIVE_DEVELOPMENT_MONSTER;
    test.ExpectEqual(world.CreateNpc(NpcType::DEVELOPMENT_GUIDE, mismatchedGuide), 0,
                     "Invalid guide creation fails without an entity");
    test.ExpectEqual(world.CreateNpc(NpcType::DEVELOPMENT_GUIDE, spawns.front()), 0,
                     "Guide spawn capacity rejects duplicate creation");
    test.ExpectEqual(world.CreatePlayer(), 12,
                     "Invalid guide creation consumes no entity ID");

    if (guide != nullptr)
    {
        NpcDefinition monsterWithoutCombat = *guide;
        monsterWithoutCombat.type = NpcType::PASSIVE_DEVELOPMENT_MONSTER;
        monsterWithoutCombat.kind = NpcKind::MONSTER;
        monsterWithoutCombat.interactions.clear();
        monsterWithoutCombat.dialogueId = DialogueId::NONE;
        test.Expect(!ContentValidator::ValidateNpcDefinition(monsterWithoutCombat).IsValid(),
                    "Monster without combat data is rejected");
    }
    return test.Finish();
}
