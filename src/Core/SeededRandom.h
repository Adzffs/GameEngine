#pragma once

#include "RandomSource.h"

#include <random>

class SeededRandom : public RandomSource
{
public:
    explicit SeededRandom(unsigned int seed);

    int NextIntInclusive(
        int minimum,
        int maximum) override;

private:
    std::mt19937 generator;
};
