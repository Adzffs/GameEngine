#include "CharacterFitPreview.h"
#include "../Content/Assets/WorldArtResources.h"
#include <cmath>
#include <stdexcept>
#include <string>

namespace
{
    using Texture = std::unique_ptr<SDL_Texture, decltype(&SDL_DestroyTexture)>;

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
    bool kneePants, bool nativeSize, bool switchable)
    : heldItems(target, root), scale(nativeSize ? 1.0f : 0.6f), kneeLength(kneePants), canSwitchTrousers(switchable)
{
    for (std::size_t facing = 0; facing < layers.size(); ++facing)
    {
        const auto *direction = CharacterFit::ByFacing[facing];
        if (!direction) continue;
        const int columns = static_cast<int>(direction->idle.columns.size() + direction->walk.columns.size());
        layers[facing] = Layers{
            LoadLayer(target, root / direction->body, columns),
            LoadLayer(target, root / (kneePants ? direction->kneePants : direction->fullPants), columns),
            LoadLayer(target, root / direction->tunic, columns),
            switchable ? LoadLayer(target, root / (kneePants ? direction->fullPants : direction->kneePants), columns)
                       : Texture(nullptr, SDL_DestroyTexture)};
    }
}

bool CharacterFitPreview::ToggleTrousers()
{
    if (!canSwitchTrousers) return false;
    for (auto &outfit : layers)
        if (outfit) outfit->trousers.swap(outfit->alternateTrousers);
    kneeLength = !kneeLength;
    return true;
}

bool CharacterFitPreview::SupportsFacing(int facing)
{
    return facing >= 0 && static_cast<std::size_t>(facing) < CharacterFit::ByFacing.size() &&
           CharacterFit::ByFacing[facing] != nullptr;
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
    const auto &direction = *CharacterFit::ByFacing[facing];
    const float halfWidth = direction.halfWidth * scale;
    const float height = direction.height * scale;
    return {groundX - halfWidth, groundY - height, 2 * halfWidth, height};
}

void CharacterFitPreview::Draw(float groundX, float groundY, const CharacterPose &pose, float travelled, Uint64 tick)
{
    using namespace CharacterFit;
    const auto &direction = *ByFacing[pose.facing];
    const bool walking = travelled > 0.002f;
    int column;
    if (pose.action != CharacterAction::Carry)
    {
        ResetMotion();
        column = direction.idle.columns.front();
    }
    else if (walking)
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
    started = pose.action == CharacterAction::Carry;
    wasWalking = walking && started;
    const SDL_FRect dest{groundX - PivotX * scale, groundY - PivotY * scale, CellWidth * scale, CellHeight * scale};
    const auto &outfit = *layers[pose.facing];
    heldItems.Draw(pose, column, dest, {outfit.body.get(), outfit.trousers.get(), outfit.tunic.get()});
}
