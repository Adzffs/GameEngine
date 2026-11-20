#include "TestSupport.h"
#include "../src/Graphics/Graphics.h"
#include "../src/Player/Player.h"
#include "../src/World/Map.h"
#include <array>
#include <cstdint>
#include <filesystem>
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
    const auto axePath = std::filesystem::path(reinterpret_cast<const char8_t *>(SDL_GetBasePath())) /
        "assets/embervale/woodcutting-pilot/textures/axes.png";
    const auto axeUtf8 = axePath.u8string();
    SDL_Surface *axePixels = SDL_LoadPNG(reinterpret_cast<const char *>(axeUtf8.c_str()));
    test.Expect(axePixels != nullptr,"Packaged axe pixels are available for blade-orientation checks");
    if (axePixels)
    {
        const std::array<SDL_FPoint,8> cuts{{{0,1},{-1,0},{1,0},{0,-1},
            {-0.707f,0.707f},{0.707f,0.707f},{-0.707f,-0.707f},{0.707f,-0.707f}}};
        const std::array<std::array<Uint8,3>,3> shadows{{{132,98,44},{64,69,66},{55,97,125}}};
        const std::array<std::array<Uint8,3>,3> edges{{{227,192,115},{166,154,126},{239,224,183}}};
        for (int tier = 0; tier < 3; ++tier) for (int facing = 0; facing < 8; ++facing)
            for (int pose : {3,4})
            {
                float edgeX=0,edgeY=0,shadowX=0,shadowY=0;
                int edgeCount=0,shadowCount=0;
                for (int y=0; y<80; ++y) for (int x=0; x<64; ++x)
                {
                    Uint8 r,g,b,a;
                    SDL_ReadSurfacePixel(axePixels,pose*64+x,(tier*8+facing)*80+y,&r,&g,&b,&a);
                    if (!a) continue;
                    const std::array<Uint8,3> rgb{r,g,b};
                    if (rgb==edges[tier]) { edgeX+=x; edgeY+=y; ++edgeCount; }
                    if (rgb==shadows[tier]) { shadowX+=x; shadowY+=y; ++shadowCount; }
                }
                test.Expect(edgeCount>0 && shadowCount>0,"Each striking axe has a readable head and cutting edge");
                if (!edgeCount || !shadowCount) continue;
                const float towardTree=(edgeX/edgeCount-shadowX/shadowCount)*cuts[facing].x+
                    (edgeY/edgeCount-shadowY/shadowCount)*cuts[facing].y;
                test.Expect(towardTree>0.35f,"The cutting edge leads toward the tree on contact and follow-through");
            }
        SDL_DestroySurface(axePixels);
    }
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
