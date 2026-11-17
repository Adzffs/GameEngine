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
          DialogueNodeId::DEVELOPMENT_GUIDE_WELCOME},
         QuestObjectiveType::GATHER_ITEM, NpcType::NONE, nullptr, {},
         QuestCategory::TUTORIAL},
        {QuestId::MINING_BASICS,
         "Mining Basics",
         QuestState::AVAILABLE,
         ItemType::COPPER_ORE,
         5,
         ItemType::COINS,
         10,
         {NpcType::DEVELOPMENT_GUIDE,
          DialogueId::DEVELOPMENT_GUIDE_INTRO,
          DialogueNodeId::DEVELOPMENT_GUIDE_WELCOME},
         QuestObjectiveType::GATHER_ITEM, NpcType::NONE, nullptr, {},
         QuestCategory::TUTORIAL},
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
         "Find Sera Flint at the east road and ask about the Ashfang tracks",
         {}, QuestCategory::SIDE},
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
          3}, QuestCategory::SIDE},
        {QuestId::FANGS_AT_THE_GATE,
         "Fangs at the Gate",
         QuestState::AVAILABLE,
         ItemType::NONE,
         3,
         ItemType::COOKED_MEAT,
         5,
         {NpcType::SERA_FLINT,
          DialogueId::SERA_FLINT_INTRO,
          DialogueNodeId::SERA_FLINT_WELCOME},
         QuestObjectiveType::DEFEAT_CHARACTER,
         NpcType::AGGRESSIVE_DEVELOPMENT_MONSTER,
         "Defeat three Ashfangs threatening the east road",
         {}, QuestCategory::MAIN},
        {QuestId::HEART_OF_THE_ASH,
         "Heart of the Ash",
         QuestState::AVAILABLE,
         ItemType::NONE,
         1,
         ItemType::COINS,
         75,
         {NpcType::DEVELOPMENT_GUIDE,
          DialogueId::DEVELOPMENT_GUIDE_INTRO,
          DialogueNodeId::DEVELOPMENT_GUIDE_WELCOME},
         QuestObjectiveType::DEFEAT_CHARACTER,
         NpcType::ELITE_DEVELOPMENT_MONSTER,
         "Defeat the Ashfang alpha beyond the east road, then choose who receives its trophy",
         {NpcType::BRANN_DEEPWELL,
          DialogueId::BRANN_DEEPWELL_INTRO,
          DialogueNodeId::BRANN_DEEPWELL_WELCOME,
          ItemType::STEEL_BAR,
          2}, QuestCategory::MAIN}};

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
            : (definition.objectiveType == QuestObjectiveType::TALK_TO_CHARACTER ||
               definition.objectiveType == QuestObjectiveType::DEFEAT_CHARACTER) &&
                definition.objectiveItem == ItemType::NONE &&
                NpcDefinitionDatabase::TryGet(definition.objectiveNpc) != nullptr &&
                (definition.objectiveType != QuestObjectiveType::DEFEAT_CHARACTER ||
                 NpcDefinitionDatabase::TryGet(definition.objectiveNpc)->kind ==
                    NpcKind::MONSTER) &&
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
        IsValidQuestCategory(definition.category) &&
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
