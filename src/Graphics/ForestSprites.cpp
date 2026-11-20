#include "ForestSprites.h"
#include "../World/Map.h"
#include "../World/Object/Resource/ResourceNode.h"
#include "../World/Object/Station/StationType.h"
#include <algorithm>
#include <cmath>
#include <filesystem>
#include <stdexcept>
#include <iostream>


void ForestSprites::Clear()
{
    for (auto &image : images)
    {
        if (image.texture) SDL_DestroyTexture(image.texture);
        image = Image{};
    }
    renderer = nullptr;
    encodedBytes = peakStagingBytes = 0;
    grass = {}; paths = {}; water = {}; stone = {}; banks = {};
    player = {}; chopping = {}; choppingAxes = {}; miningPlayer = {}; miningPickaxes = {};
    meleeBodies = {}; meleeEquipment = {};
    variedGrass = {}; variedPaths = {}; variedWater = {}; variedStone = {}; flora = {};
    variedWalls = {}; variedRoof = {}; variedFloors = {}; variedTrees = {}; variedStumps = {}; variedRocks = {};
    buildingWalls = {}; buildingRoof = {}; buildingFloors = {};
    copper = tin = rubble = tree = stump = mara = furnace = bank = craftingTable = nullptr;
    roofAlphas.clear();
    variation.Clear();
}

void ForestSprites::PrepareWorld(Map &map, const std::vector<ResourceNode> &resources,
    const std::vector<CraftingStation> &stations, const std::vector<std::unique_ptr<Entity>> &entities)
{
    variation.Prepare(map, resources, stations, entities);
}

int ForestSprites::Condition(const Building &b) const
{
    return buildingConditionPreview >= 0 && buildingConditionPreview <= 2 ? buildingConditionPreview :
        WorldArtVariation::BuildingCondition(b.id.c_str());
}

void ForestSprites::Initialize(SDL_Renderer *target)
{
    std::vector<std::filesystem::path> roots;
    if (const char *base = SDL_GetBasePath())
        roots.emplace_back(reinterpret_cast<const char8_t *>(base));
    std::error_code error;
    const auto cwd = std::filesystem::current_path(error);
    if (!error && (roots.empty() || roots.front() != cwd)) roots.push_back(cwd);
    InitializeCatalogue(target, ForestPilotLayout::Images, ForestPilotLayout::Frames, roots,
        WorldArtResources::DefaultLimits());
}

