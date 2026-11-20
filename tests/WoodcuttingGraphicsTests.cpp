#include "TestSupport.h"
#include "../src/Graphics/Graphics.h"
#include "../src/Player/Player.h"
#include "../src/World/Map.h"
#include <array>
#include <cstdint>
#include <set>

struct GraphicsTestAccess
{
    static ForestSprites &Sprites(Graphics &g) { return g.forestSprites; }
    static SDL_Renderer *Renderer(Graphics &g) { return g.renderer; }
    static const WoodcuttingPresentationState &Chopping(const Graphics &g) { return g.woodcuttingPresentation; }
    static void Delta(Graphics &g) { g.lastPresentationTick = 0; g.presentationDeltaSeconds = 0.1f; }
};

std::uint64_t Capture(SDL_Renderer *renderer)
{
    const SDL_Rect area{48,32,64,96};
    SDL_Surface *surface = SDL_RenderReadPixels(renderer,&area);
    if (!surface) return 0;
    std::uint64_t hash = 14695981039346656037ULL;
    for (int y = 0; y < surface->h; ++y) for (int x = 0; x < surface->w; ++x)
    {
        Uint8 r,g,b,a;
        SDL_ReadSurfacePixel(surface,x,y,&r,&g,&b,&a);
        for (const auto channel : {r,g,b,a}) hash = (hash ^ channel) * 1099511628211ULL;
    }
    SDL_DestroySurface(surface);
    return hash;
}

int main()
{
    TestContext test;
    Graphics graphics;
    if (!graphics.Initialize()) { test.Expect(false,"Woodcutting renderer initializes"); return test.Finish(); }
    auto *renderer = GraphicsTestAccess::Renderer(graphics);
    auto &sprites = GraphicsTestAccess::Sprites(graphics);
#if SDL_VERSION_ATLEAST(3,4,0)
    std::set<std::uint64_t> directions;
    for (int facing = 0; facing < 8; ++facing)
    {
        std::set<std::uint64_t> tiers;
        for (int tier = 0; tier < 3; ++tier)
        {
            std::array<std::uint64_t,6> poses{};
            for (int pose = 0; pose < 6; ++pose)
            {
                SDL_SetRenderDrawColor(renderer,30,40,50,255); SDL_RenderClear(renderer);
                test.Expect(sprites.DrawWoodcutting(2,2,facing,pose,tier),"Every action/tool cell renders through the cache");
                poses[pose] = Capture(renderer);
                test.Expect(poses[pose] != 0,"Rendered chopping pixels can be captured");
            }
            test.Expect(poses[0] != poses[2] && poses[2] != poses[3] && poses[3] != poses[5],
                "Ready, wind-up, contact and recovery have distinct artwork");
            tiers.insert(poses[3]);
            if (tier == 0) directions.insert(poses[3]);
        }
        test.ExpectEqual(tiers.size(),std::size_t{3},"Bronze, iron and steel blades are visually distinct");
    }
    test.ExpectEqual(directions.size(),std::size_t{8},"All eight chopping directions have distinct sprites");
    test.Expect(!sprites.DrawWoodcutting(2,2,8,0,0),"Invalid action direction falls back safely");
    test.Expect(!sprites.DrawWoodcutting(2,2,0,6,0),"Invalid action pose falls back safely");
#endif
    SDL_Texture *target = SDL_CreateTexture(renderer,SDL_PIXELFORMAT_RGBA8888,SDL_TEXTUREACCESS_TARGET,1280,720);
    test.Expect(target && SDL_SetRenderTarget(renderer,target),"Full scene renders to a readable target");
    Map map(100,100);
    Player player(1,PlayerInitializationMode::FRESH_START);
    player.GetPosition().SetPosition(2,2);
    player.GetEquipment().Equip(EquipmentSlotType::WEAPON,ItemType::BRONZE_AXE);
    std::vector<ResourceNode> resources{ResourceNode(7,ResourceType::NORMAL_TREE,3,2)};
    Action action(ActionType::GATHERING,"Chopping",5,1,7,true); action.Start(1);
    auto draw = [&](const Action *active) {
        GraphicsTestAccess::Delta(graphics);
        graphics.Render(map,{},resources,{},{},{},2,2,player.GetInventory(),player.GetEquipment(),player,active);
    };
    draw(&action);
    test.Expect(GraphicsTestAccess::Chopping(graphics).GetAnimation().has_value(),"Live render selects the gathering animation");
    for (int frame = 0; frame < 7; ++frame) draw(&action);
    test.ExpectEqual(GraphicsTestAccess::Chopping(graphics).GetPose(),3,"Real rendering advances to contact");
    graphics.SynchronizeGatheringRewards(1,{{1,7,6,ResourceType::NORMAL_TREE,3,2,2,2,ItemType::LOG,1,25}});
    draw(nullptr);
    test.Expect(!GraphicsTestAccess::Chopping(graphics).GetAnimation(),"Full renderer stops when the action ends");
    test.ExpectEqual(GraphicsTestAccess::Chopping(graphics).GetRewards().size(),std::size_t{1},
        "Final-use reward remains visible after chopping stops");
    SDL_SetRenderTarget(renderer,nullptr); SDL_DestroyTexture(target);
    return test.Finish();
}
