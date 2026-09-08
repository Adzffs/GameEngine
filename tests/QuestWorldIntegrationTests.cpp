#include "TestSupport.h"
#include "../src/Action/ActionCancelReason.h"
#include "../src/Core/RandomSource.h"
#include "../src/Core/SeededRandom.h"
#include "../src/Player/Player.h"
#include "../src/World/World.h"
#include "../src/Item/ItemDatabase.h"
#include <memory>
#include <stdexcept>
#include <utility>
#include <vector>

struct WorldTestAccess
{
    static void CancelDialogue(World& world, int actor)
    {
        world.dialogueSystem.CancelActor(actor);
    }

    static DialogueSessionId StartDialogue(
        World& world, int actor, int npc, NpcType type,
        DialogueId dialogue, DialogueNodeId node)
    {
        return world.dialogueSystem.Start(
            actor, npc, type, dialogue, node, world.currentTick);
    }

    static int CreateRawNpc(World& world, int x, int y, NpcType type)
    {
        return world.entityManager.CreateNPC(x, y, type);
    }

    static int CreateHostile(World& world, int x, int y, NpcType type)
    {
        return world.entityManager.CreateMonster(
            x, y, CombatRatings{1, 1, 1, 5}, RewardTableType::NONE,
            std::nullopt, std::nullopt, type);
    }
};

namespace {
class RecordingRandom final:public RandomSource { public: explicit RecordingRandom(std::vector<int> v):values(std::move(v)){} int NextIntInclusive(int lo,int hi)override{if(next>=values.size())throw std::runtime_error("RNG exhausted");ranges.push_back({lo,hi});int v=values[next++];return v<lo?lo:(v>hi?hi:v);} std::vector<int> values;size_t next=0;std::vector<std::pair<int,int>> ranges;};
Player* PlayerOf(World&w,int id){return dynamic_cast<Player*>(w.GetEntityByID(id));}
Player* NewPlayer(World&w){auto*p=PlayerOf(w,w.CreatePlayer());p->GetPosition().SetPosition(2,3);return p;}
const ActiveDialogueSession* Open(World&w,Player&p){w.EnqueueCommand(NpcInteractionCommand{p.GetID(),1,NpcInteractionType::TALK});w.Update();return w.GetActiveDialogueSession(p.GetID());}
bool Accept(World&w,Player&p){auto*s=Open(w,p);if(!s)return false;w.EnqueueCommand(QuestAcceptCommand{p.GetID(),1,s->sessionId,QuestId::GATHERING_BASICS});w.Update();return p.GetQuestJournal().Get().state==QuestState::ACTIVE;}
int Slot(const Inventory&i,ItemType type){const auto&s=i.GetSlots();for(int n=0;n<(int)s.size();++n)if(!s[n].IsEmpty()&&s[n].GetItemType()==type)return n;return -1;}
const ResourceNode* Resource(const World&w,ResourceType type){for(const auto&r:w.GetResources())if(r.GetResourceType()==type)return &r;return nullptr;}
bool Start(World&w,Player&p,ResourceType type){auto*r=Resource(w,type);if(!r)return false;while(r->GetRemainingUses()==0)w.Update();int slot=Slot(p.GetInventory(),ItemType::BRONZE_AXE);if(slot>=0&&!w.TryEquipInventoryItem(p.GetID(),slot))return false;p.GetPosition().SetPosition(r->GetX()-1,r->GetY());w.QueueResourceInteraction(p.GetID(),r->GetID());w.Update();return w.GetActionForEntity(p.GetID())!=nullptr;}
void Finish(World&w){for(int i=0;i<ItemDatabase::Get(ItemType::BRONZE_AXE).GetActionDurationTicks();++i)w.Update();}
CommandResultCode Result(const World&w,uint64_t id){for(const auto&r:w.GetCommandProcessingResults())if(r.commandID==id)return r.resultCode;return CommandResultCode::INVALID_COMMAND_DATA;}
}

