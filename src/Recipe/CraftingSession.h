#pragma once

#include <array>
#include <cstdint>

#include "../Inventory/InventorySlot.h"

using CraftingSessionId = std::uint64_t;
inline constexpr CraftingSessionId InvalidCraftingSessionId = 0;

struct CraftingSessionSnapshot
{
    CraftingSessionId sessionId = InvalidCraftingSessionId;
    int actorEntityID = 0;
    int stationObjectID = 0;
    std::uint64_t revision = 0;
    InventorySlot cursor;
    std::array<InventorySlot, 9> inputSlots{};
};
