#include "TestSupport.h"

#include "../src/Scheduling/TickScheduler.h"

#include <cstdint>
#include <type_traits>
#include <utility>
#include <vector>

namespace
{
    ScheduledEventData Respawn(int entityID)
    {
        return MonsterRespawnScheduledEvent{entityID};
    }
}

int main()
{
    TestContext test;
    TickScheduler scheduler;

    test.ExpectEqual(scheduler.GetScheduledCount(), std::size_t{0},
                     "Scheduler starts empty");
    test.Expect(scheduler.PopDueEvents(0).empty(),
                "Empty scheduler has no due work");

    const std::uint64_t first = scheduler.Schedule(3, 41, Respawn(41));
    const std::uint64_t second = scheduler.Schedule(3, 42, Respawn(42));
    test.ExpectEqual(first, std::uint64_t{1}, "First event ID is one");
    test.ExpectEqual(second, std::uint64_t{2}, "Event IDs increase deterministically");
    test.ExpectEqual(scheduler.Schedule(-1, 0, Respawn(1)), std::uint64_t{0},
                     "Negative due tick is rejected safely");
    test.ExpectEqual(scheduler.GetScheduledCount(), std::size_t{2},
                     "Rejected scheduling does not consume an ID or add work");
    test.Expect(scheduler.IsScheduled(first), "Scheduling records pending work");
    test.Expect(scheduler.PopDueEvents(2).empty(), "Event does not fire before due tick");

    std::vector<ScheduledEvent> due = scheduler.PopDueEvents(3);
    test.ExpectEqual(due.size(), std::size_t{2}, "Events fire exactly at due tick");
    test.ExpectEqual(due[0].eventID, first, "Equal due ticks use event ID order");
    test.ExpectEqual(due[1].eventID, second, "Equal due ordering remains deterministic");
    test.ExpectEqual(due[0].ownerEntityID, 41, "Owner ID survives scheduling");
    test.ExpectEqual(std::get<MonsterRespawnScheduledEvent>(due[0].data).monsterEntityID,
                     41, "Payload survives scheduling");
    test.Expect(!scheduler.IsScheduled(first), "Fired event is no longer scheduled");
    test.Expect(scheduler.PopDueEvents(3).empty(), "Event fires exactly once");

    const std::uint64_t later = scheduler.Schedule(10, 1, Respawn(1));
    const std::uint64_t earlier = scheduler.Schedule(5, 2, Respawn(2));
    test.ExpectEqual(later, std::uint64_t{3},
                     "Rejected schedule does not consume an event ID");
    due = scheduler.PopDueEvents(20);
    test.ExpectEqual(due.size(), std::size_t{2}, "Overdue events fire");
    test.ExpectEqual(due[0].eventID, earlier, "Different due ticks use due-tick order");
    test.ExpectEqual(due[1].eventID, later, "Later due tick follows earlier due tick");

    const std::uint64_t cancelled = scheduler.Schedule(30, 3, Respawn(3));
    test.Expect(scheduler.Cancel(cancelled), "Cancelling existing event succeeds");
    test.Expect(!scheduler.Cancel(cancelled), "Cancelling twice fails safely");
    test.Expect(!scheduler.Cancel(999999), "Cancelling unknown event fails");
    test.Expect(scheduler.PopDueEvents(30).empty(), "Cancelled event never fires");

    const std::uint64_t batched = scheduler.Schedule(40, 4, Respawn(4));
    due = scheduler.PopDueEvents(40);
    const std::uint64_t scheduledDuringProcessing =
        scheduler.Schedule(40, 5, Respawn(5));
    test.ExpectEqual(due.size(), std::size_t{1},
                     "Due processing uses a fixed batch");
    test.ExpectEqual(due[0].eventID, batched,
                     "New same-tick work is absent from current batch");
    due = scheduler.PopDueEvents(40);
    test.ExpectEqual(due[0].eventID, scheduledDuringProcessing,
                     "Same-tick work waits for next pop phase");

    static_assert(std::is_same_v<
                  decltype(std::declval<const TickScheduler &>().GetScheduledCount()),
                  std::size_t>);
    static_assert(std::is_same_v<
                  decltype(std::declval<const TickScheduler &>().GetScheduledEvent(1)),
                  std::optional<ScheduledEvent>>);

    std::vector<std::uint64_t> batchIDs;
    for (int index = 0; index < 1000; ++index)
    {
        batchIDs.push_back(scheduler.Schedule(100 + (index % 7), index, Respawn(index)));
    }
    due = scheduler.PopDueEvents(106);
    test.ExpectEqual(due.size(), std::size_t{1000},
                     "Large reasonable batch returns every event");
    bool ordered = true;
    for (std::size_t index = 1; index < due.size(); ++index)
    {
        ordered = ordered &&
                  (due[index - 1].dueTick < due[index].dueTick ||
                   (due[index - 1].dueTick == due[index].dueTick &&
                    due[index - 1].eventID < due[index].eventID));
    }
    test.Expect(ordered, "Large batch preserves deterministic ordering");
    test.ExpectEqual(scheduler.GetScheduledCount(), std::size_t{0},
                     "No permanent fired-event history remains");

    return test.Finish();
}
