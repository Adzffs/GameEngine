#include "Skill.h"

#include <cmath>

Skill::Skill()
    : xp(0),
      level(1)
{
}

void Skill::AddXP(int amount)
{
    if (amount <= 0)
    {
        return;
    }

    xp += amount;

    while (level < 99 &&
           xp >= CalculateXPForLevel(level + 1))
    {
        level++;
    }
}

int Skill::GetXP() const
{
    return xp;
}

int Skill::GetLevel() const
{
    return level;
}

int Skill::GetXPForCurrentLevel() const
{
    return CalculateXPForLevel(level);
}

int Skill::GetXPForNextLevel() const
{
    if (level >= 99)
    {
        return xp;
    }

    return CalculateXPForLevel(level + 1);
}

int Skill::CalculateXPForLevel(int targetLevel)
{
    int points = 0;

    for (int currentLevel = 1;
         currentLevel < targetLevel;
         currentLevel++)
    {
        points += static_cast<int>(
            std::floor(
                currentLevel +
                300.0 *
                    std::pow(
                        2.0,
                        currentLevel / 7.0)));
    }

    return points / 4;
}