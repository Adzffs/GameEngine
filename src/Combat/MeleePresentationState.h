#pragma once

#include "MeleeCombatFeedback.h"
#include "../Inventory/ItemType.h"
#include "../Animation/PlayerAnimationTiming.h"
#include <map>
#include <memory>
#include <optional>
#include <vector>

class Action;
class Entity;
class Player;

struct MeleeAnimation
{
    int targetID = -1;
    int facing = 0;
    ItemType weapon = ItemType::NONE;
    int actorX = 0, actorY = 0;
    int startTick = -1;
    int observedActionTick = -1;
    float elapsedSeconds = 0.0f;
    float secondsToContact = 0.0f;
    bool resolved = false;
    bool swingPlayed = false;
};

// Cosmetic state only: contact is selected by a committed combat result.
class MeleePresentationState
{
public:
    static constexpr float RecoverySeconds = 0.30f;
    static constexpr int PoseCount = PlayerAnimationTiming::MeleePoses;
    static int WeaponProfile(ItemType weapon);
    static float RecoveryDuration(ItemType weapon);
    // Capture before another catch-up tick can replace transient World events.
    void SynchronizeResults(int localActorID, const std::vector<MeleeCombatFeedback> &events);
    std::optional<ItemType> ConsumeSwing()
    { const auto result = swingPending; swingPending.reset(); return result; }
    void Update(const Player &player, const Action *action,
        const std::vector<std::unique_ptr<Entity>> &entities,
        const std::map<int, MeleeCombatFeedback> &feedbacks,
        float deltaSeconds, bool settled = true);
    const std::optional<MeleeAnimation> &GetAnimation() const { return animation; }
    int GetPose() const;
    static int Facing(int dx, int dy);

private:
    int actorID = -1;
    int lastCompletionTick = -1;
    std::optional<MeleeAnimation> animation;
    std::optional<MeleeCombatFeedback> pendingResult;
    std::optional<ItemType> swingPending;
    void SelectActor(int localActorID);
};
