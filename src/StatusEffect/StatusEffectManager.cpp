#include "StatusEffectManager.h"

#include "StatusEffectValidation.h"

#include <cstddef>
#include <limits>

namespace
{
    int ClampToIntRange(long long value)
    {
        if (value > static_cast<long long>(
                        std::numeric_limits<int>::max()))
        {
            return std::numeric_limits<int>::max();
        }

        if (value < static_cast<long long>(
                        std::numeric_limits<int>::min()))
        {
            return std::numeric_limits<int>::min();
        }

        return static_cast<int>(value);
    }

    int AddSaturating(
        int left,
        int right)
    {
        long long combined =
            static_cast<long long>(left) +
            static_cast<long long>(right);

        return ClampToIntRange(combined);
    }
}

bool StatusEffectManager::Apply(
    const StatusEffectDefinition &definition)
{
    if (!IsValidStatusEffectDefinition(
            definition))
    {
        return false;
    }

    for (ActiveStatusEffect &activeEffect :
         activeEffects)
    {
        if (activeEffect.type !=
            definition.type)
        {
            continue;
        }

        activeEffect.remainingTicks =
            definition.durationTicks;

        activeEffect.modifiers =
            definition.modifiers;

        return true;
    }

    activeEffects.push_back(
        ActiveStatusEffect{
            definition.type,
            definition.durationTicks,
            definition.modifiers});

    return true;
}

bool StatusEffectManager::Remove(
    StatusEffectType type)
{
    if (!IsKnownStatusEffectType(type))
    {
        return false;
    }

    for (std::size_t index = 0;
         index < activeEffects.size();
         ++index)
    {
        if (activeEffects[index].type != type)
        {
            continue;
        }

        activeEffects.erase(
            activeEffects.begin() +
            static_cast<std::ptrdiff_t>(index));

        return true;
    }

    return false;
}

bool StatusEffectManager::HasEffect(
    StatusEffectType type) const
{
    return FindEffect(type) != nullptr;
}

const ActiveStatusEffect *StatusEffectManager::FindEffect(
    StatusEffectType type) const
{
    for (const ActiveStatusEffect &activeEffect :
         activeEffects)
    {
        if (activeEffect.type == type)
        {
            return &activeEffect;
        }
    }

    return nullptr;
}

bool StatusEffectManager::Tick()
{
    for (ActiveStatusEffect &activeEffect :
         activeEffects)
    {
        if (activeEffect.remainingTicks > 0)
        {
            activeEffect.remainingTicks--;
        }
    }

    bool expiredAny = false;
    std::size_t writeIndex = 0;

    for (std::size_t readIndex = 0;
         readIndex < activeEffects.size();
         ++readIndex)
    {
        if (activeEffects[readIndex].remainingTicks <= 0)
        {
            expiredAny = true;
            continue;
        }

        if (writeIndex != readIndex)
        {
            activeEffects[writeIndex] =
                activeEffects[readIndex];
        }

        writeIndex++;
    }

    if (expiredAny)
    {
        activeEffects.resize(writeIndex);
    }

    return expiredAny;
}

StatusEffectModifiers
StatusEffectManager::GetCombinedModifiers() const
{
    StatusEffectModifiers combined;

    for (const ActiveStatusEffect &activeEffect :
         activeEffects)
    {
        combined.meleeAccuracy =
            AddSaturating(
                combined.meleeAccuracy,
                activeEffect.modifiers.meleeAccuracy);

        combined.meleeStrength =
            AddSaturating(
                combined.meleeStrength,
                activeEffect.modifiers.meleeStrength);

        combined.meleeDefence =
            AddSaturating(
                combined.meleeDefence,
                activeEffect.modifiers.meleeDefence);

        combined.maximumHealth =
            AddSaturating(
                combined.maximumHealth,
                activeEffect.modifiers.maximumHealth);
    }

    return combined;
}

const std::vector<ActiveStatusEffect> &
StatusEffectManager::GetActiveEffects() const
{
    return activeEffects;
}
