#include "ForestSprites.h"
#include "../World/Map.h"
#include "../World/Object/Resource/ResourceNode.h"
#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>

namespace
{
    bool ValidPngHeader(const std::filesystem::path &path,
                        const ForestPilotLayout::Image &expected)
    {
        std::error_code error;
        const auto bytes = std::filesystem::file_size(path, error);
        if (error || bytes < 24 || bytes > 4 * 1024 * 1024) return false;
        std::array<unsigned char, 24> header{};
        std::ifstream file(path, std::ios::binary);
        if (!file.read(reinterpret_cast<char *>(header.data()), header.size())) return false;
        constexpr std::array<unsigned char, 8> signature{137, 80, 78, 71, 13, 10, 26, 10};
        if (!std::equal(signature.begin(), signature.end(), header.begin()) ||
            header[12] != 'I' || header[13] != 'H' || header[14] != 'D' || header[15] != 'R')
            return false;
        const auto number = [&header](std::size_t offset)
        {
            return (static_cast<Uint32>(header[offset]) << 24) |
                (static_cast<Uint32>(header[offset + 1]) << 16) |
                (static_cast<Uint32>(header[offset + 2]) << 8) | header[offset + 3];
        };
        return number(16) == static_cast<Uint32>(expected.width) &&
            number(20) == static_cast<Uint32>(expected.height);
    }
}

void ForestSprites::Clear()
{
    for (auto &image : images)
    {
        if (image.texture) SDL_DestroyTexture(image.texture);
        image.texture = nullptr;
        image.alpha.clear();
    }
    renderer = nullptr;
}

void ForestSprites::Initialize(SDL_Renderer *target)
{
    Clear();
    renderer = target;
    for (const auto &frame : ForestPilotLayout::Frames)
    {
        if (frame.grassVariant >= 0 && frame.grassVariant < static_cast<int>(grass.size()))
            grass[frame.grassVariant] = &frame;
        if (frame.connectionMask >= 0 && frame.connectionMask < static_cast<int>(paths.size()))
            paths[frame.connectionMask] = &frame;
        if (frame.id == "resource.tree.normal.intact") tree = &frame;
        if (frame.id == "resource.tree.normal.depleted") stump = &frame;
        if (frame.id == "character.mara.idle") mara = &frame;
        if (frame.id == "station.furnace.idle") furnace = &frame;
        if (frame.id.starts_with("character.player.") && frame.facing >= 0 && frame.facing < 4 &&
            frame.animationFrame >= 0 && frame.animationFrame < 4)
            player[frame.facing][frame.animationFrame] = &frame;
    }
#if SDL_VERSION_ATLEAST(3, 4, 0)
    if (!renderer) return;
    std::vector<std::filesystem::path> roots;
    if (const char *base = SDL_GetBasePath())
        roots.emplace_back(reinterpret_cast<const char8_t *>(base));
    std::error_code error;
    const auto cwd = std::filesystem::current_path(error);
    if (!error && (roots.empty() || roots.front() != cwd)) roots.push_back(cwd);
    for (std::size_t index = 0; index < images.size(); ++index)
    {
        const auto &expected = ForestPilotLayout::Images[index];
        SDL_Surface *surface = nullptr;
        for (const auto &root : roots)
        {
            const auto path = root / expected.directory / expected.filename;
            if (!ValidPngHeader(path, expected)) continue;
            const auto utf8 = path.u8string();
            surface = SDL_LoadPNG(reinterpret_cast<const char *>(utf8.c_str()));
            if (surface) break;
        }
        if (!surface || surface->w != expected.width || surface->h != expected.height)
        {
            std::cerr << "World art unavailable: " << expected.filename << "; using basic shapes.\n";
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
            image.texture = nullptr;
            image.alpha.clear();
            std::cerr << "World texture unavailable: " << expected.filename << "; using basic shapes.\n";
        }
    }
    std::clog << "World art: " << LoadedImageCount() << '/' << images.size() << " textures cached.\n";
#else
    std::clog << "World PNG art needs SDL 3.4 or newer; using basic shapes.\n";
#endif
}

