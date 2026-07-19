#include "TestSupport.h"

#include <type_traits>
#include <array>
#include <utility>
#include <variant>

#include "../src/Action/ActionCancelReason.h"
#include "../src/Action/ActionType.h"
#include "../src/Equipment/EquipmentSlotType.h"
#include "../src/Entity/Monster/Monster.h"
#include "../src/Inventory/ItemType.h"
#include "../src/Movement/MovementRequest.h"
#include "../src/Player/Player.h"
#include "../src/Recipe/RecipeDatabase.h"
#include "../src/Recipe/RecipeType.h"
#include "../src/World/Event/ActionCancelledEvent.h"
#include "../src/World/Event/ActionCompletedEvent.h"
#include "../src/World/Event/ActionStartedEvent.h"
#include "../src/World/World.h"

static_assert(
    std::is_same_v<
        decltype(std::declval<const World &>()
                     .GetActionLifecycleEvents()),
        const std::vector<ActionLifecycleEvent> &>,
    "Action lifecycle getter must return a const vector reference");

static_assert(
    !std::is_pointer_v<
        std::remove_const_t<decltype(std::declval<ActionStartedEvent>().ownerEntityID)>>,
    "Lifecycle events must store IDs, not pointers");

struct WorldTestAccess
{
    static void SetActiveStationID(
        World &world,
        int entityID,
        int stationID)
    {
        world.activeStations[entityID] = stationID;
    }
};

namespace
{
    Player *GetPlayer(
        World &world,
        int playerID)
    {
        return dynamic_cast<Player *>(
            world.GetEntityByID(playerID));
    }

    int FindInventorySlot(
        const Inventory &inventory,
        ItemType itemType)
    {
        const auto &slots = inventory.GetSlots();

        for (int index = 0;
             index < static_cast<int>(slots.size());
             ++index)
        {
            if (!slots[index].IsEmpty() &&
                slots[index].GetItemType() == itemType)
            {
                return index;
            }
        }

        return -1;
    }

    bool EquipItem(
        World &world,
        int playerID,
        ItemType itemType)
    {
        Player *player =
            GetPlayer(world, playerID);

        if (player == nullptr)
        {
            return false;
        }

        int slotIndex = FindInventorySlot(
            player->GetInventory(),
            itemType);

        if (slotIndex < 0)
        {
            return false;
        }

        return world.TryEquipInventoryItem(
            playerID,
            slotIndex);
    }

    bool OpenFurnace(
        World &world,
        int playerID)
    {
        CraftingStation *station =
            world.GetStationAt(14, 9);

        if (station == nullptr)
        {
            return false;
        }

        world.QueueStationInteraction(
            playerID,
            station->GetID());
        world.Update();

        return true;
    }

    template <typename EventType>
    int CountEvents(
        const std::vector<ActionLifecycleEvent> &events)
    {
        int count = 0;

        for (const ActionLifecycleEvent &event : events)
        {
            if (std::holds_alternative<EventType>(
                    event.data))
            {
                count++;
            }
        }

        return count;
    }

    template <typename EventType>
    const EventType *FindFirstEvent(
        const std::vector<ActionLifecycleEvent> &events)
    {
        for (const ActionLifecycleEvent &event : events)
        {
            const EventType *typedEvent =
                std::get_if<EventType>(
                    &event.data);

            if (typedEvent != nullptr)
            {
                return typedEvent;
            }
        }

        return nullptr;
    }

    bool ContainsCancelledReason(
        const std::vector<ActionLifecycleEvent> &events,
        ActionCancelReason reason)
    {
        for (const ActionLifecycleEvent &event : events)
        {
            const ActionCancelledEvent *cancelled =
                std::get_if<ActionCancelledEvent>(
                    &event.data);

            if (cancelled != nullptr &&
                cancelled->reason == reason)
            {
                return true;
            }
        }

        return false;
    }

    int CountRecipeCompletedEvents(
        const std::vector<ActionLifecycleEvent> &events)
    {
        int count = 0;

        for (const ActionLifecycleEvent &event : events)
        {
            const ActionCompletedEvent *completed =
                std::get_if<ActionCompletedEvent>(
                    &event.data);

            if (completed != nullptr &&
                completed->actionType == ActionType::RECIPE)
            {
                count++;
            }
        }

        return count;
    }

    int CountRecipeStartedEvents(
        const std::vector<ActionLifecycleEvent> &events)
    {
        int count = 0;

        for (const ActionLifecycleEvent &event : events)
        {
            const ActionStartedEvent *started =
                std::get_if<ActionStartedEvent>(
                    &event.data);

            if (started != nullptr &&
                started->actionType == ActionType::RECIPE)
            {
                count++;
            }
        }

        return count;
    }

    bool InventoriesMatch(
        const Inventory &left,
        const Inventory &right)
    {
        const auto &leftSlots = left.GetSlots();
        const auto &rightSlots = right.GetSlots();

        for (int index = 0;
             index < static_cast<int>(leftSlots.size());
             ++index)
        {
            if (leftSlots[index].IsEmpty() != rightSlots[index].IsEmpty())
            {
                return false;
            }

            if (leftSlots[index].IsEmpty())
            {
                continue;
            }

            if (leftSlots[index].GetItemType() !=
                    rightSlots[index].GetItemType() ||
                leftSlots[index].GetAmount() !=
                    rightSlots[index].GetAmount())
            {
                return false;
            }
        }

        return true;
    }

    void AdvanceAndCountRecipeLifecycle(
        World &world,
        int ticks,
        int &completedCount,
        int &startedCount)
    {
        completedCount = 0;
        startedCount = 0;

        for (int index = 0; index < ticks; ++index)
        {
            world.Update();

            const std::vector<ActionLifecycleEvent> &events =
                world.GetActionLifecycleEvents();

            completedCount +=
                CountRecipeCompletedEvents(events);
            startedCount +=
                CountRecipeStartedEvents(events);
        }
    }

