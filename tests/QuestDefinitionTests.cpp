#include "TestSupport.h"
#include "../src/Quest/QuestDefinitionDatabase.h"
int main(){TestContext t;const auto*d=QuestDefinitionDatabase::TryGet(QuestId::GATHERING_BASICS);t.Expect(d&&d->requiredAmount==10&&d->objectiveItem==ItemType::LOG&&d->rewardItem==ItemType::COINS&&d->rewardAmount==10,"Definition is canonical");t.Expect(!IsValidQuestId(QuestId::NONE)&&QuestDefinitionDatabase::TryGet(QuestId::NONE)==nullptr,"Invalid ID rejects");return t.Finish();}
