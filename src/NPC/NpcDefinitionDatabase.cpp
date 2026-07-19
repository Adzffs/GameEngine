#include "NpcDefinitionDatabase.h"

namespace
{
    const NpcDefinition PassiveDevelopmentMonster{
        NpcType::PASSIVE_DEVELOPMENT_MONSTER,
        "Passive development monster",
        CombatRatings{5, 4, 3, 30},
        std::nullopt,
        5,
        8,
        RewardTableType::DEVELOPMENT_MONSTER};

    const NpcDefinition AggressiveDevelopmentMonster{
        NpcType::AGGRESSIVE_DEVELOPMENT_MONSTER,
        "Aggressive development monster",
        CombatRatings{5, 4, 3, 30},
        MonsterAggressionDefinition{5, 8},
        5,
        8,
        RewardTableType::DEVELOPMENT_MONSTER};
}

namespace NpcDefinitionDatabase
{
    const std::vector<NpcType> &GetAllNpcTypes()
    {
        static const std::vector<NpcType> types{
            NpcType::PASSIVE_DEVELOPMENT_MONSTER,
            NpcType::AGGRESSIVE_DEVELOPMENT_MONSTER};
        return types;
    }

    const NpcDefinition *TryGet(NpcType type)
    {
        switch (type)
        {
        case NpcType::PASSIVE_DEVELOPMENT_MONSTER:
            return &PassiveDevelopmentMonster;
        case NpcType::AGGRESSIVE_DEVELOPMENT_MONSTER:
            return &AggressiveDevelopmentMonster;
        case NpcType::NONE:
        default:
            return nullptr;
        }
    }
}
