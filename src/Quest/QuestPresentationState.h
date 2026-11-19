#pragma once

#include "QuestDefinitionDatabase.h"
#include "QuestJournal.h"
#include "../Item/ItemDatabase.h"
#include "../NPC/NpcDefinitionDatabase.h"
#include "../Recipe/RecipeDatabase.h"

#include <algorithm>
#include <string>
#include <utility>
#include <vector>

struct QuestPresentation
{
    QuestId id = QuestId::NONE;
    QuestState state = QuestState::UNAVAILABLE;
    int progress = 0;
    int required = 0;
    std::string title;
    std::string objective;
    QuestCategory category = QuestCategory::SIDE;
    std::string reward;
    int rewardAmount = 0;
    std::string guidance;
};

class QuestPresentationState
{
public:
    void Synchronize(const QuestJournal &journal)
    {
        std::vector<QuestPresentation> replacement;
        replacement.reserve(journal.GetRecords().size());
        for (const auto &[questId, record] : journal.GetRecords())
        {
            const QuestDefinition *definition =
                QuestDefinitionDatabase::TryGet(questId);
            if (definition == nullptr) continue;
            std::string objective;
            if (definition->objectiveText != nullptr && definition->objectiveText[0] != '\0')
                objective = definition->objectiveText;
            else if (definition->objectiveType == QuestObjectiveType::TALK_TO_CHARACTER ||
                definition->objectiveType == QuestObjectiveType::DEFEAT_CHARACTER)
                objective = definition->objectiveText == nullptr
                    ? "Speak with the character" : definition->objectiveText;
            else
            {
                objective = ItemDatabase::Get(definition->objectiveItem).GetName();
                if (definition->requiredAmount != 1) objective += "s";
            }
            const NpcDefinition *giver =
                NpcDefinitionDatabase::TryGet(definition->giver.npcType);
            const std::string giverName = giver == nullptr
                ? "the quest giver" : giver->name;
            std::string guidance;
            switch (record.state)
            {
            case QuestState::UNAVAILABLE:
                if (definition->prerequisiteQuest != QuestId::NONE)
                {
                    const QuestDefinition *prerequisite =
                        QuestDefinitionDatabase::TryGet(
                            definition->prerequisiteQuest);
                    guidance = "Requires: " + std::string(
                        prerequisite == nullptr ? "Earlier quest" :
                            prerequisite->name);
                }
                else
                    guidance = "Locked";
                break;
            case QuestState::AVAILABLE:
                guidance = "Start: Speak to " + giverName;
                break;
            case QuestState::ACTIVE:
                guidance = objective + ": " + std::to_string(record.progress) +
                    " / " + std::to_string(definition->requiredAmount);
                break;
            case QuestState::READY_TO_COMPLETE:
                if (definition->alternateCompletion.npcType != NpcType::NONE)
                {
                    const NpcDefinition *alternate =
                        NpcDefinitionDatabase::TryGet(
                            definition->alternateCompletion.npcType);
                    guidance = "Choose: " + giverName + " or " +
                        (alternate == nullptr ? "alternate recipient" :
                            alternate->name);
                }
                else
                    guidance = "Return to " + giverName;
                break;
            case QuestState::COMPLETED:
                guidance = "Completed";
                break;
            }
            replacement.push_back({questId, record.state, record.progress,
                definition->requiredAmount, definition->name,
                std::move(objective), definition->category,
                ItemDatabase::Get(definition->rewardItem).GetName(),
                definition->rewardAmount, std::move(guidance)});
        }
        std::stable_sort(replacement.begin(), replacement.end(),
            [](const QuestPresentation &left, const QuestPresentation &right)
            {
                const auto priority = [](QuestState state)
                {
                    switch (state)
                    {
                    case QuestState::READY_TO_COMPLETE: return 0;
                    case QuestState::ACTIVE: return 1;
                    case QuestState::AVAILABLE: return 2;
                    case QuestState::COMPLETED: return 3;
                    case QuestState::UNAVAILABLE: return 4;
                    }
                    return 4;
                };
                return priority(left.state) < priority(right.state);
            });
        quests = std::move(replacement);
    }

