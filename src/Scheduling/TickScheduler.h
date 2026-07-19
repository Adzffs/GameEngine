#pragma once

#include "ScheduledEvent.h"

#include <cstddef>
#include <cstdint>
#include <map>
#include <optional>
#include <unordered_map>
#include <vector>

class TickScheduler
{
public:
    std::uint64_t Schedule(
        int dueTick,
        int ownerEntityID,
        ScheduledEventData data);

    bool Cancel(std::uint64_t eventID);

    std::vector<ScheduledEvent> PopDueEvents(
        int currentTick);

    bool IsScheduled(std::uint64_t eventID) const;
    std::size_t GetScheduledCount() const;

    std::optional<ScheduledEvent> GetScheduledEvent(
        std::uint64_t eventID) const;

private:
    std::map<int, std::vector<ScheduledEvent>> eventsByDueTick;
    std::unordered_map<std::uint64_t, int> dueTicksByEventID;
    std::uint64_t nextEventID = 1;
};
