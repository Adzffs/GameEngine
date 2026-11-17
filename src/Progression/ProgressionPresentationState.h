#pragma once

#include "../Inventory/Inventory.h"
#include "../Skills/SkillSet.h"
#include "../Quest/QuestJournal.h"

#include <array>
#include <map>
#include <string>
#include <vector>

enum class ProgressionEventType
{
    XP_GAINED,
    LEVEL_GAINED,
    ITEM_ACQUIRED,
    QUEST_COMPLETED,
    QUEST_UNLOCKED
};

struct ProgressionEvent
{
    ProgressionEventType type = ProgressionEventType::XP_GAINED;
    SkillType skillType = SkillType::NONE;
    ItemType itemType = ItemType::NONE;
    int amount = 0;
    int level = 0;
    QuestId questId = QuestId::NONE;
};

class ProgressionPresentationState
{
public:
    std::vector<ProgressionEvent> Synchronize(
        const SkillSet &skills,
        const Inventory &inventory,
        const QuestJournal &quests)
    {
        const std::array<SkillType, 5> skillTypes{
            SkillType::ATTACK,
            SkillType::DEFENCE,
            SkillType::WOODCUTTING,
            SkillType::MINING,
            SkillType::SMITHING};

        std::map<ItemType, int> currentItems;
        for (const InventorySlot &slot : inventory.GetSlots())
        {
            if (!slot.IsEmpty())
            {
                currentItems[slot.GetItemType()] += slot.GetAmount();
            }
        }

        if (!initialized)
        {
            for (std::size_t index = 0; index < skillTypes.size(); ++index)
            {
                const Skill &skill = skills.GetSkill(skillTypes[index]);
                previousXP[index] = skill.GetXP();
                previousLevels[index] = skill.GetLevel();
            }
            previousItems = std::move(currentItems);
            for (const auto &[questId, record] : quests.GetRecords())
                previousQuestStates[questId] = record.state;
            initialized = true;
            return {};
        }

        std::vector<ProgressionEvent> events;
        for (std::size_t index = 0; index < skillTypes.size(); ++index)
        {
            const Skill &skill = skills.GetSkill(skillTypes[index]);
            const int xpGained = skill.GetXP() - previousXP[index];
            if (xpGained > 0)
            {
                events.push_back({ProgressionEventType::XP_GAINED,
                    skillTypes[index], ItemType::NONE, xpGained, 0});
            }
            if (skill.GetLevel() > previousLevels[index])
            {
                events.push_back({ProgressionEventType::LEVEL_GAINED,
                    skillTypes[index], ItemType::NONE, 0, skill.GetLevel()});
            }
            previousXP[index] = skill.GetXP();
            previousLevels[index] = skill.GetLevel();
        }

        for (const auto &[itemType, amount] : currentItems)
        {
            const int previousAmount = previousItems.contains(itemType)
                ? previousItems.at(itemType)
                : 0;
            if (amount > previousAmount)
            {
                events.push_back({ProgressionEventType::ITEM_ACQUIRED,
                    SkillType::NONE, itemType, amount - previousAmount, 0});
            }
        }
        previousItems = std::move(currentItems);
        for (const auto &[questId, record] : quests.GetRecords())
        {
            const QuestState previous = previousQuestStates.contains(questId)
                ? previousQuestStates.at(questId) : QuestState::UNAVAILABLE;
            if (record.state == QuestState::COMPLETED &&
                previous != QuestState::COMPLETED)
                events.push_back({ProgressionEventType::QUEST_COMPLETED,
                    SkillType::NONE, ItemType::NONE, 0, 0, questId});
            else if (record.state == QuestState::AVAILABLE &&
                previous == QuestState::UNAVAILABLE)
                events.push_back({ProgressionEventType::QUEST_UNLOCKED,
                    SkillType::NONE, ItemType::NONE, 0, 0, questId});
            previousQuestStates[questId] = record.state;
        }
        return events;
    }

private:
    bool initialized = false;
    std::array<int, 5> previousXP{};
    std::array<int, 5> previousLevels{};
    std::map<ItemType, int> previousItems;
    std::map<QuestId, QuestState> previousQuestStates;
};
