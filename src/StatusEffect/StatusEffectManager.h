#pragma once

#include "ActiveStatusEffect.h"
#include "StatusEffectDefinition.h"

#include <vector>

class StatusEffectManager
{
public:
    bool Apply(
        const StatusEffectDefinition &definition);

    bool Remove(
        StatusEffectType type);

    bool HasEffect(
        StatusEffectType type) const;

    const ActiveStatusEffect *FindEffect(
        StatusEffectType type) const;

    bool Tick();

    StatusEffectModifiers
    GetCombinedModifiers() const;

    const std::vector<ActiveStatusEffect> &
    GetActiveEffects() const;

private:
    std::vector<ActiveStatusEffect>
        activeEffects;
};
