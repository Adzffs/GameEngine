#pragma once

#include "ForestPilotLayout.h"
#include "WorldArtVariation.h"
#include <array>
#include <map>
#include <string>
#include <vector>

class Map;
class ResourceNode;
class CraftingStation;
class Entity;
struct Building;
enum class ResourceType;
enum class StationType;

// One renderer-owned cache shared by the world-art batches. Missing art falls back
// per image; initialization is the only place files are opened or decoded.
class ForestSprites
{
public:
    ForestSprites() = default;
    ~ForestSprites() { Clear(); }
    ForestSprites(const ForestSprites &) = delete;
    ForestSprites &operator=(const ForestSprites &) = delete;
    void Initialize(SDL_Renderer *renderer);
    void Clear();
    void PrepareWorld(Map &map, const std::vector<ResourceNode> &resources,
                      const std::vector<CraftingStation> &stations,
                      const std::vector<std::unique_ptr<Entity>> &entities);
    void SetBuildingConditionPreview(int condition) { buildingConditionPreview = condition; }
    bool DrawGround(Map &map, int tileX, int tileY) const;
    bool DrawTree(const ResourceNode &resource, const SDL_FRect &player) const;
    bool DrawMining(const ResourceNode &resource) const;
    bool HitMining(ResourceType type, int tileX, int tileY, float worldX, float worldY) const;
    bool HitTree(int tileX, int tileY, float worldX, float worldY) const;
    bool HasTree() const;
    bool HasPlayer() const;
    bool HasMara() const;
    bool DrawPlayer(float tileX, float tileY, int facing, int pose) const;
    bool DrawWoodcutting(float tileX, float tileY, int facing, int pose, int axeTier) const;
    bool DrawMara(int tileX, int tileY) const;
    bool DrawStation(StationType type, int tileX, int tileY) const;
    bool HitMara(int tileX, int tileY, float worldX, float worldY) const;
    bool HitStation(StationType type, int tileX, int tileY, float worldX, float worldY) const;
    bool DrawBuildingWall(const Building &building, int x, int y, const SDL_FRect &player) const;
    bool DrawBuildingRoof(const Building &building, const SDL_FRect &player, float deltaSeconds);
    std::size_t LoadedImageCount() const;

private:
    using Frame = ForestPilotLayout::Frame;
    struct Image
    {
        SDL_Texture *texture = nullptr;
        std::vector<Uint8> alpha;
    };
    SDL_Renderer *renderer = nullptr;
    std::array<Image, ForestPilotLayout::Images.size()> images{};
    std::array<const Frame *, 3> grass{};
    std::array<const Frame *, 16> paths{};
    std::array<const Frame *, 3> water{}, stone{};
    // N/E/S/W, outer NW/NE/SW/SE, inner NW/NE/SW/SE; grass, stone and bare dirt banks.
    std::array<std::array<const Frame *, 12>, 3> banks{};
    const Frame *copper = nullptr, *tin = nullptr, *rubble = nullptr;
    const Frame *tree = nullptr;
    const Frame *stump = nullptr;
    // South, west, east, north, southwest, southeast, northwest, northeast.
    std::array<std::array<const Frame *, 4>, 8> player{};
    std::array<std::array<const Frame *, 6>, 8> chopping{};
    std::array<std::array<std::array<const Frame *, 6>, 8>, 3> choppingAxes{};
    const Frame *mara = nullptr;
    const Frame *furnace = nullptr;
    const Frame *bank = nullptr, *craftingTable = nullptr;
    std::array<const Frame *, 16> buildingWalls{};
    std::array<const Frame *, 12> buildingRoof{};
    std::array<const Frame *, 3> buildingFloors{};
    std::map<std::string, float> roofAlphas;
    WorldArtVariation variation;
    std::array<const Frame *, 162> variedGrass{};
    std::array<const Frame *, 204> variedPaths{};
    std::array<const Frame *, 48> variedWater{};
    std::array<const Frame *, 12> variedStone{};
    std::array<const Frame *, 40> flora{};
    std::array<const Frame *, 96> variedWalls{};
    std::array<const Frame *, 72> variedRoof{};
    std::array<const Frame *, 9> variedFloors{}, variedRocks{};
    std::array<const Frame *, 3> variedTrees{}, variedStumps{};
    int buildingConditionPreview = -1;
    int Condition(const Building &building) const;
    void DressGround(const WorldArtVariation::Cell &cell, const SDL_FRect &tile) const;
    const Frame *TreeFrame(int tileX, int tileY, bool active = true) const;
    bool Available(const Frame *frame) const;
    bool Draw(const Frame *frame, const SDL_FRect &destination,
              const SDL_FRect *source = nullptr, Uint8 alpha = 255) const;
    SDL_FRect Bounds(const Frame &frame, float tileX, float tileY) const;
    bool Hit(const Frame *frame, int tileX, int tileY, float worldX, float worldY) const;
    bool DrawCharacter(const Frame *frame, float tileX, float tileY) const;
    const Frame *MiningFrame(ResourceType type, int x, int y) const;
    const Frame *StationFrame(StationType type) const;
    bool DrawWater(Map &map, int tileX, int tileY, const SDL_FRect &tile, std::size_t variant) const;
};
