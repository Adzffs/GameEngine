#include "TestSupport.h"
#include "../src/Graphics/Graphics.h"
#include <array>
#include <cmath>
#include <cstdint>
#include <set>

struct GraphicsTestAccess
{
    static void Travel(Graphics &graphics, float dx, float dy, float distance = 0.0f)
    {
        graphics.visualPlayerInitialized = true;
        graphics.visualPlayerX = 2.0f - dx;
        graphics.visualPlayerY = 2.0f - dy;
        graphics.playerWalkDistance = distance;
        // Snap to the target so the test controls the exact visual displacement.
        graphics.presentationDeltaSeconds = 10.0f;
        graphics.DrawPlayer(2, 2);
    }

    static int Facing(const Graphics &graphics) { return graphics.playerFacing; }
    static void Settle(Graphics &graphics, float dx, float dy)
    {
        graphics.visualPlayerInitialized = true;
        graphics.visualPlayerX = 2.0f - dx;
        graphics.visualPlayerY = 2.0f - dy;
        graphics.presentationDeltaSeconds = 1.0f / 60.0f;
        for (int frame = 0; frame < 120; ++frame) graphics.DrawPlayer(2, 2);
    }
    static float Distance(const Graphics &graphics) { return graphics.playerWalkDistance; }
    static SDL_Renderer *Renderer(Graphics &graphics) { return graphics.renderer; }
    static ForestSprites &Sprites(Graphics &graphics) { return graphics.forestSprites; }
    static void BeginStep(Graphics &g)
    {
        g.visualPlayerInitialized = true; g.visualPlayerX = 2; g.visualPlayerY = 2;
        g.visualMovementTargetInitialized = true;
        g.visualMovementTargetX = 2; g.visualMovementTargetY = 2;
    }
    static SDL_FPoint Step(Graphics &g, int x, int y, float delta)
    {
        g.presentationDeltaSeconds = delta;
        g.DrawPlayer(x,y);
        return {g.visualPlayerX,g.visualPlayerY};
    }
};

namespace
{
    std::uint64_t Capture(TestContext &test, SDL_Renderer *renderer)
    {
        const SDL_Rect region{48, 32, 64, 64};
        SDL_Surface *surface = SDL_RenderReadPixels(renderer, &region);
        test.Expect(surface != nullptr, "Rendered player pixels can be read back");
        if (!surface) return 0;
        std::uint64_t hash = 14695981039346656037ULL;
        for (int y = 0; y < surface->h; ++y)
            for (int x = 0; x < surface->w; ++x)
            {
                Uint8 r, g, b, a;
                SDL_ReadSurfacePixel(surface, x, y, &r, &g, &b, &a);
                for (const auto channel : {r, g, b, a})
                    hash = (hash ^ channel) * 1099511628211ULL;
            }
        SDL_DestroySurface(surface);
        return hash;
    }
}

