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
    test.Expect(definitions.size() == 1 &&
                    definitions.front().id == QuestId::GATHERING_BASICS,
                "Canonical catalogue contains Gathering Basics in stable order");
    const QuestDefinition *definition =
        QuestDefinitionDatabase::TryGet(QuestId::GATHERING_BASICS);
    test.Expect(definition != nullptr &&
                    definition->initialState == QuestState::AVAILABLE &&
                    definition->objectiveItem == ItemType::LOG &&
                    definition->requiredAmount == 10 &&
                    definition->rewardItem == ItemType::COINS &&
                    definition->rewardAmount == 10,
                "Gathering Basics definition remains exact");
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
