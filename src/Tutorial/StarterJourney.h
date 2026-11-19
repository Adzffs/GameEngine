#pragma once

#include "TutorialState.h"
#include "../Recipe/CraftingSession.h"

#include <cstdint>
#include <string>

class Player;

namespace StarterJourney
{
    struct MaterialPlan
    {
        int normalLogs = 0;
        int copperOre = 0;
        int tinOre = 0;
        int bronzeBars = 0;
        int woodenPlanks = 0;
        int leftoverPlanks = 0;
        int leftoverBars = 0;
    };

    enum class Destination
    {
        NONE, GUIDE, NORMAL_TREE, COPPER, TIN, TABLE, FURNACE, BANK, MOSSLING
    };

    struct Guidance
    {
        std::string objective;
        std::string progress;
        std::string instruction;
        Destination destination = Destination::NONE;
    };

    // Derived from the current single-output starter recipes. Includes whole
    // processing batches and fails explicitly if their bindings change.
    MaterialPlan GetMaterialPlan(bool includeShield);

    // Counts banked, equipped and temporary crafting holdings without inventing
    // historical acquisition. Readiness can recognise an already-made sword.
    std::int64_t CountOwned(const Player &player, ItemType item,
        const CraftingSessionSnapshot *crafting = nullptr);
    bool MaterialsReady(const Player &player, TutorialStep step,
        const CraftingSessionSnapshot *crafting = nullptr);
    Guidance Describe(const Player &player,
        const CraftingSessionSnapshot *crafting = nullptr);
}
