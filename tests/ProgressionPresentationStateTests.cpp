#include "TestSupport.h"

#include "../src/Progression/ProgressionPresentationState.h"

int main()
{
    TestContext test;
    SkillSet skills;
    Inventory inventory;
    ProgressionPresentationState state;

    test.Expect(state.Synchronize(skills, inventory).empty(),
        "Initial synchronization does not replay old progress");

    skills.AddXP(SkillType::WOODCUTTING, 20);
    inventory.AddItem(ItemType::LOG, 2);
    const auto rewards = state.Synchronize(skills, inventory);
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
    const auto levelRewards = state.Synchronize(skills, inventory);
    test.ExpectEqual(static_cast<int>(levelRewards.size()), 2,
        "A level gain also preserves its XP feedback");
    test.Expect(levelRewards[0].type == ProgressionEventType::XP_GAINED,
        "Level gain includes its XP event");
    test.Expect(levelRewards[1].type == ProgressionEventType::LEVEL_GAINED,
        "Crossing a threshold emits a level-up celebration");
    test.ExpectEqual(levelRewards[1].level, 2,
        "Level-up feedback reports the new level");

    inventory.RemoveItem(ItemType::LOG, 1);
    test.Expect(state.Synchronize(skills, inventory).empty(),
        "Spending or dropping items does not look like a reward");

    return test.Finish();
}
