#pragma once

#include "CharacterFitBindings.h"
#include <SDL3/SDL.h>
#include <array>
#include <filesystem>
#include <memory>
#include <optional>

// Shared by the interactive character playtest and the developer renderer.
class CharacterFitPreview
{
public:
    CharacterFitPreview(SDL_Renderer *renderer, const std::filesystem::path &pilotRoot,
        bool kneePants, bool nativeSize, bool switchable = false);
    bool ToggleTrousers();
    bool WearsKneePants() const { return kneeLength; }
    static bool SupportsFacing(int facing);
    void Draw(float groundX, float groundY, int facing, float travelled, Uint64 tick);
    void ResetMotion();
    SDL_FRect OcclusionBounds(float groundX, float groundY, int facing) const;

private:
    using Texture = std::unique_ptr<SDL_Texture, decltype(&SDL_DestroyTexture)>;
    struct Layers { Texture body, trousers, tunic, alternateTrousers; };
    SDL_Renderer *renderer;
    std::array<std::optional<Layers>, CharacterFit::ByFacing.size()> layers;
    float scale;
    bool kneeLength;
    bool canSwitchTrousers;
    float walkDistance = 0.0f;
    Uint64 idleStart = 0;
    bool wasWalking = false;
    bool started = false;
};
