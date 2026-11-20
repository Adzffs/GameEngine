#include "AudioFeedbackState.h"

void AudioFeedbackState::Reset(int actorEntityID)
{
    actorID = actorEntityID;
    lastTick = -1;
    pendingCommands.clear();
}

void AudioFeedbackState::TrackCommand(std::uint64_t commandID, AudioCue cue)
{
    if (commandID == 0 || cue >= AudioCue::COUNT) return;
    constexpr std::size_t MaximumPendingCommands = 64;
    if (pendingCommands.size() >= MaximumPendingCommands)
        pendingCommands.erase(pendingCommands.begin());
    pendingCommands.insert_or_assign(commandID, cue);
}

std::vector<AudioCue> AudioFeedbackState::ConsumeTick(int actorEntityID, int tick,
    const std::vector<ActionLifecycleEvent> &actions,
    const std::vector<GatheringRewardEvent> &rewards,
    const std::vector<MeleeCombatFeedback> &combat,
    const std::vector<CommandProcessingResult> &commands, ItemType equippedWeapon)
{
    if (actorID != actorEntityID) Reset(actorEntityID);
    if (tick <= lastTick || actorID < 0) return {};
    lastTick = tick;
    std::vector<AudioCue> cues;
    const auto add = [&](AudioCue cue)
    {
        if (cues.size() < 16) cues.push_back(cue);
    };
    // Outcomes precede incidental confirmations when the voice budget is full.
    for (const auto &event : combat)
    {
        if (event.completionTick != tick ||
            (event.attackerEntityID != actorID && event.defenderEntityID != actorID))
            continue;
        add(!event.hit ? AudioCue::MISS :
            event.actualDamageApplied > 0 ? AudioCue::HIT : AudioCue::BLOCK);
    }
    for (const auto &event : actions)
    {
        if (const auto *started = std::get_if<ActionStartedEvent>(&event.data);
            started && started->ownerEntityID == actorID &&
            started->actionType == ActionType::MELEE_ATTACK)
            add(equippedWeapon == ItemType::DEVELOPER_GODSWORD
                ? AudioCue::HEAVY_SWING : AudioCue::SWING);
        // World publishes RECIPE completion only after TryCreateRecipe succeeds.
        if (const auto *completed = std::get_if<ActionCompletedEvent>(&event.data);
            completed && completed->ownerEntityID == actorID &&
            completed->actionType == ActionType::RECIPE)
            add(AudioCue::CRAFT);
    }
    for (const auto &event : rewards)
    {
        if (event.actorEntityID == actorID && event.completionTick == tick &&
            event.amount > 0 && event.xp > 0)
            add(AudioCue::GATHER);
    }
    for (const auto &result : commands)
    {
        if (result.actorEntityID != actorID) continue;
        const auto pending = pendingCommands.find(result.commandID);
        if (pending == pendingCommands.end()) continue;
        if (result.resultCode == CommandResultCode::ACCEPTED) add(pending->second);
        pendingCommands.erase(pending);
    }
    return cues;
}
