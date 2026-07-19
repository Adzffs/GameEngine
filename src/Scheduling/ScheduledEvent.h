#pragma once

#include <cstdint>
#include <variant>

struct MonsterRespawnScheduledEvent
{
    int monsterEntityID;
};

using ScheduledEventData = std::variant<
    MonsterRespawnScheduledEvent>;

struct ScheduledEvent
{
    std::uint64_t eventID;
    int dueTick;
    int ownerEntityID;
    ScheduledEventData data;
};
