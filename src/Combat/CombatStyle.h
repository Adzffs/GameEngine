#pragma once

enum class CombatStyle
{
    ACCURATE,
    AGGRESSIVE,
    DEFENSIVE
};

inline bool IsValidCombatStyle(CombatStyle style)
{
    return style == CombatStyle::ACCURATE ||
        style == CombatStyle::AGGRESSIVE ||
        style == CombatStyle::DEFENSIVE;
}
