#include "Engine.h"
#include <chrono>
#include <thread>
#include "Core/Logger.h"
#include "../Command/ServerCommand.h"
#include "../Equipment/EquipmentSlotType.h"
#include "../Inventory/ItemType.h"
#include "../Player/Player.h"
#include "../Skills/SkillType.h"
#include "../Recipe/RecipeType.h"
#include "EngineClickRouting.h"

namespace
{
    int FindInventorySlot(
        const Inventory &inventory,
        ItemType itemType)
    {
        const auto &slots = inventory.GetSlots();

        for (int slotIndex = 0;
             slotIndex < static_cast<int>(slots.size());
             ++slotIndex)
        {
            if (!slots[slotIndex].IsEmpty() &&
                slots[slotIndex].GetItemType() == itemType)
            {
                return slotIndex;
            }
        }

        return -1;
    }
}

Engine::Engine()
    : playerID(-1)
{
    if (!graphics.Initialize())
    {
        running = false;
        return;
    }

    playerID = world.CreatePlayer();

    Player *player =
        dynamic_cast<Player *>(
            world.GetEntityByID(playerID));

    if (player != nullptr &&
        player->GetInventory().AddItem(
            ItemType::DEVELOPER_GODSWORD,
            1))
    {
        int weaponSlotIndex = FindInventorySlot(
            player->GetInventory(),
            ItemType::DEVELOPER_GODSWORD);

        if (weaponSlotIndex >= 0)
        {
            world.EnqueueCommand(
                UseInventoryItemCommand{
                    playerID,
                    weaponSlotIndex});
        }
    }
}

void Engine::Run()
{
    Logger::Info("Server Started");

    while (running)
    {
        graphics.ProcessEvents(running);

        if (!running)
        {
            break;
        }

        RecipeType requestedRecipe =
            RecipeType::NONE;

        if (graphics.ConsumeRecipeRequest(
                requestedRecipe))
        {
            world.EnqueueCommand(
                StartRecipeCommand{
                    playerID,
                    requestedRecipe});
        }

        int clickedInventorySlot = -1;

        if (graphics.ConsumeInventorySlotClick(
                clickedInventorySlot))
        {
            world.EnqueueCommand(
                UseInventoryItemCommand{
                    playerID,
                    clickedInventorySlot});
        }

        if (graphics.ConsumeWeaponSlotClick())
        {
            world.EnqueueCommand(
                UnequipItemCommand{
                    playerID,
                    EquipmentSlotType::WEAPON});
        }

        int clickedTileX;
        int clickedTileY;
        int clickedMouseX;
        int clickedMouseY;

        if (graphics.ConsumeClickedTile(
                clickedTileX,
                clickedTileY,
                clickedMouseX,
                clickedMouseY))
        {
            EngineClickRouting::HandleWorldClick(
                graphics,
                world,
                playerID,
                clickedTileX,
                clickedTileY,
                clickedMouseX,
                clickedMouseY);
        }

        if (graphics.ConsumeStationMenuClose())
        {
            world.EnqueueCommand(
                CloseStationCommand{playerID});
        }

        const Clock::TimePoint now =
            Clock::ClockType::now();

        const int dueTickCount =
            clock.GetDueTickCount(
                now,
                MaxCatchUpTicks);

        if (dueTickCount > 0)
        {
            for (int index = 0;
                 index < dueTickCount;
                 ++index)
            {
                clock.AdvanceTickDeadline();
                Update();
            }

            const Clock::TimePoint afterCatchUp =
                Clock::ClockType::now();

            if (clock.IsTickDue(afterCatchUp))
            {
                const long long backlogMs =
                    std::chrono::duration_cast<std::chrono::milliseconds>(
                        clock.GetBacklogDuration(afterCatchUp))
                        .count();

                Logger::Warn(
                    "Tick backlog exceeded catch-up limit; "
                    "clock resynchronised"
                    " | tick=" +
                    std::to_string(
                        world.GetCurrentTick()) +
                    " | processed=" +
                    std::to_string(dueTickCount) +
                    " | backlogMs=" +
                    std::to_string(backlogMs));

                clock.Resynchronise(afterCatchUp);
            }
        }
        else
        {
            const Clock::TimePoint sleepDeadline =
                clock.GetNextTickDeadline();

            const Clock::TimePoint sleepStart =
                Clock::ClockType::now();

            if (sleepDeadline > sleepStart)
            {
                constexpr auto MaxIdleSleep =
                    std::chrono::milliseconds(5);

                const auto remaining =
                    sleepDeadline - sleepStart;

                if (remaining > MaxIdleSleep)
                {
                    std::this_thread::sleep_for(
                        MaxIdleSleep);
                }
                else
                {
                    std::this_thread::sleep_until(
                        sleepDeadline);
                }
            }
        }

        Player *player =
            dynamic_cast<Player *>(
                world.GetEntityByID(playerID));

        StationType openedStationType =
            StationType::NONE;

        if (world.ConsumeOpenedStation(
                playerID,
                openedStationType))
        {
            graphics.OpenStationMenu(
                openedStationType);
        }

        if (player != nullptr)
        {
            const Skill &woodcutting =
                player->GetSkills().GetSkill(
                    SkillType::WOODCUTTING);

            const Skill &mining =
                player->GetSkills().GetSkill(
                    SkillType::MINING);

            graphics.Render(
                world.GetMap(),
                world.GetEntities(),
                world.GetResources(),
                world.GetStations(),
                world.GetMeleeCombatFeedbacks(),
                player->GetPosition().GetX(),
                player->GetPosition().GetY(),
                player->GetInventory(),
                player->GetEquipment(),
                *player,
                world.GetActionForEntity(
                    playerID));
        }
    }
    Logger::Info("Server stopped");
}

void Engine::Update()
{
    const Clock::TimePoint updateStart =
        Clock::ClockType::now();

    world.Update();

    const Clock::TimePoint updateEnd =
        Clock::ClockType::now();

    const auto updateDuration =
        std::chrono::duration_cast<std::chrono::microseconds>(
            updateEnd - updateStart);

    const bool overrun =
        RecordTickDuration(
            tickPerformanceStats,
            updateDuration,
            std::chrono::duration_cast<std::chrono::milliseconds>(
                clock.GetTickInterval()));

    if (overrun)
    {
        const long long durationMs =
            std::chrono::duration_cast<std::chrono::milliseconds>(
                updateDuration)
                .count();

        const long long budgetMs =
            std::chrono::duration_cast<std::chrono::milliseconds>(
                clock.GetTickInterval())
                .count();

        Logger::Warn(
            "Tick " +
            std::to_string(
                world.GetCurrentTick()) +
            " exceeded budget: " +
            std::to_string(durationMs) +
            " ms / " +
            std::to_string(budgetMs) +
            " ms");
    }
}
