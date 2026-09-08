#include "TestSupport.h"
#include "../src/Quest/QuestPresentationState.h"
int main(){TestContext t;QuestJournal j;QuestPresentationState p;for(auto state:{QuestState::AVAILABLE,QuestState::ACTIVE,QuestState::READY_TO_COMPLETE,QuestState::COMPLETED}){j.Get().state=state;j.Get().progress=state==QuestState::AVAILABLE?0:state==QuestState::ACTIVE?6:10;p.Synchronize(j);t.Expect(p.Get().state==state&&p.Get().progress==j.Get().progress&&p.Get().required==10,"Presentation mirrors authoritative journal and definition");}return t.Finish();}
