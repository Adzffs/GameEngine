#include "TestSupport.h"
#include "../src/Combat/MeleePresentationState.h"
#include "../src/Action/Action.h"
#include "../src/Player/Player.h"
#include "../src/Entity/Monster/Monster.h"
#include "../src/World/World.h"
#include "../src/Core/RandomSource.h"
#include <array>
#include <limits>

namespace
{
    class MaximumRandom : public RandomSource
    {
    public:
        int NextIntInclusive(int, int maximum) override { return maximum; }
    };
    MeleeCombatFeedback Result(const Player &player, int targetID, int tick, int dx = 1, int dy = 0)
    {
        MeleeCombatFeedback feedback{player.GetID(), targetID, true, 2, 3};
        feedback.completionTick = tick;
        feedback.attackerX = player.GetPosition().GetX();
        feedback.attackerY = player.GetPosition().GetY();
        feedback.defenderX = feedback.attackerX + dx;
        feedback.defenderY = feedback.attackerY + dy;
        feedback.weapon = player.GetEquipment().GetEquippedItem(EquipmentSlotType::WEAPON);
        return feedback;
    }
}

int main()
{
    TestContext test;
    constexpr std::array<std::pair<int,int>,8> directions{{{0,1},{-1,0},{1,0},{0,-1},{-1,1},{1,1},{-1,-1},{1,-1}}};
    for (int facing = 0; facing < 8; ++facing)
    {
        Player player(1, PlayerInitializationMode::FRESH_START);
        player.GetPosition().SetPosition(5,5);
        player.GetEquipment().Equip(EquipmentSlotType::WEAPON, ItemType::BRONZE_SWORD);
        const auto [dx,dy] = directions[facing];
        std::vector<std::unique_ptr<Entity>> entities;
        entities.push_back(std::make_unique<Monster>(2, 5+dx, 5+dy, CombatRatings{10,10,10,100}));
        Action action(ActionType::MELEE_ATTACK,"Melee",2,1,2,true);
        action.Start(100);
        MeleePresentationState state;
        state.Update(player,&action,entities,{},0.0f);
        test.Expect(!state.ConsumeSwing(), "A new guard does not sound a premature swing");
        test.Expect(state.GetAnimation() && state.GetAnimation()->facing == facing,
            "Wind-up faces its adjacent target in all eight directions");
        for (int frame = 0; frame < 30; ++frame) state.Update(player,&action,entities,{},0.1f);
        test.Expect(state.GetPose() < 3,"A paused world never produces an invented contact");
        action.Update(101);
        state.Update(player,&action,entities,{},0.1f);
        int swingCount = 0;
        for (int frame = 0; frame < 20; ++frame)
        {
            state.Update(player,&action,entities,{},0.1f);
            if (state.ConsumeSwing() == ItemType::BRONZE_SWORD) ++swingCount;
        }
        test.ExpectEqual(swingCount, 1, "The visible wind-up produces one sword sound despite a paused completion");
        test.ExpectEqual(state.GetPose(),2,"Final wind-up holds until an authoritative result arrives");

        std::map<int,MeleeCombatFeedback> feedbacks{{2,Result(player,2,102,dx,dy)}};
        feedbacks.at(2).hit = facing % 2 == 0;
        feedbacks.at(2).actualDamageApplied = 0;
        // No target entity is needed after contact, including a killing blow.
        state.Update(player,nullptr,{},feedbacks,0.1f);
        test.Expect(state.GetAnimation() && state.GetAnimation()->resolved && state.GetAnimation()->facing == facing,
            "Committed hits, misses and blocks animate from stored result geometry");
        test.ExpectEqual(state.GetPose(),3,"Contact starts once for a new committed attack");
        test.Expect(!state.ConsumeSwing(), "Contact cannot double a sound already played by its wind-up");
        state.Update(player,nullptr,{},feedbacks,0.1f);
        test.ExpectEqual(state.GetPose(),4,"The same result advances through follow-through instead of restarting");
        state.Update(player,nullptr,{},feedbacks,0.1f);
        test.ExpectEqual(state.GetPose(),6,"The strike returns through the authored recovery poses");
        state.Update(player,nullptr,{},feedbacks,0.1f);
        test.Expect(!state.GetAnimation(),"Recovery ends while old hit text remains visible");
        feedbacks.at(2).completionTick = 104;
        state.Update(player,nullptr,{},feedbacks,0.0f);
        test.ExpectEqual(state.GetPose(),3,"A second identical result starts a new swing");
        const float elapsed = state.GetAnimation()->elapsedSeconds;
        state.Update(player,nullptr,{},feedbacks,std::numeric_limits<float>::quiet_NaN());
        test.ExpectEqual(state.GetAnimation()->elapsedSeconds,elapsed,"Invalid frame delta cannot corrupt animation time");
        player.GetPosition().SetPosition(6,5);
        state.Update(player,nullptr,{},feedbacks,0.1f);
        test.Expect(!state.GetAnimation(),"Movement interrupts recovery immediately");

        player.GetPosition().SetPosition(5,5);
        state.Update(player,&action,entities,{},0.0f);
        state.Update(player,nullptr,entities,{},0.0f);
        test.Expect(!state.GetAnimation(),"Cancelled wind-up returns to the equipped idle pose");
        state.Update(player,&action,entities,{},0.0f,false);
        test.Expect(!state.GetAnimation(),"Approach movement does not play a planted attack");
        entities[0]->GetPosition().SetPosition(9,9);
        state.Update(player,&action,entities,{},0.0f);
        test.Expect(!state.GetAnimation(),"Out-of-range targets do not leave a ghost wind-up");
        feedbacks.at(2) = Result(player,2,105,dx,dy);
        state.Update(player,nullptr,{},feedbacks,0.0f);
        player.GetEquipment().Unequip(EquipmentSlotType::WEAPON);
        player.GetEquipment().Equip(EquipmentSlotType::WEAPON,ItemType::DEVELOPER_GODSWORD);
        state.Update(player,nullptr,{},feedbacks,0.1f);
        test.Expect(!state.GetAnimation(),"Equipment changes clear an old weapon swing");
        feedbacks.at(2) = Result(player,2,106,dx,dy);
        state.Update(player,nullptr,{},feedbacks,0.0f);
        test.Expect(state.GetAnimation() && state.GetAnimation()->weapon == ItemType::DEVELOPER_GODSWORD,
            "Two-handed contact uses the weapon from the committed result");
        Action gathering(ActionType::GATHERING,"Gather",3,1,7,true); gathering.Start(107);
        state.Update(player,&gathering,{},feedbacks,0.0f);
        test.Expect(!state.GetAnimation(),"Gathering replaces melee recovery without overlaying both weapons");
        player.ApplyDamage(player.GetMaximumHealth());
        feedbacks.at(2).completionTick = 108;
        state.Update(player,nullptr,{},feedbacks,0.0f);
        test.Expect(!state.GetAnimation(),"Dead players do not swing or replay a pending result");
    }
    {
        World world;
        const int actorID = world.CreatePlayer(PlayerInitializationMode::FRESH_START);
        const int targetID = world.CreateMonster(6,2,CombatRatings{10,10,10,1000});
        auto *player = dynamic_cast<Player *>(world.GetEntityByID(actorID));
        player->GetPosition().SetPosition(5,2);
        test.Expect(world.TryStartMeleeEngagement(actorID,targetID,2,true),"Instant-opening engagement starts");
        world.Update();
        const auto first = world.GetMeleeCombatFeedbacks().find(targetID);
        test.Expect(first != world.GetMeleeCombatFeedbacks().end(),"Instant opening publishes a real result");
        if (first != world.GetMeleeCombatFeedbacks().end())
        {
            const int firstTick = first->second.completionTick;
            test.Expect(firstTick >= 0 && first->second.attackerX == 5 && first->second.defenderX == 6,
                "Production feedback records completion identity and attack geometry");
            test.Expect(first->second.weapon == ItemType::NONE,"Unarmed production result keeps empty hands");
            world.Update(); world.Update();
            const auto repeated = world.GetMeleeCombatFeedbacks().find(targetID);
            test.Expect(repeated != world.GetMeleeCombatFeedbacks().end() && repeated->second.completionTick > firstTick,
                "Repeated production attacks publish distinct completion identities");
        }
    }
    {
        World world(std::make_unique<MaximumRandom>());
        const int actorID = world.CreatePlayer(PlayerInitializationMode::FRESH_START);
        const int targetID = world.CreateMonster(6,2,CombatRatings{0,0,0,1});
        auto *player = dynamic_cast<Player *>(world.GetEntityByID(actorID));
        player->GetPosition().SetPosition(5,2);
        player->GetEquipment().Equip(EquipmentSlotType::WEAPON,ItemType::DEVELOPER_GODSWORD);
        test.Expect(world.TryStartMeleeAttack(actorID,targetID,1),"Production killing-blow attack starts");
        world.Update();
        auto *target = dynamic_cast<Monster *>(world.GetEntityByID(targetID));
        test.Expect(target && !target->IsAlive(),"Deterministic production attack kills its target");
        const auto events = world.GetMeleeCombatEvents();
        test.Expect(!events.empty() && events.front().weapon == ItemType::DEVELOPER_GODSWORD,
            "The committed event keeps its weapon snapshot through death cleanup");
        MeleePresentationState state;
        state.SynchronizeResults(actorID, events);
        world.Update(); // The next catch-up tick replaces the transient event vector.
        state.SynchronizeResults(actorID, world.GetMeleeCombatEvents());
        state.Update(*player,nullptr,world.GetEntities(),{},0.0f);
        test.Expect(state.GetAnimation() && state.GetAnimation()->resolved && state.GetAnimation()->facing == 2,
            "A retained killing blow reaches contact after cleanup and a catch-up tick");
        test.Expect(state.ConsumeSwing() == ItemType::DEVELOPER_GODSWORD,
            "An immediate heavy contact supplies its matching swing once");
        test.Expect(!state.ConsumeSwing(), "A contact swing edge consumes once");
        state.SynchronizeResults(actorID, events);
        state.Update(*player,nullptr,world.GetEntities(),{},0.1f);
        test.Expect(!state.ConsumeSwing(), "Repeated result synchronization cannot repeat its sound");
    }
    test.Expect(MeleePresentationState::RecoveryDuration(ItemType::DEVELOPER_GODSWORD) >
        MeleePresentationState::RecoveryDuration(ItemType::BRONZE_SWORD),"Heavy weapons settle more slowly than a short sword");
    test.Expect(MeleePresentationState::RecoveryDuration(ItemType::NONE) <
        MeleePresentationState::RecoveryDuration(ItemType::BRONZE_SWORD),"Unarmed recovery is brisker than weapon recovery");
    for (const auto weapon : {ItemType::NONE,ItemType::BRONZE_SWORD,ItemType::DEVELOPER_GODSWORD,ItemType::BRONZE_AXE})
    {
        Player player(20,PlayerInitializationMode::FRESH_START);
        player.GetPosition().SetPosition(5,5);
        if (weapon != ItemType::NONE) player.GetEquipment().Equip(EquipmentSlotType::WEAPON,weapon);
        std::map<int,MeleeCombatFeedback> feedbacks{{21,Result(player,21,50)}};
        MeleePresentationState fast,slow;
        fast.Update(player,nullptr,{},feedbacks,0);
        slow.Update(player,nullptr,{},feedbacks,0);
        for (int step = 0; step < 6; ++step)
        {
            fast.Update(player,nullptr,{},feedbacks,1.0f/60.0f);
            fast.Update(player,nullptr,{},feedbacks,1.0f/60.0f);
            slow.Update(player,nullptr,{},feedbacks,1.0f/30.0f);
            test.ExpectEqual(fast.GetPose(),slow.GetPose(),"Authored attack timing is stable at 30 and 60 rendered frames per second");
        }
    }
    return test.Finish();
}
