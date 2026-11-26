#include "CharacterHeldItems.h"
#include "../Animation/PlayerAnimationTiming.h"
#include "../Content/Assets/WorldArtResources.h"
#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace
{
    using namespace CharacterEquipment;
    using Surface = std::unique_ptr<SDL_Surface, decltype(&SDL_DestroySurface)>;

    void Check(bool ok)
    {
        if (!ok) throw std::runtime_error(SDL_GetError());
    }

    const Item *Visual(ItemType type)
    {
        const auto it = std::ranges::find(Items, type, &Item::type);
        return it == Items.end() ? nullptr : &*it;
    }

    struct View
    {
        float x, y, carry, raised, contact;
        bool mainBehind, shieldBehind, flip;
    };
    // World facing order. The angle and depth travel with the anatomical hand.
    constexpr std::array<View, 8> views{{
        {0,1, -28,-84,-142, false,false,true}, {-1,0, -36,-20,-72, true,false,true},
        {1,0, 36,20,72, false,true,false}, {0,-1, 28,88,17, true,true,true},
        {-.7f,.7f, -28,-12,-110, true,false,true}, {.7f,.7f, -22,-58,98, false,true,false},
        {-.7f,-.7f, 27,80,-50, true,false,true}, {.7f,-.7f, 29,-14,54, false,true,false}
    }};

    float Swing(const CharacterPose &pose)
    {
        using namespace PlayerAnimationTiming;
        constexpr std::array<float, ChoppingPoses> chop{0, -.48f, -1, 1, 1.13f, .72f, .28f, 0};
        constexpr std::array<float, MiningPoses> mine{0, -.60f, -1, 1, .62f, .12f};
        constexpr std::array<float, MeleePoses> melee{0, -.5f, -1, 1, 1.16f, .65f, .22f, 0};
        switch (pose.action)
        {
        case CharacterAction::Chop: return chop.at(pose.phase);
        case CharacterAction::Mine: return mine.at(pose.phase);
        case CharacterAction::Melee: return melee.at(pose.phase);
        default: return 0;
        }
    }

    // Fit the existing tunic, trousers and skin through exactly the same points.
    // The feet stay on the ground. The near wrist can reach beyond the torso lean.
    struct Gesture
    {
        const View &view;
        SDL_FPoint hand;
        float swing, reach;

        SDL_FPoint Offset(float x, float y) const
        {
            const float upper = std::clamp((65 - y) / 32, 0.0f, 1.0f);
            const float wrist = std::max(0.0f, 1 - std::abs(y - hand.y) / 15) *
                                std::max(0.0f, 1 - std::abs(x - hand.x) / 11);
            const float forward = swing * 3 * upper + std::abs(swing) * reach * wrist;
            const float lift = swing < 0 ? swing * (upper + 8 * wrist) : swing * (1.2f * upper - wrist);
            return {x + view.x * forward, y + lift + view.y * forward * .50f};
        }
    };

    constexpr int GridSize = 8;
    constexpr auto Indices()
    {
        std::array<int, (GridSize - 1) * (GridSize - 1) * 6> indices{};
        int n = 0;
        for (int y = 0; y < GridSize - 1; ++y)
            for (int x = 0; x < GridSize - 1; ++x)
            {
                const int i = y * GridSize + x;
                for (int vertex : {i, i+1, i+GridSize+1, i, i+GridSize+1, i+GridSize})
                    indices[n++] = vertex;
            }
        return indices;
    }

    void DrawClothes(SDL_Renderer *renderer, const std::array<SDL_Texture *, 3> &outfit,
        const SDL_FRect &source, const SDL_FRect &cell, const Gesture &gesture, SDL_FPoint off)
    {
        if (gesture.swing == 0)
        {
            for (auto *layer : outfit) Check(SDL_RenderTexture(renderer, layer, &source, &cell));
            return;
        }
        // Both grips are mesh vertices, so the item and the artwork use exactly the same transform.
        const auto main = gesture.hand;
        std::array<float, GridSize> xs{0, main.x-11, main.x, main.x+11, off.x-8, off.x, off.x+8, source.w};
        std::array<float, GridSize> ys{0, 26, 34, main.y, off.y, 57, 69, source.h};
        std::ranges::sort(xs);
        std::ranges::sort(ys);
        std::array<SDL_Vertex, GridSize * GridSize> mesh{};
        const float scale = cell.w / source.w;
        int vertex = 0;
        for (const float y : ys)
            for (const float x : xs)
            {
                const auto p = gesture.Offset(x,y);
                mesh[vertex++] = {{cell.x + scale*p.x, cell.y + scale*p.y}, {1,1,1,1},
                    {(source.x+x)/outfit[0]->w, y/source.h}};
            }
        static constexpr auto triangles = Indices();
        for (auto *layer : outfit)
            Check(SDL_RenderGeometry(renderer, layer, mesh.data(), static_cast<int>(mesh.size()),
                triangles.data(), static_cast<int>(triangles.size())));
    }
}