void ForestSprites::InitializeCatalogue(SDL_Renderer *target,
    std::span<const ForestPilotLayout::Image> imageSpecs, std::span<const Frame> frames,
    const std::vector<std::filesystem::path> &roots, const WorldArtResources::Limits &limits)
{
    Clear();
    renderer = target;
    std::vector<WorldArtResources::Image> specs;
    try
    {
        if (imageSpecs.empty() || imageSpecs.size() > images.size() || imageSpecs.size() > limits.images ||
            frames.empty() || frames.size() > limits.frames)
            throw std::runtime_error("catalogue exceeds the bounded cache");
        for (const auto &image : imageSpecs)
            specs.push_back({image.filename, image.width, image.height, image.directory});
        std::vector<WorldArtResources::Frame> metadata;
        metadata.reserve(frames.size());
        for (const auto &f : frames)
            metadata.push_back({f.id, f.image, {f.source.x, f.source.y, f.source.w, f.source.h},
                {f.pivot.x, f.pivot.y}, {f.ground.x, f.ground.y}, f.connectionMask, f.grassVariant, f.facing, f.animationFrame});
        (void)WorldArtResources::ValidateCatalogue(specs, metadata, limits);
    }
    catch (const std::exception &error)
    {
        std::cerr << "World art catalogue unavailable: " << error.what() << "; using basic shapes.\n";
        return;
    }
    for (const auto &frame : frames)
    {
        const auto bind = [&frame](auto &frames, std::string_view prefix)
        {
            if (frame.id.starts_with(prefix) && frame.grassVariant >= 0 &&
                static_cast<std::size_t>(frame.grassVariant) < frames.size()) frames[frame.grassVariant] = &frame;
        };
        bind(variedGrass, "variation.grass."); bind(variedPaths, "variation.paths.");
        bind(variedWater, "variation.water."); bind(variedStone, "variation.stone.");
        bind(flora, "variation.flora."); bind(variedWalls, "variation.walls.");
        bind(variedRoof, "variation.roof."); bind(variedFloors, "variation.floors.");
        bind(variedTrees, "variation.trees."); bind(variedStumps, "variation.stumps.");
        bind(variedRocks, "variation.rocks.");
        if (frame.id.starts_with("terrain.grass.") && frame.grassVariant >= 0 && frame.grassVariant < static_cast<int>(grass.size()))
            grass[frame.grassVariant] = &frame;
        if (frame.id.starts_with("terrain.path.") && frame.connectionMask >= 0 && frame.connectionMask < static_cast<int>(paths.size()))
            paths[frame.connectionMask] = &frame;
        if (frame.id == "resource.tree.normal.intact") tree = &frame;
        if (frame.id == "resource.tree.normal.depleted") stump = &frame;
        if (frame.id == "character.mara.idle") mara = &frame;
        if (frame.id == "station.furnace.idle") furnace = &frame;
        if (frame.id == "station.bank.idle") bank = &frame;
        if (frame.id == "station.crafting_table.idle") craftingTable = &frame;
        constexpr std::array<std::string_view, 16> walls{"front", "back", "west", "east", "window-front", "window-west",
            "window-east", "doorway", "corner-nw", "corner-ne", "corner-sw", "corner-se",
            "cutaway-front", "cutaway-doorway", "cutaway-sw", "cutaway-se"};
        constexpr std::array<std::string_view, 12> roof{"nw", "n", "ne", "w", "center", "e", "sw", "s", "se", "ridge-w", "ridge", "ridge-e"};
        for (std::size_t i = 0; i < walls.size(); ++i)
        {
            if (frame.id.starts_with("building.wall.") && frame.id.substr(14) == walls[i]) buildingWalls[i] = &frame;
            if (i < roof.size() && frame.id.starts_with("building.roof.") && frame.id.substr(14) == roof[i]) buildingRoof[i] = &frame;
        }
        constexpr std::array<std::string_view, 3> floors{"building.floor.01", "building.floor.02", "building.floor.03"};
        for (std::size_t i = 0; i < floors.size(); ++i) if (frame.id == floors[i]) buildingFloors[i] = &frame;
        constexpr std::array<std::string_view, 3> waterIDs{"terrain.water.01", "terrain.water.02", "terrain.water.03"};
        constexpr std::array<std::string_view, 3> stoneIDs{"terrain.stone.01", "terrain.stone.02", "terrain.stone.03"};
        for (std::size_t i = 0; i < water.size(); ++i)
        {
            if (frame.id == waterIDs[i]) water[i] = &frame;
            if (frame.id == stoneIDs[i]) stone[i] = &frame;
        }
        constexpr std::array<std::string_view, 12> edges{"n", "e", "s", "w", "outer-nw", "outer-ne", "outer-sw", "outer-se",
            "inner-nw", "inner-ne", "inner-sw", "inner-se"};
        for (std::size_t i = 0; i < edges.size(); ++i)
        {
            if (frame.id.starts_with("terrain.bank.earth.") && frame.id.substr(19) == edges[i]) banks[0][i] = &frame;
            if (frame.id.starts_with("terrain.bank.stone.") && frame.id.substr(19) == edges[i]) banks[1][i] = &frame;
            if (frame.id.starts_with("terrain.bank.dirt.") && frame.id.substr(18) == edges[i]) banks[2][i] = &frame;
        }
        if (frame.id == "resource.rock.copper.intact") copper = &frame;
        if (frame.id == "resource.rock.tin.intact") tin = &frame;
        if (frame.id == "resource.rock.depleted") rubble = &frame;
        if (frame.id.starts_with("character.player.") && frame.facing >= 0 && frame.facing < static_cast<int>(player.size()) &&
            frame.animationFrame >= 0 && frame.animationFrame < PlayerAnimationTiming::WalkPoses)
            player[frame.facing][frame.animationFrame] = &frame;
        if (frame.facing >= 0 && frame.facing < 8 && frame.animationFrame >= 0 && frame.animationFrame < PlayerAnimationTiming::ChoppingPoses)
        {
            if (frame.id.starts_with("character.woodcutting.")) chopping[frame.facing][frame.animationFrame] = &frame;
            if (frame.id.starts_with("tool.axe.") && frame.grassVariant >= 0 && frame.grassVariant < 3)
                choppingAxes[frame.grassVariant][frame.facing][frame.animationFrame] = &frame;
        }
        if (frame.facing >= 0 && frame.facing < 8 && frame.animationFrame >= 0 &&
            frame.animationFrame < PlayerAnimationTiming::MiningPoses)
        {
            if (frame.id.starts_with("character.mining.")) miningPlayer[frame.facing][frame.animationFrame] = &frame;
            if (frame.id.starts_with("tool.pickaxe.") && frame.grassVariant >= 0 && frame.grassVariant < 3)
                miningPickaxes[frame.grassVariant][frame.facing][frame.animationFrame] = &frame;
        }
        if (frame.facing >= 0 && frame.facing < 8 && frame.animationFrame >= 0 && frame.animationFrame < EquippedFrames)
        {
            if (frame.id.starts_with("character.melee.") && frame.grassVariant >= 0 && frame.grassVariant < 5)
                meleeBodies[frame.grassVariant][frame.facing][frame.animationFrame] = &frame;
            if (frame.id.starts_with("equipment.melee.") && frame.grassVariant >= 0 && frame.grassVariant < 9)
                meleeEquipment[frame.grassVariant][frame.facing][frame.animationFrame] = &frame;
        }
    }
#if SDL_VERSION_ATLEAST(3, 4, 0)
    if (!renderer) return;
    const auto backendEdge = SDL_GetNumberProperty(SDL_GetRendererProperties(renderer),
        SDL_PROP_RENDERER_MAX_TEXTURE_SIZE_NUMBER, 0);
    for (std::size_t index = 0; index < specs.size(); ++index)
    {
        const auto &expected = specs[index];
        SDL_Surface *surface = nullptr;
        std::string reason = "no matching supported PNG";
        if (backendEdge > 0 && (expected.width > backendEdge || expected.height > backendEdge))
            reason = "image exceeds renderer texture-size limit";
        else for (const auto &root : roots)
        {
            try
            {
                const auto bytes = WorldArtResources::ReadPng(root / expected.directory / expected.filename, expected, limits);
                if (encodedBytes > limits.cataloguePngBytes || bytes.size() > limits.cataloguePngBytes - encodedBytes)
                    throw std::runtime_error("catalogue exceeds encoded PNG budget");
                encodedBytes += bytes.size();
                const auto cost = WorldArtResources::MeasureImage(expected.width, expected.height, limits);
                peakStagingBytes = std::max(peakStagingBytes,
                    std::max(cost.textureBytes, WorldArtResources::PreflightScratchBytes) + bytes.size());
                if (auto *stream = SDL_IOFromConstMem(bytes.data(), bytes.size()))
                    surface = SDL_LoadPNG_IO(stream, true);
                if (surface) break;
                reason = "PNG decode failed";
            }
            catch (const std::exception &error) { reason = error.what(); }
        }
        if (!surface || surface->w != expected.width || surface->h != expected.height ||
            surface->pitch <= 0 || std::uint64_t(surface->pitch) * surface->h >
                std::uint64_t(expected.width) * expected.height * 4)
        {
            std::cerr << "World art unavailable: " << expected.directory << '/' << expected.filename << ": "
                << reason << "; using basic shapes.\n";
            if (surface) SDL_DestroySurface(surface);
            continue;
        }
        auto &image = images[index];
        image.alpha.resize(static_cast<std::size_t>(surface->w) * surface->h);
        bool pixelsValid = true;
        for (int y = 0; y < surface->h && pixelsValid; ++y)
            for (int x = 0; x < surface->w; ++x)
            {
                Uint8 red, green, blue, alpha;
                if (!SDL_ReadSurfacePixel(surface, x, y, &red, &green, &blue, &alpha))
                {
                    pixelsValid = false;
                    break;
                }
                image.alpha[static_cast<std::size_t>(y) * surface->w + x] = alpha;
            }
        if (pixelsValid) image.texture = SDL_CreateTextureFromSurface(renderer, surface);
        SDL_DestroySurface(surface);
        if (!image.texture || !SDL_SetTextureScaleMode(image.texture, SDL_SCALEMODE_NEAREST) ||
            !SDL_SetTextureBlendMode(image.texture, SDL_BLENDMODE_BLEND))
        {
            if (image.texture) SDL_DestroyTexture(image.texture);
            image = Image{};
            std::cerr << "World texture unavailable: " << expected.filename << "; using basic shapes.\n";
        }
        else { image.width = expected.width; image.height = expected.height; }
    }
    const auto usage = GetResourceUsage();
    std::clog << "World art: " << LoadedImageCount() << '/' << specs.size() << " textures cached; "
        << usage.textureBytes << " RGBA estimate bytes, " << usage.alphaBytes << " alpha bytes, "
        << usage.peakStagingBytes << " peak image staging estimate bytes.\n";
#else
    std::clog << "World PNG art needs SDL 3.4 or newer; using basic shapes.\n";
#endif
}

