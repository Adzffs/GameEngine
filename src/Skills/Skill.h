#pragma once

class Skill
{
public:
    Skill();

    void AddXP(int amount);

    int GetXP() const;
    int GetLevel() const;
    int GetXPForCurrentLevel() const;
    int GetXPForNextLevel() const;

private:
    static int CalculateXPForLevel(int level);

    int xp;
    int level;
};