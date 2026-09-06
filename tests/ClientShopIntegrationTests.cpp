#include "TestSupport.h"
#include "../src/Core/Engine.h"

struct GraphicsTestAccess { static bool Click(Graphics&g,float x,float y){return g.HandleShopClick(x,y);} };
struct EngineTestAccess {
 static bool Init(Engine&e){return e.InitializeWorldForRun();}
 static World& W(Engine&e){return e.world;}
 static Graphics& G(Engine&e){return e.graphics;}
 static int Player(Engine&e){return e.playerID;}
 static bool Enqueue(Engine&e){return e.EnqueuePendingShopCommand();}
 static void Sync(Engine&e){e.SynchronizeShopPresentation();}
 static std::uint64_t PendingID(Engine&e){return e.pendingShopCommandID;}
};
struct WorldTestAccess { static void Inject(World&w,CommandProcessingResult r){w.publishedCommandProcessingResults.push_back(r);} };

int main(){
 TestContext t; Engine e; t.Expect(EngineTestAccess::Init(e),"Engine initializes"); auto &w=EngineTestAccess::W(e); auto &g=EngineTestAccess::G(e); int p=EngineTestAccess::Player(e);
 ShopOpenedEvent opened{p,1,NpcType::DEVELOPMENT_GUIDE,55,ShopId::DEVELOPMENT_GUIDE_SUPPLIES,"Supplies",ItemType::COINS,{{ItemType::COOKED_MEAT,8,3}}}; ActiveShopSession active{55,p,1,NpcType::DEVELOPMENT_GUIDE,ShopId::DEVELOPMENT_GUIDE_SUPPLIES,1}; g.SynchronizeShop(p,{opened},&active);
 auto row=g.GetShopRowRectangle(0); GraphicsTestAccess::Click(g,row.x+4,row.y+4); t.Expect(EngineTestAccess::Enqueue(e),"Buy enqueues through Engine"); auto buyID=EngineTestAccess::PendingID(e); w.Update(); WorldTestAccess::Inject(w,{buyID,p,CommandResultCode::GAMEPLAY_REJECTED}); EngineTestAccess::Sync(e); t.Expect(g.GetShopPresentationState().IsOpen(),"Rejected buy remains open"); t.Expect(EngineTestAccess::PendingID(e)==0,"Buy result consumed once"); EngineTestAccess::Sync(e); t.Expect(g.GetShopPresentationState().IsOpen(),"Duplicate result has no effect");
 GraphicsTestAccess::Click(g,row.x+row.w-4,row.y+4); t.Expect(EngineTestAccess::Enqueue(e),"Sell enqueues through Engine"); auto sellID=EngineTestAccess::PendingID(e); w.Update(); WorldTestAccess::Inject(w,{sellID,p,CommandResultCode::GAMEPLAY_REJECTED}); EngineTestAccess::Sync(e); t.Expect(g.GetShopPresentationState().IsOpen(),"Rejected sell remains open");
 auto close=g.GetShopCloseButtonRectangle(); GraphicsTestAccess::Click(g,close.x+2,close.y+2); t.Expect(EngineTestAccess::Enqueue(e),"Close enqueues through Engine"); auto closeID=EngineTestAccess::PendingID(e); w.Update(); WorldTestAccess::Inject(w,{closeID+1,p,CommandResultCode::GAMEPLAY_REJECTED}); EngineTestAccess::Sync(e); t.Expect(g.GetShopPresentationState().IsOpen(),"Mismatched result cannot reconcile close"); WorldTestAccess::Inject(w,{closeID,p,CommandResultCode::GAMEPLAY_REJECTED}); EngineTestAccess::Sync(e); t.Expect(!g.GetShopPresentationState().IsOpen(),"Rejected close dismisses");
 return t.Finish();
}
