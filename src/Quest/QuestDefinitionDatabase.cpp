#include "QuestDefinitionDatabase.h"

#include "../Item/ItemDatabase.h"
#include "../NPC/NpcDefinitionDatabase.h"

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
          DialogueNodeId::DEVELOPMENT_GUIDE_WELCOME}},
        {QuestId::MINING_BASICS,
         "Mining Basics",
         QuestState::AVAILABLE,
         ItemType::COPPER_ORE,
         5,
         ItemType::COINS,
         10,
         {NpcType::DEVELOPMENT_GUIDE,
          DialogueId::DEVELOPMENT_GUIDE_INTRO,
          DialogueNodeId::DEVELOPMENT_GUIDE_WELCOME}},
        {QuestId::TRACKS_IN_THE_ASH,
         "Tracks in the Ash",
         QuestState::AVAILABLE,
         ItemType::NONE,
         1,
         ItemType::COINS,
         15,
         {NpcType::DEVELOPMENT_GUIDE,
          DialogueId::DEVELOPMENT_GUIDE_INTRO,
          DialogueNodeId::DEVELOPMENT_GUIDE_WELCOME},
         QuestObjectiveType::TALK_TO_CHARACTER,
         NpcType::SERA_FLINT,
         "Find Sera Flint at the east road and ask about the Ashfang tracks"},
        {QuestId::SHIELD_OR_SHELTER,
         "Shield or Shelter",
         QuestState::AVAILABLE,
         ItemType::WOODEN_SHIELD,
         1,
         ItemType::COINS,
         25,
         {NpcType::TORREN_ASH,
          DialogueId::TORREN_ASH_INTRO,
          DialogueNodeId::TORREN_ASH_WELCOME},
         QuestObjectiveType::CRAFT_ITEM,
         NpcType::NONE,
         "Craft a wooden shield, then choose whether Torren or Nessa receives it",
         {NpcType::NESSA_THORNE,
          DialogueId::NESSA_THORNE_INTRO,
          DialogueNodeId::NESSA_THORNE_WELCOME,
          ItemType::COOKED_MEAT,
          3}}};

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
    const bool validObjective =
        (definition.objectiveType == QuestObjectiveType::GATHER_ITEM ||
         definition.objectiveType == QuestObjectiveType::CRAFT_ITEM)
            ? IsKnownItem(definition.objectiveItem)
            : definition.objectiveType == QuestObjectiveType::TALK_TO_CHARACTER &&
                definition.objectiveItem == ItemType::NONE &&
                NpcDefinitionDatabase::TryGet(definition.objectiveNpc) != nullptr &&
                definition.objectiveText != nullptr &&
                definition.objectiveText[0] != '\0';
    const bool noAlternateCompletion =
        definition.alternateCompletion.npcType == NpcType::NONE &&
        definition.alternateCompletion.dialogueId == DialogueId::NONE &&
        definition.alternateCompletion.interactionNodeId == DialogueNodeId::NONE &&
        definition.alternateCompletion.rewardItem == ItemType::NONE &&
        definition.alternateCompletion.rewardAmount == 0;
    const bool validAlternateCompletion =
        definition.alternateCompletion.npcType != NpcType::NONE &&
        definition.alternateCompletion.npcType != definition.giver.npcType &&
        NpcDefinitionDatabase::TryGet(
            definition.alternateCompletion.npcType) != nullptr &&
        definition.alternateCompletion.dialogueId != DialogueId::NONE &&
        definition.alternateCompletion.interactionNodeId != DialogueNodeId::NONE &&
        IsKnownItem(definition.alternateCompletion.rewardItem) &&
        definition.alternateCompletion.rewardAmount > 0;
    return IsValidQuestId(definition.id) && definition.name != nullptr &&
        definition.name[0] != '\0' && validInitialState &&
        IsValidQuestObjectiveType(definition.objectiveType) && validObjective &&
        definition.requiredAmount > 0 &&
        IsKnownItem(definition.rewardItem) && definition.rewardAmount > 0 &&
        definition.giver.npcType != NpcType::NONE &&
        definition.giver.dialogueId != DialogueId::NONE &&
        definition.giver.interactionNodeId != DialogueNodeId::NONE &&
        (noAlternateCompletion || validAlternateCompletion);
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