    bool AdvanceUntilCompletionEvent(
        World &world,
        int maxTicks,
        const std::vector<ActionLifecycleEvent> *&eventsAtCompletion)
    {
        eventsAtCompletion = nullptr;

        for (int index = 0; index < maxTicks; ++index)
        {
            world.Update();

            const std::vector<ActionLifecycleEvent> &events =
                world.GetActionLifecycleEvents();

            if (CountEvents<ActionCompletedEvent>(events) > 0)
            {
                eventsAtCompletion = &events;
                return true;
            }
        }

        return false;
    }

    bool AdvanceUntilRepeatCycleEvents(
        World &world,
        int maxTicks,
        const std::vector<ActionLifecycleEvent> *&eventsAtRepeatCycle)
    {
        eventsAtRepeatCycle = nullptr;

        for (int index = 0; index < maxTicks; ++index)
        {
            world.Update();

            const std::vector<ActionLifecycleEvent> &events =
                world.GetActionLifecycleEvents();

            if (CountEvents<ActionCompletedEvent>(events) > 0 &&
                CountEvents<ActionStartedEvent>(events) > 0)
            {
                eventsAtRepeatCycle = &events;
                return true;
            }
        }

        return false;
    }
}

int main()
{
    TestContext test;

    {
        World world;

        world.Update();

        test.Expect(
            world.GetActionLifecycleEvents().empty(),
            "No transitions in a tick publish no action lifecycle events");
    }

    {
        World world;
        int attackerID = world.CreatePlayer();
        int defenderID = world.CreatePlayer();

        Player *attacker = GetPlayer(world, attackerID);
        Player *defender = GetPlayer(world, defenderID);

        attacker->GetPosition().SetPosition(10, 10);
        defender->GetPosition().SetPosition(11, 10);

        test.Expect(
            world.TryStartMeleeAttack(
                attackerID,
                defenderID,
                4),
            "Pre-update melee start succeeds for publication lifecycle test");

        test.Expect(
            world.GetActionLifecycleEvents().empty(),
            "Lifecycle getter exposes only published events");

        world.Update();

        const std::vector<ActionLifecycleEvent> &published =
            world.GetActionLifecycleEvents();

        test.ExpectEqual(
            CountEvents<ActionStartedEvent>(published),
            1,
            "Pre-update started action is published after update");

        test.ExpectEqual(
            CountEvents<ActionCompletedEvent>(published),
            0,
            "No completion is published before completion tick");

        const std::vector<ActionLifecycleEvent> &sameTickRead =
            world.GetActionLifecycleEvents();

        test.ExpectEqual(
            static_cast<int>(sameTickRead.size()),
            static_cast<int>(published.size()),
            "Published events remain readable after update until next update starts");

        world.Update();

        test.Expect(
            world.GetActionLifecycleEvents().empty(),
            "Next update clears previously published events when no new transitions occur");
    }

    {
        World world;
        int playerID = world.CreatePlayer();
        Player *player = GetPlayer(world, playerID);

        test.Expect(
            EquipItem(
                world,
                playerID,
                ItemType::BRONZE_AXE),
            "Bronze axe equips for gathering lifecycle test setup");

        player->GetPosition().SetPosition(4, 5);

        ResourceNode *resource = world.GetResourceAt(5, 5);

        test.Expect(
            resource != nullptr,
            "Resource exists for gathering lifecycle test setup");

        world.QueueResourceInteraction(
            playerID,
            resource->GetID());

        world.Update();

        test.ExpectEqual(
            CountEvents<ActionStartedEvent>(
                world.GetActionLifecycleEvents()),
            1,
            "Events generated during update are published in the same update");

        world.Update();

        test.Expect(
            CountEvents<ActionStartedEvent>(
                world.GetActionLifecycleEvents()) <= 1,
            "Published events are not duplicated across ticks");
    }

    {
        World world;
        int playerID = world.CreatePlayer();
        Player *player = GetPlayer(world, playerID);

        test.Expect(
            EquipItem(
                world,
                playerID,
                ItemType::BRONZE_AXE),
            "Bronze axe equips for gathering start test setup");

        player->GetPosition().SetPosition(4, 5);

        ResourceNode *resource = world.GetResourceAt(5, 5);
        world.QueueResourceInteraction(
            playerID,
            resource->GetID());

        world.Update();

        test.ExpectEqual(
            CountEvents<ActionStartedEvent>(
                world.GetActionLifecycleEvents()),
            1,
            "Successful gathering start emits exactly one start event");
    }

    {
        World world;
        int playerID = world.CreatePlayer();
        Player *player = GetPlayer(world, playerID);

        player->GetPosition().SetPosition(13, 9);
        player->GetInventory().AddItem(
            ItemType::COPPER_ORE,
            1);
        player->GetInventory().AddItem(
            ItemType::TIN_ORE,
            1);

        test.Expect(
            OpenFurnace(world, playerID),
            "Furnace opens for recipe start test setup");

        test.Expect(
            world.TryStartRecipeAction(
                playerID,
                RecipeType::BRONZE_BAR),
            "Recipe action starts for lifecycle test");

        world.Update();

        test.ExpectEqual(
            CountEvents<ActionStartedEvent>(
                world.GetActionLifecycleEvents()),
            1,
            "Successful recipe start emits exactly one start event");
    }

    {
        World world;
        int attackerID = world.CreatePlayer();
        int defenderID = world.CreatePlayer();

        Player *attacker = GetPlayer(world, attackerID);
        Player *defender = GetPlayer(world, defenderID);

        attacker->GetPosition().SetPosition(10, 10);
        defender->GetPosition().SetPosition(11, 10);

        test.Expect(
            world.TryStartMeleeAttack(
                attackerID,
                defenderID,
                2),
            "Melee start succeeds for start-event test");

        world.Update();

        test.ExpectEqual(
            CountEvents<ActionStartedEvent>(
                world.GetActionLifecycleEvents()),
            1,
            "Successful melee start emits exactly one start event");

        const ActionStartedEvent *started =
            FindFirstEvent<ActionStartedEvent>(
                world.GetActionLifecycleEvents());

        test.Expect(
            started != nullptr,
            "Started event is available for payload validation");

        if (started != nullptr)
        {
            test.ExpectEqual(
                started->ownerEntityID,
                attackerID,
                "Start event owner ID matches action owner");
            test.ExpectEqual(
                started->targetID,
                defenderID,
                "Start event target ID matches action target");
            test.ExpectEqual(
                static_cast<int>(started->actionType),
                static_cast<int>(ActionType::MELEE_ATTACK),
                "Start event action type matches action");
            test.ExpectEqual(
                started->startTick,
                0,
                "Start event uses authoritative action start tick");
            test.ExpectEqual(
                started->completionTick,
                2,
                "Start event uses authoritative action completion tick");
        }
    }

    {
        World world;

        test.Expect(
            !world.TryStartMeleeAttack(
                999999,
                1,
                2),
            "Invalid melee start request fails");

        world.Update();

        test.ExpectEqual(
            CountEvents<ActionStartedEvent>(
                world.GetActionLifecycleEvents()),
            0,
            "Failed starts emit no start events");
    }

    {
        World world;
        int playerID = world.CreatePlayer();
        Player *player = GetPlayer(world, playerID);

        test.Expect(
            EquipItem(
                world,
                playerID,
                ItemType::BRONZE_AXE),
            "Bronze axe equips for gathering completion test setup");

        player->GetPosition().SetPosition(4, 5);

        ResourceNode *resource = world.GetResourceAt(5, 5);
        world.QueueResourceInteraction(
            playerID,
            resource->GetID());
        world.Update();

        const std::vector<ActionLifecycleEvent> *completionEvents = nullptr;
        test.Expect(
            AdvanceUntilCompletionEvent(
                world,
                20,
                completionEvents),
            "Gathering eventually reaches a successful completion cycle");

        if (completionEvents != nullptr)
        {
            test.ExpectEqual(
                CountEvents<ActionCompletedEvent>(*completionEvents),
                1,
                "Successful gathering cycle emits one completion event");
        }
    }

    {
        World world;
        int playerID = world.CreatePlayer();
        Player *player = GetPlayer(world, playerID);

        player->GetPosition().SetPosition(13, 9);
        player->GetInventory().AddItem(
            ItemType::COPPER_ORE,
            1);
        player->GetInventory().AddItem(
            ItemType::TIN_ORE,
            1);

        test.Expect(
            OpenFurnace(world, playerID),
            "Furnace opens for recipe completion test setup");

        test.Expect(
            world.TryStartRecipeAction(
                playerID,
                RecipeType::BRONZE_BAR),
            "Recipe action starts for completion test");

        world.Update();

        const std::vector<ActionLifecycleEvent> *completionEvents = nullptr;
        test.Expect(
            AdvanceUntilCompletionEvent(
                world,
                20,
                completionEvents),
            "Recipe action reaches completion");

        if (completionEvents != nullptr)
        {
            test.ExpectEqual(
                CountEvents<ActionCompletedEvent>(*completionEvents),
                1,
                "Successful recipe cycle emits one completion event");
        }
    }

    {
        World world;
        int playerID = world.CreatePlayer();
        Player *player = GetPlayer(world, playerID);

        player->GetPosition().SetPosition(13, 9);
        player->GetInventory().AddItem(
            ItemType::COPPER_ORE,
            2);
        player->GetInventory().AddItem(
            ItemType::TIN_ORE,
            2);

        test.Expect(
            OpenFurnace(world, playerID),
            "Furnace opens for recipe repeat lifecycle-order test setup");

        test.Expect(
            world.TryStartRecipeAction(
                playerID,
                RecipeType::BRONZE_BAR),
            "Repeating recipe action starts for lifecycle-order test");

        world.Update();

        const std::vector<ActionLifecycleEvent> *repeatCycleEvents = nullptr;
        test.Expect(
            AdvanceUntilRepeatCycleEvents(
                world,
                20,
                repeatCycleEvents),
            "Recipe repetition eventually publishes completion and next-cycle start in one tick");

        if (repeatCycleEvents != nullptr)
        {
            int completionIndex = -1;
            int restartedIndex = -1;

            for (int index = 0;
                 index < static_cast<int>(repeatCycleEvents->size());
                 ++index)
            {
                if (completionIndex < 0 &&
                    std::holds_alternative<ActionCompletedEvent>(
                        (*repeatCycleEvents)[index].data))
                {
                    completionIndex = index;
                }

                if (restartedIndex < 0 &&
                    std::holds_alternative<ActionStartedEvent>(
                        (*repeatCycleEvents)[index].data))
                {
                    restartedIndex = index;
                }
            }

            test.Expect(
                completionIndex >= 0 &&
                    restartedIndex >= 0 &&
                    completionIndex < restartedIndex,
                "Recipe repeat cycle records completion before next-cycle start");
        }
    }

    {
        World world;
        int playerID = world.CreatePlayer();
        Player *player = GetPlayer(world, playerID);

        player->GetPosition().SetPosition(13, 9);
        player->GetInventory().AddItem(
            ItemType::COPPER_ORE,
            1);
        player->GetInventory().AddItem(
            ItemType::TIN_ORE,
            1);

        test.Expect(
            OpenFurnace(world, playerID),
            "Furnace opens for failed recipe completion event test setup");

        test.Expect(
            world.TryStartRecipeAction(
                playerID,
                RecipeType::BRONZE_BAR),
            "Recipe action starts before ingredient invalidation");

        world.Update();

        player->GetInventory().RemoveItem(
            ItemType::TIN_ORE,
            1);

        for (int tick = 0;
             tick < RecipeDatabase::Get(RecipeType::BRONZE_BAR)
                        .GetActionDurationTicks();
             ++tick)
        {
            world.Update();
        }

        test.ExpectEqual(
            CountEvents<ActionCompletedEvent>(
                world.GetActionLifecycleEvents()),
            0,
            "Failed recipe completion validation emits no completion event");
    }

    {
        World world;
        int playerID = world.CreatePlayer();
        Player *player = GetPlayer(world, playerID);

        player->GetPosition().SetPosition(13, 9);

        struct RecipeAuditExpectation
        {
            RecipeType recipeType;
            std::array<RecipeIngredient, 2> ingredients;
            int ingredientCount;
        };

        const std::array<RecipeAuditExpectation, 3> expectations{{
            {
                RecipeType::BRONZE_BAR,
                {
                    RecipeIngredient{ItemType::COPPER_ORE, 1},
                    RecipeIngredient{ItemType::TIN_ORE, 1},
                },
                2,
            },
            {
                RecipeType::IRON_BAR,
                {
                    RecipeIngredient{ItemType::IRON_ORE, 1},
                    RecipeIngredient{ItemType::NONE, 0},
                },
                1,
            },
            {
                RecipeType::STEEL_BAR,
                {
                    RecipeIngredient{ItemType::IRON_ORE, 1},
                    RecipeIngredient{ItemType::COAL, 2},
                },
                2,
            },
        }};

        player->GetSkills().AddXP(
            SkillType::SMITHING,
            200000);

        for (const RecipeAuditExpectation &expectation : expectations)
        {
            const RecipeDefinition &recipe =
                RecipeDatabase::Get(expectation.recipeType);

            test.Expect(
                OpenFurnace(world, playerID),
                "Furnace opens for exact recipe integration checks");

            for (int index = 0;
                 index < expectation.ingredientCount;
                 ++index)
            {
                const RecipeIngredient &ingredient =
                    expectation.ingredients[index];

                player->GetInventory().AddItem(
                    ingredient.itemType,
                    ingredient.amount);
            }

            const int smithingXPBefore =
                player->GetSkills()
                    .GetSkill(SkillType::SMITHING)
                    .GetXP();

            for (int index = 0;
                 index < expectation.ingredientCount;
                 ++index)
            {
                const RecipeIngredient &ingredient =
                    expectation.ingredients[index];

                test.ExpectEqual(
                    player->GetInventory().GetItemAmount(ingredient.itemType),
                    ingredient.amount,
                    "Recipe integration starts with exact ingredient quantities");
            }

            const int outputBefore =
                player->GetInventory().GetItemAmount(
                    recipe.GetOutputItem());

            test.Expect(
                world.TryStartRecipeAction(
                    playerID,
                    expectation.recipeType),
                "Recipe starts for exact integration checks");

            world.Update();

            int completedCount = 0;
            int startedCount = 0;

            AdvanceAndCountRecipeLifecycle(
                world,
                recipe.GetActionDurationTicks() + 2,
                completedCount,
                startedCount);

            for (int index = 0;
                 index < expectation.ingredientCount;
                 ++index)
            {
                const RecipeIngredient &ingredient =
                    expectation.ingredients[index];

                test.ExpectEqual(
                    player->GetInventory().GetItemAmount(ingredient.itemType),
                    0,
                    "Recipe integration consumes each expected ingredient exactly once");
            }

            test.ExpectEqual(
                player->GetInventory().GetItemAmount(
                    recipe.GetOutputItem()) -
                    outputBefore,
                recipe.GetOutputAmount(),
                "Recipe integration adds the exact output quantity once");

            test.ExpectEqual(
                player->GetSkills()
                        .GetSkill(SkillType::SMITHING)
                        .GetXP() -
                    smithingXPBefore,
                recipe.GetXPReward(),
                "Recipe integration awards exact smithing XP once");

            test.ExpectEqual(
                completedCount,
                1,
                "Recipe integration emits one completion event per recipe");
            test.ExpectEqual(
                startedCount,
                0,
                "Recipe integration does not restart after one successful cycle with exact inputs");
            test.Expect(
                world.GetActionForEntity(playerID) == nullptr,
                "Recipe integration leaves no running action after materials are exhausted");
        }
    }

    {
        World world;
        int playerID = world.CreatePlayer();
        Player *player = GetPlayer(world, playerID);

        player->GetPosition().SetPosition(13, 9);
        player->GetInventory().AddItem(
            ItemType::COPPER_ORE,
            1);
        player->GetInventory().AddItem(
            ItemType::TIN_ORE,
            1);

        test.Expect(
            OpenFurnace(world, playerID),
            "Furnace opens for missing-furnace completion test setup");

        test.Expect(
            world.TryStartRecipeAction(
                playerID,
                RecipeType::BRONZE_BAR),
            "Recipe action starts before furnace-missing simulation");

        world.Update();

        const Inventory inventoryBefore =
            player->GetInventory();
        const int smithingXPBefore =
            player->GetSkills()
                .GetSkill(SkillType::SMITHING)
                .GetXP();

        WorldTestAccess::SetActiveStationID(
            world,
            playerID,
            -1);

        int completedCount = 0;
        int startedCount = 0;

        AdvanceAndCountRecipeLifecycle(
            world,
            RecipeDatabase::Get(RecipeType::BRONZE_BAR)
                    .GetActionDurationTicks() +
                2,
            completedCount,
            startedCount);

        test.Expect(
            InventoriesMatch(
                player->GetInventory(),
                inventoryBefore),
            "Missing furnace at completion leaves inventory unchanged");
        test.ExpectEqual(
            player->GetSkills()
                .GetSkill(SkillType::SMITHING)
                .GetXP(),
            smithingXPBefore,
            "Missing furnace at completion awards no smithing XP");
        test.ExpectEqual(
            player->GetInventory().GetItemAmount(ItemType::BRONZE_BAR),
            0,
            "Missing furnace at completion adds no output");
        test.ExpectEqual(
            completedCount,
            0,
            "Missing furnace at completion emits no completion event");
        test.ExpectEqual(
            startedCount,
            0,
            "Missing furnace at completion does not repeat the action");
    }

    {
        World world;
        int playerID = world.CreatePlayer();
        Player *player = GetPlayer(world, playerID);

        player->GetPosition().SetPosition(13, 9);
        player->GetInventory().AddItem(
            ItemType::COPPER_ORE,
            1);
        player->GetInventory().AddItem(
            ItemType::TIN_ORE,
            1);

        test.Expect(
            OpenFurnace(world, playerID),
            "Furnace opens for move-away completion test setup");

        test.Expect(
            world.TryStartRecipeAction(
                playerID,
                RecipeType::BRONZE_BAR),
            "Recipe action starts before move-away simulation");

        world.Update();
        player->GetPosition().SetPosition(0, 0);

        const Inventory inventoryBefore =
            player->GetInventory();
        const int smithingXPBefore =
            player->GetSkills()
                .GetSkill(SkillType::SMITHING)
                .GetXP();

        int completedCount = 0;
        int startedCount = 0;

        AdvanceAndCountRecipeLifecycle(
            world,
            RecipeDatabase::Get(RecipeType::BRONZE_BAR)
                    .GetActionDurationTicks() +
                2,
            completedCount,
            startedCount);

        test.Expect(
            InventoriesMatch(
                player->GetInventory(),
                inventoryBefore),
            "Moving away before completion leaves inventory unchanged");
        test.ExpectEqual(
            player->GetSkills()
                .GetSkill(SkillType::SMITHING)
                .GetXP(),
            smithingXPBefore,
            "Moving away before completion awards no smithing XP");
        test.ExpectEqual(
            completedCount,
            0,
            "Moving away before completion emits no completion event");
        test.ExpectEqual(
            startedCount,
            0,
            "Moving away before completion does not repeat the action");
    }

    {
        World world;
        int playerID = world.CreatePlayer();
        Player *player = GetPlayer(world, playerID);

        player->GetPosition().SetPosition(13, 9);
        player->GetInventory().AddItem(
            ItemType::COPPER_ORE,
            1);
        player->GetInventory().AddItem(
            ItemType::TIN_ORE,
            1);

        test.Expect(
            OpenFurnace(world, playerID),
            "Furnace opens for read-only station-validation boundary test setup");
        test.Expect(
            world.HasActiveStation(playerID),
            "Opening station creates active station context");

        player->GetPosition().SetPosition(0, 0);

        test.Expect(
            !world.IsStationUsableForRecipeValidation(
                playerID,
                StationType::FURNACE),
            "Read-only station validation reports unusable station out of range");
        test.Expect(
            world.HasActiveStation(playerID),
            "Read-only station validation does not mutate active station context");

        player->GetPosition().SetPosition(13, 9);

        test.Expect(
            world.TryStartRecipeAction(
                playerID,
                RecipeType::BRONZE_BAR),
            "Station context remains available after read-only validation call");
    }

    {
        World world;
        int playerID = world.CreatePlayer();
        Player *player = GetPlayer(world, playerID);

        player->GetPosition().SetPosition(13, 9);
        player->GetInventory().AddItem(
            ItemType::COPPER_ORE,
            1);
        player->GetInventory().AddItem(
            ItemType::TIN_ORE,
            1);

        test.Expect(
            OpenFurnace(world, playerID),
            "Furnace opens for explicit stale-station cleanup ownership test setup");

        test.Expect(
            world.TryStartRecipeAction(
                playerID,
                RecipeType::BRONZE_BAR),
            "Recipe action starts before out-of-range completion failure");

        world.Update();
        player->GetPosition().SetPosition(0, 0);

        for (int tick = 0;
             tick < RecipeDatabase::Get(RecipeType::BRONZE_BAR)
                            .GetActionDurationTicks() +
                        2;
             ++tick)
        {
            world.Update();
        }

        test.Expect(
            !world.HasActiveStation(playerID),
            "World-owned completion failure handling explicitly cleans stale active station context");

        player->GetPosition().SetPosition(13, 9);

        test.Expect(
            !world.TryStartRecipeAction(
                playerID,
                RecipeType::BRONZE_BAR),
            "After explicit cleanup, smithing cannot restart until station is reopened");
    }

    {
        World world;
        int playerID = world.CreatePlayer();
        Player *player = GetPlayer(world, playerID);

        player->GetPosition().SetPosition(13, 9);
        player->GetInventory().AddItem(
            ItemType::COPPER_ORE,
            1);
        player->GetInventory().AddItem(
            ItemType::TIN_ORE,
            1);

        test.Expect(
            OpenFurnace(world, playerID),
            "Furnace opens for interaction-close completion test setup");

        test.Expect(
            world.TryStartRecipeAction(
                playerID,
                RecipeType::BRONZE_BAR),
            "Recipe action starts before station close");

        world.Update();
        world.CloseStationInteraction(playerID);

        StationType stationType =
            StationType::NONE;
        test.Expect(
            !world.ConsumeOpenedStation(
                playerID,
                stationType),
            "Closing station interaction clears opened station state");

        const Inventory inventoryBefore =
            player->GetInventory();
        const int smithingXPBefore =
            player->GetSkills()
                .GetSkill(SkillType::SMITHING)
                .GetXP();

        int completedCount = 0;
        int startedCount = 0;

        AdvanceAndCountRecipeLifecycle(
            world,
            RecipeDatabase::Get(RecipeType::BRONZE_BAR)
                    .GetActionDurationTicks() +
                2,
            completedCount,
            startedCount);

        test.Expect(
            InventoriesMatch(
                player->GetInventory(),
                inventoryBefore),
            "Closed station interaction before completion leaves inventory unchanged");
        test.ExpectEqual(
            player->GetSkills()
                .GetSkill(SkillType::SMITHING)
                .GetXP(),
            smithingXPBefore,
            "Closed station interaction before completion awards no smithing XP");
        test.ExpectEqual(
            completedCount,
            0,
            "Closed station interaction before completion emits no completion event");
        test.ExpectEqual(
            startedCount,
            0,
            "Closed station interaction before completion does not repeat the action");

        test.Expect(
            !world.TryStartRecipeAction(
                playerID,
                RecipeType::BRONZE_BAR),
            "Closed station interaction keeps smithing unavailable until reopened");
    }

    {
        World world;
        int playerID = world.CreatePlayer();
        Player *player = GetPlayer(world, playerID);

        player->GetPosition().SetPosition(13, 9);
        player->GetInventory().AddItem(
            ItemType::COPPER_ORE,
            1);
        player->GetInventory().AddItem(
            ItemType::TIN_ORE,
            1);

        test.Expect(
            OpenFurnace(world, playerID),
            "Furnace opens for missing-ingredient completion test setup");

        test.Expect(
            world.TryStartRecipeAction(
                playerID,
                RecipeType::BRONZE_BAR),
            "Recipe action starts before ingredient removal");

        world.Update();

        player->GetInventory().RemoveItem(
            ItemType::TIN_ORE,
            1);

        const Inventory inventoryBefore =
            player->GetInventory();
        const int smithingXPBefore =
            player->GetSkills()
                .GetSkill(SkillType::SMITHING)
                .GetXP();

        int completedCount = 0;
        int startedCount = 0;

        AdvanceAndCountRecipeLifecycle(
            world,
            RecipeDatabase::Get(RecipeType::BRONZE_BAR)
                    .GetActionDurationTicks() +
                2,
            completedCount,
            startedCount);

        test.Expect(
            InventoriesMatch(
                player->GetInventory(),
                inventoryBefore),
            "Missing ingredients at completion leaves full inventory state unchanged");
        test.ExpectEqual(
            player->GetSkills()
                .GetSkill(SkillType::SMITHING)
                .GetXP(),
            smithingXPBefore,
            "Missing ingredients at completion awards no smithing XP");
        test.ExpectEqual(
            completedCount,
            0,
            "Missing ingredients at completion emits no completion event");
        test.ExpectEqual(
            startedCount,
            0,
            "Missing ingredients at completion does not repeat the action");
    }

    {
        World world;
        int playerID = world.CreatePlayer();
        Player *player = GetPlayer(world, playerID);

        player->GetPosition().SetPosition(13, 9);
        player->GetInventory().AddItem(
            ItemType::COPPER_ORE,
            1);
        player->GetInventory().AddItem(
            ItemType::TIN_ORE,
            1);

        test.Expect(
            OpenFurnace(world, playerID),
            "Furnace opens for single-cycle duplicate guard test setup");

        test.Expect(
            world.TryStartRecipeAction(
                playerID,
                RecipeType::BRONZE_BAR),
            "Single-cycle recipe action starts");

        world.Update();

        const int smithingXPBefore =
            player->GetSkills()
                .GetSkill(SkillType::SMITHING)
                .GetXP();
        const int copperBefore =
            player->GetInventory().GetItemAmount(ItemType::COPPER_ORE);
        const int tinBefore =
            player->GetInventory().GetItemAmount(ItemType::TIN_ORE);

        int completedCount = 0;
        int startedCount = 0;

        AdvanceAndCountRecipeLifecycle(
            world,
            RecipeDatabase::Get(RecipeType::BRONZE_BAR)
                    .GetActionDurationTicks() +
                6,
            completedCount,
            startedCount);

        test.ExpectEqual(
            player->GetInventory().GetItemAmount(ItemType::BRONZE_BAR),
            1,
            "One completed recipe cannot add output twice");
        test.ExpectEqual(
            smithingXPBefore +
                RecipeDatabase::Get(RecipeType::BRONZE_BAR)
                    .GetXPReward(),
            player->GetSkills()
                .GetSkill(SkillType::SMITHING)
                .GetXP(),
            "One completed recipe cannot award XP twice");
        test.ExpectEqual(
            copperBefore -
                RecipeDatabase::Get(RecipeType::BRONZE_BAR)
                    .GetIngredients()[0]
                    .amount,
            player->GetInventory().GetItemAmount(ItemType::COPPER_ORE),
            "One completed recipe cannot remove copper twice");
        test.ExpectEqual(
            tinBefore -
                RecipeDatabase::Get(RecipeType::BRONZE_BAR)
                    .GetIngredients()[1]
                    .amount,
            player->GetInventory().GetItemAmount(ItemType::TIN_ORE),
            "One completed recipe cannot remove tin twice");
        test.ExpectEqual(
            completedCount,
            1,
            "One-material-cycle recipe emits exactly one completion event");
        test.ExpectEqual(
            startedCount,
            0,
            "One-material-cycle recipe does not start a repeated action");
    }

    {
        World world;
        int attackerID = world.CreatePlayer();
        int defenderID = world.CreatePlayer();

        Player *attacker = GetPlayer(world, attackerID);
        Player *defender = GetPlayer(world, defenderID);

        attacker->GetPosition().SetPosition(10, 10);
        defender->GetPosition().SetPosition(11, 10);

        world.TryStartMeleeAttack(
            attackerID,
            defenderID,
            2);
        world.Update();

        const std::vector<ActionLifecycleEvent> *completionEvents = nullptr;
        test.Expect(
            AdvanceUntilCompletionEvent(
                world,
                10,
                completionEvents),
            "Melee action reaches completion");

        if (completionEvents != nullptr)
        {
            test.ExpectEqual(
                CountEvents<ActionCompletedEvent>(*completionEvents),
                1,
                "Successful melee cycle emits one completion event");
        }

        world.Update();
        world.Update();

        test.ExpectEqual(
            CountEvents<ActionCompletedEvent>(
                world.GetActionLifecycleEvents()),
            0,
            "Later ticks do not duplicate a prior completion event");
    }

    {
        World world;
        int attackerID = world.CreatePlayer();
        int defenderID = world.CreatePlayer();

        Player *attacker = GetPlayer(world, attackerID);
        Player *defender = GetPlayer(world, defenderID);

        attacker->GetPosition().SetPosition(10, 10);
        defender->GetPosition().SetPosition(11, 10);

        world.TryStartMeleeAttack(
            attackerID,
            defenderID,
            3);
        world.CancelActionsForEntity(
            attackerID,
            ActionCancelReason::INTERFACE_CLOSED);

        world.Update();

        test.ExpectEqual(
            CountEvents<ActionCompletedEvent>(
                world.GetActionLifecycleEvents()),
            0,
            "Cancelled actions emit no completion events");
    }

    {
        World world;
        int attackerID = world.CreatePlayer();
        int defenderID = world.CreatePlayer();

        Player *attacker = GetPlayer(world, attackerID);
        Player *defender = GetPlayer(world, defenderID);

        attacker->GetPosition().SetPosition(10, 10);
        defender->GetPosition().SetPosition(11, 10);

        world.TryStartMeleeAttack(
            attackerID,
            defenderID,
            2);
        world.Update();

        defender->GetPosition().SetPosition(40, 40);

        world.Update();

        test.ExpectEqual(
            CountEvents<ActionCompletedEvent>(
                world.GetActionLifecycleEvents()),
            0,
            "Failed completion validation emits no successful completion event");
    }

    {
        World world;
        int attackerID = world.CreatePlayer();
        int defenderID = world.CreatePlayer();

        Player *attacker = GetPlayer(world, attackerID);
        Player *defender = GetPlayer(world, defenderID);

        attacker->GetPosition().SetPosition(10, 10);
        defender->GetPosition().SetPosition(11, 10);

        world.TryStartMeleeAttack(
            attackerID,
            defenderID,
            3);

        world.QueueMovementRequest(
            MovementRequest(attackerID, 1, 0));
        world.Update();

        test.ExpectEqual(
            CountEvents<ActionCancelledEvent>(
                world.GetActionLifecycleEvents()),
            1,
            "Movement cancellation emits exactly one cancellation event");
        test.Expect(
            ContainsCancelledReason(
                world.GetActionLifecycleEvents(),
                ActionCancelReason::PLAYER_MOVED),
            "Movement cancellation emits PLAYER_MOVED reason");
    }

    {
        World world;
        int playerID = world.CreatePlayer();
        Player *player = GetPlayer(world, playerID);

        test.Expect(
            EquipItem(
                world,
                playerID,
                ItemType::BRONZE_AXE),
            "Bronze axe equips for invalid-tool cancellation test setup");

        player->GetPosition().SetPosition(4, 5);

        ResourceNode *resource = world.GetResourceAt(5, 5);
        world.QueueResourceInteraction(
            playerID,
            resource->GetID());
        world.Update();

        world.TryUnequipItem(
            playerID,
            EquipmentSlotType::WEAPON);
        world.Update();

        test.ExpectEqual(
            CountEvents<ActionCancelledEvent>(
                world.GetActionLifecycleEvents()),
            1,
            "Tool removal emits exactly one cancellation event");
        test.Expect(
            ContainsCancelledReason(
                world.GetActionLifecycleEvents(),
                ActionCancelReason::INVALID_TOOL),
            "Tool removal cancellation uses INVALID_TOOL reason");
    }

    {
        World world;
        int attackerID = world.CreatePlayer();
        int defenderID = world.CreatePlayer();

        Player *attacker = GetPlayer(world, attackerID);
        Player *defender = GetPlayer(world, defenderID);

        attacker->GetPosition().SetPosition(10, 10);
        defender->GetPosition().SetPosition(11, 10);

        world.TryStartMeleeEngagement(
            attackerID,
            defenderID,
            4);

        attacker->ApplyDamage(99999);
        world.Update();

        test.Expect(
            ContainsCancelledReason(
                world.GetActionLifecycleEvents(),
                ActionCancelReason::ENTITY_DIED),
            "Entity-death cleanup cancellation emits ENTITY_DIED reason");
    }

    {
        World world;
        int attackerID = world.CreatePlayer();
        int defenderID = world.CreatePlayer();

        Player *attacker = GetPlayer(world, attackerID);
        Player *defender = GetPlayer(world, defenderID);

        attacker->GetPosition().SetPosition(10, 10);
        defender->GetPosition().SetPosition(11, 10);

        world.TryStartMeleeAttack(
            attackerID,
            defenderID,
            3);
        world.CancelActionsForEntity(
            attackerID,
            ActionCancelReason::INTERFACE_CLOSED);
        world.Update();

        test.Expect(
            ContainsCancelledReason(
                world.GetActionLifecycleEvents(),
                ActionCancelReason::INTERFACE_CLOSED),
            "Explicit cancellation emits the requested reason");
    }

    {
        World world;

        world.CancelActionsForEntity(
            999999,
            ActionCancelReason::INTERFACE_CLOSED);
        world.Update();

        test.ExpectEqual(
            CountEvents<ActionCancelledEvent>(
                world.GetActionLifecycleEvents()),
            0,
            "Cancelling when no action exists emits no cancellation event");
    }

    {
        World world;
        int attackerID = world.CreatePlayer();
        int defenderID = world.CreatePlayer();

        Player *attacker = GetPlayer(world, attackerID);
        Player *defender = GetPlayer(world, defenderID);

        attacker->GetPosition().SetPosition(10, 10);
        defender->GetPosition().SetPosition(11, 10);

        world.TryStartMeleeAttack(
            attackerID,
            defenderID,
            3);
        world.CancelActionsForEntity(
            attackerID,
            ActionCancelReason::INTERFACE_CLOSED);
        world.CancelActionsForEntity(
            attackerID,
            ActionCancelReason::INTERFACE_CLOSED);

        world.Update();

        test.ExpectEqual(
            CountEvents<ActionCancelledEvent>(
                world.GetActionLifecycleEvents()),
            1,
            "Repeated cancellation requests do not duplicate events");
    }

    {
        World world;
        int playerID = world.CreatePlayer();

        Player *player = GetPlayer(world, playerID);
        player->GetPosition().SetPosition(4, 5);

        test.Expect(
            EquipItem(
                world,
                playerID,
                ItemType::BRONZE_AXE),
            "Bronze axe equips for replacement-order test setup");

        ResourceNode *resource = world.GetResourceAt(5, 5);
        world.QueueResourceInteraction(
            playerID,
            resource->GetID());
        world.Update();

        int monsterID = world.CreateMonster(
            5,
            4,
            CombatRatings{8, 8, 8, 30});

        world.QueueMeleeEngagementRequest(
            playerID,
            monsterID,
            3);
        world.Update();

        const std::vector<ActionLifecycleEvent> &events =
            world.GetActionLifecycleEvents();

        int cancelIndex = -1;
        int startIndex = -1;

        for (int index = 0;
             index < static_cast<int>(events.size());
             ++index)
        {
            if (cancelIndex < 0)
            {
                const ActionCancelledEvent *cancelled =
                    std::get_if<ActionCancelledEvent>(
                        &events[index].data);

                if (cancelled != nullptr &&
                    cancelled->ownerEntityID == playerID)
                {
                    cancelIndex = index;
                }
            }

            if (startIndex < 0)
            {
                const ActionStartedEvent *started =
                    std::get_if<ActionStartedEvent>(
                        &events[index].data);

                if (started != nullptr &&
                    started->ownerEntityID == playerID &&
                    started->actionType == ActionType::MELEE_ATTACK)
                {
                    startIndex = index;
                }
            }
        }

        test.Expect(
            cancelIndex >= 0 &&
                startIndex >= 0 &&
                cancelIndex < startIndex,
            "Replacing an action records cancellation before the new start");
    }

    {
        World world;
        int attackerID = world.CreatePlayer();
        int defenderID = world.CreatePlayer();

        Player *attacker = GetPlayer(world, attackerID);
        Player *defender = GetPlayer(world, defenderID);

        attacker->GetPosition().SetPosition(10, 10);
        defender->GetPosition().SetPosition(11, 10);

        world.TryStartMeleeEngagement(
            attackerID,
            defenderID,
            1);
        world.Update();

        const std::vector<ActionLifecycleEvent> &events =
            world.GetActionLifecycleEvents();

        int completionIndex = -1;
        int restartedIndex = -1;
        int startedCount = 0;

        for (int index = 0;
             index < static_cast<int>(events.size());
             ++index)
        {
            if (std::holds_alternative<ActionStartedEvent>(
                    events[index].data))
            {
                startedCount++;
                if (startedCount == 2)
                {
                    restartedIndex = index;
                }
            }

            if (completionIndex < 0 &&
                std::holds_alternative<ActionCompletedEvent>(
                    events[index].data))
            {
                completionIndex = index;
            }
        }

        test.Expect(
            completionIndex >= 0 &&
                restartedIndex >= 0 &&
                completionIndex < restartedIndex,
            "Repeat cycles record completion before next-cycle start");
    }

    {
        World world;

        int attackerA = world.CreatePlayer();
        int defenderA = world.CreatePlayer();
        int attackerB = world.CreatePlayer();
        int defenderB = world.CreatePlayer();

        GetPlayer(world, attackerA)->GetPosition().SetPosition(10, 10);
        GetPlayer(world, defenderA)->GetPosition().SetPosition(11, 10);

        GetPlayer(world, attackerB)->GetPosition().SetPosition(20, 20);
        GetPlayer(world, defenderB)->GetPosition().SetPosition(21, 20);

        world.TryStartMeleeAttack(
            attackerA,
            defenderA,
            1);
        world.TryStartMeleeAttack(
            attackerB,
            defenderB,
            1);

        world.Update();

        const std::vector<ActionLifecycleEvent> &events =
            world.GetActionLifecycleEvents();

        std::vector<int> completionOwners;

        for (const ActionLifecycleEvent &event : events)
        {
            const ActionCompletedEvent *completed =
                std::get_if<ActionCompletedEvent>(
                    &event.data);

            if (completed != nullptr)
            {
                completionOwners.push_back(
                    completed->ownerEntityID);
            }
        }

        test.ExpectEqual(
            static_cast<int>(completionOwners.size()),
            2,
            "Two entities can complete actions in one authoritative tick");

        if (completionOwners.size() == 2)
        {
            test.ExpectEqual(
                completionOwners[0],
                attackerA,
                "Completion order matches authoritative processing order");
            test.ExpectEqual(
                completionOwners[1],
                attackerB,
                "Completion order remains deterministic for the second entity");
        }
    }

    {
        World world;
        int attackerID = world.CreatePlayer();
        int defenderID = world.CreatePlayer();

        Player *attacker = GetPlayer(world, attackerID);
        Player *defender = GetPlayer(world, defenderID);

        attacker->GetPosition().SetPosition(10, 10);
        defender->GetPosition().SetPosition(11, 10);

        world.TryStartMeleeAttack(
            attackerID,
            defenderID,
            1);
        world.Update();

        test.Expect(
            world.GetActionForEntity(attackerID) == nullptr,
            "Completed non-repeating action is removed from the action manager");

        const ActionCompletedEvent *completed =
            FindFirstEvent<ActionCompletedEvent>(
                world.GetActionLifecycleEvents());

        test.Expect(
            completed != nullptr,
            "Completion event remains available after action removal");

        if (completed != nullptr)
        {
            test.ExpectEqual(
                completed->ownerEntityID,
                attackerID,
                "Completion event stores owner ID after action removal");
            test.ExpectEqual(
                completed->targetID,
                defenderID,
                "Completion event stores target ID after action removal");
        }
    }

    return test.Finish();
}