int main()
{
    TestContext test;
    Graphics graphics;
    if (!graphics.Initialize())
    {
        test.Expect(false, "Player animation renderer initializes");
        return test.Finish();
    }
    auto *renderer = GraphicsTestAccess::Renderer(graphics);
    auto &sprites = GraphicsTestAccess::Sprites(graphics);
    const std::array<SDL_FPoint, 8> directions{{
        {0, 1}, {-1, 0}, {1, 0}, {0, -1}, {-1, 1}, {1, 1}, {-1, -1}, {1, -1}}};
    for (std::size_t facing = 0; facing < directions.size(); ++facing)
    {
        const auto travel = directions[facing];
        GraphicsTestAccess::Travel(graphics, travel.x, travel.y);
        test.ExpectEqual(GraphicsTestAccess::Facing(graphics), static_cast<int>(facing),
            "Visual travel selects its own compass direction");
        const float expected = std::fmod(std::hypot(travel.x, travel.y), ForestPilotLayout::PlayerWalkCycleTiles);
        test.ExpectNear(GraphicsTestAccess::Distance(graphics), expected, 0.0001f,
            "Walking advances by actual travel distance for cardinal and diagonal routes");
        GraphicsTestAccess::Travel(graphics, 0, 0, 0.6f);
        test.ExpectEqual(GraphicsTestAccess::Facing(graphics), static_cast<int>(facing),
            "Stopping retains the last direction");
    }

    // A small residual from the previous axis must not turn a straight route diagonally.
    for (const auto travel : std::array<SDL_FPoint, 4>{{{1, 0.1f}, {-1, -0.1f}, {0.1f, 1}, {-0.1f, -1}}})
    {
        GraphicsTestAccess::Travel(graphics, travel.x, travel.y);
        const int expected = std::abs(travel.x) > std::abs(travel.y) ? (travel.x < 0 ? 1 : 2) : (travel.y < 0 ? 3 : 0);
        test.ExpectEqual(GraphicsTestAccess::Facing(graphics), expected,
            "Small smoothing tails keep the cardinal direction");
    }

    for (int facing = 4; facing < 8; ++facing)
        for (const auto scale : std::array<SDL_FPoint, 2>{{{1.1f, 1.0f}, {1.0f, 1.1f}}})
        {
            GraphicsTestAccess::Settle(graphics, directions[facing].x * scale.x, directions[facing].y * scale.y);
            test.ExpectEqual(GraphicsTestAccess::Facing(graphics), facing,
                "Uneven interpolation settles without snapping diagonal idle onto a cardinal axis");
        }

#if SDL_VERSION_ATLEAST(3, 4, 0)
    test.Expect(sprites.HasPlayer(), "Packaged player atlas loads through the real texture cache");
    std::set<std::uint64_t> idleDirections;
    for (int facing = 0; facing < 8; ++facing)
    {
        std::array<std::uint64_t, PlayerAnimationTiming::WalkPoses> poses{};
        for (int pose = 0; pose < PlayerAnimationTiming::WalkPoses; ++pose)
        {
            SDL_SetRenderDrawColor(renderer, 30, 40, 50, 255);
            SDL_RenderClear(renderer);
            test.Expect(sprites.DrawPlayer(2, 2, facing, pose),
                "Every direction and walk pose renders an authored sprite");
            poses[pose] = Capture(test, renderer);
        }
        idleDirections.insert(poses[0]);
        test.Expect(poses[1] != poses[0] && poses[3] != poses[0] && poses[1] != poses[3],
            "Each direction has two distinct alternating strides");
        const std::set<std::uint64_t> gait(poses.begin(),poses.end());
        test.Expect(gait.size() >= 6, "Walking has distinct contact, passing and lift poses rather than repeated neutral cells");
        SDL_SetRenderDrawColor(renderer, 30, 40, 50, 255);
        SDL_RenderClear(renderer);
        GraphicsTestAccess::Travel(graphics, directions[facing].x * 0.3f, directions[facing].y * 0.3f);
        GraphicsTestAccess::Travel(graphics, 0, 0, 0.6f);
        // Clear before drawing the stopped pose so stale stride pixels cannot mask a regression.
        SDL_SetRenderDrawColor(renderer, 30, 40, 50, 255);
        SDL_RenderClear(renderer);
        GraphicsTestAccess::Travel(graphics, 0, 0, 0.6f);
        test.ExpectEqual(Capture(test, renderer), poses[0],
            "Stopping draws the neutral pose for the retained direction");
    }
    test.ExpectEqual(idleDirections.size(), std::size_t{8},
        "The eight compass directions use visually distinct sprites");
#endif
    for (const auto destination : std::array<SDL_Point,3>{{{3,2},{3,3},{4,2}}})
        for (const float delta : {1.0f/30.0f,1.0f/60.0f})
        {
            GraphicsTestAccess::BeginStep(graphics);
            auto previous = SDL_FPoint{2,2};
            float expectedStep = 0;
            const int frames = static_cast<int>(std::round(0.6f/delta));
            for (int frame = 0; frame < frames; ++frame)
            {
                const auto next = GraphicsTestAccess::Step(graphics,destination.x,destination.y,delta);
                const float distance = std::hypot(next.x-previous.x,next.y-previous.y);
                if (frame == 0) expectedStep = distance;
                else test.ExpectNear(distance,expectedStep,0.0001f,"Walking and running retain steady visual speed throughout a step");
                previous = next;
            }
            test.ExpectNear(previous.x,static_cast<float>(destination.x),0.0001f,"Visual step reaches its authoritative X without overshoot");
            test.ExpectNear(previous.y,static_cast<float>(destination.y),0.0001f,"Visual step reaches its authoritative Y without overshoot");
        }
    return test.Finish();
}
