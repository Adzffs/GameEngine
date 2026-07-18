#pragma once

#include <optional>

#include "../Action/ActionCancelReason.h"
#include "../Graphics/Graphics.h"
#include "../Movement/MovementDestinationRequest.h"
#include "../World/World.h"

namespace EngineClickRouting
{
    constexpr int DEFAULT_MELEE_ATTACK_DURATION_TICKS = 4;

    inline bool HandleWorldClick(
        Graphics &graphics,
        World &world,
        int playerID,
        int clickedTileX,
        int clickedTileY,
        int mouseX,
        int mouseY)
    {
        ResourceNode *resource =
            world.GetResourceAt(
                clickedTileX,
                clickedTileY);

        CraftingStation *station =
            world.GetStationAt(
                clickedTileX,
                clickedTileY);

        if (resource != nullptr &&
            resource->IsActive())
        {
            world.CancelActionsForEntity(
                playerID,
                ActionCancelReason::PLAYER_MOVED);

            world.CloseStationInteraction(
                playerID);

            world.ClearPendingResourceInteraction(
                playerID);

            world.QueueResourceInteraction(
                playerID,
                resource->GetID());

            MovementDestinationRequest request(
                playerID,
                clickedTileX,
                clickedTileY);

            world.QueueMovementDestination(request);

            return true;
        }
        else if (station != nullptr)
        {
            world.CancelActionsForEntity(
                playerID,
                ActionCancelReason::PLAYER_MOVED);

            world.CloseStationInteraction(
                playerID);

            world.ClearPendingResourceInteraction(
                playerID);

            world.QueueStationInteraction(
                playerID,
                station->GetID());

            MovementDestinationRequest request(
                playerID,
                clickedTileX,
                clickedTileY);

            world.QueueMovementDestination(request);

            return true;
        }

        std::optional<int> monsterID =
            graphics.GetMonsterAtScreenPosition(
                mouseX,
                mouseY,
                world.GetEntities());

        if (monsterID.has_value())
        {
            world.QueueMeleeEngagementRequest(
                playerID,
                monsterID.value(),
                DEFAULT_MELEE_ATTACK_DURATION_TICKS);

            return true;
        }

        world.CancelActionsForEntity(
            playerID,
            ActionCancelReason::PLAYER_MOVED);

        world.CloseStationInteraction(
            playerID);

        world.ClearPendingResourceInteraction(
            playerID);

        MovementDestinationRequest request(
            playerID,
            clickedTileX,
            clickedTileY);

        world.QueueMovementDestination(request);

        return true;
    }
}