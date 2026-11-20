#include "TestSupport.h"
#include "../src/Gathering/WoodcuttingPresentationState.h"
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
        player.GetEquipment().Equip(EquipmentSlotType::WEAPON, ItemType::BRONZE_AXE);
        const ResourceNode *tree = nullptr;
        for (const auto &node : world.GetResources())
            if (node.GetResourceType() == ResourceType::NORMAL_TREE) { tree = &node; break; }
        test.Expect(tree != nullptr, "World has a real tree fixture");
        if (!tree) return;
        player.GetPosition().SetPosition(tree->GetX() - 1, tree->GetY());
        const int beforeXP = player.GetSkills().GetSkill(SkillType::WOODCUTTING).GetXP();
        const int beforeLogs = player.GetInventory().GetItemAmount(ItemType::LOG);
        world.QueueResourceInteraction(id, tree->GetID());
        world.Update();
        test.Expect(world.GetActionForEntity(id) != nullptr, "Production gathering action starts");
        test.Expect(world.GetGatheringRewardEvents().empty(), "Starting an action never reports a gathered item");
        WoodcuttingPresentationState presentation;
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
                test.ExpectEqual(event.amount, player.GetInventory().GetItemAmount(ItemType::LOG) - beforeLogs,
                    "Event amount is the actual inventory gain");
                test.ExpectEqual(event.xp, player.GetSkills().GetSkill(SkillType::WOODCUTTING).GetXP() - beforeXP,
                    "Event XP is the actual skill gain");
                test.ExpectEqual(event.resourceID, tree->GetID(), "Reward identifies its gathering target");
            }
            test.Expect(!tree->IsActive(), "Reward includes the final use that depletes a normal tree");
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
            test.Expect(presentation.GetRewards().empty(), "Failed rolls never show logs or XP");
            test.Expect(tree->IsActive(), "Failed attempt leaves the tree active");
            test.ExpectEqual(player.GetSkills().GetSkill(SkillType::WOODCUTTING).GetXP(), beforeXP,
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
    player.GetSkills().AddXP(SkillType::WOODCUTTING, 100000);
    player.GetPosition().SetPosition(5,5);
    const std::array<std::pair<int,int>,8> offsets{{{0,1},{-1,0},{1,0},{0,-1},{-1,1},{1,1},{-1,-1},{1,-1}}};
    for (int tier = 0; tier < 3; ++tier)
        for (int facing = 0; facing < 8; ++facing)
        {
            const ItemType tool = std::array{ItemType::BRONZE_AXE,ItemType::IRON_AXE,ItemType::STEEL_AXE}[tier];
            player.GetEquipment().Unequip(EquipmentSlotType::WEAPON);
            player.GetEquipment().Equip(EquipmentSlotType::WEAPON, tool);
            const auto [dx,dy] = offsets[facing];
            std::vector<ResourceNode> resources{ResourceNode(7,ResourceType::OAK_TREE,5+dx,5+dy)};
            Action action(ActionType::GATHERING,"Chop",4,1,7,true);
            WoodcuttingPresentationState state;
            state.Update(player,&action,resources,0.05f);
            test.Expect(!state.GetAnimation(), "Pending interactions never swing an axe");
            action.Start(1);
            state.Update(player,&action,resources,0.05f);
            test.Expect(state.GetAnimation().has_value(), "Valid active tree gathering animates");
            if (!state.GetAnimation()) continue;
            test.ExpectEqual(state.GetAnimation()->facing,facing,"Chopping faces its actual target in every direction");
            test.ExpectEqual(state.GetAnimation()->toolTier,tier,"Presentation uses the equipped axe tier");
            for (int frame = 0; frame < 7; ++frame) state.Update(player,&action,resources,0.1f);
            test.ExpectEqual(state.GetPose(),3,"Wind-up reaches the contact pose");
            const float elapsed = state.GetAnimation()->elapsedSeconds;
            action.Restart(5);
            state.Update(player,&action,resources,0.05f);
            test.Expect(state.GetAnimation()->elapsedSeconds > elapsed,"Repeat attempts keep the visual rhythm");
            state.Update(player,&action,resources,0.05f,false);
            test.Expect(!state.GetAnimation(),"Approaching the tree shows movement until visually settled");
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
    player.GetEquipment().Equip(EquipmentSlotType::WEAPON,ItemType::BRONZE_AXE);
    std::vector<ResourceNode> resources{ResourceNode(7,ResourceType::NORMAL_TREE,6,5)};
    Action action(ActionType::GATHERING,"Chop",5,1,7,true); action.Start(1);
    WoodcuttingPresentationState state;
    state.Update(player,&action,resources,0.05f);
    player.GetPosition().SetPosition(2,2);
    state.Update(player,&action,resources,0.05f);
    test.Expect(!state.GetAnimation(),"Moving away stops the axe");
    player.GetPosition().SetPosition(5,5);
    player.GetEquipment().Unequip(EquipmentSlotType::WEAPON);
    player.GetEquipment().Equip(EquipmentSlotType::WEAPON,ItemType::BRONZE_PICKAXE);
    state.Update(player,&action,resources,0.05f);
    test.Expect(!state.GetAnimation(),"Other tools never use chopping poses");
    player.GetEquipment().Unequip(EquipmentSlotType::WEAPON);
    player.GetEquipment().Equip(EquipmentSlotType::WEAPON,ItemType::BRONZE_AXE);
    resources[0] = ResourceNode(7,ResourceType::COPPER_ROCK,6,5);
    state.Update(player,&action,resources,0.05f);
    test.Expect(!state.GetAnimation(),"Mining is not mistaken for woodcutting");
    resources[0] = ResourceNode(7,ResourceType::NORMAL_TREE,6,5);
    Action other(ActionType::GATHERING,"Other player",5,2,7,true); other.Start(1);
    state.Update(player,&other,resources,0.05f);
    test.Expect(!state.GetAnimation(),"Other actors cannot animate the local player");
    player.ApplyDamage(player.GetMaximumHealth());
    state.Update(player,&action,resources,0.05f);
    test.Expect(!state.GetAnimation(),"Death immediately stops chopping");
    CheckWorldReward(test,true);
    CheckWorldReward(test,false);
    return test.Finish();
}
