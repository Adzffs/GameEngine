#include "TestSupport.h"

#include "../src/Graphics/Graphics.h"

struct GraphicsTestAccess
{
    static SDL_Renderer *Renderer(Graphics &graphics)
    {
        return graphics.renderer;
    }

    static void SetDisplayedInventory(
        Graphics &graphics,
        int logs,
        int planks)
    {
        graphics.displayedLogCount = logs;
        graphics.displayedPlankCount = planks;
    }
};

namespace
{
    struct Pixel
    {
        Uint8 red = 0;
        Uint8 green = 0;
        Uint8 blue = 0;
        Uint8 alpha = 0;
    };

    Pixel ReadPixel(
        SDL_Surface *surface,
        float x,
        float y)
    {
        Pixel pixel;
        SDL_ReadSurfacePixel(
            surface,
            static_cast<int>(x),
            static_cast<int>(y),
            &pixel.red,
            &pixel.green,
            &pixel.blue,
            &pixel.alpha);
        return pixel;
    }

    bool IsColor(
        const Pixel &pixel,
        Uint8 red,
        Uint8 green,
        Uint8 blue)
    {
        return pixel.red == red &&
               pixel.green == green &&
               pixel.blue == blue;
    }
}

int main()
{
    TestContext test;
    Graphics graphics;

    if (!graphics.Initialize())
    {
        test.Expect(false, "Native SDL renderer initializes");
        return test.Finish();
    }

    SDL_Renderer *renderer =
        GraphicsTestAccess::Renderer(graphics);
    const Graphics::CraftingTableLayout layout =
        graphics.GetCraftingTableLayout();

    SDL_SetRenderDrawColor(renderer, 3, 5, 7, 255);
    SDL_RenderClear(renderer);
    graphics.OpenStationMenu(StationType::CRAFTING_TABLE);
    GraphicsTestAccess::SetDisplayedInventory(
        graphics,
        1,
        6);
    graphics.DrawStationMenu();

    SDL_Surface *pixels =
        SDL_RenderReadPixels(renderer, nullptr);
    test.Expect(
        pixels != nullptr,
        "Crafting Table frame can be read from the native renderer");

    if (pixels != nullptr)
    {
        test.Expect(
            IsColor(
                ReadPixel(
                    pixels,
                    layout.panelBounds.x + 10.0f,
                    layout.panelBounds.y +
                        layout.panelBounds.h - 10.0f),
                28,
                28,
                28),
            "Crafting Table draws its bounded modal background");
        test.Expect(
            IsColor(
                ReadPixel(
                    pixels,
                    layout.gridSlots[0].x + 32.0f,
                    layout.gridSlots[0].y + 32.0f),
                65,
                58,
                50),
            "Empty crafting slots draw the intended grid surface");
        test.Expect(
            IsColor(
                ReadPixel(
                    pixels,
                    layout.craftButton.x + 5.0f,
                    layout.craftButton.y + 5.0f),
                45,
                45,
                45),
            "Craft button renders disabled when the grid has no recipe");
        test.Expect(
            IsColor(ReadPixel(pixels, 20.0f, 20.0f), 3, 5, 7),
            "Crafting modal does not paint outside its bounds");
        SDL_DestroySurface(pixels);
    }

    graphics.HandleStationMenuClick(
        layout.ingredientButtons[0].x + 10.0f,
        layout.ingredientButtons[0].y + 10.0f);
    graphics.HandleStationMenuClick(
        layout.gridSlots[4].x + 10.0f,
        layout.gridSlots[4].y + 10.0f);

    SDL_SetRenderDrawColor(renderer, 3, 5, 7, 255);
    SDL_RenderClear(renderer);
    graphics.DrawStationMenu();
    pixels = SDL_RenderReadPixels(renderer, nullptr);
    test.Expect(
        graphics.GetCraftingGrid()[4] == ItemType::LOG,
        "Rendered fixture uses the same hitbox path to place its Log");
    if (pixels != nullptr)
    {
        test.Expect(
            IsColor(
                ReadPixel(
                    pixels,
                    layout.craftButton.x + 5.0f,
                    layout.craftButton.y + 5.0f),
                64,
                92,
                60),
            "Craft button renders enabled for the authoritative Log recipe");
        test.Expect(
            IsColor(
                ReadPixel(
                    pixels,
                    layout.previewBounds.x +
                        layout.previewBounds.w - 5.0f,
                    layout.previewBounds.y +
                        layout.previewBounds.h - 5.0f),
                45,
                45,
                48),
            "Matched recipe preview remains inside its dedicated surface");
        test.Expect(
            IsColor(ReadPixel(pixels, 20.0f, 20.0f), 3, 5, 7),
            "Enabled crafting UI remains bounded to the modal");
        SDL_DestroySurface(pixels);
    }

    return test.Finish();
}