std::size_t ForestSprites::LoadedImageCount() const
{
    return std::count_if(images.begin(), images.end(), [](const Image &image) { return image.texture != nullptr; });
}

ForestSprites::ResourceUsage ForestSprites::GetResourceUsage() const
{
    ResourceUsage usage{0, 0, encodedBytes, peakStagingBytes};
    for (const auto &image : images)
    {
        if (image.texture) usage.textureBytes += std::uint64_t(image.width) * image.height * 4;
        usage.alphaBytes += image.alpha.size();
    }
    return usage;
}

bool ForestSprites::Available(const Frame *frame) const
{
    return frame && frame->image < images.size() && images[frame->image].texture;
}

bool ForestSprites::HasTree() const { return Available(tree) || Available(variedTrees[0]); }
bool ForestSprites::HasPlayer() const { return Available(player[0][0]); }
bool ForestSprites::HasMara() const { return Available(mara); }

SDL_FRect ForestSprites::Bounds(const Frame &frame, float tileX, float tileY) const
{
    return {tileX * ForestPilotLayout::TileSize + frame.ground.x - frame.pivot.x,
        tileY * ForestPilotLayout::TileSize + frame.ground.y - frame.pivot.y,
        frame.source.w, frame.source.h};
}

bool ForestSprites::Draw(const Frame *frame, const SDL_FRect &destination,
                         const SDL_FRect *source, Uint8 alpha) const
{
    if (!Available(frame)) return false;
    auto *texture = images[frame->image].texture;
    SDL_SetTextureAlphaMod(texture, alpha);
    const bool drawn = SDL_RenderTexture(renderer, texture, source ? source : &frame->source, &destination);
    SDL_SetTextureAlphaMod(texture, 255);
    return drawn;
}

