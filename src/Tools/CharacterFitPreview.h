#pragma once

#include "CharacterFitBindings.h"
#include <SDL3/SDL.h>
#include <array>
#include <filesystem>
#include <memory>
#include <optional>

// Only linked into EmbervaleRenderPreview. Bindings are generated from the
// retained garment manifests at configure time.
class CharacterFitPreview
{
public:
    CharacterFitPreview(SDL_Renderer *renderer, const std::filesystem::path &pilotRoot, bool kneePants, bool nativeSize);
    static bool SupportsFacing(int facing);
    void Draw(float groundX, float groundY, int facing, float travelled, Uint64 tick);
    void ResetMotion();
    SDL_FRect OcclusionBounds(float groundX, float groundY, int facing) const;

private:
    using Texture = std::unique_ptr<SDL_Texture, decltype(&SDL_DestroyTexture)>;
    using Layers = std::array<Texture, 3>;
    SDL_Renderer *renderer;
    std::array<std::optional<Layers>, CharacterFit::ByFacing.size()> layers;
    float scale;
    float walkDistance = 0.0f;
    Uint64 idleStart = 0;
    bool wasWalking = false;
    bool started = false;
};
