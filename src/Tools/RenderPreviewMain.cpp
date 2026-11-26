#include "../Graphics/Graphics.h"
#include "../World/World.h"
#include "../Player/Player.h"
#include "../Entity/Monster/Monster.h"
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
    void SaveFrame(SDL_Renderer *renderer, const std::filesystem::path &path)
    {
        std::unique_ptr<SDL_Surface, decltype(&SDL_DestroySurface)> pixels(
            SDL_RenderReadPixels(renderer, nullptr), SDL_DestroySurface);
        const auto name = path.u8string();
        if (!pixels || !SDL_SavePNG(pixels.get(), reinterpret_cast<const char *>(name.c_str())))
            throw std::runtime_error(SDL_GetError());
    }

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
            throw std::invalid_argument("Usage: EmbervaleRenderPreview WORLD.json OUTPUT.png [--basic] [--deplete-trees] [--deplete-rocks] [--focus X Y] [--zoom 1|1.5] [--condition new|weathered|repaired] [--walk-to X Y [--walk-via X Y] | --chop X Y | --mine X Y | --melee X Y] [--weapon bronze-sword|godsword|bronze-axe|iron-axe|steel-axe|bronze-pickaxe|iron-pickaxe|steel-pickaxe|unarmed] [--shield] [--frames N] [--fit-outfit full|knee] [--fit-scale world|native] [--frames-to NEW_DIRECTORY]");
        const auto input = std::filesystem::path(reinterpret_cast<const char8_t *>(argv[1]));
        const auto output = std::filesystem::path(reinterpret_cast<const char8_t *>(argv[2]));
        std::error_code equivalentError;
        if (output.extension() != ".png" ||
            std::filesystem::weakly_canonical(input) == std::filesystem::weakly_canonical(output) ||
            std::filesystem::equivalent(input, output, equivalentError))
            throw std::invalid_argument("Preview output must be a separate .png file");
        bool basic = false, deplete = false, depleteRocks = false;
        std::optional<std::pair<int, int>> focus;
        std::optional<std::pair<int, int>> walkTo, walkVia;
        std::optional<std::pair<int, int>> chop;
        std::optional<std::pair<int, int>> mine;
        std::optional<std::pair<int, int>> melee;
        std::optional<ItemType> weapon;
        bool shield = false;
        std::optional<std::string_view> fitOutfit, fitScale;
        std::optional<std::filesystem::path> sequence;
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
            else if ((option == "--walk-to" || option == "--walk-via") && index + 2 < argc)
            {
                const int x = Coordinate(argv[++index]);
                const int y = Coordinate(argv[++index]);
                (option == "--walk-to" ? walkTo : walkVia) = std::pair{x, y};
            }
            else if ((option == "--chop" || option == "--mine") && index + 2 < argc)
            {
                const int x = Coordinate(argv[++index]);
                const int y = Coordinate(argv[++index]);
                (option == "--chop" ? chop : mine) = std::pair{x, y};
            }
            else if (option == "--melee" && index + 2 < argc)
            {
                const int x = Coordinate(argv[++index]);
                const int y = Coordinate(argv[++index]);
                melee = std::pair{x,y};
            }
            else if (option == "--fit-outfit" && index + 1 < argc)
            {
                fitOutfit = argv[++index];
                if (*fitOutfit != "full" && *fitOutfit != "knee")
                    throw std::invalid_argument("Fit outfit must be full or knee");
            }
            else if (option == "--fit-scale" && index + 1 < argc)
            {
                fitScale = argv[++index];
                if (*fitScale != "world" && *fitScale != "native")
                    throw std::invalid_argument("Fit scale must be world or native");
            }
            else if (option == "--frames-to" && index + 1 < argc)
                sequence = std::filesystem::path(reinterpret_cast<const char8_t *>(argv[++index]));
            else if (option == "--shield") shield = true;
            else if (option == "--weapon" && index + 1 < argc)
            {
                const std::string_view value = argv[++index];
                if (value == "bronze-sword") weapon = ItemType::BRONZE_SWORD;
                else if (value == "godsword") weapon = ItemType::DEVELOPER_GODSWORD;
                else if (value == "bronze-axe") weapon = ItemType::BRONZE_AXE;
                else if (value == "iron-axe") weapon = ItemType::IRON_AXE;
                else if (value == "steel-axe") weapon = ItemType::STEEL_AXE;
                else if (value == "bronze-pickaxe") weapon = ItemType::BRONZE_PICKAXE;
                else if (value == "iron-pickaxe") weapon = ItemType::IRON_PICKAXE;
                else if (value == "steel-pickaxe") weapon = ItemType::STEEL_PICKAXE;
                else if (value == "unarmed") weapon = ItemType::NONE;
                else throw std::invalid_argument("Unknown preview weapon");
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
        if (fitScale && !fitOutfit) throw std::invalid_argument("Fit scale requires a fit outfit");
        if (fitOutfit && basic) throw std::invalid_argument("Character comparison requires world art");
        if (sequence && (sequence->empty() || !frames)) throw std::invalid_argument("Frame export requires a new directory and --frames N");
        if (walkVia && (!walkTo || !sequence))
            throw std::invalid_argument("A walk waypoint requires --walk-to and --frames-to");
        // Fixed, independent streams keep authored captures reproducible.
        // Production game launches retain their normal random sources.
        World world(StarterWorldImport::LoadFile(input), 1337U, 7331U, 1U);
        if (static_cast<int>(walkTo.has_value()) + static_cast<int>(chop.has_value()) +
            static_cast<int>(mine.has_value()) + static_cast<int>(melee.has_value()) > 1)
            throw std::invalid_argument("Select only one walk, chopping, mining or melee fixture");
        if (chop && deplete) throw std::invalid_argument("Chopping requires an intact tree");
        if (mine && depleteRocks) throw std::invalid_argument("Mining requires an intact rock");
        if ((chop || mine) && (weapon || shield))
            throw std::invalid_argument("Gathering previews select their own equipped tool");
        if (walkTo && !world.GetMap().IsValidPosition(walkTo->first, walkTo->second))
            throw std::invalid_argument("Walk preview destination must be walkable");
        if (walkVia && !world.GetMap().IsValidPosition(walkVia->first, walkVia->second))
            throw std::invalid_argument("Walk preview waypoint must be walkable");
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

        // Equipment choices are isolated presentation fixtures, never saved.
        if (weapon || melee)
            player->GetEquipment().Equip(EquipmentSlotType::WEAPON,weapon.value_or(ItemType::BRONZE_SWORD));
        if (shield) player->GetEquipment().Equip(EquipmentSlotType::SHIELD,ItemType::WOODEN_SHIELD);
        int meleeTargetID = -1;
        if (melee)
        {
            for (const auto &entity : world.GetEntities())
                if (const auto *monster = dynamic_cast<const Monster *>(entity.get()); monster && monster->IsAlive() &&
                    monster->GetPosition().GetX() == melee->first && monster->GetPosition().GetY() == melee->second)
                    meleeTargetID = monster->GetID();
            if (meleeTargetID < 0 || !world.TryStartMeleeAttack(playerID,meleeTargetID,2))
                throw std::invalid_argument("Melee preview requires an adjacent living monster at the selected coordinates");
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
        if (fitOutfit)
            graphics.EnableCharacterFitPreview("assets/embervale/custom-character-pilot", *fitOutfit == "knee", fitScale == "native");
        // A capture sequence uses a private 25-Hz clock for the presentation.
        constexpr Uint64 firstTick = 1000;
        if (sequence) graphics.SetCaptureTick(firstTick);
        graphics.SynchronizeGatheringRewards(playerID, world.GetGatheringRewardEvents());
        graphics.SynchronizeMeleeResults(playerID, world.GetMeleeCombatEvents());
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
                world.GetGroundItems(), world.GetMeleeCombatFeedbacks(), player->GetPosition().GetX(), player->GetPosition().GetY(),
                player->GetInventory(), player->GetEquipment(), *player, world.GetActionForEntity(playerID), meleeTargetID);
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
        const int frameCount = frames > 0 ? frames : (melee ? 32 : gatheringTarget ? 18 : walkTo ? 2 : 1);
        if (sequence)
        {
            if (!std::filesystem::create_directories(*sequence))
                throw std::invalid_argument("Frame output directory already exists");
            if (walkTo)
            {
                const auto first = walkVia.value_or(*walkTo);
                world.QueueMovementDestination({playerID, first.first, first.second});
            }
        }
        for (int frame = 0; frame < frameCount; ++frame)
        {
            if (sequence) graphics.SetCaptureTick(firstTick + frame * 40);
            if (frame > 0)
            {
                if (walkTo && sequence)
                {
                    if ((frame - 1) % 15 == 0)
                    {
                        if (walkVia && player->GetPosition().GetX() == walkVia->first &&
                            player->GetPosition().GetY() == walkVia->second)
                        {
                            world.QueueMovementDestination({playerID, walkTo->first, walkTo->second});
                            walkVia.reset();
                        }
                        world.Update();
                    }
                }
                else if (walkTo) player->GetPosition().SetPosition(walkTo->first, walkTo->second);
                // Bounded presentation fixture: advance real gathering every
                // fifteen 40-ms frames and consume each authoritative result.
                if (gatheringTarget && frame % 15 == 0)
                {
                    world.Update();
                    graphics.SynchronizeGatheringRewards(playerID, world.GetGatheringRewardEvents());
                }
                if (melee && frame % 15 == 0)
                {
                    world.Update();
                    graphics.SynchronizeMeleeResults(playerID, world.GetMeleeCombatEvents());
                }
                if (!sequence) SDL_Delay(40);
            }
            draw();
            if (sequence)
            {
                const std::string digits = std::to_string(1000 + frame).substr(1);
                SaveFrame(graphics.renderer, *sequence / ("frame-" + digits + ".png"));
            }
        }
        if (!output.parent_path().empty()) std::filesystem::create_directories(output.parent_path());
        SaveFrame(graphics.renderer, output);
        SDL_SetRenderTarget(graphics.renderer, nullptr);
        std::cout << "Rendered " << Graphics::WindowWidth << 'x' << Graphics::WindowHeight
            << " through the game renderer; " << (basic ? "basic" : "world")
            << " art, " << (melee ? "melee" : mine ? "mining" : chop ? "woodcutting" : walkTo ? "walking" : "static")
            << " preview. No player or world files written.\n";
        return 0;
    }
    catch (const std::exception &error)
    {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
