#include "TestSupport.h"

#include "../src/Action/Action.h"
#include "../src/Action/ActionManager.h"

namespace
{
    void AdvanceTicks(
        ActionManager &actionManager,
        int &currentWorldTick,
        int ticks,
        std::vector<Action> &completedActions)
    {
        for (int index = 0; index < ticks; ++index)
        {
            currentWorldTick++;
            completedActions = actionManager.Update(
                currentWorldTick);
        }
    }
}

int main()
{
    TestContext test;

    {
        Action action(
            ActionType::GATHERING,
            "Pending action",
            4,
            10,
            20,
            false);

        test.ExpectEqual(
            static_cast<int>(action.GetState()),
            static_cast<int>(ActionState::PENDING),
            "A new action starts pending");
        test.ExpectNear(
            action.GetProgress(),
            0.0f,
            0.001f,
            "Pending action progress starts at zero");
    }

    {
        Action action(
            ActionType::GATHERING,
            "Running action",
            4,
            10,
            20,
            false);

        action.Start(7);

        test.ExpectEqual(
            static_cast<int>(action.GetState()),
            static_cast<int>(ActionState::RUNNING),
            "Starting a positive-duration action makes it run");
        test.ExpectEqual(
            action.GetStartTick(),
            7,
            "Action records the authoritative start tick");
        test.ExpectEqual(
            action.GetCompletionTick(),
            11,
            "Action completion tick is based on the authoritative tick");

        action.Update(8);

        test.ExpectEqual(
            action.GetCurrentTick(),
            1,
            "Action progress advances using the supplied tick");
        test.ExpectNear(
            action.GetProgress(),
            0.25f,
            0.001f,
            "Action progress is computed correctly");

        action.Update(100);

        test.ExpectEqual(
            static_cast<int>(action.GetState()),
            static_cast<int>(ActionState::COMPLETED),
            "Action completes once the authoritative tick reaches duration");
        test.ExpectNear(
            action.GetProgress(),
            1.0f,
            0.001f,
            "Action progress is clamped to one on completion");
    }

    {
        Action action(
            ActionType::GATHERING,
            "Cancelled action",
            4,
            11,
            21,
            false);

        action.Start(3);
        action.Cancel(ActionCancelReason::INVALID_TOOL);
        action.Update(50);

        test.ExpectEqual(
            static_cast<int>(action.GetState()),
            static_cast<int>(ActionState::CANCELLED),
            "Cancellation changes state to cancelled");
        test.ExpectEqual(
            static_cast<int>(action.GetCancelReason()),
            static_cast<int>(ActionCancelReason::INVALID_TOOL),
            "Cancellation stores the supplied reason");
        test.Expect(
            !action.IsComplete(),
            "Cancelled actions do not become complete");
    }

    {
        Action zeroDuration(
            ActionType::GATHERING,
            "Zero duration",
            0,
            12,
            22,
            false);

        zeroDuration.Start(5);

        test.Expect(
            zeroDuration.IsComplete(),
            "Zero-duration actions complete safely");
        test.ExpectNear(
            zeroDuration.GetProgress(),
            1.0f,
            0.001f,
            "Zero-duration action progress is clamped to one");

        Action negativeDuration(
            ActionType::GATHERING,
            "Negative duration",
            -3,
            13,
            23,
            false);

        negativeDuration.Start(5);

        test.ExpectEqual(
            negativeDuration.GetDuration(),
            0,
            "Negative durations are normalized to zero");
        test.Expect(
            negativeDuration.IsComplete(),
            "Negative-duration actions complete safely");
    }

    {
        ActionManager actionManager;
        int currentWorldTick = 0;

        actionManager.StartAction(
            Action(
                ActionType::GATHERING,
                "First action",
                6,
                1,
                100,
                false),
            currentWorldTick);

        const Action *firstActiveAction =
            actionManager.GetActionForEntity(1);

        test.Expect(
            firstActiveAction != nullptr,
            "ActionManager stores the started action");
        test.ExpectEqual(
            firstActiveAction->GetStartTick(),
            0,
            "ActionManager starts actions on its authoritative tick");
        test.ExpectEqual(
            firstActiveAction->GetCompletionTick(),
            6,
            "ActionManager computes completion from its authoritative tick");

        actionManager.StartAction(
            Action(
                ActionType::RECIPE,
                "Replacement action",
                4,
                1,
                200,
                false),
            currentWorldTick);

        const Action *replacementAction =
            actionManager.GetActionForEntity(1);

        test.Expect(
            replacementAction != nullptr,
            "Replacement action remains active");
        test.ExpectEqual(
            replacementAction->GetTargetID(),
            200,
            "Starting a second action replaces the first action for the same owner");

        std::vector<Action> completedActions;
        AdvanceTicks(
            actionManager,
            currentWorldTick,
            4,
            completedActions);

        test.ExpectEqual(
            static_cast<int>(completedActions.size()),
            1,
            "Only one active action exists per owner");
        test.ExpectEqual(
            completedActions.front().GetTargetID(),
            200,
            "The replacement action is the one that completes");
        test.Expect(
            actionManager.GetActionForEntity(1) == nullptr,
            "Completed non-repeating actions are removed from the manager");
    }

    {
        ActionManager actionManager;
        int currentWorldTick = 0;
        std::vector<Action> completedActions;

        actionManager.StartAction(
            Action(
                ActionType::RECIPE,
                "Authoritative completion",
                3,
                2,
                300,
                false),
            currentWorldTick);

        AdvanceTicks(
            actionManager,
            currentWorldTick,
            2,
            completedActions);

        test.Expect(
            completedActions.empty(),
            "Action does not complete before its authoritative completion tick");

        AdvanceTicks(
            actionManager,
            currentWorldTick,
            1,
            completedActions);

        test.ExpectEqual(
            static_cast<int>(completedActions.size()),
            1,
            "Action completes exactly on the authoritative completion tick");
        test.ExpectEqual(
            completedActions.front().GetCurrentTick(),
            3,
            "Completed action current tick is clamped to duration");
    }

    {
        ActionManager actionManager;
        int currentWorldTick = 0;
        std::vector<Action> completedActions;

        actionManager.RestartAction(
            Action(
                ActionType::RECIPE,
                "Pending repeat",
                3,
                3,
                301,
                true),
            currentWorldTick);

        test.Expect(
            actionManager.GetActionForEntity(3) == nullptr,
            "RestartAction ignores pending actions");

        Action cancelledRepeating(
            ActionType::RECIPE,
            "Cancelled repeat",
            3,
            3,
            302,
            true);
        cancelledRepeating.Start(0);
        cancelledRepeating.Cancel(ActionCancelReason::PLAYER_MOVED);

        actionManager.RestartAction(
            cancelledRepeating,
            currentWorldTick);

        test.Expect(
            actionManager.GetActionForEntity(3) == nullptr,
            "RestartAction ignores cancelled actions");

        actionManager.StartAction(
            Action(
                ActionType::RECIPE,
                "Non repeating",
                2,
                4,
                400,
                false),
            currentWorldTick);

        AdvanceTicks(
            actionManager,
            currentWorldTick,
            2,
            completedActions);

        actionManager.RestartAction(
            completedActions.front(),
            currentWorldTick);

        test.Expect(
            actionManager.GetActionForEntity(4) == nullptr,
            "Non-repeating actions cannot restart");

        actionManager.StartAction(
            Action(
                ActionType::RECIPE,
                "Repeating",
                2,
                5,
                500,
                true),
            currentWorldTick);

        AdvanceTicks(
            actionManager,
            currentWorldTick,
            2,
            completedActions);

        actionManager.RestartAction(
            completedActions.front(),
            currentWorldTick);

        const Action *restartedAction =
            actionManager.GetActionForEntity(5);

        test.Expect(
            restartedAction != nullptr,
            "Completed repeating actions can restart through the manager");
        test.ExpectEqual(
            static_cast<int>(restartedAction->GetState()),
            static_cast<int>(ActionState::RUNNING),
            "Restarted repeating actions return to the running state");
    }

    {
        ActionManager actionManager;

        actionManager.CancelActionsForEntity(
            999,
            ActionCancelReason::PLAYER_MOVED);

        test.Expect(
            actionManager.GetActionForEntity(999) == nullptr,
            "Cancelling actions for an owner with no action is safe");
    }

    return test.Finish();
}