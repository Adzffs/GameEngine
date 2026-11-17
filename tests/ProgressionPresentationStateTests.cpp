#include "TestSupport.h"

#include "../src/Progression/ProgressionPresentationState.h"
#include "../src/Quest/QuestSystem.h"

int main()
{
    TestContext test;
    SkillSet skills;
    Inventory inventory;
    ProgressionPresentationState state;
    QuestJournal quests;

    test.Expect(state.Synchronize(skills, inventory, quests).empty(),
        "Initial synchronization does not replay old progress");

    skills.AddXP(SkillType::WOODCUTTING, 20);
    inventory.AddItem(ItemType::LOG, 2);
    const auto rewards = state.Synchronize(skills, inventory, quests);
    test.ExpectEqual(static_cast<int>(rewards.size()), 2,
        "XP and acquired item are both presented");
    test.Expect(rewards[0].type == ProgressionEventType::XP_GAINED,
        "XP feedback is emitted first");
    test.ExpectEqual(rewards[0].amount, 20,
        "XP feedback reports the exact gain");
    test.Expect(rewards[1].type == ProgressionEventType::ITEM_ACQUIRED,
        "Item feedback is emitted for a positive inventory change");
    test.ExpectEqual(rewards[1].amount, 2,
        "Item feedback reports the exact quantity gained");

    const int xpToLevel =
        skills.GetSkill(SkillType::ATTACK).GetXPForNextLevel();
    skills.AddXP(SkillType::ATTACK, xpToLevel);
    const auto levelRewards = state.Synchronize(skills, inventory, quests);
    test.ExpectEqual(static_cast<int>(levelRewards.size()), 2,
        "A level gain also preserves its XP feedback");
    test.Expect(levelRewards[0].type == ProgressionEventType::XP_GAINED,
        "Level gain includes its XP event");
    test.Expect(levelRewards[1].type == ProgressionEventType::LEVEL_GAINED,
        "Crossing a threshold emits a level-up celebration");
    test.ExpectEqual(levelRewards[1].level, 2,
        "Level-up feedback reports the new level");

    inventory.RemoveItem(ItemType::LOG, 1);
    test.Expect(state.Synchronize(skills, inventory, quests).empty(),
        "Spending or dropping items does not look like a reward");

    test.Expect(QuestSystem::TryRestore(quests, {
                    {QuestId::GATHERING_BASICS, QuestState::AVAILABLE, 0},
                    {QuestId::MINING_BASICS, QuestState::AVAILABLE, 0},
                    {QuestId::TRACKS_IN_THE_ASH, QuestState::COMPLETED, 1},
                    {QuestId::SHIELD_OR_SHELTER, QuestState::AVAILABLE, 0},
                    {QuestId::FANGS_AT_THE_GATE, QuestState::UNAVAILABLE, 0},
                    {QuestId::HEART_OF_THE_ASH, QuestState::UNAVAILABLE, 0}}),
                "Quest feedback fixture restores");
    const auto questEvents = state.Synchronize(skills, inventory, quests);
    test.Expect(questEvents.size() == 2 &&
                    questEvents[0].type ==
                        ProgressionEventType::QUEST_COMPLETED &&
                    questEvents[0].questId == QuestId::TRACKS_IN_THE_ASH &&
                    questEvents[1].type ==
                        ProgressionEventType::QUEST_UNLOCKED &&
                    questEvents[1].questId == QuestId::SHIELD_OR_SHELTER,
                "Quest completion and newly unlocked chapter are both announced");

    return test.Finish();
}
