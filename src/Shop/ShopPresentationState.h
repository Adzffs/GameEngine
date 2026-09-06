#pragma once
#include "ShopOpenedEvent.h"
#include "../Command/ServerCommand.h"
#include "../Shop/ShopSystem.h"
#include <optional>
#include <vector>
class ShopPresentationState {
public:
 void Synchronize(int actor,const std::vector<ShopOpenedEvent>& events,const ActiveShopSession* active);
 bool IsOpen() const{return event.has_value();}
 const ShopOpenedEvent* GetEvent() const{return event?&*event:nullptr;}
 std::optional<ServerCommandData> MakeBuy(std::size_t i,int quantity=1) const;
 std::optional<ServerCommandData> MakeSell(std::size_t i,int quantity=1) const;
 std::optional<ServerCommandData> MakeClose() const;
 void Dismiss(){if(event)dismissed=event->sessionId; event.reset(); rejectionVisible=false;}
 void ReconcileRejectedCommand() { rejectionVisible = true; }
 bool HasRejection() const { return rejectionVisible; }
 void ClearRejection() { rejectionVisible = false; }
private: std::optional<ShopOpenedEvent> event; ShopSessionId dismissed=InvalidShopSessionId; bool rejectionVisible=false;
};
