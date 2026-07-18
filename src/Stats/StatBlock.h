#pragma once

#include "StatType.h"

#include <array>
#include <cstddef>

class StatBlock
{
public:
    static constexpr std::size_t StatCount =
        static_cast<std::size_t>(StatType::COUNT);

    StatBlock();

    int Get(StatType statType) const;

    void Set(
        StatType statType,
        int value);

    void Add(
        StatType statType,
        int amount);

    StatBlock &operator+=(
        const StatBlock &other);

private:
    static bool TryGetIndex(
        StatType statType,
        std::size_t &outIndex);

    std::array<int, StatCount> values;
};

StatBlock operator+(
    const StatBlock &left,
    const StatBlock &right);
