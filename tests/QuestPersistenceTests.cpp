#include "TestSupport.h"
#include "../src/Persistence/PlayerSaveState.h"
#include "../src/Persistence/PlayerSaveTextCodec.h"
#include "../src/Player/Player.h"
#include "../src/Quest/QuestSystem.h"
#include "../src/World/World.h"
#include <filesystem>
#include <string>
#include <tuple>
#include <vector>

namespace {
std::string Replace(std::string value,const std::string& from,const std::string& to){const auto p=value.find(from);if(p!=std::string::npos)value.replace(p,from.size(),to);return value;}
void EraseLine(std::string& value,const std::string& prefix){const auto p=value.find(prefix);if(p==std::string::npos)return;const auto e=value.find('\n',p);value.erase(p,e==std::string::npos?std::string::npos:e-p+1);}
const QuestRecord* Gathering(const Player& player){return player.GetQuestJournal().TryGet(QuestId::GATHERING_BASICS);}
const QuestRecord* Mining(const Player& player){return player.GetQuestJournal().TryGet(QuestId::MINING_BASICS);}
std::vector<QuestRecord> Records(QuestState gathering,int gatheringProgress,QuestState mining=QuestState::AVAILABLE,int miningProgress=0){return {{QuestId::GATHERING_BASICS,gathering,gatheringProgress},{QuestId::MINING_BASICS,mining,miningProgress}};}
}

