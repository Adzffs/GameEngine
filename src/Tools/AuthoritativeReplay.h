#pragma once

#include "Command/ServerCommand.h"
#include "Player/Player.h"
#include "World/World.h"
#include <algorithm>
#include <array>
#include <cstdlib>
#include <stdexcept>
#include <utility>

// Bounded command/tick driver shared by production content replays.
struct AuthoritativeReplay
{
    World &world;
    Player &player;
    int ticks = 0;
    int gatheringAttempts = 0;

    void Step(bool allowDeath = false)
    {
        if (++ticks > 20000)
            throw std::runtime_error("bounded replay exceeded 20000 ticks");
        world.Update();
        for (const auto &event : world.GetActionLifecycleEvents())
            if (const auto *completed = std::get_if<ActionCompletedEvent>(&event.data);
                completed != nullptr && completed->ownerEntityID == player.GetID() &&
                completed->actionType == ActionType::GATHERING)
                ++gatheringAttempts;
        if (!allowDeath && !player.IsAlive())
            throw std::runtime_error("replay character died");
    }

    void Submit(ServerCommandData command)
    {
        const auto id = world.EnqueueCommand(std::move(command));
        Step();
        for (const auto &result : world.GetCommandProcessingResults())
            if (result.commandID == id && result.resultCode == CommandResultCode::ACCEPTED)
                return;
        throw std::runtime_error("authoritative replay command was rejected");
    }

    ShopSessionId OpenShop(int guideId)
    {
        Submit(NpcInteractionCommand{player.GetID(), guideId, NpcInteractionType::TRADE});
        for (int i = 0; i < 2000; ++i)
        {
            if (const auto *session = world.GetActiveShopSession(player.GetID()))
                return session->sessionId;
            Step();
        }
        throw std::runtime_error("could not approach/open the supplies shop");
    }

    void OpenStation(int stationId, StationType expectedType = StationType::CRAFTING_TABLE)
    {
        const CraftingStation *target = nullptr;
        for (const auto &station : world.GetStations())
            if (station.GetID() == stationId) target = &station;
        if (target == nullptr) throw std::runtime_error("required station is missing");
        Submit(InteractCommand{player.GetID(), InteractionTargetType::STATION,
            stationId, Position{target->GetX(), target->GetY()}});
        for (int i = 0; i < 2000; ++i)
        {
            Step();
            StationType type = StationType::NONE;
            if (world.ConsumeOpenedStation(player.GetID(), type) && type == expectedType)
                return;
        }
        throw std::runtime_error("could not approach/open the required station");
    }

    void Craft(const std::array<ItemType, 9> &grid)
    {
        Submit(CraftRecipeCommand{player.GetID(), grid});
        FinishAction();
    }

    void FinishAction()
    {
        for (int i = 0; world.GetActionForEntity(player.GetID()) != nullptr; ++i)
        {
            if (i >= 100)
                throw std::runtime_error("craft action did not finish");
            Step();
        }
    }

    DialogueSessionId Talk(int guideId)
    {
        Submit(NpcInteractionCommand{player.GetID(), guideId, NpcInteractionType::TALK});
        for (int i = 0; i < 2000; ++i)
        {
            if (const auto *session = world.GetActiveDialogueSession(player.GetID()))
                return session->sessionId;
            Step();
        }
        throw std::runtime_error("could not approach/talk to Mara");
    }

    int Slot(ItemType item) const
    {
        const auto &slots = player.GetInventory().GetSlots();
        for (int i = 0; i < Inventory::SlotCount; ++i)
            if (slots[i].GetItemType() == item && !slots[i].IsEmpty()) return i;
        throw std::runtime_error("required replay inventory item is missing");
    }

    void Equip(ItemType item)
    {
        Submit(UseInventoryItemCommand{player.GetID(), Slot(item)});
        if (!player.GetEquipment().IsEquipped(item))
            throw std::runtime_error("starter equipment did not equip");
    }

    void Gather(ResourceType type, ItemType item, int quantity)
    {
        while (player.GetInventory().GetItemAmount(item) < quantity)
        {
            const ResourceNode *closest = nullptr;
            int distance = 1000000;
            for (const auto &resource : world.GetResources())
                if (resource.GetResourceType() == type && resource.IsActive())
                {
                    const int candidate = std::abs(resource.GetX() - player.GetPosition().GetX()) +
                        std::abs(resource.GetY() - player.GetPosition().GetY());
                    if (candidate < distance) { closest = &resource; distance = candidate; }
                }
            if (closest == nullptr) { Step(); continue; }
            const int before = player.GetInventory().GetItemAmount(item);
            Submit(InteractCommand{player.GetID(), InteractionTargetType::RESOURCE,
                closest->GetID(), Position{closest->GetX(), closest->GetY()}});
            for (int i = 0; player.GetInventory().GetItemAmount(item) == before; ++i)
            {
                if (i >= 2000) throw std::runtime_error("starter gathering did not produce its next item");
                Step();
            }
        }
    }
};

