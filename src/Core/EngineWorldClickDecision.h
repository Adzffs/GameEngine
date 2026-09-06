#pragma once

#include <optional>

#include "../Command/ServerCommand.h"
#include "../Player/Player.h"
#include "../World/World.h"

namespace EngineWorldClickDecision
{
    inline bool Route(
        World &world,
        int playerID,
        int clickedTileX,
        int clickedTileY,
        std::optional<int> monsterID,
        std::optional<int> friendlyNpcID)
    {
        Player *player = dynamic_cast<Player *>(world.GetEntityByID(playerID));
        if (player == nullptr || !player->IsAlive()) return true;

        ResourceNode *resource = world.GetResourceAt(clickedTileX, clickedTileY);
        CraftingStation *station = world.GetStationAt(clickedTileX, clickedTileY);

        if (resource != nullptr && resource->IsActive())
        {
            world.EnqueueCommand(InteractCommand{
                playerID, InteractionTargetType::RESOURCE, resource->GetID(),
                Position(clickedTileX, clickedTileY)});
            return true;
        }
        if (station != nullptr)
        {
            world.EnqueueCommand(InteractCommand{
                playerID, InteractionTargetType::STATION, station->GetID(),
                Position(clickedTileX, clickedTileY)});
            return true;
        }
        if (monsterID.has_value())
        {
            world.EnqueueCommand(AttackCommand{playerID, *monsterID});
            return true;
        }
        if (friendlyNpcID.has_value())
        {
            world.EnqueueCommand(NpcInteractionCommand{
                playerID, *friendlyNpcID, NpcInteractionType::TALK});
            return true;
        }

        world.EnqueueCommand(MoveCommand{
            playerID, Position(clickedTileX, clickedTileY)});
        return true;
    }
}
