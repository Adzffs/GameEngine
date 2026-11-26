#include "TestSupport.h"
#include "../src/Animation/PlayerAnimationTiming.h"
#include "../src/Tools/CharacterFitPreview.h"
#include <set>
#include <vector>

namespace
{
    using Pixels = std::vector<Uint32>;
    bool SameShape(SDL_Surface *surface, const Pixels &bronze, const Pixels &variant, ItemType item)
    {
        const auto &items = CharacterEquipment::Items;
        const auto binding = std::ranges::find(items, item, &CharacterEquipment::Item::type);
        const auto material = std::ranges::find(items, ItemType::BRONZE_AXE, &CharacterEquipment::Item::type);
        const auto pixels = [&](int palette) {
            std::array<Uint32,3> result{};
            for (int i = 0; i < 3; ++i)
            {
                const auto rgb = CharacterEquipment::Palettes[palette][i];
                result[i] = SDL_MapSurfaceRGB(surface, Uint8(rgb >> 16), Uint8(rgb >> 8), Uint8(rgb));
            }
            return result;
        };
        const auto source = pixels(material->material), target = pixels(binding->material);
        for (std::size_t i = 0; i < bronze.size(); ++i)
        {
            if (bronze[i] == variant[i]) continue;
            const auto stop = std::ranges::find(source, bronze[i]);
            if (stop == source.end() || variant[i] != target[stop-source.begin()]) return false;
        }
        return true;
    }

    Pixels Capture(SDL_Renderer *renderer, SDL_Surface *surface, CharacterFitPreview &actor,
        const CharacterPose &pose, float advance = 0)
    {
        SDL_SetRenderDrawColor(renderer, 18, 15, 19, 255);
        SDL_RenderClear(renderer);
        actor.Draw(64, 92, pose, advance, 1000);
        if (!SDL_RenderPresent(renderer)) throw std::runtime_error(SDL_GetError());
        Pixels pixels;
        for (int y = 0; y < surface->h; ++y)
        {
            const auto *row = reinterpret_cast<const Uint32 *>(
                static_cast<const Uint8 *>(surface->pixels) + y * surface->pitch);
            pixels.insert(pixels.end(), row, row + surface->w);
        }
        return pixels;
    }
}

int main(int argc, char **argv)
{
    if (argc != 2) return 2;
    TestContext test;
    std::unique_ptr<SDL_Surface, decltype(&SDL_DestroySurface)> surface(
        SDL_CreateSurface(128,128,SDL_PIXELFORMAT_RGBA32), SDL_DestroySurface);
    std::unique_ptr<SDL_Renderer, decltype(&SDL_DestroyRenderer)> renderer(
        surface ? SDL_CreateSoftwareRenderer(surface.get()) : nullptr, SDL_DestroyRenderer);
    if (!renderer) { test.Expect(false, SDL_GetError()); return test.Finish(); }
    CharacterFitPreview actor(renderer.get(), argv[1], false, true, true);
    using enum ItemType;
    for (int facing = 0; facing < 8; ++facing)
    {
        auto draw = [&](ItemType weapon, ItemType shield, CharacterAction action = CharacterAction::Carry, int phase = 0) {
            return Capture(renderer.get(), surface.get(), actor, {facing, weapon, shield, action, phase});
        };
        const auto view = std::to_string(facing);
        const auto bare = draw(NONE,NONE);
        const auto shield = draw(NONE,WOODEN_SHIELD);
        test.Expect(shield != bare, "Shield silhouette in view " + view);
        for (const auto &family : {std::array{BRONZE_AXE, IRON_AXE, STEEL_AXE},
                                   std::array{BRONZE_PICKAXE, IRON_PICKAXE, STEEL_PICKAXE}})
        {
            std::set<Pixels> metals;
            const auto bronze = draw(family.front(),NONE);
            for (auto item : family)
            {
                const auto tool = draw(item,NONE);
                const auto both = draw(item,WOODEN_SHIELD);
                test.Expect(tool != bare && both != tool && both != shield, "Both hands contribute in view " + view);
                test.Expect(SameShape(surface.get(), bronze, tool, item), "Only marked metal changes in view " + view);
                metals.insert(tool);
            }
            test.Expect(metals.size() == 3, "Three distinct head colours in view " + view);
        }
        test.Expect(draw(NONE,NONE) == bare, "Unequipping restores the same idle pixels in view " + view);
        for (auto action : {CharacterAction::Chop, CharacterAction::Mine, CharacterAction::Melee})
        {
            const bool mining = action == CharacterAction::Mine;
            const auto item = mining ? BRONZE_PICKAXE : action == CharacterAction::Chop ? BRONZE_AXE : BRONZE_SWORD;
            const auto phases = mining ? PlayerAnimationTiming::MiningPoses : PlayerAnimationTiming::MeleePoses;
            std::set<Pixels> motion;
            for (int phase = 0; phase < phases; ++phase)
            {
                const auto full = draw(item,WOODEN_SHIELD,action,phase);
                actor.ToggleTrousers();
                test.Expect(draw(item,WOODEN_SHIELD,action,phase) != full, "Trouser fit during action in view " + view);
                actor.ToggleTrousers();
                motion.insert(full);
            }
            test.Expect(motion.size() >= 5, "Visible windup, contact and recovery in view " + view);
        }
        test.Expect(draw(DEVELOPER_GODSWORD,WOODEN_SHIELD) != bare, "Heavy carry in view " + view);
        // Select the same walk phase with and without equipment. No title, HUD or scenery
        // is present in this capture, so a difference must come from the character.
        for (int stride = 0; stride < 8; ++stride)
        {
            const float distance = (stride+.5f)/8;
            auto walk = [&](ItemType weapon, ItemType off) {
                actor.ResetMotion();
                return Capture(renderer.get(), surface.get(), actor, {facing,weapon,off}, distance);
            };
            const auto empty = walk(NONE,NONE);
            test.Expect(walk(STEEL_AXE,WOODEN_SHIELD) != empty, "Equipped stride in view " + view);
        }
    }
    return test.Finish();
}