int main(){TestContext t;
 {auto rng=std::make_unique<RecordingRandom>(std::vector<int>(10,1));auto*probe=rng.get();World w(std::make_unique<SeededRandom>(1),std::make_unique<SeededRandom>(2),std::move(rng));auto*p=NewPlayer(w);t.Expect(Accept(w,*p),"Quest accepts through World queue");for(int i=0;i<10;++i){t.Expect(Start(w,*p,ResourceType::NORMAL_TREE),"Normal Log gather starts through World");Finish(w);}t.Expect(p->GetQuestJournal().Get().state==QuestState::READY_TO_COMPLETE&&p->GetQuestJournal().Get().progress==10,"Ten real gathering completions reach ready at ten");t.ExpectEqual((int)probe->ranges.size(),10,"Quest tracking consumes no extra RNG");bool ranges=true;for(auto r:probe->ranges)ranges&=r.first==1&&r.second==100;t.Expect(ranges,"Gathering RNG order and ranges remain unchanged");p->GetPosition().SetPosition(2,3);auto*s=Open(w,*p);auto sid=s->sessionId;auto a=w.EnqueueCommand(QuestCompleteCommand{p->GetID(),1,sid,QuestId::GATHERING_BASICS});auto b=w.EnqueueCommand(QuestCompleteCommand{p->GetID(),1,sid,QuestId::GATHERING_BASICS});w.Update();t.Expect(Result(w,a)==CommandResultCode::ACCEPTED&&Result(w,b)==CommandResultCode::GAMEPLAY_REJECTED&&p->GetInventory().GetItemAmount(ItemType::COINS)==10,"Same-tick duplicate rewards exactly once");auto c=w.EnqueueCommand(QuestCompleteCommand{p->GetID(),1,sid,QuestId::GATHERING_BASICS});w.Update();t.Expect(Result(w,c)==CommandResultCode::GAMEPLAY_REJECTED&&p->GetInventory().GetItemAmount(ItemType::COINS)==10,"Later duplicate cannot reward");}
 {auto rng=std::make_unique<RecordingRandom>(std::vector<int>{100,1});auto*probe=rng.get();World w(std::make_unique<SeededRandom>(3),std::make_unique<SeededRandom>(4),std::move(rng));auto*p=NewPlayer(w);t.Expect(Accept(w,*p),"Exclusion fixture accepts");t.Expect(Start(w,*p,ResourceType::NORMAL_TREE),"Failed gather starts");Finish(w);t.ExpectEqual(p->GetQuestJournal().Get().progress,0,"Failed roll does not progress");t.Expect(Start(w,*p,ResourceType::NORMAL_TREE),"Cancelled gather starts");w.CancelActionsForEntity(p->GetID(),ActionCancelReason::PLAYER_MOVED);Finish(w);t.ExpectEqual(p->GetQuestJournal().Get().progress,0,"Cancelled action does not progress");t.Expect(Start(w,*p,ResourceType::NORMAL_TREE),"Stale gather starts");p->GetPosition().SetPosition(50,50);Finish(w);t.ExpectEqual(p->GetQuestJournal().Get().progress,0,"Stale action does not progress");t.ExpectEqual((int)probe->ranges.size(),1,"Cancelled and stale actions consume no roll");}
 {auto rng=std::make_unique<RecordingRandom>(std::vector<int>{1,1});World w(std::make_unique<SeededRandom>(10),std::make_unique<SeededRandom>(11),std::move(rng));auto*p=NewPlayer(w);t.Expect(Accept(w,*p),"Other-log fixture accepts");t.Expect(Start(w,*p,ResourceType::OAK_TREE),"Oak gathering starts through World");Finish(w);w.CancelActionsForEntity(p->GetID(),ActionCancelReason::PLAYER_MOVED);t.Expect(Start(w,*p,ResourceType::WILLOW_TREE),"Willow gathering starts through World");Finish(w);t.Expect(p->GetInventory().GetItemAmount(ItemType::OAK_LOG)==1&&p->GetInventory().GetItemAmount(ItemType::WILLOW_LOG)==1&&p->GetQuestJournal().Get().progress==0,"Authoritatively awarded Oak and Willow Logs do not progress quest");}
 {auto rng=std::make_unique<RecordingRandom>(std::vector<int>{1});World w(std::make_unique<SeededRandom>(5),std::make_unique<SeededRandom>(6),std::move(rng));auto*p=NewPlayer(w);t.Expect(Accept(w,*p),"Full inventory fixture accepts");t.Expect(Start(w,*p,ResourceType::NORMAL_TREE),"Full inventory gather starts");while(p->GetInventory().AddItem(ItemType::LOG,1)){}int logs=p->GetInventory().GetItemAmount(ItemType::LOG);Finish(w);t.Expect(p->GetInventory().GetItemAmount(ItemType::LOG)==logs&&p->GetQuestJournal().Get().progress==0,"Failed inventory insertion does not progress");}
 {World w(7,8,9);auto*p=NewPlayer(w);auto*s=Open(w,*p);auto sid=s->sessionId;auto nonPlayer=w.EnqueueCommand(QuestAcceptCommand{1,1,sid,QuestId::GATHERING_BASICS});auto nonNpc=w.EnqueueCommand(QuestAcceptCommand{p->GetID(),p->GetID(),sid,QuestId::GATHERING_BASICS});auto missing=w.EnqueueCommand(QuestAcceptCommand{p->GetID(),99999,sid,QuestId::GATHERING_BASICS});auto stale=w.EnqueueCommand(QuestAcceptCommand{p->GetID(),1,sid+1,QuestId::GATHERING_BASICS});auto unknown=w.EnqueueCommand(QuestAcceptCommand{p->GetID(),1,sid,static_cast<QuestId>(999)});p->GetPosition().SetPosition(50,50);auto far=w.EnqueueCommand(QuestAcceptCommand{p->GetID(),1,sid,QuestId::GATHERING_BASICS});w.Update();t.Expect(Result(w,nonPlayer)!=CommandResultCode::ACCEPTED,"Non-player actor rejects");t.Expect(Result(w,nonNpc)!=CommandResultCode::ACCEPTED,"Non-NPC target rejects");t.Expect(Result(w,missing)!=CommandResultCode::ACCEPTED,"Missing NPC rejects");t.Expect(Result(w,stale)!=CommandResultCode::ACCEPTED,"Stale session rejects");t.Expect(Result(w,unknown)!=CommandResultCode::ACCEPTED,"Unknown quest rejects");t.Expect(Result(w,far)!=CommandResultCode::ACCEPTED,"Non-adjacency rejects");t.Expect(p->GetQuestJournal().Get().state==QuestState::AVAILABLE,"Rejections preserve state");}
 {
  World w(20,21,22);auto*p=NewPlayer(w);auto*s=Open(w,*p);const auto original=s->sessionId;
  auto reject=[&](const ServerCommandData& command,const char* message){auto id=w.EnqueueCommand(command);w.Update();t.Expect(Result(w,id)!=CommandResultCode::ACCEPTED,message);};
  int hostile=WorldTestAccess::CreateHostile(w,3,3,NpcType::PASSIVE_DEVELOPMENT_MONSTER);
  reject(QuestAcceptCommand{p->GetID(),hostile,original,QuestId::GATHERING_BASICS},"Hostile/wrong-type NPC rejects individually");
  int wrongGuide=WorldTestAccess::CreateRawNpc(w,3,3,NpcType::DEVELOPMENT_GUIDE);
  reject(QuestAcceptCommand{p->GetID(),wrongGuide,original,QuestId::GATHERING_BASICS},"Wrong friendly Guide identity rejects individually");
  WorldTestAccess::CancelDialogue(w,p->GetID());
  reject(QuestAcceptCommand{p->GetID(),1,original,QuestId::GATHERING_BASICS},"Missing dialogue session rejects individually");
  auto wrongDialogue=WorldTestAccess::StartDialogue(w,p->GetID(),1,NpcType::DEVELOPMENT_GUIDE,DialogueId::NONE,DialogueNodeId::DEVELOPMENT_GUIDE_WELCOME);
  reject(QuestAcceptCommand{p->GetID(),1,wrongDialogue,QuestId::GATHERING_BASICS},"Wrong dialogue ID rejects individually");
  WorldTestAccess::CancelDialogue(w,p->GetID());
  auto wrongNode=WorldTestAccess::StartDialogue(w,p->GetID(),1,NpcType::DEVELOPMENT_GUIDE,DialogueId::DEVELOPMENT_GUIDE_INTRO,DialogueNodeId::DEVELOPMENT_GUIDE_EXPLANATION);
  reject(QuestAcceptCommand{p->GetID(),1,wrongNode,QuestId::GATHERING_BASICS},"Wrong dialogue node rejects individually");
  WorldTestAccess::CancelDialogue(w,p->GetID());
  auto valid=WorldTestAccess::StartDialogue(w,p->GetID(),1,NpcType::DEVELOPMENT_GUIDE,DialogueId::DEVELOPMENT_GUIDE_INTRO,DialogueNodeId::DEVELOPMENT_GUIDE_WELCOME);
  p->GetQuestJournal().Get().state=QuestState::ACTIVE;
  reject(QuestAcceptCommand{p->GetID(),1,valid,QuestId::GATHERING_BASICS},"Invalid ACTIVE acceptance state rejects individually");
  t.Expect(p->GetQuestJournal().Get().state==QuestState::ACTIVE,"Individual rejection cases preserve quest state");
 }
 {
  World w(30,31,32);auto*p=NewPlayer(w);auto*s=Open(w,*p);const auto sid=s->sessionId;p->ApplyDamage(p->GetCurrentHealth());
  auto accept=w.EnqueueCommand(QuestAcceptCommand{p->GetID(),1,sid,QuestId::GATHERING_BASICS});w.Update();
  t.Expect(Result(w,accept)!=CommandResultCode::ACCEPTED&&p->GetQuestJournal().Get().state==QuestState::AVAILABLE,"Dead Player rejects Accept through World queue");
  p->GetQuestJournal().Get()={QuestId::GATHERING_BASICS,QuestState::READY_TO_COMPLETE,10};
  auto complete=w.EnqueueCommand(QuestCompleteCommand{p->GetID(),1,sid,QuestId::GATHERING_BASICS});w.Update();
  t.Expect(Result(w,complete)!=CommandResultCode::ACCEPTED&&p->GetQuestJournal().Get().state==QuestState::READY_TO_COMPLETE&&p->GetInventory().GetItemAmount(ItemType::COINS)==0,"Dead Player rejects Complete without reward");
 }
 {
  World w(40,41,42);auto*p=NewPlayer(w);p->GetQuestJournal().Get()={QuestId::GATHERING_BASICS,QuestState::READY_TO_COMPLETE,10};auto*s=Open(w,*p);auto sid=s->sessionId;
  auto reject=[&](const QuestCompleteCommand& command,const char* message){auto id=w.EnqueueCommand(command);w.Update();t.Expect(Result(w,id)!=CommandResultCode::ACCEPTED&&p->GetQuestJournal().Get().state==QuestState::READY_TO_COMPLETE&&p->GetInventory().GetItemAmount(ItemType::COINS)==0,message);};
  reject({1,1,sid,QuestId::GATHERING_BASICS},"Complete rejects non-player actor without reward");
  reject({p->GetID(),99999,sid,QuestId::GATHERING_BASICS},"Complete rejects missing NPC without reward");
  reject({p->GetID(),p->GetID(),sid,QuestId::GATHERING_BASICS},"Complete rejects non-NPC target without reward");
  int hostile=WorldTestAccess::CreateHostile(w,3,3,NpcType::PASSIVE_DEVELOPMENT_MONSTER);
  reject({p->GetID(),hostile,sid,QuestId::GATHERING_BASICS},"Complete rejects hostile/wrong-type NPC without reward");
  int wrongGuide=WorldTestAccess::CreateRawNpc(w,3,3,NpcType::DEVELOPMENT_GUIDE);
  reject({p->GetID(),wrongGuide,sid,QuestId::GATHERING_BASICS},"Complete rejects wrong Guide identity without reward");
  p->GetPosition().SetPosition(50,50);
  reject({p->GetID(),1,sid,QuestId::GATHERING_BASICS},"Complete rejects non-adjacency without reward");
  p->GetPosition().SetPosition(2,3);WorldTestAccess::CancelDialogue(w,p->GetID());
  reject({p->GetID(),1,sid,QuestId::GATHERING_BASICS},"Complete rejects absent session without reward");
  s=Open(w,*p);sid=s->sessionId;
  reject({p->GetID(),1,sid+1,QuestId::GATHERING_BASICS},"Complete rejects stale/wrong session ID without reward");
  WorldTestAccess::CancelDialogue(w,p->GetID());
  auto wrongDialogue=WorldTestAccess::StartDialogue(w,p->GetID(),1,NpcType::DEVELOPMENT_GUIDE,DialogueId::NONE,DialogueNodeId::DEVELOPMENT_GUIDE_WELCOME);
  reject({p->GetID(),1,wrongDialogue,QuestId::GATHERING_BASICS},"Complete rejects wrong dialogue ID without reward");
  WorldTestAccess::CancelDialogue(w,p->GetID());
  auto wrongNode=WorldTestAccess::StartDialogue(w,p->GetID(),1,NpcType::DEVELOPMENT_GUIDE,DialogueId::DEVELOPMENT_GUIDE_INTRO,DialogueNodeId::DEVELOPMENT_GUIDE_EXPLANATION);
  reject({p->GetID(),1,wrongNode,QuestId::GATHERING_BASICS},"Complete rejects wrong node without reward");
  WorldTestAccess::CancelDialogue(w,p->GetID());sid=WorldTestAccess::StartDialogue(w,p->GetID(),1,NpcType::DEVELOPMENT_GUIDE,DialogueId::DEVELOPMENT_GUIDE_INTRO,DialogueNodeId::DEVELOPMENT_GUIDE_WELCOME);
  reject({p->GetID(),1,sid,static_cast<QuestId>(999)},"Complete rejects invalid quest ID without reward");
 }
 return t.Finish();}
