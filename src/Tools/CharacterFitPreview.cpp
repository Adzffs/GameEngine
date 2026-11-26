#include "CharacterFitPreview.h"
#include "CharacterFitBindings.h"
#include "../Content/Assets/WorldArtResources.h"
#include <cmath>
#include <stdexcept>
#include <string>

namespace
{
    using Texture = std::unique_ptr<SDL_Texture, decltype(&SDL_DestroyTexture)>;

    constexpr int SouthFacing = 0, EastFacing = 2;

    const CharacterFit::Direction &BindingFor(int facing)
    {
        return facing == EastFacing ? CharacterFit::East : CharacterFit::South;
    }

    Texture LoadLayer(SDL_Renderer *renderer, const std::filesystem::path &path, int columns)
    {
        const WorldArtResources::Image spec{"layer.png", columns * CharacterFit::CellWidth, CharacterFit::CellHeight, ""};
        const auto bytes = WorldArtResources::ReadPng(path, spec);
        auto *stream = SDL_IOFromConstMem(bytes.data(), bytes.size());
        if (!stream) throw std::runtime_error(SDL_GetError());
        std::unique_ptr<SDL_Surface, decltype(&SDL_DestroySurface)> surface(SDL_LoadPNG_IO(stream, true), SDL_DestroySurface);
        if (!surface || surface->w != spec.width || surface->h != spec.height)
            throw std::runtime_error("Cannot decode character layer " + path.string());
        Texture texture(SDL_CreateTextureFromSurface(renderer, surface.get()), SDL_DestroyTexture);
        if (!texture || !SDL_SetTextureScaleMode(texture.get(), SDL_SCALEMODE_NEAREST) ||
            !SDL_SetTextureBlendMode(texture.get(), SDL_BLENDMODE_BLEND))
            throw std::runtime_error(SDL_GetError());
        return texture;
    }

    std::array<Texture, 3> LoadOutfit(SDL_Renderer *renderer, const std::filesystem::path &root,
        const CharacterFit::Direction &direction, bool kneePants)
    {
        const int columns = static_cast<int>(direction.idle.columns.size() + direction.walk.columns.size());
        return {LoadLayer(renderer, root / direction.body, columns),
                LoadLayer(renderer, root / (kneePants ? direction.kneePants : direction.fullPants), columns),
                LoadLayer(renderer, root / direction.tunic, columns)};
    }

    int ColumnAt(const CharacterFit::Clip &clip, int phase)
    {
        for (std::size_t i = 0; i < clip.columns.size(); ++i)
        {
            if (phase < clip.durations[i]) return clip.columns[i];
            phase -= clip.durations[i];
        }
        return clip.columns.front();
    }
}

CharacterFitPreview::CharacterFitPreview(SDL_Renderer *target, const std::filesystem::path &root,
    bool kneePants, bool nativeSize)
    : renderer(target),
      southLayers(LoadOutfit(target, root, CharacterFit::South, kneePants)),
      eastLayers(LoadOutfit(target, root, CharacterFit::East, kneePants)),
      scale(nativeSize ? 1.0f : 0.6f)
{
}

bool CharacterFitPreview::SupportsFacing(int facing)
{
    return facing == SouthFacing || facing == EastFacing;
}

void CharacterFitPreview::ResetMotion()
{
    started = false;
    wasWalking = false;
    walkDistance = 0.0f;
}

SDL_FRect CharacterFitPreview::OcclusionBounds(float groundX, float groundY, int facing) const
{
    // The world also reads the bottom centre of this rectangle as the feet.
    const auto &direction = BindingFor(facing);
    const float halfWidth = direction.halfWidth * scale;
    const float height = direction.height * scale;
    return {groundX - halfWidth, groundY - height, 2 * halfWidth, height};
}

void CharacterFitPreview::Draw(float groundX, float groundY, int facing, float travelled, Uint64 tick)
{
    using namespace CharacterFit;
    const auto &direction = BindingFor(facing);
    const bool walking = travelled > 0.002f;
    int column;
    if (walking)
    {
        // One authored walk loop per accepted tile, independent of render FPS.
        walkDistance = std::fmod(walkDistance + travelled, 1.0f);
        column = ColumnAt(direction.walk, static_cast<int>(walkDistance * direction.walk.milliseconds));
    }
    else
    {
        if (!started || wasWalking) idleStart = tick;
        walkDistance = 0.0f;
        column = ColumnAt(direction.idle, static_cast<int>((tick - idleStart) % direction.idle.milliseconds));
    }
    started = true;
    wasWalking = walking;
    const SDL_FRect source{static_cast<float>(column * CellWidth), 0, CellWidth, CellHeight};
    const SDL_FRect dest{groundX - PivotX * scale, groundY - PivotY * scale, CellWidth * scale, CellHeight * scale};
    for (const auto &layer : (facing == EastFacing ? eastLayers : southLayers))
        if (!SDL_RenderTexture(renderer, layer.get(), &source, &dest))
            throw std::runtime_error(SDL_GetError());
}
