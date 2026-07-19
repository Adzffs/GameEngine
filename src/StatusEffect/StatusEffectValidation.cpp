#include "StatusEffectValidation.h"

namespace
{
    bool AreAllModifiersZero(
        const StatusEffectModifiers &modifiers)
    {
        return modifiers.meleeAccuracy == 0 &&
               modifiers.meleeStrength == 0 &&
               modifiers.meleeDefence == 0 &&
               modifiers.maximumHealth == 0;
    }
}

bool IsKnownStatusEffectType(
    StatusEffectType type)
{
    switch (type)
    {
    case StatusEffectType::COMBAT_BOOST:
    case StatusEffectType::COMBAT_REDUCTION:
    case StatusEffectType::MAX_HEALTH_BOOST:
        return true;

    case StatusEffectType::NONE:
    default:
        return false;
    }
}

bool IsValidStatusEffectDefinition(
    const StatusEffectDefinition &definition)
{
    if (!IsKnownStatusEffectType(
            definition.type))
    {
        return false;
    }

    if (definition.durationTicks <= 0)
    {
        return false;
    }

    if (AreAllModifiersZero(
            definition.modifiers))
    {
        return false;
    }

    return true;
}
