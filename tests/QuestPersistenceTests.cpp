#include "TestSupport.h"
#include "../src/Persistence/PlayerSaveState.h"
#include "../src/Persistence/PlayerSaveTextCodec.h"
#include "../src/Player/Player.h"
#include "../src/Quest/QuestSystem.h"
#include "../src/World/World.h"
#include <filesystem>

namespace {
std::string Replace(std::string value,const std::string& from,const std::string& to){const auto position=value.find(from);if(position!=std::string::npos)value.replace(position,from.size(),to);return value;}
}

int main(){
 TestContext t;Player player(1);player.GetQuestJournal().Get()={QuestId::GATHERING_BASICS,QuestState::ACTIVE,2};auto save=PlayerSaveState::Capture(player);std::string encoded;PlayerSaveValidationReport report;
 t.Expect(PlayerSaveTextCodec::TryEncode(save,encoded,report)&&encoded.find("version=2")!=std::string::npos&&encoded.find("quest.GATHERING_BASICS=ACTIVE,2")!=std::string::npos,"Version 2 encodes canonically");
 auto decoded=PlayerSaveTextCodec::Decode(encoded);t.Expect(decoded.IsSuccess()&&decoded.GetSaveData()->version==2&&decoded.GetSaveData()->gatheringBasicsState==QuestState::ACTIVE&&decoded.GetSaveData()->gatheringBasicsProgress==2,"Version 2 round trips");
 save.version=1;std::string unchanged="unchanged";t.Expect(!PlayerSaveTextCodec::TryEncode(save,unchanged,report)&&unchanged=="unchanged","Version 1 encoding rejects without output");
 std::string legacy=Replace(encoded,"version=2","version=1");const auto questStart=legacy.find("quest.GATHERING_BASICS=");legacy.erase(questStart,legacy.find('\n',questStart)-questStart+1);auto migrated=PlayerSaveTextCodec::Decode(legacy);t.Expect(migrated.IsSuccess()&&migrated.GetSaveData()->version==2&&migrated.GetSaveData()->gatheringBasicsState==QuestState::AVAILABLE&&migrated.GetSaveData()->gatheringBasicsProgress==0,"Actual version 1 text migrates to available zero");
 t.Expect(!PlayerSaveTextCodec::Decode(Replace(encoded,"quest.GATHERING_BASICS=ACTIVE,2","quest.GATHERING_BASICS=UNKNOWN,2")).IsSuccess(),"Unknown state rejects");
 t.Expect(!PlayerSaveTextCodec::Decode(Replace(encoded,"quest.GATHERING_BASICS=ACTIVE,2","quest.GATHERING_BASICS=ACTIVE,10")).IsSuccess(),"Impossible state progress rejects");
 Player completed(2);completed.GetQuestJournal().Get()={QuestId::GATHERING_BASICS,QuestState::COMPLETED,10};auto completedSave=PlayerSaveState::Capture(completed);auto restored=PlayerSaveState::TryCreatePlayer(3,completedSave,report);t.Expect(restored&&!QuestSystem::Complete(restored->GetQuestJournal(),restored->GetInventory())&&restored->GetInventory().GetItemAmount(ItemType::COINS)==0,"Restored completed quest cannot reward again");
 for(const auto state:{QuestState::ACTIVE,QuestState::READY_TO_COMPLETE}){const auto path=std::filesystem::temp_directory_path()/(state==QuestState::ACTIVE?"gameengine_quest_active.save":"gameengine_quest_ready.save");std::filesystem::remove(path);World first(10U,11U,12U);const int id=first.CreatePlayer();auto*source=dynamic_cast<Player*>(first.GetEntityByID(id));source->GetQuestJournal().Get()={QuestId::GATHERING_BASICS,state,state==QuestState::ACTIVE?6:10};t.Expect(first.SavePlayerToFile(id,path).IsSuccess(),"Quest lifecycle state saves through World file path");World second(13U,14U,15U);auto loaded=second.LoadOrCreatePlayerFromFile(path);auto*target=dynamic_cast<Player*>(second.GetEntityByID(loaded.GetPlayerEntityID()));t.Expect(loaded.IsSuccess()&&target&&target->GetQuestJournal().Get().state==state&&target->GetQuestJournal().Get().progress==(state==QuestState::ACTIVE?6:10),"ACTIVE/READY quest lifecycle restores through World file path");std::filesystem::remove(path);}
 return t.Finish();
}
