#pragma once

#include "ForestPilotLayout.h"
#include <array>
#include <vector>

class Map;
class ResourceNode;

// One renderer-owned cache shared by the forest and cast pilots. Missing art falls back
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
    bool DrawGround(Map &map, int tileX, int tileY) const;
    bool DrawTree(const ResourceNode &resource, const SDL_FRect &player) const;
    bool HitTree(int tileX, int tileY, float worldX, float worldY) const;
    bool HasTree() const;
    bool HasPlayer() const;
    bool HasMara() const;
    bool DrawPlayer(float tileX, float tileY, int facing, int pose) const;
    bool DrawMara(int tileX, int tileY) const;
    bool DrawFurnace(int tileX, int tileY) const;
    bool HitMara(int tileX, int tileY, float worldX, float worldY) const;
    bool HitFurnace(int tileX, int tileY, float worldX, float worldY) const;
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
    const Frame *tree = nullptr;
    const Frame *stump = nullptr;
    std::array<std::array<const Frame *, 4>, 4> player{};
    const Frame *mara = nullptr;
    const Frame *furnace = nullptr;
    bool Available(const Frame *frame) const;
    bool Draw(const Frame *frame, const SDL_FRect &destination,
              const SDL_FRect *source = nullptr, Uint8 alpha = 255) const;
    SDL_FRect Bounds(const Frame &frame, float tileX, float tileY) const;
    bool Hit(const Frame *frame, int tileX, int tileY, float worldX, float worldY) const;
    bool DrawCharacter(const Frame *frame, float tileX, float tileY) const;
};
