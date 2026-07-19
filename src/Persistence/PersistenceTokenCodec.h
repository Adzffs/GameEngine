#pragma once

#include "../Equipment/EquipmentSlotType.h"
#include "../Inventory/ItemType.h"
#include "../Skills/SkillType.h"

#include <optional>
#include <string_view>

class PersistenceTokenCodec
{
public:
    // These tokens are versioned persistence identities, not display names or
    // enum spellings. Existing mappings must remain stable when C++ names change.
    static std::optional<std::string_view> TryGetItemToken(ItemType itemType);
    static std::optional<ItemType> TryParseItemToken(std::string_view token);
    static std::optional<std::string_view> TryGetSkillToken(SkillType skillType);
    static std::optional<SkillType> TryParseSkillToken(std::string_view token);
    static std::optional<std::string_view> TryGetEquipmentSlotToken(EquipmentSlotType slotType);
    static std::optional<EquipmentSlotType> TryParseEquipmentSlotToken(std::string_view token);
};
