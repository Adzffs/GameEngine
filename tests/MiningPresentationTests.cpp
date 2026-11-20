#include "TestSupport.h"
#include "../src/Gathering/GatheringPresentationState.h"
#include "../src/Action/Action.h"
#include "../src/Player/Player.h"
#include "../src/World/Object/Resource/ResourceNode.h"
#include "../src/World/World.h"
#include "../src/Core/SeededRandom.h"
#include <algorithm>
#include <array>
#include <memory>

namespace
{
    class FixedRandom : public RandomSource
    {
    public:
        explicit FixedRandom(int value) : value(value) {}
        int NextIntInclusive(int minimum, int maximum) override { return std::clamp(value, minimum, maximum); }
    private:
        int value;
    };

    void CheckWorldReward(TestContext &test, bool success)
    {
        World world(std::make_unique<SeededRandom>(1), std::make_unique<SeededRandom>(2),
            std::make_unique<FixedRandom>(success ? 1 : 100));
        const int id = world.CreatePlayer(PlayerInitializationMode::FRESH_START);
        auto &player = *dynamic_cast<Player *>(world.GetEntityByID(id));
        player.GetEquipment().Equip(EquipmentSlotType::WEAPON, ItemType::BRONZE_PICKAXE);
        const ResourceNode *rock = nullptr;
        for (const auto &node : world.GetResources())
            if (node.GetResourceType() == ResourceType::COPPER_ROCK) { rock = &node; break; }
        test.Expect(rock != nullptr, "World has a real rock fixture");
        if (!rock) return;
        player.GetPosition().SetPosition(rock->GetX() - 1, rock->GetY());
        const int beforeXP = player.GetSkills().GetSkill(SkillType::MINING).GetXP();
        const int beforeOre = player.GetInventory().GetItemAmount(ItemType::COPPER_ORE);
        world.QueueResourceInteraction(id, rock->GetID());
        world.Update();
        test.Expect(world.GetActionForEntity(id) != nullptr, "Production gathering action starts");
        test.Expect(world.GetGatheringRewardEvents().empty(), "Starting an action never reports a gathered item");
        GatheringPresentationState presentation{SkillType::MINING};
        for (int tick = 0; tick < 5; ++tick)
        {
            world.Update();
            presentation.SynchronizeRewards(id, world.GetGatheringRewardEvents());
        }
        if (success)
        {
            test.ExpectEqual(world.GetGatheringRewardEvents().size(), std::size_t{1}, "Committed gather publishes one reward");
            if (!world.GetGatheringRewardEvents().empty())
            {
                const auto &event = world.GetGatheringRewardEvents().front();
                test.ExpectEqual(event.amount, player.GetInventory().GetItemAmount(ItemType::COPPER_ORE) - beforeOre,
                    "Event amount is the actual inventory gain");
                test.ExpectEqual(event.xp, player.GetSkills().GetSkill(SkillType::MINING).GetXP() - beforeXP,
                    "Event XP is the actual skill gain");
                test.ExpectEqual(event.resourceID, rock->GetID(), "Reward identifies its gathering target");
            }
            test.Expect(!rock->IsActive(), "Reward includes the final use that depletes a normal rock");
            test.Expect(world.GetActionForEntity(id) == nullptr, "Depletion stops the production action");
            presentation.SynchronizeRewards(id, world.GetGatheringRewardEvents());
            test.ExpectEqual(presentation.GetRewards().size(), std::size_t{1}, "Repeated reads do not duplicate reward notices");
            world.Update(); presentation.SynchronizeRewards(id, world.GetGatheringRewardEvents());
            world.Update(); presentation.SynchronizeRewards(id, world.GetGatheringRewardEvents());
            test.ExpectEqual(presentation.GetRewards().size(), std::size_t{1}, "Catch-up updates retain success feedback before a render");
            test.Expect(world.GetGatheringRewardEvents().empty(), "World reward publication expires after its tick");
        }
        else
        {
            test.Expect(world.GetGatheringRewardEvents().empty(), "Failed rolls never publish success feedback");
            test.Expect(presentation.GetRewards().empty(), "Failed rolls never show ore or XP");
            test.Expect(rock->IsActive(), "Failed attempt leaves the rock active");
            test.ExpectEqual(player.GetSkills().GetSkill(SkillType::MINING).GetXP(), beforeXP,
                "Animation cannot grant XP after a failed roll");
            world.CancelActionsForEntity(id, ActionCancelReason::NONE);
            world.Update();
            presentation.Update(player, world.GetActionForEntity(id), world.GetResources(), 0.05f);
            test.Expect(!presentation.GetAnimation(), "Production cancellation stops presentation");
        }
    }
}

