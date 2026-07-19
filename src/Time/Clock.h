#pragma once

#include <chrono>
#include <limits>

class Clock
{
public:
    using ClockType = std::chrono::steady_clock;
    using TimePoint = ClockType::time_point;
    using Duration = ClockType::duration;

    Clock();
    explicit Clock(TimePoint startTime);

    bool IsTickDue() const;
    bool IsTickDue(TimePoint now) const;

    void AdvanceTickDeadline();

    int GetDueTickCount(int maxDueTicks) const;
    int GetDueTickCount(
        TimePoint now,
        int maxDueTicks) const;

    void ResynchroniseFromNow();
    void Resynchronise(TimePoint now);

    TimePoint GetNextTickDeadline() const;
    Duration GetTickInterval() const;
    Duration GetBacklogDuration(TimePoint now) const;

private:
    TimePoint nextTickDeadline;

    static constexpr std::chrono::milliseconds TickInterval{
        600};
};