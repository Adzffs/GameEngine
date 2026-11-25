#pragma once

#include <SDL3/SDL.h>
#include <array>
#include <filesystem>
#include <memory>

// Only linked into EmbervaleRenderPreview. Bindings are generated from the
// retained garment manifest at configure time.
class CharacterFitPreview
{
public:
    CharacterFitPreview(SDL_Renderer *renderer, const std::filesystem::path &pilotRoot, bool kneePants, bool nativeSize);
    void Draw(float groundX, float groundY, bool walking, float travelled, Uint64 tick);
    SDL_FRect OcclusionBounds(float groundX, float groundY) const;

private:
    using Texture = std::unique_ptr<SDL_Texture, decltype(&SDL_DestroyTexture)>;
    SDL_Renderer *renderer;
    std::array<Texture, 3> layers;
    float scale;
    float walkDistance = 0.0f;
    Uint64 idleStart = 0;
    bool wasWalking = false;
    bool started = false;
};
