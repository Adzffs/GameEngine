#include "NpcDefinitionDatabase.h"

namespace
{
    const NpcDefinition DevelopmentGuide{
        NpcType::DEVELOPMENT_GUIDE,
        "Mara Vale",
        NpcKind::FRIENDLY,
        std::nullopt,
        {NpcInteractionType::TALK, NpcInteractionType::TRADE},
        DialogueId::DEVELOPMENT_GUIDE_INTRO,
        ShopId::DEVELOPMENT_GUIDE_SUPPLIES,
        "Village warden and quartermaster",
        "Prepare newcomers before the Cinder Warden reaches Embervale",
        "Practical, warm, and fond of short field lessons",
        "Trusts Torren the smith, but worries his pride outruns his caution",
        "Mara watches the road and quietly counts every traveller who returns."};

    const NpcDefinition PassiveDevelopmentMonster{
        NpcType::PASSIVE_DEVELOPMENT_MONSTER,
        "Mossling",
        NpcKind::MONSTER,
        NpcCombatDefinition{CombatRatings{3, 2, 2, 20}, 6,
            RewardTableType::DEVELOPMENT_MONSTER, std::nullopt, 8},
        {}, DialogueId::NONE, ShopId::NONE};

    const NpcDefinition AggressiveDevelopmentMonster{
        NpcType::AGGRESSIVE_DEVELOPMENT_MONSTER,
        "Ashfang",
        NpcKind::MONSTER,
        NpcCombatDefinition{CombatRatings{8, 6, 4, 35}, 4,
            RewardTableType::AGGRESSIVE_MONSTER,
            MonsterAggressionDefinition{4, 7}, 10},
        {}, DialogueId::NONE, ShopId::NONE};

    const NpcDefinition DurableDevelopmentMonster{
        NpcType::DURABLE_DEVELOPMENT_MONSTER,
        "Stoneback",
        NpcKind::MONSTER,
        NpcCombatDefinition{CombatRatings{10, 8, 18, 90}, 6,
            RewardTableType::DURABLE_MONSTER,
            MonsterAggressionDefinition{3, 6}, 14},
        {}, DialogueId::NONE, ShopId::NONE};

    const NpcDefinition EliteDevelopmentMonster{
        NpcType::ELITE_DEVELOPMENT_MONSTER,
        "Cinder Warden",
        NpcKind::MONSTER,
        NpcCombatDefinition{CombatRatings{18, 16, 14, 140}, 4,
            RewardTableType::ELITE_MONSTER,
            MonsterAggressionDefinition{5, 8}, 24},
        {}, DialogueId::NONE, ShopId::NONE};

    const NpcDefinition TorrenAsh{
        NpcType::TORREN_ASH,
        "Torren Ash",
        NpcKind::FRIENDLY,
        std::nullopt,
        {NpcInteractionType::TALK},
        DialogueId::TORREN_ASH_INTRO,
        ShopId::NONE,
        "Smith and workshop keeper",
        "Prove his steelwork can protect Embervale",
        "Blunt, competitive, and fond of craft metaphors",
        "Respects Mara, but thinks she is too cautious",
        "Every tool in Torren's reach has been repaired twice and praised three times."};
}

namespace NpcDefinitionDatabase
{
    const std::vector<NpcType> &GetAllNpcTypes()
    {
        static const std::vector<NpcType> types{
            NpcType::DEVELOPMENT_GUIDE,
            NpcType::PASSIVE_DEVELOPMENT_MONSTER,
            NpcType::AGGRESSIVE_DEVELOPMENT_MONSTER,
            NpcType::DURABLE_DEVELOPMENT_MONSTER,
            NpcType::ELITE_DEVELOPMENT_MONSTER,
            NpcType::TORREN_ASH};
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
        case NpcType::DURABLE_DEVELOPMENT_MONSTER:
            return &DurableDevelopmentMonster;
        case NpcType::ELITE_DEVELOPMENT_MONSTER:
            return &EliteDevelopmentMonster;
        case NpcType::TORREN_ASH:
            return &TorrenAsh;
        case NpcType::NONE:
        default:
            return nullptr;
        }
    }
}
