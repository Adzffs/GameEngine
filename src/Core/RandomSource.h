#pragma once

class RandomSource
{
public:
    virtual ~RandomSource() = default;

    // Inclusive range. If minimum > maximum, values are sampled from [maximum, minimum].
    virtual int NextIntInclusive(
        int minimum,
        int maximum) = 0;

    bool RollPercentage(int percentage)
    {
        if (percentage <= 0)
        {
            return false;
        }

        if (percentage >= 100)
        {
            return true;
        }

        const int roll = NextIntInclusive(
            1,
            100);

        return roll <= percentage;
    }
};