std::size_t ForestSprites::LoadedImageCount() const
{
    return std::count_if(images.begin(), images.end(), [](const Image &image) { return image.texture != nullptr; });
}

bool ForestSprites::Available(const Frame *frame) const
{
    return frame && frame->image < images.size() && images[frame->image].texture;
}

bool ForestSprites::HasTree() const { return Available(tree); }
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
    const auto type = map.GetTileType(x, y);
    const SDL_FRect tile{static_cast<float>(x * ForestPilotLayout::TileSize),
        static_cast<float>(y * ForestPilotLayout::TileSize), 32.0f, 32.0f};
    if (type == TileType::GRASS || type == TileType::TREE)
    {
        const auto variant = (static_cast<unsigned int>(x) * 73856093U ^
                              static_cast<unsigned int>(y) * 19349663U) % grass.size();
        return Draw(grass[variant], tile);
    }
    if (type != TileType::DIRT) return false;
    const auto dirt = [&map](int tx, int ty)
    {
        return map.IsInBounds(tx, ty) && map.GetTileType(tx, ty) == TileType::DIRT;
    };
    const bool north = dirt(x, y - 1), east = dirt(x + 1, y);
    const bool south = dirt(x, y + 1), west = dirt(x - 1, y);
    const int mask = (north ? 1 : 0) | (east ? 2 : 0) | (south ? 4 : 0) | (west ? 8 : 0);
    if (!Available(paths[mask]) || !Available(paths[15])) return false;
    const std::array<bool, 4> interior{{
        north && west && dirt(x - 1, y - 1), north && east && dirt(x + 1, y - 1),
        south && west && dirt(x - 1, y + 1), south && east && dirt(x + 1, y + 1)}};
    // Fill interior quarters on wide roads; only exposed corners retain grass.
    for (int quarter = 0; quarter < 4; ++quarter)
    {
        const float qx = static_cast<float>((quarter % 2) * 16);
        const float qy = static_cast<float>((quarter / 2) * 16);
        const auto *frame = interior[quarter] ? paths[15] : paths[mask];
        const SDL_FRect source{frame->source.x + (interior[quarter] ? 8.0f : qx),
            frame->source.y + (interior[quarter] ? 8.0f : qy), 16.0f, 16.0f};
        const SDL_FRect destination{tile.x + qx, tile.y + qy, 16.0f, 16.0f};
        if (!Draw(frame, destination, &source)) return false;
    }
    return true;
}

bool ForestSprites::DrawTree(const ResourceNode &resource, const SDL_FRect &player) const
{
    if (resource.GetResourceType() != ResourceType::NORMAL_TREE) return false;
    const auto *frame = resource.IsActive() ? tree : stump;
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
    return Hit(tree, tileX, tileY, worldX, worldY);
}

bool ForestSprites::DrawPlayer(float tileX, float tileY, int facing, int pose) const
{
    const auto *frame = player[std::clamp(facing, 0, 3)][std::clamp(pose, 0, 3)];
    return DrawCharacter(frame, tileX, tileY);
}

bool ForestSprites::DrawMara(int tileX, int tileY) const
{
    return DrawCharacter(mara, static_cast<float>(tileX), static_cast<float>(tileY));
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

bool ForestSprites::DrawFurnace(int tileX, int tileY) const
{
    return Available(furnace) && Draw(furnace, Bounds(*furnace, tileX, tileY));
}

bool ForestSprites::HitMara(int tileX, int tileY, float worldX, float worldY) const
{
    return Hit(mara, tileX, tileY, worldX, worldY);
}

bool ForestSprites::HitFurnace(int tileX, int tileY, float worldX, float worldY) const
{
    return Hit(furnace, tileX, tileY, worldX, worldY);
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
    return image.alpha[static_cast<std::size_t>(sourceY) * ForestPilotLayout::Images[frame->image].width + sourceX] >= 128;
}
