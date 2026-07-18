#include "StatBlock.h"

StatBlock::StatBlock()
{
    values.fill(0);
}

int StatBlock::Get(StatType statType) const
{
    std::size_t index = 0;

    if (!TryGetIndex(
            statType,
            index))
    {
        return 0;
    }

    return values[index];
}

void StatBlock::Set(
    StatType statType,
    int value)
{
    std::size_t index = 0;

    if (!TryGetIndex(
            statType,
            index))
    {
        return;
    }

    values[index] = value;
}

void StatBlock::Add(
    StatType statType,
    int amount)
{
    std::size_t index = 0;

    if (!TryGetIndex(
            statType,
            index))
    {
        return;
    }

    values[index] += amount;
}

StatBlock &StatBlock::operator+=(
    const StatBlock &other)
{
    for (std::size_t index = 0;
         index < values.size();
         ++index)
    {
        values[index] +=
            other.values[index];
    }

    return *this;
}

bool StatBlock::TryGetIndex(
    StatType statType,
    std::size_t &outIndex)
{
    int rawValue =
        static_cast<int>(statType);

    if (rawValue < 0)
    {
        return false;
    }

    std::size_t index =
        static_cast<std::size_t>(rawValue);

    if (index >= StatCount)
    {
        return false;
    }

    outIndex = index;
    return true;
}

StatBlock operator+(
    const StatBlock &left,
    const StatBlock &right)
{
    StatBlock combined = left;
    combined += right;
    return combined;
}
