#include "TestSupport.h"
#include "../src/Graphics/Graphics.h"
#include "../src/Player/Player.h"
#include "../src/Entity/Monster/Monster.h"
#include "../src/World/Map.h"
#include <array>
#include <cstdint>
#include <set>

struct GraphicsTestAccess
{
    static ForestSprites &Sprites(Graphics &g) { return g.forestSprites; }
    static SDL_Renderer *Renderer(Graphics &g) { return g.renderer; }
    static const MeleePresentationState &Melee(const Graphics &g) { return g.meleePresentation; }
    static void Delta(Graphics &g) { g.lastPresentationTick = 0; g.presentationDeltaSeconds = 0.1f; }
};

std::uint64_t Capture(SDL_Renderer *renderer)
{
    const SDL_Rect region{32,24,96,96};
    SDL_Surface *surface = SDL_RenderReadPixels(renderer,&region);
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
    if (!graphics.Initialize()) { test.Expect(false,"Melee renderer initializes"); return test.Finish(); }
    auto *renderer = GraphicsTestAccess::Renderer(graphics);
    auto &sprites = GraphicsTestAccess::Sprites(graphics);
    SDL_Texture *target = SDL_CreateTexture(renderer,SDL_PIXELFORMAT_RGBA8888,SDL_TEXTUREACCESS_TARGET,1280,720);
    test.Expect(target && SDL_SetRenderTarget(renderer,target),"Melee uses a readable render target");
#if SDL_VERSION_ATLEAST(3,4,0)
    constexpr std::array<ItemType,9> weapons{ItemType::NONE,ItemType::BRONZE_SWORD,ItemType::DEVELOPER_GODSWORD,
        ItemType::BRONZE_AXE,ItemType::IRON_AXE,ItemType::STEEL_AXE,
        ItemType::BRONZE_PICKAXE,ItemType::IRON_PICKAXE,ItemType::STEEL_PICKAXE};
    std::set<std::uint64_t> directionPixels;
    for (int facing = 0; facing < 8; ++facing)
    {
        std::set<std::uint64_t> weaponPixels;
        for (const auto weapon : weapons)
        {
            std::array<std::uint64_t,PlayerAnimationTiming::MeleePoses> poses{};
            for (int pose = 0; pose < PlayerAnimationTiming::MeleePoses; ++pose)
            {
                SDL_SetRenderDrawColor(renderer,13,19,23,255); SDL_RenderClear(renderer);
                test.Expect(sprites.DrawEquippedPlayer(2,2,facing,pose,weapon,ItemType::NONE,true),
                    "Every melee pose renders for every weapon and facing");
                poses[pose] = Capture(renderer);
                test.Expect(poses[pose] != 0,"Melee output contains readable pixels");
            }
            test.Expect(poses[0] != poses[2] && poses[2] != poses[3] && poses[3] != poses[5],
                "Distinct wind-up/contact/recovery for facing " + std::to_string(facing) + " weapon " + std::to_string(static_cast<int>(weapon)));
            weaponPixels.insert(poses[3]);
            if (weapon == ItemType::BRONZE_SWORD) directionPixels.insert(poses[3]);
            for (int walk = 0; walk < PlayerAnimationTiming::WalkPoses; ++walk)
            {
                SDL_SetRenderDrawColor(renderer,13,19,23,255); SDL_RenderClear(renderer);
                test.Expect(sprites.DrawEquippedPlayer(2,2,facing,walk,weapon,ItemType::WOODEN_SHIELD),
                    "Standing and walking keep equipped weapons and shields visible");
            }
            if (weapon != ItemType::NONE)
            {
                SDL_SetRenderDrawColor(renderer,13,19,23,255); SDL_RenderClear(renderer);
                sprites.DrawEquippedPlayer(2,2,facing,PlayerAnimationTiming::MeleePoses-1,weapon,ItemType::NONE,true);
                const auto settled = Capture(renderer);
                SDL_SetRenderDrawColor(renderer,13,19,23,255); SDL_RenderClear(renderer);
                sprites.DrawEquippedPlayer(2,2,facing,0,weapon,ItemType::NONE);
                test.ExpectEqual(Capture(renderer),settled,"Recovery rejoins the exact equipped standing pose without a weapon snap");
            }
        }
        test.ExpectEqual(weaponPixels.size(),weapons.size(),"Each weapon, metal tier and unarmed punch has distinct pixels");
        SDL_SetRenderDrawColor(renderer,13,19,23,255); SDL_RenderClear(renderer);
        sprites.DrawEquippedPlayer(2,2,facing,0,ItemType::BRONZE_SWORD,ItemType::NONE);
        const auto bare = Capture(renderer);
        SDL_SetRenderDrawColor(renderer,13,19,23,255); SDL_RenderClear(renderer);
        sprites.DrawEquippedPlayer(2,2,facing,0,ItemType::BRONZE_SWORD,ItemType::WOODEN_SHIELD);
        test.Expect(Capture(renderer) != bare,"Equipped shield remains visible in facing " + std::to_string(facing));
    }
    test.ExpectEqual(directionPixels.size(),std::size_t{8},"All eight sword-contact directions remain distinct");
    test.Expect(!sprites.DrawEquippedPlayer(2,2,8,0,ItemType::BRONZE_SWORD,ItemType::NONE,true),"Invalid direction falls back safely");
    test.Expect(!sprites.DrawEquippedPlayer(2,2,0,PlayerAnimationTiming::MeleePoses,ItemType::BRONZE_SWORD,ItemType::NONE,true),"Invalid attack pose falls back safely");
    test.Expect(!sprites.DrawEquippedPlayer(2,2,0,0,ItemType::NONE,ItemType::NONE),"Empty hands retain the normal walking sprites");
#endif
    Map map(100,100);
    Player player(1,PlayerInitializationMode::FRESH_START);
    player.GetPosition().SetPosition(2,2);
    player.GetEquipment().Equip(EquipmentSlotType::WEAPON,ItemType::BRONZE_SWORD);
    std::vector<std::unique_ptr<Entity>> entities;
    entities.push_back(std::make_unique<Monster>(2,3,2,CombatRatings{8,8,8,100}));
    Action action(ActionType::MELEE_ATTACK,"Attack",2,1,2,true); action.Start(10);
    std::map<int,MeleeCombatFeedback> feedbacks;
    auto draw = [&](const Action *active) {
        GraphicsTestAccess::Delta(graphics);
        graphics.Render(map,entities,{},{},{},feedbacks,2,2,player.GetInventory(),player.GetEquipment(),player,active);
    };
    draw(&action);
    test.Expect(GraphicsTestAccess::Melee(graphics).GetAnimation() &&
        !GraphicsTestAccess::Melee(graphics).GetAnimation()->resolved,"Full scene selects the live attack wind-up");
    for (int frame = 0; frame < 20; ++frame) draw(&action);
    test.Expect(GraphicsTestAccess::Melee(graphics).GetPose() < 3,"Rendering alone never resolves a swing");
    auto &contact = feedbacks[2];
    contact = MeleeCombatFeedback{1,2,true,10,3,12,2,2,3,2,ItemType::BRONZE_SWORD};
    graphics.SynchronizeMeleeResults(1, {contact});
    feedbacks.clear();
    entities.clear();
    draw(nullptr);
    test.ExpectEqual(GraphicsTestAccess::Melee(graphics).GetPose(),3,"A killing blow renders contact after the defender is removed");
    draw(nullptr);
    test.ExpectEqual(GraphicsTestAccess::Melee(graphics).GetPose(),4,"Full scene advances smoothly into follow-through");
    player.GetEquipment().Unequip(EquipmentSlotType::WEAPON);
    draw(nullptr);
    test.Expect(!GraphicsTestAccess::Melee(graphics).GetAnimation(),"Unequipping clears the old swing in the full scene");
    test.ExpectEqual(player.GetCurrentHealth(),player.GetMaximumHealth(),"Cosmetic animation never changes player health");
    SDL_SetRenderTarget(renderer,nullptr); SDL_DestroyTexture(target);
    return test.Finish();
}
