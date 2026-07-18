#include "CombatService.h"

#include <stdexcept>
#include <utility>

CombatService::CombatService(unsigned int seed)
    : randomSource(std::make_unique<SeededRandom>(seed))
{
}

CombatService::CombatService(std::unique_ptr<RandomSource> randomSource)
    : randomSource(std::move(randomSource))
{
    if (!this->randomSource)
    {
        throw std::invalid_argument("CombatService requires a non-null RandomSource");
    }
}

MeleeAttackResult CombatService::ResolveMeleeAttack(
    const CombatRatings &attacker,
    const CombatRatings &defender,
    HealthPool &defenderHealth)
{
    return meleeAttackResolver.Resolve(
        attacker,
        defender,
        defenderHealth,
        *randomSource);
}
