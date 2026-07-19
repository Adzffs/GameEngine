#pragma once

#include <chrono>
#include <cstdint>

struct TickPerformanceStats
{
    std::chrono::microseconds lastDuration{};
    std::chrono::microseconds maximumDuration{};
    std::uint64_t overrunCount = 0;
};

bool RecordTickDuration(
    TickPerformanceStats &stats,
    std::chrono::microseconds updateDuration,
    std::chrono::milliseconds tickBudget);
