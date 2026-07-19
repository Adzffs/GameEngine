#include "TickScheduler.h"

#include <algorithm>
#include <utility>

std::uint64_t TickScheduler::Schedule(
    int dueTick,
    int ownerEntityID,
    ScheduledEventData data)
{
    if (dueTick < 0)
    {
        return 0;
    }

    const std::uint64_t eventID = nextEventID;
    ScheduledEvent event{
        eventID,
        dueTick,
        ownerEntityID,
        std::move(data)};

    eventsByDueTick[dueTick].push_back(std::move(event));
    dueTicksByEventID.emplace(eventID, dueTick);
    ++nextEventID;
    return eventID;
}

bool TickScheduler::Cancel(std::uint64_t eventID)
{
    auto indexIterator = dueTicksByEventID.find(eventID);
    if (indexIterator == dueTicksByEventID.end())
    {
        return false;
    }

    auto bucketIterator = eventsByDueTick.find(indexIterator->second);
    if (bucketIterator == eventsByDueTick.end())
    {
        dueTicksByEventID.erase(indexIterator);
        return false;
    }

    auto &events = bucketIterator->second;
    auto eventIterator = std::find_if(
        events.begin(),
        events.end(),
        [eventID](const ScheduledEvent &event)
        {
            return event.eventID == eventID;
        });

    if (eventIterator == events.end())
    {
        dueTicksByEventID.erase(indexIterator);
        return false;
    }

    events.erase(eventIterator);
    dueTicksByEventID.erase(indexIterator);
    if (events.empty())
    {
        eventsByDueTick.erase(bucketIterator);
    }
    return true;
}

std::vector<ScheduledEvent> TickScheduler::PopDueEvents(
    int currentTick)
{
    std::vector<ScheduledEvent> dueEvents;
    auto iterator = eventsByDueTick.begin();
    while (iterator != eventsByDueTick.end() &&
           iterator->first <= currentTick)
    {
        for (ScheduledEvent &event : iterator->second)
        {
            dueTicksByEventID.erase(event.eventID);
            dueEvents.push_back(std::move(event));
        }
        iterator = eventsByDueTick.erase(iterator);
    }
    return dueEvents;
}

bool TickScheduler::IsScheduled(std::uint64_t eventID) const
{
    return dueTicksByEventID.find(eventID) !=
           dueTicksByEventID.end();
}

std::size_t TickScheduler::GetScheduledCount() const
{
    return dueTicksByEventID.size();
}

std::optional<ScheduledEvent> TickScheduler::GetScheduledEvent(
    std::uint64_t eventID) const
{
    auto indexIterator = dueTicksByEventID.find(eventID);
    if (indexIterator == dueTicksByEventID.end())
    {
        return std::nullopt;
    }

    auto bucketIterator = eventsByDueTick.find(indexIterator->second);
    if (bucketIterator == eventsByDueTick.end())
    {
        return std::nullopt;
    }

    auto eventIterator = std::find_if(
        bucketIterator->second.begin(),
        bucketIterator->second.end(),
        [eventID](const ScheduledEvent &event)
        {
            return event.eventID == eventID;
        });
    return eventIterator == bucketIterator->second.end()
               ? std::nullopt
               : std::optional<ScheduledEvent>(*eventIterator);
}