CharacterHeldItems::CharacterHeldItems(SDL_Renderer *target, const std::filesystem::path &root)
    : renderer(target)
{
    using namespace CharacterEquipment;
    const WorldArtResources::Image spec{"shapes.png", Width*Columns, Height*Rows, ""};
    const auto bytes = WorldArtResources::ReadPng(root / "held-items" / spec.filename, spec);
    auto *stream = SDL_IOFromConstMem(bytes.data(), bytes.size());
    Check(stream != nullptr);
    Surface original(SDL_LoadPNG_IO(stream, true), SDL_DestroySurface);
    Check(original != nullptr);
    for (unsigned ink = 0; ink < inks.size(); ++ink)
    {
        Surface sheet(SDL_ConvertSurface(original.get(), SDL_PIXELFORMAT_RGBA32), SDL_DestroySurface);
        Check(sheet != nullptr);
        for (int y = 0; y < sheet->h; ++y)
        {
            auto *pixel = static_cast<Uint8 *>(sheet->pixels) + y*sheet->pitch;
            for (int x = 0; x < sheet->w; ++x, pixel += 4)
            {
                const unsigned stop = pixel[0] == 255 && pixel[1] == 0 && pixel[2] <= 3 ? pixel[2] : 0;
                if (ink == 0 && stop && pixel[3])
                    metalStops[(y/Height)*Columns + x/Width] |= 1U << stop;
                if (ink != stop) pixel[3] = 0;
                if (ink != 0) pixel[0] = pixel[1] = pixel[2] = 255;
            }
        }
        inks[ink].reset(SDL_CreateTextureFromSurface(renderer, sheet.get()));
        Check(inks[ink] != nullptr);
        Check(SDL_SetTextureScaleMode(inks[ink].get(), SDL_SCALEMODE_NEAREST));
        Check(SDL_SetTextureBlendMode(inks[ink].get(), SDL_BLENDMODE_BLEND));
    }
}

void CharacterHeldItems::Draw(const CharacterPose &pose, int column, const SDL_FRect &cell,
    const std::array<SDL_Texture *, 3> &outfit) const
{
    using namespace CharacterEquipment;
    using namespace CharacterFit;
    const auto &hands = ByFacing[pose.facing]->hands[column];
    const auto &view = views[pose.facing];
    const auto *weapon = Visual(pose.mainHand);
    const auto *shield = Visual(pose.offHand);
    const bool heavy = weapon && weapon->shape.motion == Motion::Heavy;
    const float swing = Swing(pose);
    const SDL_FPoint main{hands.mainX+.5f, hands.mainY+.5f}, off{hands.offX+.5f, hands.offY+.5f};
    const Gesture gesture{view, main, swing, weapon ? (heavy ? 5.0f : 7.0f) : 8.0f};
    const auto grip = gesture.Offset(main.x,main.y);
    const float scale = cell.w/CellWidth;
    auto drawItem = [&](const Item &item, SDL_FPoint anchor, float angle) {
        const bool flip = item.shape.mirrorByFacing && view.flip;
        const int image = item.shape.views[pose.facing];
        const float zoom = scale * 1.35f;
        const SDL_FPoint pivot{(flip ? Width-GripX : GripX)*zoom, GripY*zoom};
        const SDL_FRect src{static_cast<float>((image%Columns)*Width),static_cast<float>((image/Columns)*Height),
            Width,Height};
        const SDL_FRect dest{cell.x + scale*anchor.x - pivot.x, cell.y + scale*anchor.y - pivot.y,
            Width*zoom, Height*zoom};
        for (unsigned ink = 0; ink < inks.size(); ++ink)
        {
            auto *texture = inks[ink].get();
            if (ink > 0)
            {
                if (!(metalStops[image] & (1U << ink))) continue;
                const auto rgb = Palettes[item.material][ink-1];
                Check(SDL_SetTextureColorMod(texture, Uint8(rgb >> 16), Uint8(rgb >> 8), Uint8(rgb)));
            }
            Check(SDL_RenderTextureRotated(renderer, texture, &src, &dest, angle, &pivot,
                flip ? SDL_FLIP_HORIZONTAL : SDL_FLIP_NONE));
        }
    };
    auto drawWeapon = [&] {
        float target = swing < 0 ? view.raised : view.contact;
        if (pose.action == CharacterAction::Melee && weapon->shape.motion != Motion::Overhead)
            target += (view.flip ? -1 : 1) * (swing < 0 ? -18.0f : 15.0f);
        const float angle = view.carry + (target-view.carry)*std::abs(swing);
        drawItem(*weapon, grip, angle);
    };
    auto drawShield = [&] {
        const auto anchor = heavy ? gesture.Offset(40-8*view.x,36) : gesture.Offset(off.x+2*view.x,off.y-2);
        drawItem(*shield, anchor, heavy ? -12.0f : 0.0f);
    };
    const bool shieldBehind = heavy ? view.y >= 0 : view.shieldBehind;
    if (shield && shieldBehind) drawShield();
    if (weapon && view.mainBehind) drawWeapon();

    const SDL_FRect source{static_cast<float>(column*CellWidth),0,CellWidth,CellHeight};
    DrawClothes(renderer, outfit, source, cell, gesture, off);
    if (weapon && !view.mainBehind)
    {
        drawWeapon();
        // Cover the shaft with the fitted near-hand pixels from this exact source pose.
        const SDL_FRect fingers{source.x+main.x-1.5f,main.y-1.5f,3,2};
        const SDL_FRect dest{cell.x+(grip.x-1.5f)*scale,cell.y+(grip.y-1.5f)*scale,3*scale,2*scale};
        Check(SDL_RenderTexture(renderer, outfit[0], &fingers, &dest));
    }
    if (shield && !shieldBehind) drawShield();
}
