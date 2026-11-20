#include "MeleePresentationState.h"
#include "../Action/Action.h"
#include "../Entity/Entity.h"
#include "Combatant.h"
#include "../Player/Player.h"
#include <algorithm>
#include <cmath>

int MeleePresentationState::Facing(int dx, int dy)
{
    return dx == 0 ? (dy < 0 ? 3 : 0) : dy == 0 ? (dx < 0 ? 1 : 2) :
        dy > 0 ? (dx < 0 ? 4 : 5) : (dx < 0 ? 6 : 7);
}

int MeleePresentationState::WeaponProfile(ItemType weapon)
{
    if (weapon == ItemType::NONE) return 0;
    if (weapon == ItemType::BRONZE_SWORD) return 1;
    return weapon == ItemType::DEVELOPER_GODSWORD ? 2 : 3;
}

float MeleePresentationState::RecoveryDuration(ItemType weapon)
{
    int milliseconds = 0;
    for (const auto duration : PlayerAnimationTiming::RecoveryMilliseconds[WeaponProfile(weapon)])
        milliseconds += duration;
    return milliseconds / 1000.0f;
}

void MeleePresentationState::SelectActor(int localActorID)
{
    if (actorID != localActorID)
    {
        actorID = localActorID;
        lastCompletionTick = -1;
        animation.reset();
        pendingResult.reset();
        swingPending.reset();
    }
}

void MeleePresentationState::SynchronizeResults(int localActorID,
    const std::vector<MeleeCombatFeedback> &events)
{
    SelectActor(localActorID);
    for (const auto &event : events)
        if (event.attackerEntityID == actorID && event.completionTick > lastCompletionTick &&
            (!pendingResult || event.completionTick > pendingResult->completionTick))
            pendingResult = event;
}

void MeleePresentationState::Update(const Player &player, const Action *action,
    const std::vector<std::unique_ptr<Entity>> &entities,
    const std::map<int, MeleeCombatFeedback> &feedbacks, float deltaSeconds, bool settled)
{
    SelectActor(player.GetID());
    swingPending.reset();
    const float delta = std::isfinite(deltaSeconds) ? std::clamp(deltaSeconds, 0.0f, 0.1f) : 0.0f;
    const int x = player.GetPosition().GetX(), y = player.GetPosition().GetY();
    const auto weapon = player.GetEquipment().GetEquippedItem(EquipmentSlotType::WEAPON);
    const auto committed = pendingResult;
    pendingResult.reset();
    const MeleeCombatFeedback *newest = committed && committed->completionTick > lastCompletionTick
        ? &*committed : nullptr;
    for (const auto &[id, feedback] : feedbacks)
        if (feedback.attackerEntityID == actorID && feedback.remainingTicks > 0 &&
            feedback.completionTick > lastCompletionTick &&
            (!newest || feedback.completionTick > newest->completionTick)) newest = &feedback;
    if (newest) lastCompletionTick = newest->completionTick;

    if (!player.IsAlive() || !settled ||
        (action && action->IsRunning() && action->GetType() != ActionType::MELEE_ATTACK))
    {
        animation.reset();
        return;
    }
    if (animation && (animation->actorX != x || animation->actorY != y || animation->weapon != weapon))
        animation.reset();

    if (newest && newest->attackerX == x && newest->attackerY == y && newest->weapon == weapon)
    {
        const bool swung = animation && !animation->resolved && animation->swingPlayed &&
            animation->targetID == newest->defenderEntityID;
        animation = MeleeAnimation{newest->defenderEntityID,
            Facing(newest->defenderX - x, newest->defenderY - y), weapon, x, y};
        animation->resolved = true;
        animation->swingPlayed = true;
        if (!swung) swingPending = weapon;
        return;
    }
    if (animation && animation->resolved)
    {
        animation->elapsedSeconds += delta;
        if (animation->elapsedSeconds + 0.00001f < RecoveryDuration(animation->weapon)) return;
        animation.reset();
    }

    if (!action || !action->IsRunning() || action->GetOwnerID() != actorID ||
        action->GetType() != ActionType::MELEE_ATTACK || action->GetDuration() <= 0)
    {
        animation.reset();
        return;
    }
    const auto target = std::find_if(entities.begin(), entities.end(),
        [&](const auto &entity) { return entity->GetID() == action->GetTargetID(); });
    if (target == entities.end()) { animation.reset(); return; }
    const auto *combatant = dynamic_cast<const Combatant *>(target->get());
    const int dx = (*target)->GetPosition().GetX() - x, dy = (*target)->GetPosition().GetY() - y;
    if (!combatant || !combatant->IsAlive() || std::abs(dx) > 1 || std::abs(dy) > 1 || (!dx && !dy))
    {
        animation.reset();
        return;
    }
    if (!animation || animation->targetID != action->GetTargetID() ||
        animation->startTick != action->GetStartTick())
    {
        animation = MeleeAnimation{action->GetTargetID(), Facing(dx, dy), weapon, x, y};
        animation->startTick = action->GetStartTick();
    }
    animation->facing = Facing(dx, dy);
    if (animation->observedActionTick != action->GetCurrentTick())
    {
        animation->observedActionTick = action->GetCurrentTick();
        animation->elapsedSeconds = 0.0f;
    }
    else animation->elapsedSeconds += delta;
    // Interpolate the wind-up within a 600ms tick, then hold the raised pose
    // until the world actually resolves it. Pauses never invent a strike.
    animation->secondsToContact = std::max(0.0f,
        (action->GetDuration() - action->GetCurrentTick()) * 0.6f - std::min(animation->elapsedSeconds, 0.6f));
    if (!animation->swingPlayed && GetPose() > 0)
    {
        animation->swingPlayed = true;
        swingPending = weapon;
    }
}

int MeleePresentationState::GetPose() const
{
    if (!animation) return 0;
    const int profile = WeaponProfile(animation->weapon);
    if (animation->resolved)
    {
        float boundary = 0.0f;
        const auto &durations = PlayerAnimationTiming::RecoveryMilliseconds[profile];
        for (int frame = 0; frame < static_cast<int>(durations.size()); ++frame)
        {
            boundary += durations[frame] / 1000.0f;
            if (animation->elapsedSeconds + 0.00001f < boundary) return frame + 3;
        }
        return PoseCount - 1;
    }
    // Wait in a readable guard; reserve the actual wind-up for the last part
    // of the authoritative action instead of slowly rotating for its duration.
    const float window = PlayerAnimationTiming::WindupMilliseconds[profile] / 1000.0f;
    if (animation->secondsToContact > window) return 0;
    return animation->secondsToContact > window * 0.4f ? 1 : 2;
}
