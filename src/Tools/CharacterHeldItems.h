#pragma once

#include "CharacterEquipmentBindings.h"
#include "CharacterFitBindings.h"
#include <SDL3/SDL.h>
#include <filesystem>
#include <memory>

enum class CharacterAction { Carry, Chop, Mine, Melee };

struct CharacterPose
{
    int facing;
    ItemType mainHand, offHand;
    CharacterAction action = CharacterAction::Carry;
    int phase = 0;
};

class CharacterHeldItems
{
public:
    CharacterHeldItems(SDL_Renderer *renderer, const std::filesystem::path &root);
    void Draw(const CharacterPose &pose, int column, const SDL_FRect &cell,
        const std::array<SDL_Texture *, 3> &outfit) const;

private:
    struct DestroyTexture { void operator()(SDL_Texture *texture) const { SDL_DestroyTexture(texture); } };
    SDL_Renderer *renderer;
    // Fixed colours and three metal masks are shared by every item and material.
    std::array<std::unique_ptr<SDL_Texture, DestroyTexture>, 4> inks;
    std::array<unsigned, CharacterEquipment::Rows * CharacterEquipment::Columns> metalStops{};
};
