#include "TestSupport.h"

#include "../src/Time/Clock.h"
#include "../src/Time/TickPerformance.h"

#include <chrono>
#include <cstdint>
#include <limits>
#include <type_traits>

int main()
{
    static_assert(
        std::is_same_v<
            Clock::ClockType,
            std::chrono::steady_clock>,
        "Clock must use steady_clock for monotonic pacing");

    TestContext test;

    using namespace std::chrono;
    using TimePoint = Clock::TimePoint;

    const TimePoint origin =
        TimePoint{};

    {
        Clock clock(origin);

        test.Expect(
            !clock.IsTickDue(
                origin + milliseconds(599)),
            "Clock is not due before first deadline");
        test.Expect(
            clock.IsTickDue(
                origin + milliseconds(600)),
            "Clock is due exactly at first deadline");
    }

    {
        Clock clock(origin);

        TimePoint firstDeadline =
            clock.GetNextTickDeadline();

        clock.AdvanceTickDeadline();

        TimePoint secondDeadline =
            clock.GetNextTickDeadline();

        test.ExpectEqual(
            duration_cast<milliseconds>(
                secondDeadline - firstDeadline)
                .count(),
            600LL,
            "Advancing one deadline adds exactly 600 ms");
    }

    {
        Clock clock(origin);

        const TimePoint lateNow =
            origin + milliseconds(620);

        test.Expect(
            clock.IsTickDue(lateNow),
            "A late due check still reports due");

        clock.AdvanceTickDeadline();

        test.ExpectEqual(
            duration_cast<milliseconds>(
                clock.GetNextTickDeadline() - origin)
                .count(),
            1200LL,
            "A 20 ms late check does not shift the next fixed deadline");
    }

    {
        Clock clock(origin);

        for (int index = 0; index < 5; ++index)
        {
            clock.AdvanceTickDeadline();
        }

        test.ExpectEqual(
            duration_cast<milliseconds>(
                clock.GetNextTickDeadline() - origin)
                .count(),
            3600LL,
            "Repeated deadline advances stay aligned to fixed timeline");
    }

    {
        Clock clock(origin);

        const int dueCount =
            clock.GetDueTickCount(
                origin + milliseconds(2500),
                std::numeric_limits<int>::max());

        test.ExpectEqual(
            dueCount,
            4,
            "Large delay reports multiple due intervals");
    }

    {
        Clock clock(origin);
        const TimePoint now =
            origin + milliseconds(2500);

        const int first =
            clock.GetDueTickCount(
                now,
                100);
        const int second =
            clock.GetDueTickCount(
                now,
                100);

        test.ExpectEqual(
            first,
            second,
            "Catch-up due-count calculation is deterministic");

        const int capped =
            clock.GetDueTickCount(
                now,
                3);

        test.ExpectEqual(
            capped,
            3,
            "Catch-up due-count is capped at configured maximum");
    }

    {
        Clock clock(origin);

        const TimePoint resyncNow =
            origin + milliseconds(5000);

        clock.Resynchronise(resyncNow);

        test.ExpectEqual(
            duration_cast<milliseconds>(
                clock.GetNextTickDeadline() - resyncNow)
                .count(),
            600LL,
            "Resynchronisation moves next deadline relative to supplied now");
    }

    {
        Clock clock(origin);

        test.ExpectEqual(
            clock.GetDueTickCount(
                origin + milliseconds(200),
                50),
            0,
            "No due-count is produced before deadline");
        test.ExpectEqual(
            clock.GetDueTickCount(
                origin + milliseconds(3000),
                0),
            0,
            "Non-positive max due tick cap returns zero");
        test.ExpectEqual(
            clock.GetDueTickCount(
                origin + hours(24),
                2),
            2,
            "Large time gaps still respect explicit due-count caps");
    }

    {
        TickPerformanceStats stats;

        const bool overrun =
            RecordTickDuration(
                stats,
                microseconds(500000),
                milliseconds(600));

        test.Expect(
            !overrun,
            "Duration below budget does not report overrun");
        test.ExpectEqual(
            stats.lastDuration.count(),
            500000LL,
            "Last duration updates from recorded tick");
        test.ExpectEqual(
            stats.maximumDuration.count(),
            500000LL,
            "Maximum duration updates on first record");
        test.ExpectEqual(
            static_cast<long long>(stats.overrunCount),
            0LL,
            "Duration below budget does not increment overrun count");
    }

    {
        TickPerformanceStats stats;

        RecordTickDuration(
            stats,
            microseconds(500000),
            milliseconds(600));
        RecordTickDuration(
            stats,
            microseconds(550000),
            milliseconds(600));

        test.ExpectEqual(
            stats.maximumDuration.count(),
            550000LL,
            "Maximum duration increases when larger duration is observed");

        RecordTickDuration(
            stats,
            microseconds(400000),
            milliseconds(600));

        test.ExpectEqual(
            stats.maximumDuration.count(),
            550000LL,
            "Maximum duration does not decrease on shorter updates");
    }

    {
        TickPerformanceStats stats;

        const bool equalBudgetOverrun =
            RecordTickDuration(
                stats,
                microseconds(600000),
                milliseconds(600));

        test.Expect(
            !equalBudgetOverrun,
            "Equal-to-budget duration is not treated as overrun");
        test.ExpectEqual(
            static_cast<long long>(stats.overrunCount),
            0LL,
            "Equal-to-budget duration does not increment overrun count");

        const bool aboveBudgetOverrun =
            RecordTickDuration(
                stats,
                microseconds(600001),
                milliseconds(600));

        test.Expect(
            aboveBudgetOverrun,
            "Duration above budget is treated as overrun");
        test.ExpectEqual(
            static_cast<long long>(stats.overrunCount),
            1LL,
            "Duration above budget increments overrun count exactly once");
    }

    {
        TickPerformanceStats stats;
        int worldTick = 123;

        RecordTickDuration(
            stats,
            microseconds(1000),
            milliseconds(600));

        test.ExpectEqual(
            worldTick,
            123,
            "Tick performance stats update does not own or modify world tick state");
    }

    return test.Finish();
}
