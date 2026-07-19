#include "PersistenceTokenCodec.h"

#include <array>
#include <utility>

namespace
{
    template <typename T, std::size_t N>
    std::optional<std::string_view> GetToken(
        T value,
        const std::array<std::pair<T, std::string_view>, N> &mappings)
    {
        for (const auto &[mappedValue, token] : mappings)
            if (mappedValue == value)
                return token;
        return std::nullopt;
    }

    template <typename T, std::size_t N>
    std::optional<T> ParseToken(
        std::string_view token,
        const std::array<std::pair<T, std::string_view>, N> &mappings)
    {
        for (const auto &[value, mappedToken] : mappings)
            if (mappedToken == token)
                return value;
        return std::nullopt;
    }

    constexpr std::array ITEM_MAPPINGS{
        std::pair{ItemType::NONE, std::string_view{"NONE"}},
        std::pair{ItemType::LOG, std::string_view{"LOG"}},
        std::pair{ItemType::OAK_LOG, std::string_view{"OAK_LOG"}},
        std::pair{ItemType::WILLOW_LOG, std::string_view{"WILLOW_LOG"}},
        std::pair{ItemType::COPPER_ORE, std::string_view{"COPPER_ORE"}},
        std::pair{ItemType::TIN_ORE, std::string_view{"TIN_ORE"}},
        std::pair{ItemType::IRON_ORE, std::string_view{"IRON_ORE"}},
        std::pair{ItemType::COAL, std::string_view{"COAL"}},
        std::pair{ItemType::BRONZE_BAR, std::string_view{"BRONZE_BAR"}},
        std::pair{ItemType::IRON_BAR, std::string_view{"IRON_BAR"}},
        std::pair{ItemType::STEEL_BAR, std::string_view{"STEEL_BAR"}},
        std::pair{ItemType::COINS, std::string_view{"COINS"}},
        std::pair{ItemType::COOKED_MEAT, std::string_view{"COOKED_MEAT"}},
        std::pair{ItemType::BRONZE_AXE, std::string_view{"BRONZE_AXE"}},
        std::pair{ItemType::IRON_AXE, std::string_view{"IRON_AXE"}},
        std::pair{ItemType::STEEL_AXE, std::string_view{"STEEL_AXE"}},
        std::pair{ItemType::BRONZE_PICKAXE, std::string_view{"BRONZE_PICKAXE"}},
        std::pair{ItemType::IRON_PICKAXE, std::string_view{"IRON_PICKAXE"}},
        std::pair{ItemType::STEEL_PICKAXE, std::string_view{"STEEL_PICKAXE"}},
        std::pair{ItemType::BRONZE_SWORD, std::string_view{"BRONZE_SWORD"}},
        std::pair{ItemType::DEVELOPER_GODSWORD, std::string_view{"DEVELOPER_GODSWORD"}},
        std::pair{ItemType::WOODEN_SHIELD, std::string_view{"WOODEN_SHIELD"}}};

    constexpr std::array SKILL_MAPPINGS{
        std::pair{SkillType::ATTACK, std::string_view{"ATTACK"}},
        std::pair{SkillType::DEFENCE, std::string_view{"DEFENCE"}},
        std::pair{SkillType::WOODCUTTING, std::string_view{"WOODCUTTING"}},
        std::pair{SkillType::MINING, std::string_view{"MINING"}},
        std::pair{SkillType::SMITHING, std::string_view{"SMITHING"}}};

    constexpr std::array SLOT_MAPPINGS{
        std::pair{EquipmentSlotType::HEAD, std::string_view{"HEAD"}},
        std::pair{EquipmentSlotType::BODY, std::string_view{"BODY"}},
        std::pair{EquipmentSlotType::LEGS, std::string_view{"LEGS"}},
        std::pair{EquipmentSlotType::WEAPON, std::string_view{"WEAPON"}},
        std::pair{EquipmentSlotType::SHIELD, std::string_view{"SHIELD"}}};
}

std::optional<std::string_view> PersistenceTokenCodec::TryGetItemToken(ItemType value) { return GetToken(value, ITEM_MAPPINGS); }
std::optional<ItemType> PersistenceTokenCodec::TryParseItemToken(std::string_view token) { return ParseToken(token, ITEM_MAPPINGS); }
std::optional<std::string_view> PersistenceTokenCodec::TryGetSkillToken(SkillType value) { return GetToken(value, SKILL_MAPPINGS); }
std::optional<SkillType> PersistenceTokenCodec::TryParseSkillToken(std::string_view token) { return ParseToken(token, SKILL_MAPPINGS); }
std::optional<std::string_view> PersistenceTokenCodec::TryGetEquipmentSlotToken(EquipmentSlotType value) { return GetToken(value, SLOT_MAPPINGS); }
std::optional<EquipmentSlotType> PersistenceTokenCodec::TryParseEquipmentSlotToken(std::string_view token) { return ParseToken(token, SLOT_MAPPINGS); }
