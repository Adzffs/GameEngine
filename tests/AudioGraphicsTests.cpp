#include "TestSupport.h"
#include "../src/Graphics/Graphics.h"

struct GraphicsTestAccess
{
    static void OpenSettings(Graphics &graphics)
    {
        graphics.sidePanelOpen = true;
        graphics.selectedTab = Graphics::SidePanelTab::SETTINGS;
    }
    static bool ClickSound(Graphics &graphics, float x)
    {
        const auto bounds = graphics.GetSidePanelLayout().contentBounds;
        return graphics.HandleSidePanelClick(bounds.x + x, bounds.y + 230, false);
    }
    static void DrawSettings(Graphics &graphics, const char *capture)
    {
        SDL_SetRenderDrawColor(graphics.renderer, 25, 35, 30, 255);
        SDL_RenderClear(graphics.renderer);
        graphics.DrawSettings();
        if (capture)
        {
            SDL_Surface *surface = SDL_RenderReadPixels(graphics.renderer, nullptr);
            if (surface) { SDL_SaveBMP(surface, capture); SDL_DestroySurface(surface); }
        }
        SDL_RenderPresent(graphics.renderer);
    }
};

int main(int argc, char **argv)
{
    TestContext test;
    Graphics graphics;
    test.Expect(graphics.Initialize(), "Settings initialize with the real SDL renderer");
    GraphicsTestAccess::OpenSettings(graphics);
    test.Expect(GraphicsTestAccess::ClickSound(graphics, 10), "Quieter button consumes its click");
    test.ExpectEqual(graphics.GetAudioSettings().GetEffectsVolume(), 55, "Quieter changes effects volume");
    GraphicsTestAccess::ClickSound(graphics, 60);
    test.ExpectEqual(graphics.GetAudioSettings().GetEffectsVolume(), 65, "Louder changes effects volume");
    GraphicsTestAccess::ClickSound(graphics, 120);
    test.Expect(graphics.GetAudioSettings().IsMuted(), "Mute button toggles sound");
    SDL_Event event{};
    event.type = SDL_EVENT_KEY_DOWN;
    event.key.scancode = SDL_SCANCODE_F8;
    event.key.repeat = false;
    SDL_PushEvent(&event);
    bool running = true;
    graphics.ProcessEvents(running);
    test.Expect(!graphics.GetAudioSettings().IsMuted(), "F8 toggles mute through the real event loop");
    event.key.repeat = true;
    SDL_PushEvent(&event);
    graphics.ProcessEvents(running);
    test.Expect(!graphics.GetAudioSettings().IsMuted(), "Key repeat does not bounce mute");
    event.key.repeat = false;
    event.key.scancode = SDL_SCANCODE_F9;
    SDL_PushEvent(&event);
    graphics.ProcessEvents(running);
    test.ExpectEqual(graphics.GetAudioSettings().GetEffectsVolume(), 55,
        "F9 lowers effects volume through the event loop");
    event.key.scancode = SDL_SCANCODE_F10;
    SDL_PushEvent(&event);
    graphics.ProcessEvents(running);
    test.ExpectEqual(graphics.GetAudioSettings().GetEffectsVolume(), 65,
        "F10 raises effects volume through the event loop");
    GraphicsTestAccess::DrawSettings(graphics, argc > 1 ? argv[1] : nullptr);
    return test.Finish();
}
