#include "TestSupport.h"
#include "../src/Content/ContentValidator.h"
#include "../src/Quest/QuestDefinitionDatabase.h"
#include "../src/Quest/QuestJournal.h"

struct QuestJournalTestAccess
{
    static QuestJournal FromDefinitions(
        const std::vector<QuestDefinition> &definitions)
    {
        return QuestJournal(definitions);
    }
};

int main()
{
    TestContext test;
    const auto &definitions = QuestDefinitionDatabase::GetAll();
    test.Expect(definitions.size() == 6 &&
                    definitions[0].id == QuestId::GATHERING_BASICS &&
                    definitions[1].id == QuestId::MINING_BASICS &&
                    definitions[2].id == QuestId::TRACKS_IN_THE_ASH &&
                    definitions[3].id == QuestId::SHIELD_OR_SHELTER &&
                    definitions[4].id == QuestId::FANGS_AT_THE_GATE &&
                    definitions[5].id == QuestId::HEART_OF_THE_ASH,
                "Canonical catalogue contains all quests in stable ID order");
    const QuestDefinition *definition =
        QuestDefinitionDatabase::TryGet(QuestId::GATHERING_BASICS);
    test.Expect(definition != nullptr &&
                    definition->initialState == QuestState::AVAILABLE &&
                    definition->objectiveItem == ItemType::LOG &&
                    definition->requiredAmount == 10 &&
                    definition->rewardItem == ItemType::COINS &&
                    definition->rewardAmount == 10,
                "Gathering Basics definition remains exact");
    const QuestDefinition *mining =
        QuestDefinitionDatabase::TryGet(QuestId::MINING_BASICS);
    test.Expect(mining != nullptr &&
                    mining->name == std::string_view("Mining Basics") &&
                    mining->initialState == QuestState::AVAILABLE &&
                    mining->objectiveItem == ItemType::COPPER_ORE &&
                    mining->requiredAmount == 5 &&
                    mining->rewardItem == ItemType::COINS &&
                    mining->rewardAmount == 10 &&
                    mining->giver.npcType == NpcType::DEVELOPMENT_GUIDE &&
                    mining->giver.dialogueId == DialogueId::DEVELOPMENT_GUIDE_INTRO &&
                    mining->giver.interactionNodeId == DialogueNodeId::DEVELOPMENT_GUIDE_WELCOME,
                "Mining Basics definition remains exact");
    test.Expect(definition != nullptr &&
                    definition->giver.npcType == NpcType::DEVELOPMENT_GUIDE &&
                    definition->giver.dialogueId ==
                        DialogueId::DEVELOPMENT_GUIDE_INTRO &&
                    definition->giver.interactionNodeId ==
                        DialogueNodeId::DEVELOPMENT_GUIDE_WELCOME,
                "Quest giver context is authoritative content");
    test.Expect(QuestDefinitionDatabase::IsValidCatalogue(definitions) &&
                    ContentValidator::ValidateQuestDefinitions(definitions).IsValid(),
                "Production quest catalogue validates");
    test.Expect(!IsValidQuestId(QuestId::NONE) &&
                    IsValidQuestId(QuestId::GATHERING_BASICS) &&
                    IsValidQuestId(QuestId::MINING_BASICS) &&
                    QuestDefinitionDatabase::TryGet(QuestId::NONE) == nullptr,
                "NONE and unknown quest IDs reject");

    QuestDefinition unavailable = *definition;
    unavailable.initialState = QuestState::UNAVAILABLE;
    test.Expect(QuestDefinitionDatabase::IsValidDefinition(unavailable),
                "UNAVAILABLE is a valid authored initial state");
    const QuestJournal unavailableJournal =
        QuestJournalTestAccess::FromDefinitions({unavailable});
    const QuestRecord *unavailableRecord = unavailableJournal.TryGet(
        QuestId::GATHERING_BASICS);
    test.Expect(unavailableRecord != nullptr &&
                    unavailableRecord->state == QuestState::UNAVAILABLE &&
                    unavailableRecord->progress == 0,
                "Journal initialization derives test-only UNAVAILABLE state from definition");
    unavailable.initialState = QuestState::ACTIVE;
    test.Expect(!QuestDefinitionDatabase::IsValidDefinition(unavailable),
                "ACTIVE is not a valid authored initial state");
    unavailable.initialState = QuestState::READY_TO_COMPLETE;
    test.Expect(!QuestDefinitionDatabase::IsValidDefinition(unavailable),
                "READY is not a valid authored initial state");
    unavailable.initialState = QuestState::COMPLETED;
    test.Expect(!QuestDefinitionDatabase::IsValidDefinition(unavailable),
                "COMPLETED is not a valid authored initial state");

    std::vector<QuestDefinition> duplicated{*definition, *definition};
    test.Expect(!QuestDefinitionDatabase::IsValidCatalogue(duplicated),
                "Duplicate quest IDs invalidate catalogue");
    QuestDefinition invalid = *definition;
    invalid.name = "";
    test.Expect(!QuestDefinitionDatabase::IsValidDefinition(invalid),
                "Empty quest name rejects");
    invalid = *definition; invalid.objectiveItem = ItemType::NONE;
    test.Expect(!QuestDefinitionDatabase::IsValidDefinition(invalid),
                "Unknown objective item rejects");
    QuestDefinition investigation = *definition;
    investigation.objectiveType = QuestObjectiveType::TALK_TO_CHARACTER;
    investigation.objectiveItem = ItemType::NONE;
    investigation.objectiveNpc = NpcType::SERA_FLINT;
    investigation.objectiveText = "Ask Sera about the tracks";
    investigation.requiredAmount = 1;
    test.Expect(QuestDefinitionDatabase::IsValidDefinition(investigation),
                "A specific-character investigation objective validates");
    investigation.objectiveNpc = NpcType::NONE;
    test.Expect(!QuestDefinitionDatabase::IsValidDefinition(investigation),
                "Investigation objectives require a real character");
    investigation.objectiveNpc = NpcType::SERA_FLINT;
    investigation.objectiveText = "";
    test.Expect(!QuestDefinitionDatabase::IsValidDefinition(investigation),
                "Investigation objectives require player-facing journal text");
    QuestDefinition crafting = *definition;
    crafting.objectiveType = QuestObjectiveType::CRAFT_ITEM;
    crafting.objectiveItem = ItemType::WOODEN_PLANKS;
    test.Expect(QuestDefinitionDatabase::IsValidDefinition(crafting),
                "Crafting objectives accept a known output item");
    crafting.objectiveItem = ItemType::NONE;
    test.Expect(!QuestDefinitionDatabase::IsValidDefinition(crafting),
                "Crafting objectives reject an unknown output item");
    QuestDefinition choice = *definition;
    choice.alternateCompletion = {
        NpcType::NESSA_THORNE, DialogueId::NESSA_THORNE_INTRO,
        DialogueNodeId::NESSA_THORNE_WELCOME, ItemType::COOKED_MEAT, 3};
    test.Expect(QuestDefinitionDatabase::IsValidDefinition(choice),
                "A complete alternate recipient and reward validates");
    choice.alternateCompletion.rewardItem = ItemType::NONE;
    test.Expect(!QuestDefinitionDatabase::IsValidDefinition(choice),
                "A partial alternate completion definition rejects");
    choice.alternateCompletion = {
        definition->giver.npcType, definition->giver.dialogueId,
        definition->giver.interactionNodeId, ItemType::COOKED_MEAT, 3};
    test.Expect(!QuestDefinitionDatabase::IsValidDefinition(choice),
                "Alternate completion must represent a real choice of recipient");
    QuestDefinition combat = *definition;
    combat.objectiveType = QuestObjectiveType::DEFEAT_CHARACTER;
    combat.objectiveItem = ItemType::NONE;
    combat.objectiveNpc = NpcType::AGGRESSIVE_DEVELOPMENT_MONSTER;
    combat.objectiveText = "Defeat the Ashfangs";
    test.Expect(QuestDefinitionDatabase::IsValidDefinition(combat),
                "Combat objectives accept a known monster type");
    combat.objectiveNpc = NpcType::SERA_FLINT;
    test.Expect(!QuestDefinitionDatabase::IsValidDefinition(combat),
                "Combat objectives reject friendly characters as targets");
    const QuestDefinition *conclusion =
        QuestDefinitionDatabase::TryGet(QuestId::HEART_OF_THE_ASH);
    test.Expect(conclusion != nullptr &&
                    conclusion->objectiveNpc ==
                        NpcType::ELITE_DEVELOPMENT_MONSTER &&
                    conclusion->requiredAmount == 1 &&
                    conclusion->giver.npcType == NpcType::DEVELOPMENT_GUIDE &&
                    conclusion->alternateCompletion.npcType ==
                        NpcType::BRANN_DEEPWELL &&
                    conclusion->alternateCompletion.rewardItem ==
                        ItemType::STEEL_BAR,
                "Starter conclusion combines the elite fight with a reward choice");
    invalid = *definition; invalid.requiredAmount = 0;
    test.Expect(!QuestDefinitionDatabase::IsValidDefinition(invalid),
                "Non-positive requirement rejects");
    invalid = *definition; invalid.rewardItem = ItemType::NONE;
    test.Expect(!QuestDefinitionDatabase::IsValidDefinition(invalid),
                "Unknown reward item rejects");
    invalid = *definition; invalid.rewardAmount = 0;
    test.Expect(!QuestDefinitionDatabase::IsValidDefinition(invalid),
                "Non-positive reward rejects");
    invalid = *definition; invalid.giver.npcType = NpcType::NONE;
    test.Expect(!ContentValidator::ValidateQuestDefinition(invalid).IsValid(),
                "Missing quest giver rejects content validation");
    invalid = *definition; invalid.giver.dialogueId = DialogueId::NONE;
    test.Expect(!ContentValidator::ValidateQuestDefinition(invalid).IsValid(),
                "Missing dialogue rejects content validation");
    invalid = *definition; invalid.giver.interactionNodeId = DialogueNodeId::NONE;
    test.Expect(!ContentValidator::ValidateQuestDefinition(invalid).IsValid(),
                "Missing dialogue node rejects content validation");
    invalid = *definition;
    invalid.giver.npcType = NpcType::PASSIVE_DEVELOPMENT_MONSTER;
    test.Expect(!ContentValidator::ValidateQuestDefinition(invalid).IsValid(),
                "Hostile quest giver rejects content validation");
    return test.Finish();
}