bool ForestSprites::DrawGround(Map &map, int x, int y) const
{
    // Legacy walls keep the primitive pass; authored shells use the object queue.
    const auto *building = map.GetBuildingAt(x, y);
    if (map.GetTileType(x, y) == TileType::WALL && !building) return false;
    const auto type = map.GetGroundType(x, y);
    const SDL_FRect tile{static_cast<float>(x * ForestPilotLayout::TileSize),
        static_cast<float>(y * ForestPilotLayout::TileSize), 32.0f, 32.0f};
    const auto variant = (static_cast<unsigned int>(x) * 73856093U ^
                          static_cast<unsigned int>(y) * 19349663U) % grass.size();
    const auto &cell = variation.At(x, y);
    const auto *grassFrame = Available(variedGrass[cell.grass]) ? variedGrass[cell.grass] : grass[variant];
    if (type == TileType::GRASS)
    {
        if (!Draw(grassFrame, tile)) return false;
        DressGround(cell, tile);
        return true;
    }
    if (type == TileType::STONE)
    {
        const Frame *floor = nullptr;
        if (building)
        {
            floor = variedFloors[Condition(*building) * 3 + variant];
            if (!Available(floor)) floor = buildingFloors[variant];
            if (!building->IsWall(x, y) && !building->IsDoor(x, y) && Available(floor)) return Draw(floor, tile);
        }
        if (!Draw(Available(variedStone[cell.stone]) ? variedStone[cell.stone] : stone[variant], tile)) return false;
        if (building && Available(floor))
        {
            // Extend planks up to the inner edges of the eight-pixel wall plan.
            // This is presentation only: the authored wall cells remain blocked.
            const SDL_FRect room{building->x * 32.0f + 20, building->y * 32.0f + 20,
                (building->width - 1) * 32.0f - 8, (building->height - 1) * 32.0f - 8};
            SDL_FRect coverage{};
            if (building->IsDoor(x, y)) coverage = {tile.x, tile.y, 32, 20};
            else SDL_GetRectIntersectionFloat(&tile, &room, &coverage);
            if (coverage.w > 0 && coverage.h > 0)
            {
                const SDL_FRect source{floor->source.x + coverage.x - tile.x,
                    floor->source.y + coverage.y - tile.y, coverage.w, coverage.h};
                if (!Draw(floor, coverage, &source)) return false;
            }
        }
        DressGround(cell, tile);
        return true;
    }
    if (type == TileType::WATER) return DrawWater(map, x, y, tile, variant);
    if (type != TileType::DIRT) return false;
    const auto dirt = [&map](int tx, int ty)
    {
        if (!map.IsInBounds(tx, ty)) return false;
        const auto ground = map.GetGroundType(tx, ty);
        // Roads meet water cuts and stone floors at the boundary without a grass seam.
        return ground == TileType::DIRT || ground == TileType::WATER || ground == TileType::STONE;
    };
    const bool north = dirt(x, y - 1), east = dirt(x + 1, y);
    const bool south = dirt(x, y + 1), west = dirt(x - 1, y);
    const int mask = (north ? 1 : 0) | (east ? 2 : 0) | (south ? 4 : 0) | (west ? 8 : 0);
    const bool transparentPaths = Available(variedPaths[cell.path * 17 + mask]) &&
        Available(variedPaths[cell.path * 17 + 16]) && Available(grassFrame);
    if (!transparentPaths && (!Available(paths[mask]) || !Available(paths[15]))) return false;
    if (transparentPaths && !Draw(grassFrame, tile)) return false;
    const std::array<bool, 4> interior{{
        north && west && dirt(x - 1, y - 1), north && east && dirt(x + 1, y - 1),
        south && west && dirt(x - 1, y + 1), south && east && dirt(x + 1, y + 1)}};
    // Fill interior quarters on wide roads; only exposed corners retain grass.
    for (int quarter = 0; quarter < 4; ++quarter)
    {
        const float qx = static_cast<float>((quarter % 2) * 16);
        const float qy = static_cast<float>((quarter / 2) * 16);
        const auto *frame = transparentPaths ? variedPaths[cell.path * 17 + (interior[quarter] ? 16 : mask)] :
            interior[quarter] ? paths[15] : paths[mask];
        const SDL_FRect source{frame->source.x + (interior[quarter] && !transparentPaths ? 8.0f : qx),
            frame->source.y + (interior[quarter] && !transparentPaths ? 8.0f : qy), 16.0f, 16.0f};
        const SDL_FRect destination{tile.x + qx, tile.y + qy, 16.0f, 16.0f};
        if (!Draw(frame, destination, &source)) return false;
    }
    DressGround(cell, tile);
    return true;
}