    const std::vector<QuestPresentation> &GetQuests() const
    {
        return quests;
    }

    const QuestPresentation *GetSuggestedQuest() const
    {
        // Finish committed work before offering another quest. Optional basic
        // tasks remain available without hiding the starter story's first lead.
        for (const auto &quest : quests)
            if (quest.state == QuestState::READY_TO_COMPLETE || quest.state == QuestState::ACTIVE)
                return &quest;
        for (const auto &quest : quests)
            if (quest.state == QuestState::AVAILABLE && quest.category != QuestCategory::TUTORIAL)
                return &quest;
        for (const auto &quest : quests)
            if (quest.state == QuestState::AVAILABLE) return &quest;
        return nullptr;
    }

    static std::string RewardText(ItemType item, int amount)
    {
        return std::to_string(amount) + (item == ItemType::COINS ? " " : " x ") +
            ItemDatabase::Get(item).GetName();
    }

    static std::string ProgressRule(QuestObjectiveType type)
    {
        switch (type)
        {
        case QuestObjectiveType::GATHER_ITEM:
            return "Gather after accepting; existing items do not add progress.";
        case QuestObjectiveType::CRAFT_ITEM:
            return "Craft after accepting. Crafted items are yours to use.";
        case QuestObjectiveType::TALK_TO_CHARACTER:
            return "Speak to the named character after accepting.";
        case QuestObjectiveType::DEFEAT_CHARACTER:
            return "Your kills after accepting count, including respawns.";
        }
        return {};
    }

    static std::vector<std::string> GetDetailLines(const QuestPresentation &quest)
    {
        const auto *definition = QuestDefinitionDatabase::TryGet(quest.id);
        if (definition == nullptr) return {quest.title, quest.guidance};
        const auto *giver = NpcDefinitionDatabase::TryGet(definition->giver.npcType);
        const std::string giverName = giver == nullptr ? "Quest giver" : giver->name;
        std::vector<std::string> lines{quest.title, quest.guidance,
            "Goal: " + quest.objective,
            "Progress: " + std::to_string(quest.progress) + " / " + std::to_string(quest.required),
            (quest.state == QuestState::COMPLETED ? "Contact: " : "Start / claim: ") + giverName,
            giverName + ": " + RewardText(definition->rewardItem, definition->rewardAmount)};
        if (definition->alternateCompletion.npcType != NpcType::NONE)
        {
            const auto *alternate = NpcDefinitionDatabase::TryGet(definition->alternateCompletion.npcType);
            lines.push_back("OR " + (alternate == nullptr ? std::string("Alternate contact") : alternate->name) +
                ": " + RewardText(definition->alternateCompletion.rewardItem, definition->alternateCompletion.rewardAmount));
            lines.push_back(quest.state == QuestState::COMPLETED ?
                "Reward options shown; this quest is already complete." :
                "Choose one reward; either completes this quest.");
        }
        lines.push_back(ProgressRule(definition->objectiveType));
        if (definition->objectiveType == QuestObjectiveType::CRAFT_ITEM)
            for (const auto type : RecipeDatabase::GetAllRecipeTypes())
            {
                const auto &recipe = RecipeDatabase::Get(type);
                if (recipe.GetOutputItem() != definition->objectiveItem) continue;
                std::string materials = "Materials: ";
                for (const auto &ingredient : recipe.GetIngredients())
                {
                    if (materials != "Materials: ") materials += " + ";
                    materials += RewardText(ingredient.itemType, ingredient.amount);
                }
                if (recipe.GetRequiredStationType() == StationType::CRAFTING_TABLE) materials += " (crafting table)";
                else if (recipe.GetRequiredStationType() == StationType::FURNACE) materials += " (furnace)";
                lines.push_back(std::move(materials));
                break;
            }
        if (quest.state != QuestState::UNAVAILABLE)
            if (const auto *prerequisite = QuestDefinitionDatabase::TryGet(definition->prerequisiteQuest))
                lines.push_back("Requires: " + std::string(prerequisite->name));
        return lines;
    }

private:
    std::vector<QuestPresentation> quests;
};
