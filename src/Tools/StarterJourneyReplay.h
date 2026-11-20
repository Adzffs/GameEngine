#pragma once

#include "Content/Export/StarterWorldSnapshot.h"
#include <string>

class World;
class Player;

struct StarterJourneyReplayResult
{
    std::string json;
    bool passed = false;
};

// Continues the actual unfunded upgrade replay. No direct player/world edits.
StarterJourneyReplayResult RunStarterJourneyContinuation(World &world, Player &player,
    const StarterWorldExport::Snapshot &snapshot, int preparedTicks);
