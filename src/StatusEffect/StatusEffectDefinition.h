#pragma once

#include "StatusEffectModifiers.h"
#include "StatusEffectType.h"

struct StatusEffectDefinition
{
    StatusEffectType type =
        StatusEffectType::NONE;

    int durationTicks = 0;

    StatusEffectModifiers modifiers;
};
