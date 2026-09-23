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
                        {QuestId::MINING_BASICS, QuestState::AVAILABLE, 0}}),
                    "Fixture restores through controlled QuestSystem API");
        presentation.Synchronize(journal);
        const auto &quests = presentation.GetQuests();
        test.Expect(quests.size() == 2 &&
                        quests.front().id == QuestId::GATHERING_BASICS &&
                        quests.front().state == state &&
                        quests.front().progress == progress &&
                        quests.front().required == 10 &&
                        quests.front().title == "Gathering Basics" &&
                        quests.front().objective == "Logs",
                    "Presentation copies ordered authoritative journal values");
        test.Expect(quests[1].id == QuestId::MINING_BASICS &&
                        quests[1].required == 5 &&
                        quests[1].title == "Mining Basics" &&
                        quests[1].objective == "Copper ores",
                    "Presentation includes Mining Basics second in catalogue order");
    }
    return test.Finish();
}
