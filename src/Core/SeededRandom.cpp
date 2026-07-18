#include "SeededRandom.h"

#include <algorithm>

SeededRandom::SeededRandom(unsigned int seed)
    : generator(seed)
{
}

int SeededRandom::NextIntInclusive(
    int minimum,
    int maximum)
{
    if (minimum > maximum)
    {
        std::swap(minimum, maximum);
    }

    if (minimum == maximum)
    {
        return minimum;
    }

    std::uniform_int_distribution<int> distribution(
        minimum,
        maximum);

    return distribution(generator);
}
