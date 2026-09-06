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