int main()
{
    TestContext test;
    Player player(1, PlayerInitializationMode::FRESH_START);
    player.GetSkills().AddXP(SkillType::MINING, 100000);
    player.GetPosition().SetPosition(5,5);
    const std::array<std::pair<int,int>,8> offsets{{{0,1},{-1,0},{1,0},{0,-1},{-1,1},{1,1},{-1,-1},{1,-1}}};
    for (int tier = 0; tier < 3; ++tier)
        for (int facing = 0; facing < 8; ++facing)
        {
            const ItemType tool = std::array{ItemType::BRONZE_PICKAXE,ItemType::IRON_PICKAXE,ItemType::STEEL_PICKAXE}[tier];
            player.GetEquipment().Unequip(EquipmentSlotType::WEAPON);
            player.GetEquipment().Equip(EquipmentSlotType::WEAPON, tool);
            const auto [dx,dy] = offsets[facing];
            std::vector<ResourceNode> resources{ResourceNode(7,ResourceType::IRON_ROCK,5+dx,5+dy)};
            Action action(ActionType::GATHERING,"Mine",4,1,7,true);
            GatheringPresentationState state{SkillType::MINING};
            state.Update(player,&action,resources,0.05f);
            test.Expect(!state.GetAnimation(), "Pending interactions never swing a pickaxe");
            action.Start(1);
            state.Update(player,&action,resources,0.05f);
            test.Expect(state.GetAnimation().has_value(), "Valid active rock gathering animates");
            if (!state.GetAnimation()) continue;
            test.ExpectEqual(state.GetAnimation()->facing,facing,"Mineping faces its actual target in every direction");
            test.ExpectEqual(state.GetAnimation()->toolTier,tier,"Presentation uses the equipped axe tier");
            for (int frame = 0; frame < 7; ++frame) state.Update(player,&action,resources,0.1f);
            test.ExpectEqual(state.GetPose(),3,"Wind-up reaches the contact pose");
            const float elapsed = state.GetAnimation()->elapsedSeconds;
            action.Restart(5);
            state.Update(player,&action,resources,0.05f);
            test.Expect(state.GetAnimation()->elapsedSeconds > elapsed,"Repeat attempts keep the visual rhythm");
            state.Update(player,&action,resources,0.05f,false);
            test.Expect(!state.GetAnimation(),"Approaching the rock shows movement until visually settled");
            state.Update(player,&action,resources,0.05f);
            action.Cancel(ActionCancelReason::NONE);
            state.Update(player,&action,resources,0.05f);
            test.Expect(!state.GetAnimation(),"Cancelling immediately stops the swing");
            action.Start(9);
            state.Update(player,&action,resources,0.05f);
            while (resources[0].IsActive()) resources[0].ConsumeUse();
            state.Update(player,&action,resources,0.05f);
            test.Expect(!state.GetAnimation(),"Depleted targets stop even if a stale action is presented");
        }
    player.GetEquipment().Unequip(EquipmentSlotType::WEAPON);
    player.GetEquipment().Equip(EquipmentSlotType::WEAPON,ItemType::BRONZE_PICKAXE);
    std::vector<ResourceNode> resources{ResourceNode(7,ResourceType::COPPER_ROCK,6,5)};
    Action action(ActionType::GATHERING,"Mine",5,1,7,true); action.Start(1);
    GatheringPresentationState state{SkillType::MINING};
    state.Update(player,&action,resources,0.05f);
    player.GetPosition().SetPosition(2,2);
    state.Update(player,&action,resources,0.05f);
    test.Expect(!state.GetAnimation(),"Moving away stops the pickaxe");
    player.GetPosition().SetPosition(5,5);
    player.GetEquipment().Unequip(EquipmentSlotType::WEAPON);
    player.GetEquipment().Equip(EquipmentSlotType::WEAPON,ItemType::BRONZE_AXE);
    state.Update(player,&action,resources,0.05f);
    test.Expect(!state.GetAnimation(),"Other tools never use mining poses");
    player.GetEquipment().Unequip(EquipmentSlotType::WEAPON);
    player.GetEquipment().Equip(EquipmentSlotType::WEAPON,ItemType::BRONZE_PICKAXE);
    resources[0] = ResourceNode(7,ResourceType::NORMAL_TREE,6,5);
    state.Update(player,&action,resources,0.05f);
    test.Expect(!state.GetAnimation(),"Woodcutting is not mistaken for mining");
    resources[0] = ResourceNode(7,ResourceType::COPPER_ROCK,6,5);
    Action other(ActionType::GATHERING,"Other player",5,2,7,true); other.Start(1);
    state.Update(player,&other,resources,0.05f);
    test.Expect(!state.GetAnimation(),"Other actors cannot animate the local player");
    for (const auto type : {ResourceType::COPPER_ROCK,ResourceType::TIN_ROCK,ResourceType::IRON_ROCK,ResourceType::COAL_ROCK})
    {
        resources[0] = ResourceNode(7,type,6,5);
        state.Update(player,&action,resources,0.05f);
        test.Expect(state.GetAnimation().has_value(),"Every existing ore tier uses the mining animation");
    }
    player.ApplyDamage(player.GetMaximumHealth());
    state.Update(player,&action,resources,0.05f);
    test.Expect(!state.GetAnimation(),"Death immediately stops mining");
    GatheringPresentationState wood{SkillType::WOODCUTTING}, mining{SkillType::MINING};
    const std::vector<GatheringRewardEvent> woodEvent{{1,7,20,ResourceType::NORMAL_TREE,6,5,5,5,ItemType::LOG,1,25}};
    wood.SynchronizeRewards(1,woodEvent); mining.SynchronizeRewards(1,woodEvent);
    test.Expect(mining.GetRewards().empty(),"Tree rewards cannot appear as mining XP");
    test.ExpectEqual(wood.GetRewards().size(),std::size_t{1},"Shared presentation preserves woodcutting rewards");
    const std::vector<GatheringRewardEvent> oreEvent{{1,8,21,ResourceType::TIN_ROCK,6,5,5,5,ItemType::TIN_ORE,1,18}};
    wood.SynchronizeRewards(1,oreEvent); mining.SynchronizeRewards(1,oreEvent);
    test.ExpectEqual(mining.GetRewards().size(),std::size_t{1},"Ore rewards route only to mining feedback");
    test.ExpectEqual(wood.GetRewards().size(),std::size_t{1},"Mining does not add a woodcutting reward notice");
    Player novice(3,PlayerInitializationMode::FRESH_START);
    novice.GetPosition().SetPosition(5,5);
    novice.GetEquipment().Equip(EquipmentSlotType::WEAPON,ItemType::STEEL_PICKAXE);
    Action noviceAction(ActionType::GATHERING,"Mine",3,3,7,true); noviceAction.Start(1);
    resources[0]=ResourceNode(7,ResourceType::COPPER_ROCK,6,5);
    state.Update(novice,&noviceAction,resources,0.05f);
    test.Expect(!state.GetAnimation(),"A tool above the player's level cannot animate a gather");
    novice.GetEquipment().Unequip(EquipmentSlotType::WEAPON);
    novice.GetEquipment().Equip(EquipmentSlotType::WEAPON,ItemType::BRONZE_PICKAXE);
    resources[0]=ResourceNode(7,ResourceType::IRON_ROCK,6,5);
    state.Update(novice,&noviceAction,resources,0.05f);
    test.Expect(!state.GetAnimation(),"Ore above the player's mining level cannot animate a gather");
    state.Update(novice,&noviceAction,{},0.05f);
    test.Expect(!state.GetAnimation(),"Missing rocks immediately stop the pickaxe");
    CheckWorldReward(test,true);
    CheckWorldReward(test,false);
    return test.Finish();
}