void ForestSprites::DressGround(const WorldArtVariation::Cell &cell, const SDL_FRect &tile) const
{
    for (const int index : {cell.leaves, cell.detail, cell.plant})
        if (index >= 0 && index < static_cast<int>(flora.size())) Draw(flora[index], tile);
}

bool ForestSprites::DrawWater(Map &map, int x, int y, const SDL_FRect &tile, std::size_t variant) const
{
    const auto *waterFrame = Available(variedWater[variation.At(x, y).water]) ? variedWater[variation.At(x, y).water] : water[variant];
    if (!Available(waterFrame) || !Available(banks[0][0]) || !Available(banks[1][0]) || !Available(banks[2][0])) return false;
    const auto land = [&map](int tx, int ty)
    {
        return map.IsInBounds(tx, ty) && map.GetGroundType(tx, ty) != TileType::WATER;
    };
    const auto family = [&map](int tx, int ty)
    {
        if (!map.IsInBounds(tx, ty)) return 0;
        const auto ground = map.GetGroundType(tx, ty);
        return ground == TileType::STONE ? 1 : ground == TileType::DIRT ? 2 : 0;
    };
    if (!Draw(waterFrame, tile)) return false;
    for (int quarter = 0; quarter < 4; ++quarter)
    {
        const int dx = quarter % 2 == 0 ? -1 : 1, dy = quarter < 2 ? -1 : 1;
        const bool vertical = land(x, y + dy), horizontal = land(x + dx, y);
        int edge;
        int bankFamily;
        if (vertical || horizontal)
        {
            edge = vertical && horizontal ? 4 + quarter : vertical ? (dy < 0 ? 0 : 2) : (dx < 0 ? 3 : 1);
            bankFamily = vertical ? family(x, y + dy) : family(x + dx, y);
        }
        else if (land(x + dx, y + dy))
        {
            edge = 8 + quarter;
            bankFamily = family(x + dx, y + dy);
        }
        else continue;
        const auto *frame = banks[bankFamily][edge];
        if (!Available(frame)) return false;
        const float qx = static_cast<float>((quarter % 2) * 16), qy = static_cast<float>((quarter / 2) * 16);
        const SDL_FRect source{frame->source.x + qx, frame->source.y + qy, 16.0f, 16.0f};
        const SDL_FRect destination{tile.x + qx, tile.y + qy, 16.0f, 16.0f};
        if (!Draw(frame, destination, &source)) return false;
    }
    return true;
}

const ForestSprites::Frame *ForestSprites::MiningFrame(ResourceType type, int x, int y) const
{
    const auto variant = WorldArtVariation::Hash(x, y, 24) % 3;
    if (type == ResourceType::COPPER_ROCK && Available(variedRocks[variant])) return variedRocks[variant];
    if (type == ResourceType::TIN_ROCK && Available(variedRocks[3 + variant])) return variedRocks[3 + variant];
    if (type == ResourceType::COPPER_ROCK) return copper;
    if (type == ResourceType::TIN_ROCK) return tin;
    return nullptr;
}

bool ForestSprites::DrawMining(const ResourceNode &resource) const
{
    const auto *intact = MiningFrame(resource.GetResourceType(), resource.GetX(), resource.GetY());
    if (!intact) return false;
    const auto *depleted = variedRocks[6 + WorldArtVariation::Hash(resource.GetX(), resource.GetY(), 24) % 3];
    const auto *frame = resource.IsActive() ? intact : Available(depleted) ? depleted : rubble;
    return Available(frame) && Draw(frame, Bounds(*frame, resource.GetX(), resource.GetY()));
}

