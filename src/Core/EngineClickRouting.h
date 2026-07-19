#pragma once

#include <optional>

#include "../Command/ServerCommand.h"
#include "../Graphics/Graphics.h"
#include "../Player/Player.h"
#include "../World/World.h"

namespace EngineClickRouting
{
    inline bool HandleWorldClick(
        Graphics &graphics,
        World &world,
        int playerID,
        int clickedTileX,
        int clickedTileY,
        int mouseX,
        int mouseY)
    {
        Entity *entity =
            world.GetEntityByID(playerID);

        Player *player =
            dynamic_cast<Player *>(entity);

        if (player == nullptr ||
            !player->IsAlive())
        {
            return true;
        }

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
            world.EnqueueCommand(
                InteractCommand{
                    playerID,
                    InteractionTargetType::RESOURCE,
                    resource->GetID(),
                    Position(clickedTileX, clickedTileY)});

            return true;
        }
        else if (station != nullptr)
        {
            world.EnqueueCommand(
                InteractCommand{
                    playerID,
                    InteractionTargetType::STATION,
                    station->GetID(),
                    Position(clickedTileX, clickedTileY)});

            return true;
        }

        std::optional<int> monsterID =
            graphics.GetMonsterAtScreenPosition(
                mouseX,
                mouseY,
                world.GetEntities());

        if (monsterID.has_value())
        {
            world.EnqueueCommand(
                AttackCommand{
                    playerID,
                    monsterID.value()});

            return true;
        }

        world.EnqueueCommand(
            MoveCommand{
                playerID,
                Position(clickedTileX, clickedTileY)});

        return true;
    }
}
