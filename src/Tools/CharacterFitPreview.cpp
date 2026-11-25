#include "CharacterFitPreview.h"
#include "CharacterFitBindings.h"
#include "../Content/Assets/WorldArtResources.h"
#include <cmath>
#include <numeric>
#include <stdexcept>
#include <string>

namespace
{
    using Texture = std::unique_ptr<SDL_Texture, decltype(&SDL_DestroyTexture)>;

    Texture LoadLayer(SDL_Renderer *renderer, const std::filesystem::path &path)
    {
        constexpr int columns = CharacterFit::IdleColumns.size() + CharacterFit::WalkColumns.size();
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

    template<std::size_t N>
    int ColumnAt(const std::array<int, N> &columns, const std::array<int, N> &durations, int phase)
    {
        for (std::size_t i = 0; i < N; ++i)
        {
            if (phase < durations[i]) return columns[i];
            phase -= durations[i];
        }
        return columns.front();
    }
}

CharacterFitPreview::CharacterFitPreview(SDL_Renderer *target, const std::filesystem::path &root,
    bool kneePants, bool nativeSize)
    : renderer(target),
      layers{LoadLayer(target, root / CharacterFit::Body),
             LoadLayer(target, root / (kneePants ? CharacterFit::KneePants : CharacterFit::FullPants)),
             LoadLayer(target, root / CharacterFit::Tunic)},
      scale(nativeSize ? 1.0f : 0.6f)
{
}

SDL_FRect CharacterFitPreview::OcclusionBounds(float groundX, float groundY) const
{
    // The twelve retained body silhouettes span x=22..56 and y=8..70.
    // The world also reads the bottom centre of this rectangle as the feet.
    const float halfWidth = (CharacterFit::PivotX - 22) * scale;
    const float height = (CharacterFit::PivotY - 8) * scale;
    return {groundX - halfWidth, groundY - height, 2 * halfWidth, height};
}

void CharacterFitPreview::Draw(float groundX, float groundY, bool walking, float travelled, Uint64 tick)
{
    using namespace CharacterFit;
    constexpr int walkMs = std::accumulate(WalkDurations.begin(), WalkDurations.end(), 0);
    constexpr int idleMs = std::accumulate(IdleDurations.begin(), IdleDurations.end(), 0);
    int column;
    if (walking)
    {
        // One authored walk loop per accepted tile, independent of render FPS.
        walkDistance = std::fmod(walkDistance + travelled, 1.0f);
        column = ColumnAt(WalkColumns, WalkDurations, static_cast<int>(walkDistance * walkMs));
    }
    else
    {
        if (!started || wasWalking) idleStart = tick;
        walkDistance = 0.0f;
        column = ColumnAt(IdleColumns, IdleDurations, static_cast<int>((tick - idleStart) % idleMs));
    }
    started = true;
    wasWalking = walking;
    const SDL_FRect source{static_cast<float>(column * CellWidth), 0, CellWidth, CellHeight};
    const SDL_FRect dest{groundX - PivotX * scale, groundY - PivotY * scale, CellWidth * scale, CellHeight * scale};
    for (const auto &layer : layers)
        if (!SDL_RenderTexture(renderer, layer.get(), &source, &dest))
            throw std::runtime_error(SDL_GetError());
}
