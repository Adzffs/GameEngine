#pragma once

#include "MonsterAIIntent.h"

class EntityManager;
class Monster;

class MonsterAISystem
{
public:
    MonsterAIIntent Evaluate(
        const Monster &monster,
        const EntityManager &entityManager,
        bool respawnSuppressed = false) const;

    bool IsValidTarget(
        const Monster &monster,
        const EntityManager &entityManager,
        int targetEntityID) const;
};