bool ForestSprites::HitMining(ResourceType type, int tileX, int tileY, float worldX, float worldY) const
{
    return Hit(MiningFrame(type, tileX, tileY), tileX, tileY, worldX, worldY);
}

bool ForestSprites::DrawBuildingWall(const Building &b, int x, int y, const SDL_FRect &player) const
{
    const int column = x - b.x, row = y - b.y;
    int index;
    if (row == 0) index = column == 0 ? 8 : column == b.width - 1 ? 9 : 1;
    else if (row == b.height - 1)
        index = column == 0 ? 10 : column == b.width - 1 ? 11 : b.IsDoor(x, y) ? 7 :
            column == 2 || column == b.width - 3 ? 4 : 0;
    else index = column == 0 ? (row == b.height / 2 ? 5 : 2) : (row == b.height / 2 ? 6 : 3);
    const int condition = Condition(b);
    const int px = static_cast<int>(std::floor((player.x + player.w / 2) / 32));
    const int py = static_cast<int>(std::floor((player.y + player.h) / 32));
    if (b.Contains(px, py) && row == b.height - 1)
        index = column == 0 ? 14 : column == b.width - 1 ? 15 : b.IsDoor(x, y) ? 13 : 12;
    const bool repair = row == b.height - 1 && (column == b.doorOffset + 1 || column == 1);
    const int panelCondition = condition == 2 ? (repair ? 2 : 1) : condition;
    const auto *varied = variedWalls[panelCondition * 32 + WorldArtVariation::Hash(x, y, 6) % 2 * 16 + index];
    const auto *frame = Available(varied) ? varied : buildingWalls[index];
    if (!Available(frame)) return false;
    const auto bounds = Bounds(*frame, static_cast<float>(x), static_cast<float>(y));
    const bool overlap = SDL_HasRectIntersectionFloat(&bounds, &player);
    const bool behind = player.y + player.h <= y * 32.0f + 24.0f;
    return Draw(frame, bounds, nullptr, overlap && behind ? 110 : 255);
}

bool ForestSprites::DrawBuildingRoof(const Building &b, const SDL_FRect &player, float deltaSeconds)
{
    const int condition = Condition(b), material = WorldArtVariation::BuildingMaterial(b.id.c_str());
    const auto roofFrame = [&](int part, float localX, float localY)
    {
        const bool repair = part < 9 && localX >= 64 && localX < 128 && localY >= 32 && localY < 64;
        const int surfaceCondition = condition == 2 ? (repair ? 2 : 1) : condition;
        const auto *frame = variedRoof[material * 36 + surfaceCondition * 12 + part];
        return Available(frame) ? frame : buildingRoof[part];
    };
    for (int i = 0; i < 12; ++i) if (!Available(roofFrame(i, 0, 0))) return false;
    // The south eave meets the new wall cap instead of exposing a strip of floor.
    const SDL_FRect bounds{b.x * 32.0f - 8.0f, b.y * 32.0f - 48.0f, b.width * 32.0f + 16.0f, b.height * 32.0f + 24.0f};
    const int px = static_cast<int>(std::floor((player.x + player.w / 2) / 32));
    const int py = static_cast<int>(std::floor((player.y + player.h) / 32));
    const float target = b.Contains(px, py) || SDL_HasRectIntersectionFloat(&bounds, &player) ? 25.0f : 255.0f;
    auto [entry, inserted] = roofAlphas.try_emplace(b.id, target);
    entry->second += (target - entry->second) * std::clamp(deltaSeconds * 12.0f, 0.0f, 1.0f);
    const auto alpha = static_cast<Uint8>(std::round(entry->second));
    const std::array<float, 3> xs{0, 32, bounds.w - 32}, ys{0, 32, bounds.h - 32};
    const std::array<float, 3> widths{32, bounds.w - 64, 32}, heights{32, bounds.h - 64, 32};
    // Repeat native tiles; clip the final repeat instead of stretching its pixels.
    for (int row = 0; row < 3; ++row) for (int column = 0; column < 3; ++column)
        for (float y = 0; y < heights[row]; y += 32) for (float x = 0; x < widths[column]; x += 32)
        {
            const auto *frame = roofFrame(row * 3 + column, xs[column] + x, ys[row] + y);
            const float w = std::min(32.0f, widths[column] - x), h = std::min(32.0f, heights[row] - y);
            const SDL_FRect source{frame->source.x, frame->source.y, w, h};
            const SDL_FRect dest{bounds.x + xs[column] + x, bounds.y + ys[row] + y, w, h};
            if (!Draw(frame, dest, &source, alpha)) return false;
        }
    for (int section = 0; section < 3; ++section)
        for (float x = 0; x < widths[section]; x += 32)
        {
            const auto *frame = roofFrame(9 + section, xs[section] + x, 16);
            const float w = std::min(32.0f, widths[section] - x);
            const SDL_FRect source{frame->source.x, frame->source.y, w, 32};
            const SDL_FRect dest{bounds.x + xs[section] + x, bounds.y + 16, w, 32};
            if (!Draw(frame, dest, &source, alpha)) return false;
        }
    return true;
}

