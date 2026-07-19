#include "Clock.h"

#include <algorithm>

Clock::Clock()
    : Clock(ClockType::now())
{
}

Clock::Clock(TimePoint startTime)
    : nextTickDeadline(startTime + TickInterval)
{
}

bool Clock::IsTickDue() const
{
    return IsTickDue(ClockType::now());
}

bool Clock::IsTickDue(TimePoint now) const
{
    return now >= nextTickDeadline;
}

void Clock::AdvanceTickDeadline()
{
    nextTickDeadline += TickInterval;
}

int Clock::GetDueTickCount(
    int maxDueTicks) const
{
    return GetDueTickCount(
        ClockType::now(),
        maxDueTicks);
}

int Clock::GetDueTickCount(
    TimePoint now,
    int maxDueTicks) const
{
    if (maxDueTicks <= 0)
    {
        return 0;
    }

    if (now < nextTickDeadline)
    {
        return 0;
    }

    const Duration overdueDuration =
        now - nextTickDeadline;

    const auto elapsedIntervals =
        overdueDuration / TickInterval;

    long long dueCount =
        static_cast<long long>(elapsedIntervals) + 1LL;

    if (dueCount < 0)
    {
        return 0;
    }

    const int maxSafe =
        std::min(
            maxDueTicks,
            std::numeric_limits<int>::max());

    if (dueCount > static_cast<long long>(maxSafe))
    {
        return maxSafe;
    }

    return static_cast<int>(dueCount);
}

void Clock::ResynchroniseFromNow()
{
    Resynchronise(ClockType::now());
}

void Clock::Resynchronise(TimePoint now)
{
    nextTickDeadline = now + TickInterval;
}

Clock::TimePoint Clock::GetNextTickDeadline() const
{
    return nextTickDeadline;
}

Clock::Duration Clock::GetTickInterval() const
{
    return TickInterval;
}

Clock::Duration Clock::GetBacklogDuration(TimePoint now) const
{
    if (now <= nextTickDeadline)
    {
        return Duration::zero();
    }

    return now - nextTickDeadline;
}