#include "TestSupport.h"
#include "../src/Quest/QuestPresentationState.h"
#include "../src/Quest/QuestSystem.h"

int main()
{
    TestContext test;
    QuestJournal journal;
    QuestPresentationState presentation;
    for (const auto &[state, progress] : std::vector<std::pair<QuestState, int>>{
             {QuestState::AVAILABLE, 0},
             {QuestState::ACTIVE, 6},
             {QuestState::READY_TO_COMPLETE, 10},
             {QuestState::COMPLETED, 10}})
    {
        test.Expect(QuestSystem::TryRestore(journal, {
                        {QuestId::GATHERING_BASICS, state, progress},
                        {QuestId::MINING_BASICS, QuestState::AVAILABLE, 0},
                        {QuestId::TRACKS_IN_THE_ASH, QuestState::AVAILABLE, 0},
                        {QuestId::SHIELD_OR_SHELTER, QuestState::AVAILABLE, 0},
                        {QuestId::FANGS_AT_THE_GATE, QuestState::AVAILABLE, 0},
                        {QuestId::HEART_OF_THE_ASH, QuestState::AVAILABLE, 0}}),
                    "Fixture restores through controlled QuestSystem API");
        presentation.Synchronize(journal);
        const auto &quests = presentation.GetQuests();
        const std::size_t gatheringIndex =
            state == QuestState::COMPLETED ? quests.size() - 1 : 0;
        const std::size_t miningIndex =
            state == QuestState::COMPLETED ? 0 : 1;
        test.Expect(quests.size() == 6 &&
                        quests[gatheringIndex].id == QuestId::GATHERING_BASICS &&
                        quests[gatheringIndex].state == state &&
                        quests[gatheringIndex].progress == progress &&
                        quests[gatheringIndex].required == 10 &&
                        quests[gatheringIndex].title == "Gathering Basics" &&
                        quests[gatheringIndex].objective == "Gather normal Logs" &&
                        quests[gatheringIndex].category == QuestCategory::TUTORIAL &&
                        quests[gatheringIndex].reward == "Coins" &&
                        quests[gatheringIndex].rewardAmount == 10 &&
                        !quests[gatheringIndex].guidance.empty(),
                    "Presentation copies objective, category and reward values");
        test.Expect(quests[miningIndex].id == QuestId::MINING_BASICS &&
                        quests[miningIndex].required == 5 &&
                        quests[miningIndex].title == "Mining Basics" &&
                        quests[miningIndex].objective == "Gather Copper ore",
                    "Active work stays ahead of completed journal history");
    }
    test.Expect(QuestSystem::TryRestore(journal, {
                    {QuestId::GATHERING_BASICS, QuestState::AVAILABLE, 0},
                    {QuestId::MINING_BASICS, QuestState::AVAILABLE, 0},
                    {QuestId::TRACKS_IN_THE_ASH, QuestState::AVAILABLE, 0},
                    {QuestId::SHIELD_OR_SHELTER,
                        QuestState::READY_TO_COMPLETE, 1},
                    {QuestId::FANGS_AT_THE_GATE, QuestState::AVAILABLE, 0},
                    {QuestId::HEART_OF_THE_ASH, QuestState::AVAILABLE, 0}}),
                "Choice guidance fixture restores");
    presentation.Synchronize(journal);
    test.Expect(!presentation.GetQuests().empty() &&
                    presentation.GetQuests().front().id ==
                        QuestId::SHIELD_OR_SHELTER &&
                    presentation.GetQuests().front().guidance ==
                        "Choose: Torren Ash or Nessa Thorne",
                "Ready choice quests name both valid reward contacts");
    test.Expect(presentation.GetSuggestedQuest() != nullptr &&
                    presentation.GetSuggestedQuest()->id == QuestId::SHIELD_OR_SHELTER,
                "A ready reward choice takes priority over unstarted quests");
    const auto details = QuestPresentationState::GetDetailLines(presentation.GetQuests().front());
    test.Expect(std::find(details.begin(), details.end(), "Torren Ash: 25 Coins") != details.end() &&
                    std::find(details.begin(), details.end(), "OR Nessa Thorne: 3 x Cooked meat") != details.end(),
                "Both reward options remain visible before claiming either one");
    QuestJournal fresh;
    presentation.Synchronize(fresh);
    test.Expect(presentation.GetSuggestedQuest() != nullptr &&
                    presentation.GetSuggestedQuest()->id == QuestId::TRACKS_IN_THE_ASH,
                "Optional unstarted basics do not hide the first story lead");
    QuestSystem::Accept(fresh, QuestId::MINING_BASICS);
    presentation.Synchronize(fresh);
    test.Expect(presentation.GetSuggestedQuest() != nullptr &&
                    presentation.GetSuggestedQuest()->id == QuestId::MINING_BASICS,
                "An accepted optional task remains ahead of a new story offer");
    return test.Finish();
}