bool ForestSprites::DrawTree(const ResourceNode &resource, const SDL_FRect &player) const
{
    if (resource.GetResourceType() != ResourceType::NORMAL_TREE) return false;
    const auto *frame = TreeFrame(resource.GetX(), resource.GetY(), resource.IsActive());
    if (!Available(frame)) return false;
    const auto bounds = Bounds(*frame, resource.GetX(), resource.GetY());
    const float footY = bounds.y + frame->pivot.y;
    const bool behind = player.y + player.h < footY;
    const bool overlap = player.x < bounds.x + bounds.w && player.x + player.w > bounds.x &&
        player.y < bounds.y + bounds.h && player.y + player.h > bounds.y;
    return Draw(frame, bounds, nullptr, resource.IsActive() && behind && overlap ? 155 : 255);
}

bool ForestSprites::HitTree(int tileX, int tileY, float worldX, float worldY) const
{
    return Hit(TreeFrame(tileX, tileY), tileX, tileY, worldX, worldY);
}

const ForestSprites::Frame *ForestSprites::TreeFrame(int x, int y, bool active) const
{
    const auto index = WorldArtVariation::Hash(x, y, 20) % 3;
    const auto *frame = active ? variedTrees[index] : variedStumps[index];
    return Available(frame) ? frame : active ? tree : stump;
}

bool ForestSprites::DrawPlayer(float tileX, float tileY, int facing, int pose) const
{
    const auto *frame = player[std::clamp(facing, 0, static_cast<int>(player.size()) - 1)][std::clamp(pose, 0, PlayerAnimationTiming::WalkPoses - 1)];
    return DrawCharacter(frame, tileX, tileY);
}

bool ForestSprites::DrawMara(int tileX, int tileY) const
{
    return DrawCharacter(mara, static_cast<float>(tileX), static_cast<float>(tileY));
}

bool ForestSprites::DrawWoodcutting(float tileX, float tileY, int facing, int pose, int axeTier) const
{
    if (facing < 0 || facing >= 8 || pose < 0 || pose >= PlayerAnimationTiming::ChoppingPoses || axeTier < 0 || axeTier >= 3) return false;
    const auto *body = chopping[facing][pose], *axe = choppingAxes[axeTier][facing][pose];
    if (!Available(body) || !Available(axe)) return false;
    const bool behind = facing == 3 || facing == 6 || facing == 7;
    if (behind) Draw(axe, Bounds(*axe, tileX, tileY));
    DrawCharacter(body, tileX, tileY);
    if (!behind) Draw(axe, Bounds(*axe, tileX, tileY));
    return true;
}

bool ForestSprites::DrawMiningAction(float tileX, float tileY, int facing, int pose, int pickaxeTier) const
{
    if (facing < 0 || facing >= 8 || pose < 0 || pose >= PlayerAnimationTiming::MiningPoses || pickaxeTier < 0 || pickaxeTier >= 3) return false;
    const auto *body = miningPlayer[facing][pose], *pickaxe = miningPickaxes[pickaxeTier][facing][pose];
    if (!Available(body) || !Available(pickaxe)) return false;
    const bool behind = facing == 3 || facing == 6 || facing == 7;
    if (behind) Draw(pickaxe, Bounds(*pickaxe, tileX, tileY));
    DrawCharacter(body, tileX, tileY);
    if (!behind) Draw(pickaxe, Bounds(*pickaxe, tileX, tileY));
    return true;
}

