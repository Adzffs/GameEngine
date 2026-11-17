#pragma once

#include <optional>

#include "../Command/ServerCommand.h"
#include "../Graphics/Graphics.h"
#include "../Player/Player.h"
#include "../World/World.h"
#include "EngineWorldClickDecision.h"

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
        if (const GroundItem *groundItem =
                world.GetGroundItemAt(clickedTileX, clickedTileY);
            groundItem != nullptr)
        {
            world.EnqueueCommand(TakeGroundItemCommand{
                playerID, groundItem->id});
            return true;
        }

        std::optional<int> monsterID =
            graphics.GetMonsterAtScreenPosition(
                mouseX,
                mouseY,
                world.GetEntities());

        std::optional<int> friendlyNpcID =
            graphics.GetFriendlyNpcAtScreenPosition(
                mouseX,
                mouseY,
                world.GetEntities());

        return EngineWorldClickDecision::Route(
            world, playerID, clickedTileX, clickedTileY,
            monsterID, friendlyNpcID);
    }
}
