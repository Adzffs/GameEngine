#pragma once

enum class TutorialStep
{
    MEET_GUIDE = 0,
    GATHER_LOG,
    MAKE_PLANKS,
    MINE_ORES,
    SMELT_BRONZE,
    CRAFT_WEAPON,
    EQUIP_WEAPON,
    SAFE_COMBAT,
    LOOT_AND_BANK,
    RETURN_AND_CHOOSE,
    COMPLETE
};

class TutorialState
{
public:
    TutorialStep GetStep() const { return step; }
    bool IsCompleted() const { return step == TutorialStep::COMPLETE; }
    bool IsSkipped() const { return skipped; }

    bool Advance()
    {
        if (IsCompleted()) return false;
        step = static_cast<TutorialStep>(static_cast<int>(step) + 1);
        return true;
    }

    void Skip()
    {
        step = TutorialStep::COMPLETE;
        skipped = true;
    }

    bool TryRestore(TutorialStep restoredStep, bool restoredSkipped)
    {
        const int value = static_cast<int>(restoredStep);
        if (value < static_cast<int>(TutorialStep::MEET_GUIDE) ||
            value > static_cast<int>(TutorialStep::COMPLETE) ||
            (restoredSkipped && restoredStep != TutorialStep::COMPLETE))
            return false;
        step = restoredStep;
        skipped = restoredSkipped;
        return true;
    }

private:
    TutorialStep step = TutorialStep::MEET_GUIDE;
    bool skipped = false;
};
