#include "ShopPresentationState.h"

namespace {
bool Matches(const ShopOpenedEvent& e, int actor, const ActiveShopSession& s) {
    return e.actorEntityID == actor && e.actorEntityID == s.actorEntityID &&
        e.npcEntityID == s.npcEntityID && e.npcType == s.npcType &&
        e.shopId == s.shopId && e.sessionId == s.sessionId;
}
}

void ShopPresentationState::Synchronize(int actor,const std::vector<ShopOpenedEvent>& events,const ActiveShopSession* active){
 const ShopOpenedEvent* latest=nullptr; for(const auto& e:events) if(e.actorEntityID==actor) latest=&e;
 if(latest){
  rejectionVisible=false;
  if(latest->sessionId==dismissed){event.reset();return;}
  if(!active || !Matches(*latest,actor,*active)){event.reset();return;}
  event=*latest; return;
 }
 if(event && (!active || !Matches(*event,actor,*active))){event.reset();rejectionVisible=false;}
}
std::optional<ServerCommandData> ShopPresentationState::MakeBuy(std::size_t i,int q) const{ if(!event||i>=event->entries.size()||q<=0||!event->entries[i].buyPrice.has_value())return std::nullopt; return ShopBuyCommand{event->actorEntityID,event->sessionId,event->entries[i].itemType,q}; }
std::optional<ServerCommandData> ShopPresentationState::MakeSell(std::size_t i,int q) const{ if(!event||i>=event->entries.size()||q<=0||!event->entries[i].sellPrice.has_value())return std::nullopt; return ShopSellCommand{event->actorEntityID,event->sessionId,event->entries[i].itemType,q}; }
std::optional<ServerCommandData> ShopPresentationState::MakeClose() const{ if(!event)return std::nullopt; return ShopCloseCommand{event->actorEntityID,event->sessionId}; }
