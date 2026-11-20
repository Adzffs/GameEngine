#include "../Graphics/Graphics.h"
#include "../World/World.h"
#include "../Player/Player.h"
#include "../Content/Import/StarterWorldRuntimeInput.h"
#include <charconv>
#include <filesystem>
#include <iostream>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string_view>

namespace
{
    int Coordinate(std::string_view value)
    {
        int coordinate = 0;
        const auto result = std::from_chars(value.data(), value.data() + value.size(), coordinate);
        if (result.ec != std::errc{} || result.ptr != value.data() + value.size())
            throw std::invalid_argument("Focus coordinates must be integers");
        return coordinate;
    }
}

int main(int argc, char **argv)
{
    try
    {
        if (argc < 3)
            throw std::invalid_argument("Usage: EmbervaleRenderPreview WORLD.json OUTPUT.png [--basic] [--deplete-trees] [--deplete-rocks] [--focus X Y] [--zoom 1|1.5] [--condition new|weathered|repaired] [--walk-to X Y | --chop X Y | --mine X Y] [--frames N]");
        const auto input = std::filesystem::path(reinterpret_cast<const char8_t *>(argv[1]));
        const auto output = std::filesystem::path(reinterpret_cast<const char8_t *>(argv[2]));
        std::error_code equivalentError;
        if (output.extension() != ".png" ||
            std::filesystem::weakly_canonical(input) == std::filesystem::weakly_canonical(output) ||
            std::filesystem::equivalent(input, output, equivalentError))
            throw std::invalid_argument("Preview output must be a separate .png file");
        bool basic = false, deplete = false, depleteRocks = false;
        std::optional<std::pair<int, int>> focus;
        std::optional<std::pair<int, int>> walkTo;
        std::optional<std::pair<int, int>> chop;
        std::optional<std::pair<int, int>> mine;
        float zoom = 1.0f;
        int frames = 0, condition = -1;
        for (int index = 3; index < argc; ++index)
        {
            const std::string_view option = argv[index];
            if (option == "--basic") basic = true;
            else if (option == "--condition" && index + 1 < argc)
            {
                const std::string_view value = argv[++index];
                if (value == "new") condition = 0;
                else if (value == "weathered") condition = 1;
                else if (value == "repaired") condition = 2;
                else throw std::invalid_argument("Building condition must be new, weathered or repaired");
            }
            else if (option == "--deplete-trees") deplete = true;
            else if (option == "--deplete-rocks") depleteRocks = true;
            else if (option == "--focus" && index + 2 < argc)
            {
                const int x = Coordinate(argv[++index]);
                const int y = Coordinate(argv[++index]);
                focus = std::pair{x, y};
            }
            else if (option == "--walk-to" && index + 2 < argc)
            {
                const int x = Coordinate(argv[++index]);
                const int y = Coordinate(argv[++index]);
                walkTo = std::pair{x, y};
            }
            else if ((option == "--chop" || option == "--mine") && index + 2 < argc)
            {
                const int x = Coordinate(argv[++index]);
                const int y = Coordinate(argv[++index]);
                (option == "--chop" ? chop : mine) = std::pair{x, y};
            }
            else if (option == "--zoom" && index + 1 < argc)
            {
                const std::string_view value = argv[++index];
                if (value == "1") zoom = 1.0f;
                else if (value == "1.5") zoom = 1.5f;
                else throw std::invalid_argument("Preview zoom must be 1 or 1.5");
            }
            else if (option == "--frames" && index + 1 < argc)
            {
                frames = Coordinate(argv[++index]);
                if (frames < 1 || frames > 120) throw std::invalid_argument("Preview frame count must be 1..120");
            }
            else throw std::invalid_argument("Unknown or incomplete preview option");
        }
        // Fixed, independent streams keep authored captures reproducible.
        // Production game launches retain their normal random sources.
        World world(StarterWorldImport::LoadFile(input), 1337U, 7331U, 1U);
        if (chop && (mine || walkTo || deplete)) throw std::invalid_argument("Chopping preview requires an intact target and no walk-to fixture");
        if (mine && (walkTo || depleteRocks)) throw std::invalid_argument("Mining preview requires an intact target and no walk-to fixture");
        if (walkTo && !world.GetMap().IsValidPosition(walkTo->first, walkTo->second))
            throw std::invalid_argument("Walk preview destination must be walkable");
        const int playerID = world.CreatePlayer(PlayerInitializationMode::FRESH_START);
        auto *player = dynamic_cast<Player *>(world.GetEntityByID(playerID));
        if (!player) throw std::runtime_error("Preview player could not be created");
        if (focus)
        {
            if (!world.GetMap().IsValidPosition(focus->first, focus->second))
                throw std::invalid_argument("Focus must be a walkable map position");
            player->GetPosition().SetPosition(focus->first, focus->second);
        }
        if (deplete || depleteRocks)
            for (const auto &resource : world.GetResources())
                if ((deplete && resource.GetResourceType() == ResourceType::NORMAL_TREE) ||
                    (depleteRocks && (resource.GetResourceType() == ResourceType::COPPER_ROCK ||
                                     resource.GetResourceType() == ResourceType::TIN_ROCK)))
                {
                    auto *node = world.GetResourceAt(resource.GetX(), resource.GetY());
                    while (node->IsActive()) node->ConsumeUse();
                }

        const auto gatheringTarget = chop ? chop : mine;
        if (gatheringTarget)
        {
            auto *node = world.GetResourceAt(gatheringTarget->first, gatheringTarget->second);
            const bool valid = node && (chop ? node->GetResourceType() == ResourceType::NORMAL_TREE :
                node->GetResourceType() == ResourceType::COPPER_ROCK || node->GetResourceType() == ResourceType::TIN_ROCK);
            if (!valid) throw std::invalid_argument("Fresh gathering preview requires a normal tree or copper/tin rock");
            const ItemType tool = chop ? ItemType::BRONZE_AXE : ItemType::BRONZE_PICKAXE;
            const auto &slots = player->GetInventory().GetSlots();
            bool equipped = false;
            for (std::size_t slot = 0; slot < slots.size(); ++slot)
                if (slots[slot].GetItemType() == tool)
                {
                    equipped = world.TryEquipInventoryItem(playerID, static_cast<int>(slot));
                    break;
                }
            if (!equipped) throw std::runtime_error("Preview could not equip the starting gathering tool");
            world.QueueResourceInteraction(playerID, node->GetID());
            world.Update();
            const auto *action = world.GetActionForEntity(playerID);
            if (!action || action->GetType() != ActionType::GATHERING || action->GetTargetID() != node->GetID())
                throw std::invalid_argument("Gathering focus must be adjacent to its target");
        }

        SDL_SetHint(SDL_HINT_VIDEO_DRIVER, "dummy");
        SDL_SetHint(SDL_HINT_RENDER_DRIVER, "software");
        Graphics graphics;
        if (!graphics.Initialize()) throw std::runtime_error("Preview renderer initialization failed");
        graphics.SynchronizeGatheringRewards(playerID, world.GetGatheringRewardEvents());
        graphics.SetCompiledMapLabelsVisible(false);
        graphics.SetBuildingConditionPreview(condition);
        if (basic)
        {
            SDL_Event toggle{};
            toggle.type = SDL_EVENT_KEY_DOWN;
            toggle.key.scancode = SDL_SCANCODE_F6;
            SDL_PushEvent(&toggle);
            bool running = true;
            graphics.ProcessEvents(running);
        }
        std::unique_ptr<SDL_Texture, decltype(&SDL_DestroyTexture)> target(
            SDL_CreateTexture(graphics.renderer, SDL_PIXELFORMAT_RGBA8888,
                SDL_TEXTUREACCESS_TARGET, Graphics::WindowWidth, Graphics::WindowHeight), SDL_DestroyTexture);
        if (!target || !SDL_SetRenderTarget(graphics.renderer, target.get()))
            throw std::runtime_error(SDL_GetError());
        const auto draw = [&]
        {
            graphics.Render(world.GetMap(), world.GetEntities(), world.GetResources(), world.GetStations(),
                world.GetGroundItems(), {}, player->GetPosition().GetX(), player->GetPosition().GetY(),
                player->GetInventory(), player->GetEquipment(), *player, world.GetActionForEntity(playerID));
        };
        // Initialize the real camera before applying the same anchored zoom
        // used by mouse-wheel input in the game.
        draw();
        while (graphics.GetCameraZoom() + 0.001f < zoom)
        {
            const float before = graphics.GetCameraZoom();
            graphics.AdjustCameraZoom(1, Graphics::WindowWidth * 0.5f, Graphics::WindowHeight * 0.5f);
            if (graphics.GetCameraZoom() == before) throw std::runtime_error("Requested preview zoom is unavailable");
        }
        const int frameCount = frames > 0 ? frames : (gatheringTarget ? 18 : walkTo ? 2 : 1);
        for (int frame = 0; frame < frameCount; ++frame)
        {
            if (frame > 0)
            {
                if (walkTo) player->GetPosition().SetPosition(walkTo->first, walkTo->second);
                // Bounded presentation fixture: advance real gathering every
                // fifteen 40-ms frames and consume each authoritative result.
                if (gatheringTarget && frame % 15 == 0)
                {
                    world.Update();
                    graphics.SynchronizeGatheringRewards(playerID, world.GetGatheringRewardEvents());
                }
                SDL_Delay(40);
            }
            draw();
        }
        std::unique_ptr<SDL_Surface, decltype(&SDL_DestroySurface)> pixels(
            SDL_RenderReadPixels(graphics.renderer, nullptr), SDL_DestroySurface);
        if (!pixels) throw std::runtime_error(SDL_GetError());
        if (!output.parent_path().empty()) std::filesystem::create_directories(output.parent_path());
        if (!SDL_SavePNG(pixels.get(), argv[2])) throw std::runtime_error(SDL_GetError());
        SDL_SetRenderTarget(graphics.renderer, nullptr);
        std::cout << "Rendered " << pixels->w << 'x' << pixels->h
            << " through the game renderer; " << (basic ? "basic" : "world")
            << " art, " << (mine ? "mining" : chop ? "woodcutting" : walkTo ? "walking" : "static")
            << " preview. No player or world files written.\n";
        return 0;
    }
    catch (const std::exception &error)
    {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
