#include "NpcDefinitionDatabase.h"

namespace
{
    const NpcDefinition DevelopmentGuide{
        NpcType::DEVELOPMENT_GUIDE,
        "Development guide",
        NpcKind::FRIENDLY,
        std::nullopt,
        {NpcInteractionType::TALK},
        DialogueId::DEVELOPMENT_GUIDE_INTRO};

    const NpcDefinition PassiveDevelopmentMonster{
        NpcType::PASSIVE_DEVELOPMENT_MONSTER,
        "Passive development monster",
        NpcKind::MONSTER,
        NpcCombatDefinition{CombatRatings{5, 4, 3, 30}, 5,
            RewardTableType::DEVELOPMENT_MONSTER, std::nullopt, 8},
        {}, DialogueId::NONE};

    const NpcDefinition AggressiveDevelopmentMonster{
        NpcType::AGGRESSIVE_DEVELOPMENT_MONSTER,
        "Aggressive development monster",
        NpcKind::MONSTER,
        NpcCombatDefinition{CombatRatings{5, 4, 3, 30}, 5,
            RewardTableType::DEVELOPMENT_MONSTER,
            MonsterAggressionDefinition{5, 8}, 8},
        {}, DialogueId::NONE};
}

namespace NpcDefinitionDatabase
{
    const std::vector<NpcType> &GetAllNpcTypes()
    {
        static const std::vector<NpcType> types{
            NpcType::DEVELOPMENT_GUIDE,
            NpcType::PASSIVE_DEVELOPMENT_MONSTER,
            NpcType::AGGRESSIVE_DEVELOPMENT_MONSTER};
        return types;
    }

    const NpcDefinition *TryGet(NpcType type)
    {
        switch (type)
        {
        case NpcType::DEVELOPMENT_GUIDE:
            return &DevelopmentGuide;
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