int main(){
 TestContext t; Player player(1); QuestSystem::TryRestore(player.GetQuestJournal(),Records(QuestState::ACTIVE,2,QuestState::ACTIVE,3)); auto save=PlayerSaveState::Capture(player); std::string encoded; PlayerSaveValidationReport report;
 t.Expect(PlayerSaveTextCodec::TryEncode(save,encoded,report)&&encoded.find("version=4")!=std::string::npos&&encoded.find("quest_count=2")!=std::string::npos&&encoded.find("quest.GATHERING_BASICS=ACTIVE,2")!=std::string::npos&&encoded.find("quest.MINING_BASICS=ACTIVE,3")!=std::string::npos,"Version 4 encodes canonical ordered quest collection");
 auto decoded=PlayerSaveTextCodec::Decode(encoded); t.Expect(decoded.IsSuccess()&&decoded.GetSaveData()->version==4&&decoded.GetSaveData()->quests.size()==2&&decoded.GetSaveData()->quests[0].progress==2&&decoded.GetSaveData()->quests[1].progress==3,"Version 4 quest collection round trips");
 save.version=2; std::string unchanged="unchanged"; t.Expect(!PlayerSaveTextCodec::TryEncode(save,unchanged,report)&&unchanged=="unchanged","Legacy versions cannot be encoded");
 save=PlayerSaveState::Capture(player);save.quests.clear();unchanged="unchanged";t.Expect(!PlayerSaveTextCodec::TryEncode(save,unchanged,report)&&unchanged=="unchanged","Missing quest encoding rejects without output mutation");
 save=PlayerSaveState::Capture(player);save.quests.push_back(save.quests.front());unchanged="unchanged";t.Expect(!PlayerSaveTextCodec::TryEncode(save,unchanged,report)&&unchanged=="unchanged","Duplicate quest encoding rejects without output mutation");
 save=PlayerSaveState::Capture(player);save.quests.front().id=QuestId::NONE;unchanged="unchanged";t.Expect(!PlayerSaveTextCodec::TryEncode(save,unchanged,report)&&unchanged=="unchanged","NONE quest encoding rejects without output mutation");
 save=PlayerSaveState::Capture(player);save.quests.front().id=static_cast<QuestId>(999);unchanged="unchanged";t.Expect(!PlayerSaveTextCodec::TryEncode(save,unchanged,report)&&unchanged=="unchanged","Unknown quest encoding rejects without output mutation");
 std::string v3=Replace(encoded,"version=4","version=3"); EraseLine(v3,"quest.MINING_BASICS="); v3=Replace(v3,"quest_count=2","quest_count=1"); auto decodedV3=PlayerSaveTextCodec::Decode(v3); t.Expect(decodedV3.IsSuccess()&&decodedV3.GetSaveData()->version==4&&decodedV3.GetSaveData()->quests.size()==2&&decodedV3.GetSaveData()->quests[0].state==QuestState::ACTIVE&&decodedV3.GetSaveData()->quests[1].state==QuestState::AVAILABLE,"Version 3 preserves Gathering and initializes Mining");
 std::string v2=Replace(v3,"version=3","version=2"); EraseLine(v2,"quest_count="); auto decodedV2=PlayerSaveTextCodec::Decode(v2); t.Expect(decodedV2.IsSuccess()&&decodedV2.GetSaveData()->version==4&&decodedV2.GetSaveData()->quests.size()==2&&decodedV2.GetSaveData()->quests.front().state==QuestState::ACTIVE&&decodedV2.GetSaveData()->quests[1].state==QuestState::AVAILABLE,"Version 2 migrates to current in memory");
 for(const auto& [token,state,progress]:std::vector<std::tuple<std::string,QuestState,int>>{{"AVAILABLE,0",QuestState::AVAILABLE,0},{"ACTIVE,2",QuestState::ACTIVE,2},{"READY_TO_COMPLETE,10",QuestState::READY_TO_COMPLETE,10},{"COMPLETED,10",QuestState::COMPLETED,10}}){auto historical=Replace(v2,"ACTIVE,2",token);auto result=PlayerSaveTextCodec::Decode(historical);t.Expect(result.IsSuccess()&&result.GetSaveData()->quests.front().state==state&&result.GetSaveData()->quests.front().progress==progress,"Every valid historical version 2 quest state migrates");}
 {auto completedV2=PlayerSaveTextCodec::Decode(Replace(v2,"ACTIVE,2","COMPLETED,10"));auto migratedPlayer=completedV2.IsSuccess()?PlayerSaveState::TryCreatePlayer(99,*completedV2.GetSaveData(),report):nullptr;t.Expect(migratedPlayer&&!QuestSystem::Complete(migratedPlayer->GetQuestJournal(),QuestId::GATHERING_BASICS,migratedPlayer->GetInventory())&&migratedPlayer->GetInventory().GetItemAmount(ItemType::COINS)==0,"Version 2 completed migration cannot reward again");}
 std::string v1=Replace(v2,"version=2","version=1"); EraseLine(v1,"quest.GATHERING_BASICS="); auto migrated=PlayerSaveTextCodec::Decode(v1); t.Expect(migrated.IsSuccess()&&migrated.GetSaveData()->version==4&&migrated.GetSaveData()->quests.size()==2&&migrated.GetSaveData()->quests[0].state==QuestState::AVAILABLE&&migrated.GetSaveData()->quests[1].state==QuestState::AVAILABLE,"Version 1 initializes both canonical quests");
 t.Expect(!PlayerSaveTextCodec::Decode(Replace(encoded,"quest.GATHERING_BASICS=ACTIVE,2","quest.GATHERING_BASICS=UNKNOWN,2")).IsSuccess(),"Unknown state rejects");
 t.Expect(!PlayerSaveTextCodec::Decode(Replace(encoded,"quest.GATHERING_BASICS=ACTIVE,2","quest.GATHERING_BASICS=ACTIVE,10")).IsSuccess(),"Impossible state progress rejects");
 {auto missing=encoded;EraseLine(missing,"quest_count=");t.Expect(!PlayerSaveTextCodec::Decode(missing).IsSuccess(),"Version 4 missing quest count rejects");}
 t.Expect(!PlayerSaveTextCodec::Decode(Replace(encoded,"quest_count=2","quest_count=1")).IsSuccess(),"Version 4 mismatched quest count rejects");
 t.Expect(!PlayerSaveTextCodec::Decode(Replace(encoded,"quest.GATHERING_BASICS=ACTIVE,2\n","quest.GATHERING_BASICS=ACTIVE,2\nquest.GATHERING_BASICS=ACTIVE,2\n")).IsSuccess(),"Duplicate quest records reject");
 t.Expect(!PlayerSaveTextCodec::Decode(Replace(encoded,"quest.GATHERING_BASICS","quest.UNKNOWN")).IsSuccess(),"Unknown quest records reject");
 {auto missing=encoded;EraseLine(missing,"quest.MINING_BASICS=");t.Expect(!PlayerSaveTextCodec::Decode(missing).IsSuccess(),"Missing Mining record rejects");}
 t.Expect(!PlayerSaveTextCodec::Decode(Replace(encoded,"quest.MINING_BASICS=ACTIVE,3","quest.MINING_BASICS=ACTIVE")).IsSuccess(),"Malformed Mining record rejects");
 t.Expect(!PlayerSaveTextCodec::Decode(Replace(encoded,"quest.MINING_BASICS=ACTIVE,3","quest.MINING_BASICS=READY_TO_COMPLETE,4")).IsSuccess(),"Impossible Mining state and progress reject");
 {auto reversed=Replace(encoded,"quest.GATHERING_BASICS=ACTIVE,2\nquest.MINING_BASICS=ACTIVE,3\n","quest.MINING_BASICS=ACTIVE,3\nquest.GATHERING_BASICS=ACTIVE,2\n");t.Expect(!PlayerSaveTextCodec::Decode(reversed).IsSuccess(),"Incorrect version 4 quest order rejects");}
 Player completed(2); QuestSystem::TryRestore(completed.GetQuestJournal(),Records(QuestState::COMPLETED,10,QuestState::COMPLETED,5)); auto restored=PlayerSaveState::TryCreatePlayer(3,PlayerSaveState::Capture(completed),report); t.Expect(restored&&!QuestSystem::Complete(restored->GetQuestJournal(),QuestId::GATHERING_BASICS,restored->GetInventory())&&!QuestSystem::Complete(restored->GetQuestJournal(),QuestId::MINING_BASICS,restored->GetInventory())&&restored->GetInventory().GetItemAmount(ItemType::COINS)==0,"Restored completed quests cannot reward again");
 for(auto state:{QuestState::ACTIVE,QuestState::READY_TO_COMPLETE}){const auto path=std::filesystem::temp_directory_path()/(state==QuestState::ACTIVE?"gameengine_quest_active.save":"gameengine_quest_ready.save");std::filesystem::remove(path);World first(10U,11U,12U);auto* source=dynamic_cast<Player*>(first.GetEntityByID(first.CreatePlayer()));const int gatheringProgress=state==QuestState::ACTIVE?6:10;const int miningProgress=state==QuestState::ACTIVE?3:5;QuestSystem::TryRestore(source->GetQuestJournal(),Records(state,gatheringProgress,state,miningProgress));t.Expect(first.SavePlayerToFile(source->GetID(),path).IsSuccess(),"Both quest states save through World file path");World second(13U,14U,15U);auto loaded=second.LoadOrCreatePlayerFromFile(path);auto* target=dynamic_cast<Player*>(second.GetEntityByID(loaded.GetPlayerEntityID()));auto* gathering=target?Gathering(*target):nullptr;auto* mining=target?Mining(*target):nullptr;t.Expect(loaded.IsSuccess()&&gathering&&mining&&gathering->state==state&&gathering->progress==gatheringProgress&&mining->state==state&&mining->progress==miningProgress,"Both quest states restore through World file path");std::filesystem::remove(path);}
 return t.Finish();
}
