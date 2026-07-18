#pragma once

struct EntityDiedEvent
{
    static constexpr int InvalidKillerEntityID = -1;

    const int deadEntityID;
    const int killerEntityID;
    const int deathTick;

    EntityDiedEvent(
        int deadEntityID,
        int killerEntityID,
        int deathTick)
        : deadEntityID(deadEntityID),
          killerEntityID(killerEntityID),
          deathTick(deathTick)
    {
    }
};
