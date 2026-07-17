#pragma once

#include <algorithm>
#include <random>

class Random
{
public:
    static bool RollPercentage(int chance)
    {
        chance = std::clamp(
            chance,
            0,
            100);

        static std::mt19937 generator(
            std::random_device{}());

        std::uniform_int_distribution<int>
            distribution(1, 100);

        return distribution(generator) <= chance;
    }
};