#pragma once

#include "ForestPilotLayout.h"
#include "WorldArtVariation.h"
#include "../Content/Assets/WorldArtResources.h"
#include "../Inventory/ItemType.h"
#include "../Animation/PlayerAnimationTiming.h"
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
    void DrawCharacterShadow(float footX, float footY) const;
    bool DrawWoodcutting(float tileX, float tileY, int facing, int pose, int axeTier) const;
    bool DrawMiningAction(float tileX, float tileY, int facing, int pose, int pickaxeTier) const;
    bool DrawEquippedPlayer(float tileX, float tileY, int facing, int pose,
        ItemType weapon, ItemType shield, bool attacking = false) const;
    bool DrawShieldIcon(const SDL_FRect &slot) const;
    bool DrawMara(int tileX, int tileY) const;
    bool DrawStation(StationType type, int tileX, int tileY) const;
    bool HitMara(int tileX, int tileY, float worldX, float worldY) const;
    bool HitStation(StationType type, int tileX, int tileY, float worldX, float worldY) const;
    bool DrawBuildingWall(const Building &building, int x, int y, const SDL_FRect &player) const;
    bool DrawBuildingRoof(const Building &building, const SDL_FRect &player, float deltaSeconds);
    std::size_t LoadedImageCount() const;
    struct ResourceUsage
    {
        std::uint64_t textureBytes = 0, alphaBytes = 0, encodedBytes = 0, peakStagingBytes = 0;
    };
    ResourceUsage GetResourceUsage() const;

private:
    friend struct WorldArtResourceTestAccess;
    using Frame = ForestPilotLayout::Frame;
    struct Image
    {
        SDL_Texture *texture = nullptr;
        std::vector<Uint8> alpha;
        int width = 0, height = 0;
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
    std::array<std::array<const Frame *, PlayerAnimationTiming::WalkPoses>, 8> player{};
    std::array<std::array<const Frame *, PlayerAnimationTiming::ChoppingPoses>, 8> chopping{};
    std::array<std::array<std::array<const Frame *, PlayerAnimationTiming::ChoppingPoses>, 8>, 3> choppingAxes{};
    // Unarmed, sword, two-handed, axe, pickaxe; eight walk and eight attack poses.
    static constexpr int EquippedFrames = PlayerAnimationTiming::WalkPoses + PlayerAnimationTiming::MeleePoses;
    std::array<std::array<std::array<const Frame *, EquippedFrames>, 8>, 5> meleeBodies{};
    std::array<std::array<std::array<const Frame *, EquippedFrames>, 8>, 9> meleeEquipment{};
    std::array<std::array<const Frame *, PlayerAnimationTiming::MiningPoses>, 8> miningPlayer{};
    std::array<std::array<std::array<const Frame *, PlayerAnimationTiming::MiningPoses>, 8>, 3> miningPickaxes{};
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
    std::uint64_t encodedBytes = 0, peakStagingBytes = 0;
    void InitializeCatalogue(SDL_Renderer *target, std::span<const ForestPilotLayout::Image> imageSpecs,
        std::span<const Frame> frames, const std::vector<std::filesystem::path> &roots,
        const WorldArtResources::Limits &limits);
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
