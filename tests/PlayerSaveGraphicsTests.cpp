#include "TestSupport.h"
#include "../src/Graphics/Graphics.h"
#include "../src/Player/Player.h"
#include "../src/Tutorial/StarterJourney.h"

struct GraphicsTestAccess
{
    static Graphics::PlayerSaveNotice Notice(const Graphics &graphics) { return graphics.playerSaveNotice; }
    static Uint64 Until(const Graphics &graphics) { return graphics.playerSaveNoticeUntil; }
    static bool Draw(Graphics &graphics, Uint64 now, const char *capture = nullptr)
    {
        SDL_SetRenderDrawColor(graphics.renderer, 45, 65, 45, 255);
        SDL_RenderClear(graphics.renderer);
        Player player(1, PlayerInitializationMode::FRESH_START);
        graphics.DrawWorldHud(player, {}, "Mara in the village");
        graphics.DrawSidePanelTabs();
        graphics.DrawPlayerSaveNotice(now);
        SDL_Surface *surface = SDL_RenderReadPixels(graphics.renderer, nullptr);
        const bool captured = surface != nullptr && (!capture || SDL_SaveBMP(surface, capture));
        if (surface) SDL_DestroySurface(surface);
        SDL_RenderPresent(graphics.renderer);
        return captured;
    }
};

int main(int argc, char **argv)
{
    TestContext test;
    Graphics graphics;
    test.Expect(graphics.Initialize(), "Save notice uses the real SDL renderer");
    graphics.SetPlayerSaveNotice(Graphics::PlayerSaveNotice::RETRYING);
    test.Expect(GraphicsTestAccess::Draw(graphics, 1000000, argc > 1 ? argv[1] : nullptr) &&
        GraphicsTestAccess::Notice(graphics) == Graphics::PlayerSaveNotice::RETRYING,
        "Filesystem warning remains visible until a successful save");
    graphics.SetPlayerSaveNotice(Graphics::PlayerSaveNotice::CRAFTING_BLOCKED);
    test.Expect(GraphicsTestAccess::Draw(graphics, 2000000) &&
        GraphicsTestAccess::Notice(graphics) == Graphics::PlayerSaveNotice::CRAFTING_BLOCKED,
        "Crafting failure renders actionable feedback without expiration");
    graphics.SetPlayerSaveNotice(Graphics::PlayerSaveNotice::SAVED);
    const auto until = GraphicsTestAccess::Until(graphics);
    test.Expect(GraphicsTestAccess::Draw(graphics, until - 1) &&
        GraphicsTestAccess::Notice(graphics) == Graphics::PlayerSaveNotice::SAVED,
        "Successful retry replaces the failure warning with confirmation");
    test.Expect(GraphicsTestAccess::Draw(graphics, until) &&
        GraphicsTestAccess::Notice(graphics) == Graphics::PlayerSaveNotice::NONE,
        "Success notice expires without hiding future failures");
    return test.Finish();
}
