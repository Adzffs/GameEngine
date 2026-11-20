#pragma once

#include "AudioCue.h"
#include "../Combat/MeleeCombatFeedback.h"
#include "../Command/CommandProcessingResult.h"
#include "../Gathering/GatheringRewardEvent.h"
#include "../Inventory/ItemType.h"
#include "../World/Event/ActionLifecycleEvent.h"
#include <cstdint>
#include <map>
#include <vector>

// Presentation only: consumes published facts, never changes gameplay or RNG.
// The client calls ConsumeTick after EVERY world update, including catch-up.
class AudioFeedbackState
{
public:
    void Reset(int actorEntityID);
    void TrackCommand(std::uint64_t commandID, AudioCue cue);
    std::vector<AudioCue> ConsumeTick(int actorEntityID, int tick,
        const std::vector<ActionLifecycleEvent> &actions,
        const std::vector<GatheringRewardEvent> &rewards,
        const std::vector<MeleeCombatFeedback> &combat,
        const std::vector<CommandProcessingResult> &commands,
        ItemType equippedWeapon = ItemType::NONE);

private:
    int actorID = -1;
    int lastTick = -1;
    std::map<std::uint64_t, AudioCue> pendingCommands;
};
