#include "TestSupport.h"
#include "../src/Audio/AudioFeedbackState.h"
#include "../src/World/World.h"
#include "../src/Player/Player.h"
#include "../src/Gathering/GatheringPresentationState.h"
#include "../src/Core/RandomSource.h"
#include <algorithm>
#include <memory>

namespace
{
    bool Contains(const std::vector<AudioCue> &cues, AudioCue cue)
    { return std::find(cues.begin(), cues.end(), cue) != cues.end(); }
    class HitRolls : public RandomSource
    {
        int count = 0;
        int NextIntInclusive(int minimum, int maximum) override
        { return count++ % 3 == 1 ? minimum : maximum; }
    };
}

int main()
{
    TestContext test;
    AudioSettings settings;
    test.ExpectEqual(settings.GetEffectsVolume(), 65, "Restrained default effects volume");
    settings.SetEffectsVolume(999);
    test.ExpectNear(settings.GetGain(), 1, .001f, "Effects volume clamps high");
    settings.ToggleMute();
    test.ExpectNear(settings.GetGain(), 0, .001f, "Mute is independent from saved volume");
    test.ExpectEqual(settings.GetEffectsVolume(), 100, "Mute retains volume");
    settings.SetEffectsVolume(-99);
    settings.ToggleMute();
    test.ExpectNear(settings.GetGain(), 0, .001f, "Zero volume remains silent after unmuting");
    AudioFeedbackState state;
    state.Reset(7);
    state.TrackCommand(11, AudioCue::EQUIP);
    state.TrackCommand(12, AudioCue::EAT);
    const std::vector<CommandProcessingResult> commands{
        {11, 7, CommandResultCode::ACCEPTED}, {11, 7, CommandResultCode::ACCEPTED},
        {12, 7, CommandResultCode::GAMEPLAY_REJECTED}};
    const std::vector<ActionLifecycleEvent> actions{
        {ActionStartedEvent{7, 8, ActionType::MELEE_ATTACK, 10, 11}},
        {ActionCompletedEvent{7, 100, ActionType::RECIPE, 10}},
        {ActionCompletedEvent{9, 100, ActionType::RECIPE, 10}}};
    const std::vector<MeleeCombatFeedback> combat{
        {7, 8, true, 4, 3, 10}, {8, 7, true, 0, 3, 10},
        {7, 8, false, 0, 3, 10}, {9, 8, true, 4, 3, 10},
        {8, 7, true, 4, 2, 9}};
    const std::vector<GatheringRewardEvent> rewards{
        {7, 2, 10, ResourceType::NORMAL_TREE, 1, 1, 0, 1, ItemType::LOG, 1, 25}};
    const auto cues = state.ConsumeTick(7, 10, actions, rewards, combat, commands,
        ItemType::DEVELOPER_GODSWORD);
    test.ExpectEqual(cues.size(), std::size_t{7}, "Only local current committed facts produce cues");
    test.Expect(Contains(cues, AudioCue::HIT) && Contains(cues, AudioCue::BLOCK) &&
        Contains(cues, AudioCue::MISS), "Hit, zero damage and miss differ");
    test.Expect(Contains(cues, AudioCue::CRAFT) && Contains(cues, AudioCue::GATHER),
        "Actual crafting and gathering completions confirm success");
    test.Expect(Contains(cues, AudioCue::HEAVY_SWING), "Heavy weapon has a distinct swing");
    test.ExpectEqual(std::count(cues.begin(), cues.end(), AudioCue::EQUIP),
        std::ptrdiff_t{1}, "Accepted command confirms only once");
    test.Expect(!Contains(cues, AudioCue::EAT), "Rejected food command never confirms");
    test.Expect(state.ConsumeTick(7, 10, actions, rewards, combat, commands).empty(),
        "Repeated synchronization does not repeat a tick");
    test.Expect(state.ConsumeTick(7, 11, {}, {}, {}, commands).empty(),
        "Confirmed and rejected command identities are retired");
    test.Expect(Contains(state.ConsumeTick(7, 12,
        {{ActionStartedEvent{7, 8, ActionType::MELEE_ATTACK, 12, 13}}}, {}, {}, {}),
        AudioCue::SWING), "Next catch-up tick delivers its independent event");
    std::vector<ActionLifecycleEvent> many;
    for (int i = 0; i < 50; ++i)
        many.push_back({ActionCompletedEvent{7, i, ActionType::RECIPE, 13}});
    test.ExpectEqual(state.ConsumeTick(7, 13, many, {}, {}, {}).size(),
        std::size_t{16}, "One tick has a bounded output budget");
    state.TrackCommand(30, AudioCue::EAT);
    test.Expect(state.ConsumeTick(99, 1, {}, {}, {},
        {{30, 99, CommandResultCode::ACCEPTED}}).empty(),
        "Actor switch clears stale pending command identities");
    state.Reset(7);
    for (std::uint64_t id = 1; id <= 65; ++id) state.TrackCommand(id, AudioCue::EQUIP);
    test.Expect(state.ConsumeTick(7, 1, {}, {}, {},
        {{1, 7, CommandResultCode::ACCEPTED}}).empty(), "Pending command history is bounded");

    for (const auto skill : {SkillType::WOODCUTTING, SkillType::MINING})
    {
        Player gatherer(7, PlayerInitializationMode::FRESH_START);
        gatherer.GetPosition().SetPosition(5, 5);
        gatherer.GetEquipment().Equip(EquipmentSlotType::WEAPON,
            skill == SkillType::MINING ? ItemType::BRONZE_PICKAXE : ItemType::BRONZE_AXE);
        std::vector<ResourceNode> resources{{8,
            skill == SkillType::MINING ? ResourceType::COPPER_ROCK : ResourceType::NORMAL_TREE, 5, 6}};
        Action action(ActionType::GATHERING, "Gather", 5, 7, 8, true);
        action.Start(1);
        GatheringPresentationState presentation(skill);
        presentation.Update(gatherer, &action, resources, 0);
        int contacts = 0;
        for (int frame = 0; frame < 50; ++frame)
        {
            presentation.Update(gatherer, &action, resources, .05f);
            if (presentation.ConsumeContact())
            {
                ++contacts;
                test.Expect(!presentation.ConsumeContact(), "Contact edge consumes once");
            }
        }
        test.ExpectEqual(contacts, 2, "Repeated gathering loop has one contact per authored cycle");
        presentation.Update(gatherer, nullptr, resources, .1f);
        test.Expect(!presentation.ConsumeContact(), "Interrupted gathering has no stray contact");
        test.ExpectEqual(gatherer.GetSkills().GetSkill(skill).GetXP(), 0,
            "Contact presentation never grants gameplay XP");
    }

    // A lethal hit cleans display feedback but keeps its committed per-tick event.
    World world(std::make_unique<HitRolls>());
    const int attackerID = world.CreatePlayer(), defenderID = world.CreatePlayer();
    auto *attacker = dynamic_cast<Player *>(world.GetEntityByID(attackerID));
    auto *defender = dynamic_cast<Player *>(world.GetEntityByID(defenderID));
    attacker->GetPosition().SetPosition(10, 10);
    defender->GetPosition().SetPosition(11, 10);
    defender->ApplyDamage(defender->GetCurrentHealth() - 1);
    test.Expect(world.TryStartMeleeAttack(attackerID, defenderID, 1),
        "Lethal committed-outcome fixture starts a real action");
    world.Update();
    test.Expect(!defender->IsAlive(), "Real fixture resolves a lethal hit");
    test.Expect(!world.GetMeleeCombatEvents().empty(), "Fatal outcome survives entity display cleanup");
    state.Reset(attackerID);
    test.Expect(Contains(state.ConsumeTick(attackerID, world.GetCurrentTick(),
        world.GetActionLifecycleEvents(), world.GetGatheringRewardEvents(),
        world.GetMeleeCombatEvents(), {}), AudioCue::HIT), "Fatal hit remains audible");
    world.Update();
    test.Expect(world.GetMeleeCombatEvents().empty(), "Committed combat events expire next tick");
    return test.Finish();
}
