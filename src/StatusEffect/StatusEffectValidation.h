#pragma once

#include "StatusEffectDefinition.h"

bool IsKnownStatusEffectType(
    StatusEffectType type);

bool IsValidStatusEffectDefinition(
    const StatusEffectDefinition &definition);
