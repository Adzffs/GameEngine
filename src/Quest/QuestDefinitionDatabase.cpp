#include "QuestDefinitionDatabase.h"

#include "../Item/ItemDatabase.h"

#include <set>

namespace
{
    const std::vector<QuestDefinition> Definitions{
        {QuestId::GATHERING_BASICS,
         "Gathering Basics",
         QuestState::AVAILABLE,
         ItemType::LOG,
         10,
         ItemType::COINS,
         10,
         {NpcType::DEVELOPMENT_GUIDE,
          DialogueId::DEVELOPMENT_GUIDE_INTRO,
          DialogueNodeId::DEVELOPMENT_GUIDE_WELCOME}}};

    bool IsKnownItem(ItemType itemType)
    {
        if (itemType == ItemType::NONE) return false;
        for (ItemType known : ItemDatabase::GetAllItemTypes())
            if (known == itemType) return true;
        return false;
    }
}

const std::vector<QuestDefinition> &QuestDefinitionDatabase::GetAll()
{
    return Definitions;
}

const QuestDefinition *QuestDefinitionDatabase::TryGet(QuestId id)
{
    for (const QuestDefinition &definition : Definitions)
        if (definition.id == id) return &definition;
    return nullptr;
}

bool QuestDefinitionDatabase::IsValidDefinition(
    const QuestDefinition &definition)
{
    const bool validInitialState =
        definition.initialState == QuestState::AVAILABLE ||
        definition.initialState == QuestState::UNAVAILABLE;
    return IsValidQuestId(definition.id) && definition.name != nullptr &&
        definition.name[0] != '\0' && validInitialState &&
        IsKnownItem(definition.objectiveItem) && definition.requiredAmount > 0 &&
        IsKnownItem(definition.rewardItem) && definition.rewardAmount > 0 &&
        definition.giver.npcType != NpcType::NONE &&
        definition.giver.dialogueId != DialogueId::NONE &&
        definition.giver.interactionNodeId != DialogueNodeId::NONE;
}

bool QuestDefinitionDatabase::IsValidCatalogue(
    const std::vector<QuestDefinition> &definitions)
{
    std::set<QuestId> ids;
    QuestId previous = QuestId::NONE;
    for (const QuestDefinition &definition : definitions)
    {
        if (!IsValidDefinition(definition) || !ids.insert(definition.id).second ||
            (previous != QuestId::NONE &&
             static_cast<int>(definition.id) <= static_cast<int>(previous)))
            return false;
        previous = definition.id;
    }
    return !definitions.empty();
}
