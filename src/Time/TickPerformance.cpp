#include "TickPerformance.h"

bool RecordTickDuration(
    TickPerformanceStats &stats,
    std::chrono::microseconds updateDuration,
    std::chrono::milliseconds tickBudget)
{
    if (updateDuration < std::chrono::microseconds::zero())
    {
        updateDuration = std::chrono::microseconds::zero();
    }

    stats.lastDuration = updateDuration;

    if (updateDuration > stats.maximumDuration)
    {
        stats.maximumDuration = updateDuration;
    }

    const bool overrun =
        updateDuration >
        std::chrono::duration_cast<std::chrono::microseconds>(
            tickBudget);

    if (overrun)
    {
        stats.overrunCount++;
    }

    return overrun;
}
