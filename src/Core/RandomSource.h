#pragma once

class RandomSource
{
public:
    virtual ~RandomSource() = default;

    // Inclusive range. If minimum > maximum, values are sampled from [maximum, minimum].
    virtual int NextIntInclusive(
        int minimum,
        int maximum) = 0;
};
