#pragma once

#include "StatusEffectModifiers.h"
#include "StatusEffectType.h"

struct ActiveStatusEffect
{
    StatusEffectType type =
        StatusEffectType::NONE;

    int remainingTicks = 0;

    StatusEffectModifiers modifiers;
};