bool ForestSprites::DrawEquippedPlayer(float tileX, float tileY, int facing, int pose,
    ItemType weapon, ItemType shield, bool attacking) const
{
    if (facing < 0 || facing >= 8 || pose < 0 || pose >= (attacking ? PlayerAnimationTiming::MeleePoses : PlayerAnimationTiming::WalkPoses)) return false;
    const int slot = [&] {
        switch (weapon)
        {
        case ItemType::BRONZE_SWORD: return 0;
        case ItemType::DEVELOPER_GODSWORD: return 1;
        case ItemType::BRONZE_AXE: return 2;
        case ItemType::IRON_AXE: return 3;
        case ItemType::STEEL_AXE: return 4;
        case ItemType::BRONZE_PICKAXE: return 5;
        case ItemType::IRON_PICKAXE: return 6;
        case ItemType::STEEL_PICKAXE: return 7;
        default: return -1;
        }
    }();
    const bool hasShield = shield == ItemType::WOODEN_SHIELD;
    if (!attacking && slot < 0 && !hasShield) return false;
    const int style = slot == 1 ? 2 : slot < 0 ? 0 : slot >= 2 && slot <= 4 ? 3 : slot >= 5 ? 4 : 1;
    const int frame = attacking ? pose + PlayerAnimationTiming::WalkPoses : pose;
    const auto *body = meleeBodies[style][facing][frame];
    const auto *held = slot >= 0 ? meleeEquipment[slot][facing][frame] : nullptr;
    // A two-handed weapon carries the equipped shield across the back.
    const auto *offhand = hasShield ? meleeEquipment[8][style == 2 ? 3 : facing][style == 2 ? 0 : frame] : nullptr;
    if (!Available(body) || (slot >= 0 && !Available(held)) || (hasShield && !Available(offhand))) return false;
    const bool weaponBehind = facing == 1 || facing == 3 || facing == 6;
    const bool shieldBehind = style == 2 || facing == 2 || facing == 3 || facing == 7;
    if (offhand && shieldBehind) Draw(offhand, Bounds(*offhand, tileX, tileY));
    if (held && weaponBehind) Draw(held, Bounds(*held, tileX, tileY));
    DrawCharacter(body, tileX, tileY);
    if (held && !weaponBehind) Draw(held, Bounds(*held, tileX, tileY));
    if (offhand && !shieldBehind) Draw(offhand, Bounds(*offhand, tileX, tileY));
    return true;
}

bool ForestSprites::DrawCharacter(const Frame *frame, float tileX, float tileY) const
{
    if (!Available(frame)) return false;
    SDL_BlendMode previousBlend;
    Uint8 red, green, blue, alpha;
    SDL_GetRenderDrawBlendMode(renderer, &previousBlend);
    SDL_GetRenderDrawColor(renderer, &red, &green, &blue, &alpha);
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, 17, 22, 18, 78);
    const float footX = tileX * ForestPilotLayout::TileSize + frame->ground.x;
    const float footY = tileY * ForestPilotLayout::TileSize + frame->ground.y;
    for (int row = -2; row <= 2; ++row)
    {
        const float halfWidth = 10.0f - std::abs(row) * 2.0f;
        SDL_RenderLine(renderer, footX - halfWidth, footY + row, footX + halfWidth, footY + row);
    }
    SDL_SetRenderDrawBlendMode(renderer, previousBlend);
    SDL_SetRenderDrawColor(renderer, red, green, blue, alpha);
    return Draw(frame, Bounds(*frame, tileX, tileY));
}

const ForestSprites::Frame *ForestSprites::StationFrame(StationType type) const
{
    switch (type)
    {
    case StationType::FURNACE: return furnace;
    case StationType::BANK: return bank;
    case StationType::CRAFTING_TABLE: return craftingTable;
    default: return nullptr;
    }
}

bool ForestSprites::DrawStation(StationType type, int tileX, int tileY) const
{
    const auto *frame = StationFrame(type);
    return Available(frame) && Draw(frame, Bounds(*frame, tileX, tileY));
}

bool ForestSprites::HitMara(int tileX, int tileY, float worldX, float worldY) const
{
    return Hit(mara, tileX, tileY, worldX, worldY);
}

bool ForestSprites::HitStation(StationType type, int tileX, int tileY, float worldX, float worldY) const
{
    return Hit(StationFrame(type), tileX, tileY, worldX, worldY);
}

bool ForestSprites::Hit(const Frame *frame, int tileX, int tileY, float worldX, float worldY) const
{
    if (!Available(frame)) return false;
    const auto bounds = Bounds(*frame, tileX, tileY);
    const int x = static_cast<int>(std::floor(worldX - bounds.x));
    const int y = static_cast<int>(std::floor(worldY - bounds.y));
    if (x < 0 || y < 0 || x >= frame->source.w || y >= frame->source.h) return false;
    const auto &image = images[frame->image];
    const int sourceX = static_cast<int>(frame->source.x) + x;
    const int sourceY = static_cast<int>(frame->source.y) + y;
    const auto pixel = static_cast<std::size_t>(sourceY) * image.width + sourceX;
    return pixel < image.alpha.size() && image.alpha[pixel] >= 128;
}
